#include "os.hpp"
#include "log.hpp"
#include "md5.hpp"
#include <cassert>
#include <cstdarg>
#include <fstream>
#include <sys/stat.h>
#include <chrono>
#if _WIN32
#include <Windows.h>
#include <detours.h>
#include <pathcch.h>
#include <sddl.h>
#include <shlobj_core.h>
#include <shlwapi.h>
#include <userenv.h>
#define WIN32_NO_STATUS
#include <Pdh.h>
#include <WbemCli.h>
#include <comutil.h>
#include <rpcdce.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "pdh.lib")
namespace iris {
namespace win {
namespace pdh {
static PDH_HQUERY cpuQuery = INVALID_HANDLE_VALUE;
static PDH_HCOUNTER cpuTotal;
void init() {
  PdhOpenQueryW(NULL, NULL, &cpuQuery);
  // You can also use L"\\Processor(*)\\% Processor Time" and get individual CPU values with
  // PdhGetFormattedCounterArray()
  PdhAddEnglishCounterW(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
  PdhCollectQueryData(cpuQuery);
}
double getCurrentValue() {
  if (cpuQuery == INVALID_HANDLE_VALUE) {
    init();
  }
  PDH_FMT_COUNTERVALUE counterVal;
  PdhCollectQueryData(cpuQuery);
  PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
  return counterVal.doubleValue;
}
} // namespace pdh
void scoped_handle::close() {
  if (m_handle) {
    ::CloseHandle(m_handle);
    m_handle = NULL;
  }
}
class scope_process_info {
public:
  scope_process_info(PROCESS_INFORMATION in) : m_proc_info(in) {}
  ~scope_process_info() {
    if (m_proc_info.hProcess) {
      CloseHandle(m_proc_info.hProcess);
    }
  }
  HANDLE proc() const { return m_proc_info.hProcess; }
  HANDLE thread() const { return m_proc_info.hThread; }

private:
  PROCESS_INFORMATION m_proc_info;
};
} // namespace win
} // namespace iris
static SYSTEM_INFO sSysInfo = {};
#else
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#if __APPLE__
#import <CoreFoundation/CoreFoundation.h>
#else
#include <iconv.h>
#endif
extern char** environ;

bool ReadFromPipe(int fd, std::string* output) {
  char buffer[257] = {0};
  int bytes_read = 0;
  do {
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
  } while (bytes_read == -1 && errno == EINTR);
  buffer[256] = 0;
  if (bytes_read == -1) {
    return errno == EAGAIN || errno == EWOULDBLOCK;
  } else if (bytes_read <= 0) {
    return false;
  }
  if (bytes_read > 0) {
    output->append(buffer, bytes_read);
  }
  return true;
}

bool WaitForExit(int pid, int* exit_code) {
  int status;
  if (waitpid(pid, &status, 0) < 0) {
    // PLOG(ERROR) << "waitpid";
    return false;
  }
  if (WIFEXITED(status)) {
    *exit_code = WEXITSTATUS(status);
    return true;
  } else if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGINT || WTERMSIG(status) == SIGTERM || WTERMSIG(status) == SIGHUP)
      return false;
  }
  return false;
}
int is_dir(const char* path) {
  struct stat statbuf;
  if (stat(path, &statbuf) != 0)
    return 0;
  return S_ISDIR(statbuf.st_mode);
}
namespace nix {
class scope_fd {
public:
  scope_fd(int in_fd) : m_fd(in_fd) {}
  ~scope_fd() { reset(); }
  void reset() {
    if (m_fd != 0) {
      close(m_fd);
      m_fd = 0;
    }
  }
  int get() const { return m_fd; }

private:
  int m_fd;
};
} // namespace nix

#endif

namespace iris {
static void add(unsigned char* bytes, int offset, uint32_t value) {
  int i;
  for (i = 0; i < 4; ++i) {
    bytes[offset++] = (unsigned char)(value & 0xff);
    value >>= 8;
  }
}
uint32_t do_hash(const char* str, int seed) {
  /* DJB2 hashing; see http://www.cse.yorku.ca/~oz/hash.html */
  uint32_t hash = 5381;
  if (seed != 0) {
    hash = hash * 33 + seed;
  }
  while (*str) {
    hash = hash * 33 + (*str);
    str++;
  }
  return hash;
}
string uuid(const string& name) {
  string ret;
  ret.reserve(37);
  ret.resize(36);
  unsigned char bytes[16];

  if (!name.empty()) {
    add(bytes, 0, do_hash(name.c_str(), 0));
    add(bytes, 4, do_hash(name.c_str(), 'i'));
    add(bytes, 8, do_hash(name.c_str(), 'r'));
    add(bytes, 12, do_hash(name.c_str(), 'i'));
  } else {
#if _WIN32
    ::CoCreateGuid((GUID*)bytes);
#else
    int result;

    /* not sure how to get a UUID here, so I fake it */
    FILE* rnd = fopen("/dev/urandom", "rb");
    result = fread(bytes, 16, 1, rnd);
    fclose(rnd);
    if (!result) {
      return 0;
    }
#endif
  }
  sprintf(ret.data(), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
          bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8],
          bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
  return ret;
}

#if _WIN32
void wait_for_debugger() {
  while (!IsDebuggerPresent()) {
    ::Sleep(10);
  }
}

static HANDLE stdout_handle = INVALID_HANDLE_VALUE;
static FILE* con_fp = NULL;
static FILE* con_fp_err = NULL;

bool check_attach_console() {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h != NULL && h != INVALID_HANDLE_VALUE)
    return true;

  BOOL ret = AttachConsole(ATTACH_PARENT_PROCESS);
  if (ret != TRUE) {
    AllocConsole();

    stdout_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);

    HANDLE hStdout
        = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStdout);

    freopen_s(&con_fp, "CONOUT$", "w", stdout);
    freopen_s(&con_fp_err, "CONOUT$", "w", stderr);
  } else {
    return true;
  }

  return false;
}

string wc_to_utf8(const wchar_t* wstr) {
  if (!wstr)
    return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  std::string strTo(size_needed - 1, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed - 1, NULL, NULL);
  return strTo;
}

void get_last_err(uint32_t& code, string& message) {
  code = GetLastError();
  LPTSTR lpBuffer = NULL;
  DWORD length = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                                    | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPTSTR)&lpBuffer, 0, NULL);
  if (length == 0) {
    char tmp[100] = {0};
    sprintf_s(tmp, "{δ�����������(%d)}", code);
    message = tmp;
  } else {
    if (length >= 2 && lpBuffer[length - 1] == '\n' && lpBuffer[length - 2] == '\r') {
      lpBuffer[length - 2] = 0;
      lpBuffer[length - 1] = 0;
    }
    message = lpBuffer;
    LocalFree(lpBuffer);
  }
}
#endif

wstring to_utf16(string const& str) {
  if (str.empty())
    return wstring();
#if _WIN32
  size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
  if (charsNeeded == 0)
    return wstring();

  vector<wchar_t> buffer(charsNeeded);
  int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &buffer[0],
                                             (int)buffer.size());
  if (charsConverted == 0)
    return wstring();

  return wstring(&buffer[0], charsConverted);
#else
  return wstring();
#endif
}

uint64_t get_total_physical_memory() {
#if _WIN32
  struct tt_phys_mem {
    tt_phys_mem() : statex{0} { 
      statex.dwLength = sizeof(statex);
      ::GlobalMemoryStatusEx(&statex);
    }
    MEMORYSTATUSEX statex;
  };
  static tt_phys_mem mem_stat;
  return mem_stat.statex.ullTotalPhys;
#else
  return 0;
#endif
}

uint64_t get_available_physical_memory() {
#if _WIN32
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex); // I misunderstand that
  ::GlobalMemoryStatusEx(&statex);
  return statex.ullAvailPhys;
#else
  return 0;
#endif
}

string to_utf8(wstring const& wstr) {
  if (wstr.empty())
    return std::string();
#if _WIN32
  int size_needed
      = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
#elif __APPLE__
  std::string strTo;
  CFStringRef ref = CFStringCreateWithCharactersNoCopy(NULL, (const UniChar*)wstr.c_str(),
                                                       wstr.size(), kCFAllocatorNull);

  CFIndex length = CFStringGetLength(ref);
  if (length <= 0)
    return "";

  CFIndex bufsize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
  char* buf = (char*)calloc(bufsize, 1);

  if (CFStringGetCString(ref, buf, bufsize, kCFStringEncodingUTF8))
    strTo = buf;
  return strTo;
#else
  std::string strTo;
  iconv_t utf16_to_utf8 = iconv_open("UTF-8", "UTF-16LE");
  const uint16_t* in16char = (const uint16_t*)wstr.c_str();
  size_t in16len = wstr.size();
  char* utf8 = nullptr;
  size_t utf8len = 0;
  size_t result = iconv(utf16_to_utf8, (char**)&in16char, &in16len, &utf8, &utf8len);
  iconv_close(utf16_to_utf8);
  return strTo;
#endif
}

int get_num_cores() {
#if _WIN32
  if (sSysInfo.dwNumberOfProcessors == 0) {
    ::GetSystemInfo(&sSysInfo);
  }
  return sSysInfo.dwNumberOfProcessors;
#else
  return 8;
#endif
}

string_list split(string data, string token) {
  string_list output;
  size_t pos = string::npos;
  do {
    pos = data.find(token);
    output.push_back(data.substr(0, pos));
    if (string::npos != pos)
      data = data.substr(pos + token.size());
  } while (string::npos != pos);
  return output;
}

bool execute_command_line(string const& command_line,
                          string const& startup_dir,
                          string& _stdout,
                          string& _stderr,
                          int& ret_code) {
#if _WIN32
  SECURITY_ATTRIBUTES sa_attr;
  sa_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa_attr.bInheritHandle = TRUE;
  sa_attr.lpSecurityDescriptor = nullptr;
  HANDLE out_read = nullptr;
  HANDLE out_write = nullptr;
  if (!CreatePipe(&out_read, &out_write, &sa_attr, 0)) {
    XB_LOGE("Failed to create pipe");
    return false;
  }
  win::scoped_handle scoped_out_read(out_read);
  win::scoped_handle scoped_out_write(out_write);
  HANDLE err_read = nullptr;
  HANDLE err_write = nullptr;
  if (!CreatePipe(&err_read, &err_write, &sa_attr, 0)) {
    XB_LOGE("Failed to create pipe");
    return false;
  }
  win::scoped_handle scoped_err_read(err_read);
  win::scoped_handle scoped_err_write(err_write);
  if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0)) {
    XB_LOGE("Failed to disable pipe inheritance");
    return false;
  }
  if (!SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0)) {
    XB_LOGE("Failed to disable pipe inheritance");
    return false;
  }
  STARTUPINFO start_info = {};
  start_info.cb = sizeof(STARTUPINFO);
  start_info.dwFlags = STARTF_USESHOWWINDOW;
  start_info.wShowWindow = SW_HIDE;
  start_info.hStdOutput = out_write;
  start_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  start_info.hStdError = err_write;
  // start_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  start_info.dwFlags |= STARTF_USESTDHANDLES;
  // base::string16 cmdline_writable = cmdline_str;
  // Create the child process.
  PROCESS_INFORMATION temp_process_info = {};
  if (!CreateProcessA(nullptr, (LPSTR)command_line.c_str(), nullptr, nullptr,
                      TRUE, // Handles are inherited.
                      NORMAL_PRIORITY_CLASS, nullptr,
                      startup_dir.empty() ? NULL : startup_dir.c_str(), &start_info,
                      &temp_process_info)) {
    return false;
  }
  win::scope_process_info proc_info(temp_process_info);
  scoped_out_write.close();
  scoped_err_write.close();
  const int kBufferSize = 1024;
  char buffer[kBufferSize];
  for (;;) {
    DWORD bytes_read = 0;
    BOOL success = ReadFile(out_read, buffer, kBufferSize, &bytes_read, nullptr);
    if (!success || bytes_read == 0)
      break;
    _stdout.append(buffer, bytes_read);
  }
  for (;;) {
    DWORD bytes_read = 0;
    BOOL success = ReadFile(err_read, buffer, kBufferSize, &bytes_read, nullptr);
    if (!success || bytes_read == 0)
      break;
    _stderr.append(buffer, bytes_read);
  }
  ::WaitForSingleObject(proc_info.proc(), INFINITE);
  DWORD dw_exit_code = 0;
  ::GetExitCodeProcess(proc_info.proc(), &dw_exit_code);
  ret_code = static_cast<int>(dw_exit_code);
  return true;
#else
  ret_code = EXIT_FAILURE;
  int out_fd[2], err_fd[2];
  pid_t pid;
  if (pipe(out_fd) < 0)
    return false;
  nix::scope_fd out_read(out_fd[0]), out_write(out_fd[1]);
  if (pipe(err_fd) < 0)
    return false;
  nix::scope_fd err_read(err_fd[0]), err_write(err_fd[1]);
  if (out_read.get() >= FD_SETSIZE || err_read.get() >= FD_SETSIZE)
    return false;
  string_list argv = split(command_line, " ");
  vector<char*> args;
  if (argv.size() > 0) {
    for (size_t i = 0; i < argv.size(); i++) {
      args.push_back((char*)argv[i].c_str());
    }
  }
  args.push_back(nullptr);
  switch (pid = fork()) {
  case -1: // error
    return false;
  case 0: // child
  {
#if __APPLE__
    ::sigignore(SIGTRAP);
#endif
    int dev_null = open("/dev/null", O_WRONLY);
    if (dev_null < 0)
      _exit(127);
    if (dup2(out_write.get(), STDOUT_FILENO) < 0)
      _exit(127);
    if (dup2(err_write.get(), STDERR_FILENO) < 0)
      _exit(127);
    dup2(dev_null, STDIN_FILENO);

    err_read.reset();
    out_read.reset();
    out_write.reset();
    err_write.reset();

    chdir(startup_dir.c_str());
    int ret = execvp(args[0], args.data());
    _exit(ret);
    break;
  }
  default: {
    out_write.reset();
    err_write.reset();
    bool out_open = true, err_open = true;
    while (out_open || err_open) {
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(out_read.get(), &read_fds);
      FD_SET(err_read.get(), &read_fds);
      int res = -1;
      do {
        res = select(std::max(out_read.get(), err_read.get()) + 1, &read_fds, nullptr, nullptr,
                     nullptr);
      } while (res == -1 && errno == EINTR);
      if (res <= 0) {
        break;
      }
      if (FD_ISSET(out_read.get(), &read_fds))
        out_open = ReadFromPipe(out_read.get(), &_stdout);
      if (FD_ISSET(err_read.get(), &read_fds))
        err_open = ReadFromPipe(err_read.get(), &_stderr);
    }
    return WaitForExit(pid, &ret_code);
  }
  }

  return false;
#endif
}
bool start_command_line_as_su(string const& command_line, string const& startup_dir) {
#if _WIN32
  SHELLEXECUTEINFOA sei = {sizeof(sei)};
  sei.lpVerb = "runas";
  string exec = command_line;
  string args;
  string::size_type arg0_pos = command_line.find(".exe");
  if (arg0_pos != string::npos) {
    if (command_line[0] != '"') {
      exec = command_line.substr(0, arg0_pos + 4);
      if (arg0_pos + 5 < command_line.length()) {
        args = command_line.substr(arg0_pos + 5, command_line.length() - arg0_pos - 5);
      }
    } else {
      exec = command_line.substr(1, arg0_pos + 3);
      if (arg0_pos + 6 < command_line.length()) {
        args = command_line.substr(arg0_pos + 5, command_line.length() - arg0_pos - 5);
      }
    }
  }
  sei.lpFile = exec.c_str();
  sei.lpParameters = args.c_str();
  sei.lpDirectory = startup_dir.c_str();
  sei.hwnd = NULL;
  sei.nShow = SW_NORMAL;
  if (!ShellExecuteExA(&sei)) {
    DWORD dwError = GetLastError();
    if (dwError == ERROR_CANCELLED) {
      // The user refused to allow privileges elevation.
      XB_LOGE("error: User did not allow elevation.");
    }
    return false;
  } else {
    return true;
  }
#endif
  return false;
}
void list_dirs(string const& dir, vector<string>& out_dirs) {
#if _WIN32
  WIN32_FIND_DATAA ffd = {};
  string search_dir = dir;
  if (dir.find_last_of("\\") != dir.length() - 1 || dir.find_last_of("/") != dir.length() - 1) {
    search_dir += "\\*";
  } else {
    search_dir += "*";
  }
  HANDLE hFind = ::FindFirstFileA(search_dir.c_str(), &ffd);
  if (hFind == INVALID_HANDLE_VALUE) {
    return;
  }
  do {
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (strcmp(".", ffd.cFileName) && strcmp("..", ffd.cFileName)) {
        out_dirs.push_back(ffd.cFileName);
      }
    }
  } while (::FindNextFileA(hFind, &ffd) != 0);
  ::FindClose(hFind);
#else

#endif
}
bool exists(string const& file) {
#if _WIN32
  return ::GetFileAttributesA(file.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
  return false;
#endif
}
string get_user_dir() {
#if _WIN32
  // HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/Windows NT/CurrentVersion/ProfileList/[SID]
  // ProfileImagePath
  string ret;
  HANDLE hToken;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
    return ret;
  }
  // TOKEN_USER
  DWORD len = 0;
  ::GetTokenInformation(hToken, TokenUser, NULL, 0, &len);
  if (len > 0) {
    PTOKEN_USER usr = (PTOKEN_USER)calloc(1, len);
    ::GetTokenInformation(hToken, TokenUser, usr, len, &len);
    LPSTR sid = NULL;
    ::ConvertSidToStringSidA(usr->User.Sid, &sid);
    auto key
        = string::format("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", sid);
    LocalFree(sid);
    HKEY hkey = NULL;
    long ret_ext = ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, (PHKEY)&hkey);
    DWORD buflen = 0;
    char buffer[1024] = {0};
    LSTATUS status
        = ::RegQueryValueExA((HKEY)hkey, "ProfileImagePath", NULL, NULL, (LPBYTE)buffer, &buflen);
    status
        = ::RegQueryValueExA((HKEY)hkey, "ProfileImagePath", NULL, NULL, (LPBYTE)buffer, &buflen);
    ::RegCloseKey(hkey);
    return buffer;
  }
  return "";
#else
  return string();
#endif
}
bool is_an_admin() {
#if _WIN32
  BOOL b;
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
  if (b) {
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
      b = FALSE;
    }
    FreeSid(AdministratorsGroup);
  }
  return (b == TRUE);
#else
  return false;
#endif
}
bool launched_from_console() {
#if _WIN32
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  return h != NULL && h != INVALID_HANDLE_VALUE;
#else
  return isatty(STDOUT_FILENO);
#endif
}
int get_console_width() {
  int width = 0;
#if _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  width = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
#else
  // struct winsize max;
  // ioctl(0, TIOCGWINSZ, &max);
  // width = max.ws_col;
#endif
  return width;
}
bool path::exists(string const& path) {
#if _WIN32
  return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
  return ::access(path.c_str(), F_OK) != -1;
#endif
}
string path::file_name(string const& file_path) {
  auto path_sep = file_path.find_last_of("/\\");
  if (path_sep != string::npos) {
    return file_path.substr(path_sep + 1, file_path.length() - path_sep - 1);
  } else {
    return file_path;
  }
}
string path::file_dir(string const& file_path) {
  auto found = file_path.find_last_of("/\\");
  if (found != string::npos) {
    return file_path.substr(0, found);
  } else {
    return ".";
  }
}
string path::file_content_md5(string const& file_path) {
  FILE* fp = fopen(file_path.c_str(), "rb");
  if (fp == nullptr)
    return "";
  MD5 md5;
  string buffer;
  buffer.resize(4 * 1024 * 1024);
  size_t bytes = 0;
  while ((bytes = fread((char*)buffer.data(), 1, buffer.size(), fp)) != 0) {
    md5.update(buffer.data(), bytes);
  }
  fclose(fp);
  return md5.Str();
}
string path::file_string(string const& file_path) {
  FILE* fp = fopen(file_path.c_str(), "r");
  if (fp == nullptr)
    return "";
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  string str;
  str.resize(size, '\0');
  size_t read = fread(str.data(), size, 1, fp);
  // if(read == size);
  fclose(fp);
  return str;
}
string path::file_basename(string const& file_path) {
  auto path_sep = file_path.find_last_of("/\\");
  auto ext_sep = file_path.find_last_of(".");
  if (path_sep != string::npos) {
    if (ext_sep != string::npos && ext_sep > path_sep)
      return file_path.substr(path_sep + 1, ext_sep - path_sep - 1);
    else
      return file_path.substr(path_sep + 1, file_path.length() - path_sep - 1);
  } else {
    if (ext_sep != string::npos && ext_sep > 0)
      return file_path.substr(0, ext_sep);
    else
      return file_path;
  }
}
string path::get_user_dir() {
#if _WIN32
  return ::iris::get_user_dir();
#else
  return string(getenv("HOME"));
#endif
}
/* https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
 */
float get_cpu_overall_usage() {
#if _WIN32
  return win::pdh::getCurrentValue();
#else
  return 0.0f;
#endif
}

void get_sys_mem_info(int64_t& physical_total, int64_t& physical_avail) {
#if _WIN32
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  ::GlobalMemoryStatusEx(&statex);
  physical_total = (int64_t)statex.ullTotalPhys;
  physical_avail = (int64_t)statex.ullAvailPhys;
#else
#endif
}

int64_t get_cpu_frequency() {
#if _WIN32
  LARGE_INTEGER freq = {};
  ::QueryPerformanceFrequency(&freq);
  return freq.QuadPart;
#else
  return 0;
#endif
}

string get_compute_name() {
#if _WIN32
  //::GetComputerNameExA(COMPUTER_NAME_FORMAT::)
  static char buffer[256] = {0};
  DWORD length = 0;
  ::GetComputerNameA(NULL, &length);
  if (length < 256) {
    ::GetComputerNameA(buffer, &length);
    return buffer;
  } else {
    string ret_name;
    ret_name.reserve((size_t)length + 1u);
    ret_name.resize(length);
    ::GetComputerNameA((LPSTR)ret_name.data(), &length);
    return ret_name;
  }
#else
  return "";
#endif
}

#if _WIN32
typedef HRESULT(STDAPICALLTYPE* GetDpiForMonitorProc)(HMONITOR Monitor,
                                                      int DPIType,
                                                      uint32_t* DPIX,
                                                      uint32_t* DPIY);
typedef enum _PROCESS_DPI_AWARENESS {
  PROCESS_DPI_UNAWARE = 0,
  PROCESS_SYSTEM_DPI_AWARE = 1,
  PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef HRESULT(STDAPICALLTYPE* GetProcessDpiAwarenessProc)(HANDLE hProcess,
                                                            PROCESS_DPI_AWARENESS* Value);
static GetDpiForMonitorProc GetDpiForMonitor = nullptr;
#endif
float get_dpi_at_point(int x, int y) {
#if _WIN32
  if (!GetDpiForMonitor) {
    auto shcore = GetModuleHandleA("shcore.dll");
    GetDpiForMonitor = (GetDpiForMonitorProc)GetProcAddress(shcore, "GetDpiForMonitor");
    // GetProcessDpiAwarenessProc GetProcessDpiAwareness
    //     = (GetProcessDpiAwarenessProc)GetProcAddress(shcore, "GetProcessDpiAwareness");
    // PROCESS_DPI_AWARENESS CurrentAwareness = PROCESS_DPI_UNAWARE;
    // GetProcessDpiAwareness(nullptr, &CurrentAwareness);
    /*   (void* User32Dll = FPlatformProcess::GetDllHandle(TEXT("user32.dll"))) {
         typedef BOOL(WINAPI * SetProcessDpiAwareProc)(void);
         SetProcessDpiAwareProc SetProcessDpiAware
             = (SetProcessDpiAwareProc)FPlatformProcess::GetDllExport(User32Dll,
                                                                      TEXT("SetProcessDPIAware"));*/
  }
#endif
  float scale = 1.0f;
#if _WIN32
  if (GetDpiForMonitor) {
    POINT Position = {static_cast<LONG>(x), static_cast<LONG>(y)};
    HMONITOR Monitor = MonitorFromPoint(Position, MONITOR_DEFAULTTONEAREST);
    if (Monitor) {
      uint32_t DPIX = 0;
      uint32_t DPIY = 0;
      if (SUCCEEDED(GetDpiForMonitor(Monitor, 0 /*MDT_EFFECTIVE_DPI*/, &DPIX, &DPIY))) {
        scale = (float)DPIX / 96.0f;
      }
    }
  } else {
    HDC Context = GetDC(nullptr);
    int DPI = GetDeviceCaps(Context, LOGPIXELSX);
    scale = (float)DPI / 96.0f;
    ReleaseDC(nullptr, Context);
  }
#endif
  return scale;
}

string_list env_pathes() {
  string_list pathes;
#if _WIN32
  // DWORD SearchPathA(LPCSTR lpPath, LPCSTR lpFileName, LPCSTR lpExtension, DWORD nBufferLength,
  //                  LPSTR lpBuffer, LPSTR * lpFilePart);
  char buff[2048] = {0};
  // size_t length_ = 0;
  // getenv_s(&length_, buff, "Path");
  LPSTR ret = nullptr;
  ::SearchPathA(NULL, "ispc.exe", ".exe", 2048, buff, &ret);
  string path_buffer;
  path_buffer.resize(4096);
  DWORD length
      = ::GetEnvironmentVariableA("PATH", (LPSTR)path_buffer.data(), (DWORD)path_buffer.size());
  if (length < 4096) {
    auto sep = path_buffer.find(";");
    string section = path_buffer;
    while (sep != string::npos) {
      std::string path = section.substr(0, sep);
      pathes.push_back(path);
      section = section.substr(sep + 1, section.length() - sep - 1);
      sep = section.find_first_of(';');
    }
  }
#endif
  return pathes;
}

string_list path::list_dirs(string const& dir) {
  string_list out_dirs;
#if _WIN32
  WIN32_FIND_DATAA ffd = {};
  string search_dir = dir;
  if (dir.find_last_of("\\") != dir.length() - 1 || dir.find_last_of("/") != dir.length() - 1) {
    search_dir += "\\*";
  } else {
    search_dir += "*";
  }
  HANDLE hFind = FindFirstFileA(search_dir.c_str(), &ffd);
  if (hFind == INVALID_HANDLE_VALUE) {
    return string_list();
  }
  do {
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (strcmp(".", ffd.cFileName) && strcmp("..", ffd.cFileName)) {
        out_dirs.push_back(ffd.cFileName);
      }
    }
  } while (FindNextFileA(hFind, &ffd) != 0);
  FindClose(hFind);
#else
  DIR* dir_;
  struct dirent* ent_;
  string s_dir = dir + "/";
  if ((dir_ = opendir(s_dir.c_str())) != NULL) {
    while ((ent_ = readdir(dir_)) != NULL) {
      if (ent_->d_type == DT_DIR && strcmp(".", ent_->d_name) && strcmp("..", ent_->d_name)) {
        out_dirs.push_back(ent_->d_name);
      }
    }
    closedir(dir_);
  }
#endif
  return out_dirs;
}

template <typename T>
struct mat {
  mat(size_t _w = 0, size_t _h = 0) : w(_w), h(_h) { data.resize(w * h, T()); }

  void set_all(const T& v) {
    for (T& d : data) {
      d = v;
    }
  }

  const T& get(size_t x, size_t y) const {
    assert(x < w && y < h);
    return data[y * w + x];
  }

  T& get(size_t x, size_t y) {
    assert(x < w && y < h);
    return data[y * w + x];
  }

  vector<T> data;
  size_t w;
  size_t h;
};

// ** pattern match
bool str_star_pat_match(const string& str, const string& pat) {
  if (str.length() == 0 || pat.length() == 0)
    return false;
  if (pat.find("*") == string::npos)
    return str == pat;
  mat<int> res(pat.length() + 1, str.length() + 1);
  res.set_all(0);
  res.get(0, 0) = true;
  for (size_t pi = 0; pi < pat.length(); pi++) {
    if (pat[pi] == '*') {
      res.get(pi + 1, 0) = true;
    } else {
      break;
    }
  }
  for (size_t pi = 0; pi < pat.length(); pi++) {
    char p = pat[pi];
    for (size_t si = 0; si < str.length(); si++) {
      char s = str[si];
      if (p == '*') {
        if (res.get(pi, si) || res.get(pi, si + 1) || res.get(pi + 1, si)) {
          res.get(pi + 1, si + 1) = true;
        } else {
          res.get(pi + 1, si + 1) = false;
        }
        continue;
      } else if (p == s) {
        res.get(pi + 1, si + 1) = res.get(pi, si);
        continue;
      }
      res.get(pi + 1, si + 1) = false;
    }
  }
  return res.get(pat.length(), str.length());
}

string_list path::list_files(string const& dir, bool has_pattern) {
  string_list out_files;
#if _WIN32
  WIN32_FIND_DATAA ffd = {};
  string search_dir = dir;
  if (!has_pattern) {
    if (dir.find_last_of("\\") != dir.length() - 1 || dir.find_last_of("/") != dir.length() - 1) {
      search_dir += "\\*";
    } else {
      search_dir += "*";
    }
  }
  HANDLE hFind = FindFirstFileA(search_dir.c_str(), &ffd);
  if (hFind == INVALID_HANDLE_VALUE) {
    return string_list();
  }
  do {
    if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
        || (ffd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)) {
      if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        out_files.push_back(ffd.cFileName);
      }
    }
  } while (FindNextFileA(hFind, &ffd) != 0);
  FindClose(hFind);
#else
  DIR* cdir = nullptr;
  dirent* cptr = nullptr;
  string pdir = dir;
  string patt;
  if (has_pattern) {
    auto ppos = pdir.find("*");
    if (ppos != string::npos) {
      pdir = pdir.substr(0, ppos);
      patt = dir.substr(ppos, dir.length() - ppos);
    }
  }
  /*if(!has_pattern)*/ {
    if ((cdir = ::opendir(pdir.c_str())) == nullptr) {
      return out_files;
    }
    while ((cptr = ::readdir(cdir)) != nullptr) {
      if (!str_star_pat_match(string(cptr->d_name), patt))
        continue;
      if (::strcmp(cptr->d_name, ".") == 0 || ::strcmp(cptr->d_name, "..") == 0) {
        continue;
      } else if (cptr->d_type == DT_REG) { // file
        out_files.push_back(cptr->d_name);
      } else if (cptr->d_type == DT_LNK) { // link file
      } else if (cptr->d_type == DT_DIR) { // dir
      }
    }
    ::closedir(cdir);
  }
#endif
  return out_files;
}
string_list path::list_files_by_ext(string const& dir, string const& ext, bool recurse) {
  string_list out_files;
  queue<string> search_dirs;
  set<string> hist_dirs;
  search_dirs.push(dir);
  while (!search_dirs.empty()) {
    string cur_dir = search_dirs.front();
    search_dirs.pop();
    auto ret_files = list_files(cur_dir, false);
    for (auto& file : ret_files) {
      if (file.rfind(ext) == (file.length() - ext.length()) || ext.empty()) {
        out_files.push_back(path::join(cur_dir, file));
      }
    }
    if (recurse) {
      auto ret_dirs = list_dirs(cur_dir);
      hist_dirs.insert(cur_dir);
      for (auto& dir : ret_dirs) {
        auto next_dir = path::join(cur_dir, dir);
        if (hist_dirs.find(next_dir) == hist_dirs.end()) {
          search_dirs.push(next_dir);
        }
      }
    }
  }
  return out_files;
}
bool path::file_copy(const string& src, const string& dest) {
#if _WIN32
  return TRUE == ::CopyFileA(src.c_str(), dest.c_str(), FALSE);
#else
  return false;
#endif
}
bool path::file_remove(const string& file) {
#if _WIN32
  return TRUE == ::DeleteFileA(file.c_str());
#else
  return ::unlink(file.c_str()) == 0;
#endif
}
bool path::is_absolute(string const& path) {
#if _WIN32
  if (path.length() >= 2) {
    return path[1] == ':' && isalpha(path[0]);
  }
  return false;
#else
  if (path.length() >= 1) {
    return path[0] == '/';
  }
  return false;
#endif
}
bool path::make(string const& dir) {
  string::size_type pos = 0;
  string make_dir = dir;
  do {
    pos = make_dir.find_first_of("\\/", pos + 1);
    string cur_dir = make_dir.substr(0, pos);
#if _WIN32
    if (cur_dir.endswith(":") || exists(cur_dir))
      continue;
    BOOL ret = CreateDirectoryA(cur_dir.c_str(), NULL);
    if (ret == FALSE)
      return false;
#else
    if (!exists(cur_dir)) {
      int ret = mkdir(cur_dir.c_str(), S_IRWXU);
      if (ret == -1)
        return false;
    }
#endif
  } while (pos != string::npos);
  return true;
}

static bool path_end_slash(string const& p) {
  return p.back() == '/' || p.back() == '\\';
}

static string path_strip_end_slash(string const& p) {
  if (p.length() > 1 && path_end_slash(p)) {
    return p.substr(0, p.length() - 1);
  }
  return p;
}

static bool path_start_with_cur_dir(string const& p) {
  if (p.length() >= 2) {
    string prefix = p.substr(0, 2);
    return prefix == ".\\" || prefix == "./";
  }
  return false;
}

static string path_strip_start_cur_dir(string const& p) {
  if (path_start_with_cur_dir(p)) {
    if (p.length() > 2) {
      return p.substr(2, p.length() - 2);
    } else {
      return "";
    }
  }
  return p;
}

static string path_strip_slash_and_curdir(string const& p) {
  return path_strip_end_slash(path_strip_start_cur_dir(p));
}

string path::relative_to(string const& dir, string const& dest_dir) {
  string result;
  string _dir = path_strip_end_slash(dir);
  string _dest_dir = path_strip_end_slash(dest_dir);
  replace(_dir.begin(), _dir.end(), '\\', '/');
  replace(_dest_dir.begin(), _dest_dir.end(), '\\', '/');
  string::size_type sec = string::npos;
  string::size_type check_len = dir.length() > dest_dir.length() ? dest_dir.length() : dir.length();
  for (sec = 0; sec < check_len; sec++) {
    if (_dest_dir[sec] != _dir[sec]) {
      break;
    }
  }
  if (sec == dest_dir.length()) {
    if (_dir[sec] == '/') {
      result = _dir.substr(sec + 1, _dir.length() - sec - 1);
    } else {
      sec = _dest_dir.find_last_of("/");
      if (sec != string::npos) {
        string sec_dir = _dir.substr(sec + 1, _dir.length() - sec - 1);
        result = path::join("..", sec_dir);
      } else // dest dir is "c:"
      {
#if _WIN32
        result = _dir.substr(2, _dir.length() - 2);
#else
        result = "";
#endif
      }
    }
  } else {
    string postfix = _dir.substr(sec, _dir.length() - sec);
    string path;
    string remain = _dest_dir.substr(sec, _dest_dir.length() - sec);
    string::size_type pos = 0;
    string::size_type poss = 0;
    do {
      pos = remain.find('/', poss);
      poss = pos + 1;
      path = path::join(path, "..");
    } while (pos != string::npos);
    result = path::join(path, postfix);
  }
  //#if _WIN32
  //  replace(result.begin(), result.end(), '/', '\\');
  //#endif
  return result;
}

string path::join(string const& a, string const& b, const string& c) {
  string na = path_strip_slash_and_curdir(a);
  string nb = path_strip_slash_and_curdir(b);
  string nc = path_strip_slash_and_curdir(c);
  string ret_path;
  if (!na.empty()) {
    ret_path += na;
  }
  if (!nb.empty()) {
    if (!ret_path.empty()) {
      ret_path += "/";
      ret_path += nb;
    } else {
      ret_path += nb;
    }
  }
  if (!nc.empty()) {
    if (!ret_path.empty()) {
      ret_path += "/";
      ret_path += nc;
    } else {
      ret_path += nc;
    }
  }
  replace(ret_path.begin(), ret_path.end(), '\\', '/');
  // resolve ..
  string::size_type ppos = string::npos;
  do {
    ppos = ret_path.find("..");
    if (ppos == 0)
      break;
    if (ppos != string::npos) {
      string second = ret_path.substr(ppos + 2, ret_path.length() - ppos - 2);
      string::size_type fpos = ret_path.rfind("/", ppos - 2);
      if (fpos != string::npos) {
        string first = ret_path.substr(0, fpos);
        ret_path = first + second;
      }
    }
  } while (ppos != string::npos);
  return ret_path;
}
string path::current_executable() {
#if _WIN32
  HMODULE hModule = GetModuleHandleA(NULL);
  CHAR path[MAX_PATH];
  GetModuleFileNameA(hModule, path, MAX_PATH);
  return path;
#else
  return string();
#endif
}
string path::current_dir() {
#if _WIN32
  DWORD length = ::GetCurrentDirectoryA(0, nullptr);
  string ret;
  ret.reserve(length);
  ret.resize(length - 1);
  GetCurrentDirectoryA(length, ret.data());
  return ret;
#else
  return "";
#endif
}
string path::program_dir() {
  string file_path = current_executable();
  return file_dir(file_path);
}
uint64_t path::get_file_timestamp(string const& file) {
  struct stat result;
  if (stat(file.c_str(), &result) == 0) {
    return result.st_mtime;
  }
  return 0;
}

struct env_impl {
  env_impl(bool copy = true)
#if _WIN32
    : holding_vars(NULL),
      new_vars(NULL),
      var_size(0)
#else
    : var_size(0)
#endif
  {
#if _WIN32
    new_vars = NULL;
    /*
        Var1=Value1\0
        Var2=Value2\0
        Var3=Value3\0
        VarN=ValueN\0\0
    */
    holding_vars = NULL;
    if (copy) {
      holding_vars = ::GetEnvironmentStrings();
      char* line = holding_vars;
      do {
        size_t len = strlen(line);
        if (len == 0)
          break;
        string p_line(line);
        auto pos = p_line.find_first_of("=");
        string key = p_line.substr(0, pos);
        string val = p_line.substr(pos + 1, p_line.length() - pos - 1);
        vars[key] = val;
        line = line + len + 1;
      } while (true);
      assign_to_wenv();
    }
    ::FreeEnvironmentStringsA(holding_vars);
    holding_vars = NULL;
#endif
  }

  env_impl(const wchar_t* strs) 
#if _WIN32
    : holding_vars(NULL),
      new_vars(NULL),
      var_size(0)
#else
    : var_size(0)
#endif
  {
#if _WIN32
    holding_vars = NULL;
    const wchar_t* line = strs;
    do {
      size_t len = wcslen(line);
      if (len == 0)
        break;
      std::wstring p_line(line);
      auto pos = p_line.find_first_of(L"=");
      string key = to_utf8(p_line.substr(0, pos));
      string val = to_utf8(p_line.substr(pos + 1, p_line.length() - pos - 1));
      vars[key] = val;
      line = line + len + 1;
    } while (true);
#endif
    assign_to_wenv();
  }

  ~env_impl() {
#if _WIN32
    if (new_vars) {
      delete[] new_vars;
      new_vars = nullptr;
    }
#endif
  }

  void* data() const {
#if _WIN32
    return new_vars;
#else
    return NULL;
#endif
  }

  size_t size() const {
    return var_size;
  }

  void update(string const& key, string const& value) {
    vars[key] = value;
    assign_to_wenv();
  }

  void assign_to_wenv() {
#if _WIN32
    size_t char_count = 0;
    vars.erase("");
    for (auto& p : vars) {
      char_count += p.first.length() + 1 /*=*/ + p.second.length() + 1 /*\0*/;
    }
    char_count += 1;
    if (new_vars) {
      delete[] new_vars;
    }
    new_vars = new wchar_t[char_count];
    var_size = char_count * 2;
    wchar_t* ptr = new_vars;
    for (auto& p : vars) {
      wstring key = to_utf16(p.first);
      wstring val = to_utf16(p.second);
      wstring line = key + L"=" + val;
      wcscpy_s(ptr, (int)line.length() + 1, line.c_str());
      ptr += line.length() + 1;
      *(ptr) = L'\0';
    }
    *(ptr) = L'\0';
#endif
  }

#if _WIN32
  LPCH holding_vars;
  LPWCH new_vars;
#endif
  size_t var_size;
  map<string, string> vars;
};

sub_process::env::env(bool inherit_from_parent) {
  m_env_handle = new env_impl(inherit_from_parent);
}

sub_process::env::env(const wchar_t* strs) {
  m_env_handle = new env_impl(strs);
}

sub_process::env::~env() {
  if (m_env_handle) {
    auto h = (env_impl*)m_env_handle;
    delete h;
    m_env_handle = nullptr;
  }
}
void* sub_process::env::data() const {
  auto h = (env_impl*)m_env_handle;
  return h->data();
}
size_t sub_process::env::size() const {
  auto h = (env_impl*)m_env_handle;
  return h->size();
}
void sub_process::env::update(string const& key, string const& val) {
  auto h = (env_impl*)m_env_handle;
  h->update(key, val);
}

class sub_process_impl {
public:
  sub_process_impl(bool console, void* usr_data)
    : m_console(console),
      m_exit_code(0),
      m_data(usr_data)
#if _WIN32
      ,
      m_child(NULL),
      m_pipe(NULL)
#else
      ,
      m_fd(-1),
      m_pid(-1)
#endif
  {
  }
  ~sub_process_impl();

  bool start(sub_process_set* set,
             const string& cmdline,
             string_list const& hookdlls,
             void*,
             const sub_process::env* env);
  bool done() const;
  sub_process::status finish();
  void on_pipe_ready() {
#if _WIN32
    DWORD bytes;
    if (!GetOverlappedResult(m_pipe, &m_overlapped, &bytes, TRUE)) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        CloseHandle(m_pipe);
        m_pipe = NULL;
        return;
      }
      XB_LOGE("GetOverlappedResult");
    }

    if (m_is_reading && bytes)
      m_buf.append(m_overlappedbuf, bytes);

    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_is_reading = true;
    if (!::ReadFile(m_pipe, m_overlappedbuf, sizeof(m_overlappedbuf), &bytes, &m_overlapped)) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        CloseHandle(m_pipe);
        m_pipe = NULL;
        return;
      }
      if (GetLastError() != ERROR_IO_PENDING)
        XB_LOGE("ReadFile");
    }
#else
    char buf[4 << 10];
    ssize_t len = read(m_fd, buf, sizeof(buf));
    if (len > 0) {
      m_buf.append(buf, len);
    } else {
      if (len < 0)
        XB_LOGE("read: %s", strerror(errno));
      close(m_fd);
      m_fd = -1;
    }
#endif
  }
  bool m_console;
  string m_buf;
  string m_cmd;
  int m_exit_code;
  void* m_data;
#if _WIN32
  HANDLE setup_pipe(HANDLE ioport, void*);

  HANDLE m_child;
  HANDLE m_pipe;
  OVERLAPPED m_overlapped;
  char m_overlappedbuf[4 << 10];
  bool m_is_reading;
#else
  int m_fd;
  pid_t m_pid;
#endif
};

sub_process::sub_process(bool console, void* usr_data)
  : m_d(make_unique<sub_process_impl>(console, usr_data)) {}

sub_process::~sub_process() {}

bool sub_process::start(struct sub_process_set* set,
                        const string& cmd,
                        string_list const& hookdlls,
                        const env* pe) {
  return m_d->start(set, cmd, hookdlls, this, pe);
}

void sub_process::on_pipe_ready() {
  m_d->on_pipe_ready();
}

sub_process::status sub_process::finish() {
  return m_d->finish();
}

int iris::sub_process::exit_code() const {
  return m_d->m_exit_code;
}

bool sub_process::done() const {
  return m_d->done();
}

void* sub_process::data() const {
  return m_d->m_data;
}

const string& sub_process::get_output() {
  return m_d->m_buf;
}

const string& sub_process::get_cmd() const {
  return m_d->m_cmd;
}

struct sub_process_set_impl {
  vector<sub_process*> m_running;
  queue<sub_process*> m_finished;
  int running_limit = -1;

#ifdef _WIN32
  static BOOL WINAPI notify_interrupted(DWORD dwCtrlType);
  HANDLE m_ioport;
  static unordered_map<sub_process_set_impl*, HANDLE> set_ports;
#else
  static void set_interrupted_flag(int signum);
  static void handle_pending_interrupted();
  /// Store the signal number that causes the interruption.
  /// 0 if not interruption.
  static int s_interrupted;

  static bool is_interrupted() {
    return s_interrupted != 0;
  }

  struct sigaction m_old_int_act;
  struct sigaction m_old_term_act;
  struct sigaction m_old_hup_act;
  sigset_t m_old_mask;
#endif
  void clear();
  bool do_work();

  sub_process_set_impl()
#if _WIN32
    : m_ioport(INVALID_HANDLE_VALUE)
#endif
  {
#if _WIN32
    m_ioport = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    if (!m_ioport)
      XB_LOGE("sub_process_set_impl::CreateIoCompletionPort failed.");
    set_ports.insert({this, m_ioport});
    if (!SetConsoleCtrlHandler(notify_interrupted, TRUE))
      XB_LOGE("sub_process_set_impl::SetConsoleCtrlHandler failed");
#else
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &set, &m_old_mask) < 0)
      XB_LOGE("sigprocmask: %s", strerror(errno));

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = set_interrupted_flag;
    if (sigaction(SIGINT, &act, &m_old_int_act) < 0)
      XB_LOGE("sigaction: %s", strerror(errno));
    if (sigaction(SIGTERM, &act, &m_old_term_act) < 0)
      XB_LOGE("sigaction: %s", strerror(errno));
    if (sigaction(SIGHUP, &act, &m_old_hup_act) < 0)
      XB_LOGE("sigaction: %s", strerror(errno));
#endif
  }

  ~sub_process_set_impl() {
#if _WIN32
    clear();
    // SetConsoleCtrlHandler(notify_interrupted, FALSE);
    if (m_ioport != INVALID_HANDLE_VALUE) {
      set_ports.erase(this);
      CloseHandle(m_ioport);
      m_ioport = INVALID_HANDLE_VALUE;
    }
#else
    if (sigaction(SIGINT, &m_old_int_act, 0) < 0)
      XB_LOGE("sigaction: %s", strerror(errno));
    if (sigaction(SIGTERM, &m_old_term_act, 0) < 0)
      XB_LOGE("sigaction: %s", strerror(errno));
    if (sigaction(SIGHUP, &m_old_hup_act, 0) < 0)
      XB_LOGE("sigaction: %s", strerror(errno));
    if (sigprocmask(SIG_SETMASK, &m_old_mask, 0) < 0)
      XB_LOGE("sigprocmask: %s", strerror(errno));
#endif
  }
};

sub_process_impl::~sub_process_impl() {
#ifndef _WIN32
  if (m_fd >= 0)
    close(m_fd);
  if (m_fd != -1)
    finish();
#endif
}

bool sub_process_impl::start(iris::sub_process_set* set,
                             const string& cmdline,
                             string_list const& hookdlls,
                             void* arg,
                             const sub_process::env* env) {
  m_cmd = cmdline;
#if _WIN32
  HANDLE child_pipe = setup_pipe(set->m_d->m_ioport, arg);

  SECURITY_ATTRIBUTES security_attributes;
  memset(&security_attributes, 0, sizeof(SECURITY_ATTRIBUTES));
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = TRUE;
  HANDLE nul
      = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    &security_attributes, OPEN_EXISTING, 0, NULL);
  if (nul == INVALID_HANDLE_VALUE)
    XB_LOGE("couldn't open nul");

  STARTUPINFOW startup_info;
  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(STARTUPINFOW);
  if (!m_console) {
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput = nul;
    startup_info.wShowWindow = SW_HIDE;
    startup_info.hStdOutput = child_pipe;
    startup_info.hStdError = child_pipe;
  }
  PROCESS_INFORMATION process_info;
  memset(&process_info, 0, sizeof(process_info));
  auto wcmd = to_utf16(cmdline);
  // ctrl-c, except for subprocesses in console pools.
  DWORD process_flags = m_console ? 0 : (CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW);
  if (env) {
    process_flags |= CREATE_UNICODE_ENVIRONMENT;
  }
  if (hookdlls.empty()) {
    // only support 8191 chars
    if (!CreateProcessW(NULL, (LPWSTR)wcmd.c_str(), NULL, NULL,
                        /* inherit handles */ TRUE, process_flags,
                        /*current environment*/ env ? env->data() : NULL, /*current dir*/ NULL,
                        &startup_info, &process_info)) {
      DWORD error = GetLastError();
      if (error == ERROR_FILE_NOT_FOUND) {
        if (child_pipe)
          CloseHandle(child_pipe);
        CloseHandle(m_pipe);
        CloseHandle(nul);
        m_pipe = NULL;
        m_buf
            = "CreateProcess failed: The system cannot find the file "
              "specified.\n";
        return true;
      } else {
        XB_LOGE("CreateProcess (%s) failed.", cmdline.c_str());
      }
    }
  } else {
    if (!DetourCreateProcessWithDllW(NULL, (LPWSTR)wcmd.c_str(), NULL, NULL,
                                     /* inherit handles */ TRUE, process_flags,
                                     env ? env->data() : /*current environment*/ NULL,
                                     /*current dir*/ NULL, &startup_info, &process_info,
                                     hookdlls[0].c_str(), NULL)) {
      DWORD error = GetLastError();
      if (error == ERROR_FILE_NOT_FOUND) {
        if (child_pipe)
          CloseHandle(child_pipe);
        CloseHandle(m_pipe);
        CloseHandle(nul);
        m_pipe = NULL;
        m_buf
            = "CreateProcessWithHookDLL failed: The system cannot find the file "
              "specified.\n";
        return true;
      } else {
        if (error == ERROR_INVALID_HANDLE) {
          XB_LOGE("CreateProcess (%s) failed: invalid handle!", cmdline.c_str());
        } else {
          XB_LOGE("CreateProcess (%s) failed.", cmdline.c_str());
        }
      }
    }
  }
  if (child_pipe)
    CloseHandle(child_pipe);
  CloseHandle(nul);

  CloseHandle(process_info.hThread);
  m_child = process_info.hProcess;

  return true;
#else
  int output_pipe[2];
  if (pipe(output_pipe) < 0) {
    XB_LOGE("pipe: %s", strerror(errno));
  }
  m_fd = output_pipe[0];
  posix_spawn_file_actions_t action;
  if (posix_spawn_file_actions_init(&action) != 0)
    XB_LOGE("posix_spawn_file_actions_init: %s", strerror(errno));

  if (posix_spawn_file_actions_addclose(&action, output_pipe[0]) != 0)
    XB_LOGE("posix_spawn_file_actions_addclose: %s", strerror(errno));

  posix_spawnattr_t attr;
  if (posix_spawnattr_init(&attr) != 0)
    XB_LOGE("posix_spawnattr_init: %s", strerror(errno));

  short flags = 0;

  flags |= POSIX_SPAWN_SETSIGMASK;
  if (posix_spawnattr_setsigmask(&attr, &set->m_d->m_old_mask) != 0)
    XB_LOGE("posix_spawnattr_setsigmask: %s", strerror(errno));
  if (!m_console) {
    flags |= POSIX_SPAWN_SETPGROUP;
    if (posix_spawn_file_actions_addopen(&action, 0, "/dev/null", O_RDONLY, 0) != 0) {
      XB_LOGE("posix_spawn_file_actions_addopen: %s", strerror(errno));
    }

    if (posix_spawn_file_actions_adddup2(&action, output_pipe[1], 1) != 0)
      XB_LOGE("posix_spawn_file_actions_adddup2: %s", strerror(errno));
    if (posix_spawn_file_actions_adddup2(&action, output_pipe[1], 2) != 0)
      XB_LOGE("posix_spawn_file_actions_adddup2: %s", strerror(errno));
    if (posix_spawn_file_actions_addclose(&action, output_pipe[1]) != 0)
      XB_LOGE("posix_spawn_file_actions_addclose: %s", strerror(errno));
  }
#ifdef POSIX_SPAWN_USEVFORK
  flags |= POSIX_SPAWN_USEVFORK;
#endif

  if (posix_spawnattr_setflags(&attr, flags) != 0)
    XB_LOGE("posix_spawnattr_setflags: %s", strerror(errno));

  const char* spawned_args[] = {"/bin/sh", "-c", cmdline.c_str(), NULL};
  if (posix_spawn(&m_pid, "/bin/sh", &action, &attr, const_cast<char**>(spawned_args), environ)
      != 0)
    XB_LOGE("posix_spawn: %s", strerror(errno));

  if (posix_spawnattr_destroy(&attr) != 0)
    XB_LOGE("posix_spawnattr_destroy: %s", strerror(errno));
  if (posix_spawn_file_actions_destroy(&action) != 0)
    XB_LOGE("posix_spawn_file_actions_destroy: %s", strerror(errno));

  close(output_pipe[1]);
  return true;
#endif
}

bool sub_process_impl::done() const {
#if _WIN32
  return m_pipe == NULL;
#else
  return m_fd == -1;
#endif
}

sub_process::status sub_process_impl::finish() {
#if _WIN32
  if (!m_child)
    return sub_process::exit_failure;
  WaitForSingleObject(m_child, INFINITE);
  DWORD exit_code = 0;
  GetExitCodeProcess(m_child, &exit_code);
  m_exit_code = (int)exit_code;
  CloseHandle(m_child);
  m_child = NULL;
  return exit_code == 0                ? sub_process::exit_success
         : exit_code == CONTROL_C_EXIT ? sub_process::exit_interrupted
                                       : sub_process::exit_failure;
#else
  assert(m_pid != -1);
  int status;
  if (waitpid(m_pid, &status, 0) < 0)
    XB_LOGE("waitpid(%d): %s", m_pid, strerror(errno));
  m_pid = -1;

  if (WIFEXITED(status)) {
    int exit = WEXITSTATUS(status);
    if (exit == 0)
      return sub_process::exit_success;
  } else if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGINT || WTERMSIG(status) == SIGTERM || WTERMSIG(status) == SIGHUP)
      return sub_process::exit_interrupted;
  }
  return sub_process::exit_failure;
#endif
}

#if _WIN32

unordered_map<sub_process_set_impl*, HANDLE> sub_process_set_impl::set_ports;

BOOL sub_process_set_impl::notify_interrupted(DWORD dwCtrlType) {
  if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
    for (auto sp : set_ports) {
      if (!PostQueuedCompletionStatus(sp.first->m_ioport, 0, 0, NULL))
        XB_LOGE("PostQueuedCompletionStatus");
      set_ports.erase(sp.first);
    }
    return TRUE;
  }

  return FALSE;
}
HANDLE sub_process_impl::setup_pipe(HANDLE ioport, void* arg) {
  char pipe_name[100];
  snprintf(pipe_name, sizeof(pipe_name), "\\\\.\\pipe\\irisbuild_pid%lu_sp%p",
           GetCurrentProcessId(), arg);

  m_pipe = ::CreateNamedPipeA(pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE,
                              PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, NULL);
  if (m_pipe == INVALID_HANDLE_VALUE)
    XB_LOGE("CreateNamedPipe");

  if (!CreateIoCompletionPort(m_pipe, ioport, (ULONG_PTR)arg, 0))
    XB_LOGE("CreateIoCompletionPort");

  memset(&m_overlapped, 0, sizeof(m_overlapped));
  if (!ConnectNamedPipe(m_pipe, &m_overlapped) && GetLastError() != ERROR_IO_PENDING) {
    XB_LOGE("ConnectNamedPipe");
  }

  // Get the write end of the pipe as a handle inheritable across processes.
  HANDLE output_write_handle
      = CreateFileA(pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  HANDLE output_write_child;
  if (!DuplicateHandle(GetCurrentProcess(), output_write_handle, GetCurrentProcess(),
                       &output_write_child, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
    XB_LOGE("DuplicateHandle");
  }
  CloseHandle(output_write_handle);

  return output_write_child;
}

#else
int sub_process_set_impl::s_interrupted = 0;
void sub_process_set_impl::set_interrupted_flag(int signum) {
  s_interrupted = signum;
}
void sub_process_set_impl::handle_pending_interrupted() {
  sigset_t pending;
  sigemptyset(&pending);
  if (sigpending(&pending) == -1) {
    XB_LOGE("sigpending");
    return;
  }
  if (sigismember(&pending, SIGINT))
    s_interrupted = SIGINT;
  else if (sigismember(&pending, SIGTERM))
    s_interrupted = SIGTERM;
  else if (sigismember(&pending, SIGHUP))
    s_interrupted = SIGHUP;
}
#endif

sub_process_set::sub_process_set() : m_d(make_unique<sub_process_set_impl>()) {}

sub_process_set::~sub_process_set() {}
sub_process* sub_process_set::add(string const& cmdline,
                                  bool use_console,
                                  void* usr_data,
                                  string_list const& hookdlls,
                                  const sub_process::env* penv) {
  sub_process* subprocess = new sub_process(use_console, usr_data);
  if (!subprocess->start(this, cmdline, hookdlls, penv)) {
    delete subprocess;
    return 0;
  }
  on_new_subprocess(subprocess);
  return subprocess;
}
bool sub_process_set::do_work() {
  return m_d->do_work();
}
bool sub_process_set::has_running_procs() const {
  return !m_d->m_running.empty();
}
int sub_process_set::num_running_procs() const {
  return (int)m_d->m_running.size();
}
sub_process* sub_process_set::next_finished() {
  if (m_d->m_finished.empty())
    return nullptr;
  sub_process* subproc = m_d->m_finished.front();
  m_d->m_finished.pop();
  return subproc;
}

void sub_process_set::clear() {
  m_d->clear();
}

void sub_process_set::set_limit(int num_procs) {
  m_d->running_limit = num_procs;
}

void sub_process_set::on_new_subprocess(sub_process* subproc) {
#if _WIN32
  if (subproc->m_d->m_child)
#endif
    m_d->m_running.push_back(subproc);
#if _WIN32
  else
    m_d->m_finished.push(subproc);
#endif
}

void sub_process_set_impl::clear() {
  for (vector<sub_process*>::iterator i = m_running.begin(); i != m_running.end(); ++i) {
#if _WIN32
    if ((*i)->m_d->m_child && !(*i)->m_d->m_console) {
      if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, GetProcessId((*i)->m_d->m_child))) {
        XB_LOGE("sub_process_set_impl::GenerateConsoleCtrlEvent failed.");
      }
    }
#else
    // Since the foreground process is in our process group, it will receive
    // the interruption signal (i.e. SIGINT or SIGTERM) at the same time as us.
    if (!(*i)->m_d->m_console)
      kill(-(*i)->m_d->m_pid, s_interrupted);
#endif
  }

  for (vector<sub_process*>::iterator i = m_running.begin(); i != m_running.end(); ++i)
    delete *i;
  m_running.clear();
}
bool sub_process_set_impl::do_work() {
#if _WIN32
  DWORD bytes_read;
  sub_process* subproc;
  OVERLAPPED* overlapped;

  ULONG_PTR ptr = NULL;
  if (!GetQueuedCompletionStatus(sub_process_set_impl::m_ioport, &bytes_read, &ptr, &overlapped,
                                 INFINITE)) {
    DWORD error = GetLastError();
    if (error != ERROR_BROKEN_PIPE) {
      XB_LOGE("%s:GetQueuedCompletionStatus", __FUNCTION__);
    }
  }
  subproc = (sub_process*)ptr;
  if (!subproc)
    return true;

  subproc->on_pipe_ready();

  if (subproc->done()) {
    vector<sub_process*>::iterator end = remove(m_running.begin(), m_running.end(), subproc);
    if (m_running.end() != end) {
      m_finished.push(subproc);
      m_running.resize(end - m_running.begin());
    }
  }
  return false;
#else
  fd_set set;
  int nfds = 0;
  FD_ZERO(&set);

  for (vector<sub_process*>::iterator i = m_running.begin(); i != m_running.end(); ++i) {
    int fd = (*i)->m_d->m_fd;
    if (fd >= 0) {
      FD_SET(fd, &set);
      if (nfds < fd + 1)
        nfds = fd + 1;
    }
  }

  s_interrupted = 0;
  int ret = pselect(nfds, &set, 0, 0, 0, &m_old_mask);
  if (ret == -1) {
    if (errno != EINTR) {
      perror("iris: pselect");
      return false;
    }
    return is_interrupted();
  }

  handle_pending_interrupted();
  if (is_interrupted())
    return true;

  for (vector<sub_process*>::iterator i = m_running.begin(); i != m_running.end();) {
    int fd = (*i)->m_d->m_fd;
    if (fd >= 0 && FD_ISSET(fd, &set)) {
      (*i)->on_pipe_ready();
      if ((*i)->done()) {
        m_finished.push(*i);
        i = m_running.erase(i);
        continue;
      }
    }
    ++i;
  }

  return is_interrupted();
#endif
}

dynamic_library::dynamic_library(const char* name) {
#if _WIN32
  handle = LoadLibraryA(name);
#endif
}

dynamic_library::~dynamic_library() {
#if _WIN32
  FreeLibrary((HMODULE)handle);
#endif
}

void* dynamic_library::resolve_symbol(const char* symbol) {
#if _WIN32
  return GetProcAddress((HMODULE)handle, symbol);
#else
  return NULL;
#endif
}

size_t get_drive_infos(vector<drive_info>& drives, bool btest_speed) {
  size_t cur = drives.size();
#if _WIN32
  char str_drives[MAX_PATH] = {0};
  char* p_drive = str_drives;
  DWORD size = GetLogicalDriveStringsA(MAX_PATH, str_drives);
  if (size > 0 && size < MAX_PATH) {
    while (*p_drive) {
      switch (GetDriveTypeA(p_drive)) {
      case DRIVE_UNKNOWN:
      case DRIVE_REMOTE:
      case DRIVE_CDROM:
      case DRIVE_NO_ROOT_DIR:
        break;
      case DRIVE_RAMDISK:
      case DRIVE_REMOVABLE:
      case DRIVE_FIXED: {
        drive_info info;
        info.name = p_drive;
        ULARGE_INTEGER avail, total, free_space;
        if (GetDiskFreeSpaceExA(p_drive, &avail, &total, &free_space)) {
          info.total_space = total.QuadPart;
          info.available_space = avail.QuadPart;
          drives.push_back(info);
        }
        break;
      }
      }
      p_drive += strlen(p_drive) + 1;
    }
  }
#endif
  size_t num = drives.size();
  auto test_speed = [](const string& vol, int64_t& write_speed, int64_t& read_speed) {
    write_speed = 0;
    read_speed = 0;
#if _WIN32
    // write 100MB test
    const size_t sz_data = 1024 * 1024 * 100;
    vector<uint8_t> data(sz_data);
    auto file_path = vol + "testFile.x";
    if (vol == "C:\\") {
      auto usr = get_user_dir();
      if (!usr.empty() && usr[0] == 'C') {
        file_path = usr + "\\testFile.x";
      }
    }
    HANDLE test_file
        = ::CreateFileA(file_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_NO_BUFFERING,
                        NULL);
    if (test_file == INVALID_HANDLE_VALUE) {
      DWORD ret = GetLastError();
      return false;
    }
    auto start = std::chrono::high_resolution_clock::now();
    DWORD bytes = 0;
    ::WriteFile(test_file, data.data(), (DWORD)data.size(), &bytes, NULL);
    int try_cnt = 0;
    while (bytes < data.size()) {
      DWORD written = 0;
      ::WriteFile(test_file, data.data() + bytes, (DWORD)(data.size() - bytes), &written, NULL);
      if (written == 0) {
        try_cnt++;
      }
      if (try_cnt == 4) {
        break;
      }
      bytes += written;
    }
    FlushFileBuffers(test_file);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);
    write_speed = int64_t(100.0 / ms.count() * 1000.0); // MB/s
    ::CloseHandle(test_file);
    try_cnt = 0;
    // test read speed, might be wrong
    test_file
        = ::CreateFileA(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_NO_BUFFERING,
                        NULL);
    start = std::chrono::high_resolution_clock::now();
    DWORD read = 0;
    ::ReadFile(test_file, data.data(), sz_data, &read, NULL);
    ::FlushFileBuffers(test_file);
    ::CloseHandle(test_file);
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);
    read_speed = int64_t(100.0 / ms.count() * 1000.0); // MB/s
    ::DeleteFileA(file_path.c_str());
#endif
    return true;
  };
  if (btest_speed) {
    for (size_t i = cur; i < num; i++) {
      test_speed(drives[i].name, drives[i].write_speed, drives[i].read_speed);
    }
  }
  return num - cur;
}

static constexpr int sz_print_buf = 8192;
static thread_local char print_buf[sz_print_buf];

string string::to_win_path() const {
  string result = *this;
  std::replace(result.begin(), result.end(), '/', '\\');
  return result;
}

string string::format(const char* fmt, ...) {
  string str;
  va_list args;
  va_start(args, fmt);
  size_t length = ::vsnprintf(print_buf, sz_print_buf, fmt, args);
  if (length > sz_print_buf) {
    str.reserve(length + 1);
    str.resize(length);
    ::vsnprintf(str.data(), length + 1, fmt, args);
  } else {
    str = print_buf;
  }
  va_end(args);
  return str;
}

} // namespace iris
