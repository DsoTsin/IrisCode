#include "anisette.h"
#include "authutils.h"
#include "exception.h"

#include "developer.h"

#include <ShlObj_core.h>
#include <Windows.h>
#include <filesystem>
#include <openssl/evp.h>

#define id void *
#define SEL void *
// 0x86 jump instruction is 5 bytes total (opcode + 4 byte address).
#define JUMP_INSTRUCTION_SIZE 5

namespace iris {
namespace fs = std::filesystem;
extern void install_hooks(bool begin);
extern void create_dirs();

static std::string uuid;

typedef id(__cdecl *GETCLASSFUNC)(const char *name);
typedef id(__cdecl *REGISTERSELFUNC)(const char *name);
typedef id(__cdecl *SENDMSGFUNC)(id self, void *_cmd);
typedef id(__cdecl *SENDMSGFUNC_OBJ)(id self, void *_cmd, id parameter1);
typedef id(__cdecl *SENDMSGFUNC_INT)(id self, void *_cmd, int parameter1);
typedef id *(__cdecl *COPYMETHODLISTFUNC)(id cls, unsigned int *outCount);
typedef id(__cdecl *GETMETHODNAMEFUNC)(id method);
typedef const char *(__cdecl *GETSELNAMEFUNC)(SEL sel);
typedef id(__cdecl *GETOBJCCLASSFUNC)(id obj);

typedef id(__cdecl *GETOBJECTFUNC)();

typedef id(__cdecl *CLIENTINFOFUNC)(id obj);
typedef void(__cdecl *CFRELEASEFUNC)(id obj);
typedef id(__cdecl *COPYANISETTEDATAFUNC)(void *, int, void *);

GETCLASSFUNC objc_getClass;
REGISTERSELFUNC sel_registerName;
SENDMSGFUNC objc_msgSend;
COPYMETHODLISTFUNC class_copyMethodList;
GETMETHODNAMEFUNC method_getName;
GETSELNAMEFUNC sel_getName;
GETOBJCCLASSFUNC object_getClass;

GETOBJECTFUNC GetDeviceID;
GETOBJECTFUNC GetLocalUserID;
CLIENTINFOFUNC GetClientInfo;
COPYANISETTEDATAFUNC CopyAnisetteData;
CFRELEASEFUNC CFRelease = NULL;

class ObjcObject {
public:
  id isa;

  std::string description() const {
    id descriptionSEL = sel_registerName("description");
    id descriptionNSString = objc_msgSend((void *)this, descriptionSEL);

    id cDescriptionSEL = sel_registerName("UTF8String");
    const char *cDescription =
        ((const char *(*)(id, SEL))objc_msgSend)(descriptionNSString,
                                                 cDescriptionSEL);

    std::string description(cDescription);
    // CFRelease(descriptionNSString);
    return description;
  }
};

static std::string clientInfo;

id __cdecl ALTClientInfoReplacementFunction(void *arg) {
#if 1
  ObjcObject *NSString = (ObjcObject *)objc_getClass("NSString");
  id stringInit = sel_registerName("stringWithUTF8String:");

  ObjcObject *clientInfo = (ObjcObject *)((
      id(*)(id, SEL,
            const char *))objc_msgSend)(NSString, stringInit,
                                        "<MacBookPro15,1> <Mac OS "
                                        "X;10.15.2;19C57> <com.apple.AuthKit/1 "
                                        "(com.apple.dt.Xcode/3594.4.19)>");

  odslog("Swizzled Client Info: " << clientInfo->description());

  return clientInfo;
#else
  auto ret = GetClientInfo(arg);
  clientInfo = ((ObjcObject *)ret)->description();
  return ret;
#endif
}

static std::string udid =
    "99384B3A.CB274885.00000105.54B34136.20B14F1E.AB737EB4.350A3EFB";

id __cdecl ALTDeviceIDReplacementFunction() {
#if 1
  ObjcObject *NSString = (ObjcObject *)objc_getClass("NSString");
  id stringInit = sel_registerName("stringWithUTF8String:");

  ObjcObject *deviceID = (ObjcObject *)((id(*)(
      id, SEL, const char *))objc_msgSend)(NSString, stringInit, udid.c_str());

  odslog("Swizzled Device ID: " << deviceID->description());

  return deviceID;
#else
  return GetDeviceID();
#endif
}

void convert_filetime(struct timeval *out_tv, const FILETIME *filetime) {
  // Microseconds between 1601-01-01 00:00:00 UTC and 1970-01-01 00:00:00 UTC
  static const uint64_t EPOCH_DIFFERENCE_MICROS = 11644473600000000ull;

  // First convert 100-ns intervals to microseconds, then adjust for the
  // epoch difference
  uint64_t total_us = (((uint64_t)filetime->dwHighDateTime << 32) |
                       (uint64_t)filetime->dwLowDateTime) /
                      10;
  total_us -= EPOCH_DIFFERENCE_MICROS;

  // Convert to (seconds, microseconds)
  out_tv->tv_sec = (time_t)(total_us / 1000000);
  out_tv->tv_usec = (long)(total_us % 1000000);
}

class LocalMachineReg {
public:
  LocalMachineReg(const std::string &path) : root_(NULL) {
    RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.c_str(), 0, 0x20019, &root_);
  }
  ~LocalMachineReg() {
    if (root_) {
      RegCloseKey(root_);
      root_ = NULL;
    }
  }

  std::string get_string(std::string const &key) const {
    std::string ret;
    DWORD value_len = 0;
    if (0 ==
        RegQueryValueExA(root_, key.c_str(), NULL, NULL, NULL, &value_len)) {
      ret.reserve(value_len + 1);
      ret.resize(value_len);
      RegQueryValueExA(root_, key.c_str(), NULL, NULL, (LPBYTE)ret.data(),
                       &value_len);
    }
    return ret;
  }

private:
  HKEY root_;
};

bool Anisette::load_icloud() {
  wchar_t *programFilesCommonDirectory;
  SHGetKnownFolderPath(FOLDERID_ProgramFilesCommon, 0, NULL,
                       &programFilesCommonDirectory);

  fs::path appleDirectoryPath(programFilesCommonDirectory);
  appleDirectoryPath.append("Apple");

  fs::path internetServicesDirectoryPath(appleDirectoryPath);
  internetServicesDirectoryPath.append("Internet Services");

  fs::path iCloudMainPath(internetServicesDirectoryPath);
  iCloudMainPath.append("iCloud_main.dll");

  HINSTANCE iCloudMain = LoadLibraryW(iCloudMainPath.c_str());
  if (iCloudMain == NULL) {
    return false;
  }

  // Retrieve known exported function address to provide reference point for
  // accessing private functions.
  uintptr_t exportedFunctionAddress =
      (uintptr_t)GetProcAddress(iCloudMain, "PL_FreeArenaPool");
  size_t exportedFunctionDisassembledOffset = 0x1aa2a0;

  /* Reprovision Anisette Function */
  size_t anisetteFunctionDisassembledOffset = 0x241ee0;
  size_t difference = anisetteFunctionDisassembledOffset - 0x1aa2a0;

  CopyAnisetteData =
      (COPYANISETTEDATAFUNC)(exportedFunctionAddress + difference);
  if (CopyAnisetteData == NULL) {
    return false;
  }

  /* Anisette Data Functions */
  size_t clientInfoFunctionDisassembledOffset = 0x23e730;
  size_t clientInfoFunctionRelativeOffset =
      clientInfoFunctionDisassembledOffset - exportedFunctionDisassembledOffset;
  GetClientInfo = (CLIENTINFOFUNC)(exportedFunctionAddress +
                                   clientInfoFunctionRelativeOffset);

  size_t deviceIDFunctionDisassembledOffset = 0x23d8b0;
  size_t deviceIDFunctionRelativeOffset =
      deviceIDFunctionDisassembledOffset - exportedFunctionDisassembledOffset;
  GetDeviceID =
      (GETOBJECTFUNC)(exportedFunctionAddress + deviceIDFunctionRelativeOffset);

  size_t localUserIDFunctionDisassembledOffset = 0x23db30;
  size_t localUserIDFunctionRelativeOffset =
      localUserIDFunctionDisassembledOffset -
      exportedFunctionDisassembledOffset;
  GetLocalUserID = (GETOBJECTFUNC)(exportedFunctionAddress +
                                   localUserIDFunctionRelativeOffset);

  if (GetClientInfo == NULL || GetDeviceID == NULL || GetLocalUserID == NULL) {
    return false;
  }

#if 0
  {
    /** Client Info Swizzling */
    int64_t *targetFunction = (int64_t *)GetClientInfo;
    int64_t *replacementFunction = (int64_t *)&ALTClientInfoReplacementFunction;

    SYSTEM_INFO system;
    GetSystemInfo(&system);
    int pageSize = system.dwAllocationGranularity;

    uintptr_t startAddress = (uintptr_t)targetFunction;
    uintptr_t endAddress = startAddress + 1;
    uintptr_t pageStart = startAddress & -pageSize;

    // Mark page containing the target function implementation as writable so we
    // can inject our own instruction.
    DWORD permissions = 0;
    BOOL value = VirtualProtect((LPVOID)pageStart, endAddress - pageStart,
                                PAGE_EXECUTE_READWRITE, &permissions);

    if (!value) {
      return false;
    }

    int32_t jumpOffset =
        (int64_t)replacementFunction -
        ((int64_t)targetFunction +
         JUMP_INSTRUCTION_SIZE); // Add jumpInstructionSize because offset is
                                 // relative to _next_ instruction.

    // Construct jump instruction.
    // Jump doesn't return execution to target function afterwards, allowing us
    // to completely replace the implementation.
    char instruction[5];
    instruction[0] = '\xE9'; // E9 = "Jump near (relative)" opcode
    ((int32_t *)(instruction + 1))[0] =
        jumpOffset; // Next 4 bytes = jump offset

    // Replace first instruction in target target function with our
    // unconditional jump to replacement function.
    char *functionImplementation = (char *)targetFunction;
    for (int i = 0; i < JUMP_INSTRUCTION_SIZE; i++) {
      functionImplementation[i] = instruction[i];
    }
  }

  {
    /** Device ID Swizzling */
    int64_t *targetFunction = (int64_t *)GetDeviceID;
    int64_t *replacementFunction = (int64_t *)&ALTDeviceIDReplacementFunction;

    SYSTEM_INFO system;
    GetSystemInfo(&system);
    int pageSize = system.dwAllocationGranularity;

    uintptr_t startAddress = (uintptr_t)targetFunction;
    uintptr_t endAddress = startAddress + 1;
    uintptr_t pageStart = startAddress & -pageSize;

    // Mark page containing the target function implementation as writable so we
    // can inject our own instruction.
    DWORD permissions = 0;
    BOOL value = VirtualProtect((LPVOID)pageStart, endAddress - pageStart,
                                PAGE_EXECUTE_READWRITE, &permissions);

    if (!value) {
      return false;
    }

    int32_t jumpOffset =
        (int64_t)replacementFunction -
        ((int64_t)targetFunction +
         JUMP_INSTRUCTION_SIZE); // Add jumpInstructionSize because offset is
                                 // relative to _next_ instruction.

    // Construct jump instruction.
    // Jump doesn't return execution to target function afterwards, allowing us
    // to completely replace the implementation.
    char instruction[5];
    instruction[0] = '\xE9'; // E9 = "Jump near (relative)" opcode
    ((int32_t *)(instruction + 1))[0] =
        jumpOffset; // Next 4 bytes = jump offset

    // Replace first instruction in target target function with our
    // unconditional jump to replacement function.
    char *functionImplementation = (char *)targetFunction;
    for (int i = 0; i < JUMP_INSTRUCTION_SIZE; i++) {
      functionImplementation[i] = instruction[i];
    }
  }
#endif
  return true;
}

bool Anisette::load_dependencies() {
  LocalMachineReg internetSvReg("SOFTWARE\\Apple Inc.\\Internet Services");
  LocalMachineReg applAppSupReg(
      "SOFTWARE\\Apple Inc.\\Apple Application Support");
  auto internetSv = internetSvReg.get_string("InstallDir");
  auto applAppSup = applAppSupReg.get_string("InstallDir");
  fs::path internetServicesDirectoryPath(internetSv.c_str());
  if (!fs::exists(internetServicesDirectoryPath)) {
    throw Exception(
        ADSError::LibraryLoadError,
        "Windows Registry \"LOCAL_MACHINE\\SOFTWARE\\Apple Inc.\\Internet "
        "Services\" not found, cannot load iCloud dependencies.");
  }

  fs::path aosKitPath(internetServicesDirectoryPath);
  aosKitPath.append("AOSKit.dll");

  if (!fs::exists(aosKitPath)) {
    throw Exception(ADSError::LibraryLoadError, "Missing AOSKit.dll");
  }

  fs::path applicationSupportDirectoryPath(applAppSup.c_str());
  if (!fs::exists(applicationSupportDirectoryPath)) {
    throw Exception(ADSError::LibraryLoadError,
                    "Missing Application Support Folder (x86)");
  }

  fs::path objcPath(applicationSupportDirectoryPath);
  objcPath.append("objc.dll");

  if (!fs::exists(objcPath)) {
    throw Exception(ADSError::LibraryLoadError, "Missing objc.dll");
  }

  fs::path foundationPath(applicationSupportDirectoryPath);
  foundationPath.append("Foundation.dll");

  if (!fs::exists(foundationPath)) {
    throw Exception(ADSError::LibraryLoadError,
                            "Missing Foundation.dll");
  }

  fs::path cflib_path(applicationSupportDirectoryPath);
  cflib_path.append("CoreFoundation.dll");
  if (!fs::exists(cflib_path)) {
    throw Exception(ADSError::LibraryLoadError,
                            "Missing CoreFoundation.dll");
  }

  BOOL result = SetCurrentDirectoryW(applicationSupportDirectoryPath.c_str());
  DWORD dwError = GetLastError();
  HINSTANCE objcLibrary = LoadLibraryW(objcPath.c_str());
  HINSTANCE cflib = LoadLibraryW(cflib_path.c_str());
  HINSTANCE foundationLibrary = LoadLibraryW(foundationPath.c_str());
  HINSTANCE AOSKit = LoadLibraryW(aosKitPath.c_str());

  dwError = GetLastError();

  if (objcLibrary == NULL || cflib == NULL || AOSKit == NULL ||
      foundationLibrary == NULL) {
    char buffer[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, 256,
                   NULL);
    throw Exception(ADSError::LibraryLoadError, buffer);
  }

  /* Objective-C runtime functions */
  objc_getClass = (GETCLASSFUNC)GetProcAddress(objcLibrary, "objc_getClass");
  sel_registerName =
      (REGISTERSELFUNC)GetProcAddress(objcLibrary, "sel_registerName");
  objc_msgSend = (SENDMSGFUNC)GetProcAddress(objcLibrary, "objc_msgSend");
  class_copyMethodList =
      (COPYMETHODLISTFUNC)GetProcAddress(objcLibrary, "class_copyMethodList");
  method_getName =
      (GETMETHODNAMEFUNC)GetProcAddress(objcLibrary, "method_getName");
  sel_getName = (GETSELNAMEFUNC)GetProcAddress(objcLibrary, "sel_getName");
  object_getClass =
      (GETOBJCCLASSFUNC)GetProcAddress(objcLibrary, "object_getClass");
  CFRelease = (CFRELEASEFUNC)GetProcAddress(cflib, "CFRelease");

  if (objc_getClass == NULL || CFRelease == NULL) {
    throw Exception(ADSError::LibraryLoadError,
                    "objc.dll is not 32bits dll ??");
  }
  return true;
}

bool Anisette::reprovision() {
  void *ret = nullptr;
  auto res = CopyAnisetteData(0, 1, &ret);
  if (ret) {
    auto error = ((ObjcObject *)ret)->description();
    CFRelease(ret);
    odslog("Reprovision failed, " << error);
    throw Exception(ADSError::InvalidAnisetteData, error);
  }
  if (res) {
    auto desc = ((ObjcObject *)res)->description();
    CFRelease(res);
    odslog("Reprovision succeed, " << desc);
  }
  return true;
}

Anisette &Anisette::get() {
  static Anisette ani;
  return ani;
}

bool Anisette::try_init() { return load_dependencies() && load_icloud(); }

std::string base64(const unsigned char *input, int length) {
  const auto pl = 4 * ((length + 2) / 3);
  bytes tmp;
  tmp.resize(pl + 1);
  tmp[pl] = 0;
  const auto ol = EVP_EncodeBlock(tmp.data(), input, length);
  return std::string(tmp.data(), tmp.data() + pl);
}

void Anisette::provision() {
  std::lock_guard<std::mutex> lock(prov_lock);

  install_hooks(true);
getkeys:
  ObjcObject *NSString = (ObjcObject *)objc_getClass("NSString");
  id stringInit = sel_registerName("stringWithUTF8String:");

  /* One-Time Pasword */
  ObjcObject *dsidString = (ObjcObject *)((
      id(*)(id, SEL, const char *))objc_msgSend)(NSString, stringInit, "-2");
  ObjcObject *machineIDKey = (ObjcObject *)((
      id(*)(id, SEL, const char *))objc_msgSend)(NSString, stringInit,
                                                 "X-Apple-MD-M");
  ObjcObject *otpKey = (ObjcObject *)((id(*)(
      id, SEL, const char *))objc_msgSend)(NSString, stringInit, "X-Apple-MD");

  ObjcObject *AOSUtilities = (ObjcObject *)objc_getClass("AOSUtilities");
  ObjcObject *headers = (ObjcObject *)((id(*)(
      id, SEL, id))objc_msgSend)(AOSUtilities,
                                 sel_registerName("retrieveOTPHeadersForDSID:"),
                                 dsidString);

  ObjcObject *machineID = (ObjcObject *)((id(*)(
      id, SEL, id))objc_msgSend)(headers, sel_registerName("objectForKey:"),
                                 machineIDKey);
  ObjcObject *otp = (ObjcObject *)((id(*)(
      id, SEL, id))objc_msgSend)(headers, sel_registerName("objectForKey:"),
                                 otpKey);

  if (otp == NULL || machineID == NULL) {
    reprovision();
    reprov_count_++;
    if (reprov_count_ < 4) {
      goto getkeys;
    }

    install_hooks(false);
    return;
  }

  install_hooks(false);
  odslog("OTP: " << otp->description() << "\n"
                 << "MachineID: " << machineID->description());

  /* Device Hardware */
  ObjcObject *deviceDescription =
      (ObjcObject *)ALTClientInfoReplacementFunction(NULL);
#if 0
  ObjcObject *deviceID = (ObjcObject *)ALTDeviceIDReplacementFunction();
#else
  ObjcObject *deviceID = (ObjcObject *)GetDeviceID();
#endif

  if (deviceDescription == NULL || deviceID == NULL) {
    return;
  }

#if SPOOF_MAC
  ObjcObject *localUserID = (ObjcObject *)GetLocalUserID();
#else
  std::string description = deviceID->description();

  // TODO
  std::vector<unsigned char> deviceIDData(description.begin(),
                                          description.end());

  std::string encodedDeviceID =
      base64(deviceIDData.data(), deviceIDData.size());

  ObjcObject *localUserID = (ObjcObject *)((
      id(*)(id, SEL, const char *))objc_msgSend)(NSString, stringInit,
                                                 encodedDeviceID.c_str());
#endif

  std::string deviceSerialNumber = "C02LKHBBFD57";

  if (localUserID == NULL) {
    return;
  }

  FILETIME systemTime;
  GetSystemTimeAsFileTime(&systemTime);

  TIMEVAL date;
  convert_filetime(&date, &systemTime);

  machineID_ = machineID->description();
  oneTimePassword_ = otp->description();
  localUserID_ = localUserID->description();
  routingInfo_ = 17106176;
  deviceUniqueIdentifier_ = deviceID->description();
  deviceSerialNumber_ = deviceSerialNumber;
  deviceDescription_ = deviceDescription->description();
  date_ = {date.tv_sec, date.tv_usec};
  locale_ = "en_US";
  timeZone_ = "PST";

  odslog("Anisette: \n\tmachineId:" << machineID_
                                    << "\n\tOTP:" << oneTimePassword_
                                    << "\n\tLocalUser:" << localUserID_
                                    << "\n\tRoutingInfo:" << routingInfo_
                                    << "\n\tUDID:" << deviceUniqueIdentifier_);
}

std::map<std::string, plist_t> Anisette::make_dict() {
  time_t time;
  struct tm *tm;
  char dateString[64];
  time = date_.tv_sec;
  tm = localtime(&time);
  // TODO
  strftime(dateString, sizeof dateString, "%FT%T%z", tm);

  std::lock_guard<std::mutex> lock(prov_lock);
  return {
      {"X-Apple-Locale", plist_new_string(locale_.c_str())},
      {"X-Apple-I-TimeZone", plist_new_string(timeZone_.c_str())},
      {"X-Apple-I-MD", plist_new_string(oneTimePassword_.c_str())},
      {"X-Apple-I-MD-LU", plist_new_string(localUserID_.c_str())},
      {"X-Apple-I-MD-M", plist_new_string(machineID_.c_str())},
      {"X-Apple-I-MD-RINFO", plist_new_uint(routingInfo_)},
      {"X-Apple-I-Client-Time", plist_new_string(dateString)},
      {"X-Mme-Device-Id", plist_new_string(deviceUniqueIdentifier_.c_str())},
      {"X-Apple-I-SRL-NO", plist_new_string(deviceSerialNumber_.c_str())},
  };
}

std::map<std::string, std::string> Anisette::make_string_dict(bool with_srl) {
  time_t time;
  struct tm *tm;
  char dateString[64];
  time = date_.tv_sec;
  tm = localtime(&time);

  strftime(dateString, sizeof dateString, "%FT%T%z", tm);

  char buff[256] = {0};
  itoa(routingInfo_, buff, 10);

  std::lock_guard<std::mutex> lock(prov_lock);
  if (with_srl)
    return {
        {std::string("X-Apple-Locale"), locale_},
        {"X-Apple-I-TimeZone", timeZone_},
        {"X-Apple-I-MD", oneTimePassword_},
        {"X-Apple-I-MD-LU", localUserID_},
        {"X-Apple-I-MD-M", machineID_},
        {"X-Apple-I-MD-RINFO", buff},
        {"X-Apple-I-Client-Time", dateString},
        {"X-Mme-Device-Id", deviceUniqueIdentifier_},
        {"X-Apple-I-SRL-NO", deviceSerialNumber_},
        {"X-Mme-Client-Info", deviceDescription_}, // !!!!
    };
  else
    return {
        {std::string("X-Apple-Locale"), locale_},
        {"X-Apple-I-TimeZone", timeZone_},
        {"X-Apple-I-MD", oneTimePassword_},
        {"X-Apple-I-MD-LU", localUserID_},
        {"X-Apple-I-MD-M", machineID_},
        {"X-Apple-I-MD-RINFO", buff},
        {"X-Apple-I-Client-Time", dateString},
        {"X-Mme-Device-Id", deviceUniqueIdentifier_},
        {"X-Mme-Client-Info", deviceDescription_}, // !!!!
    };
}

Anisette::Anisette() : reprov_count_(0), inited_(false) {
  create_dirs();
  inited_ = try_init();
}
Anisette::~Anisette() { inited_ = false; }

} // namespace iris