#include "meta7z.h"

//#define _7Z_NO_METHOD_LZMA2 1
#define _7ZIP_PPMD_SUPPPORT 1
#include "C/7zAlloc.c"
#include "C/7zBuf.c"
//#include "C/7zBuf2.c"
#include "C/7zArcIn.c"
#include "C/7zCrc.c"
#include "C/7zCrcOpt.c"
#include "C/7zDec.c"
#include "C/7zFile.c"
#include "C/7zStream.c"
#include "C/Aes.c"
#include "C/AesOpt.c"

#define SzAlloc SzAllocA
#define SzFree SzFreeA
#include "C/Alloc.c"
#undef Print
#undef SzAlloc
#undef SzFree

#undef kTopValue
#include "C/Bcj2.c"
#include "C/Bra.c"
#include "C/Bra86.c"
#include "C/BraIA64.c"
#include "C/CpuArch.c"
#include "C/Delta.c"
#undef MOVE_POS
#define v128 v128_t
#include "C/LzFind.c"
#undef v128
#undef MOVE_POS
#undef RINOK_THREAD
#include "C/LzFindMt.c"
#undef RINOK_THREAD
#include "C/LzFindOpt.c"
#include "C/Lzma2Dec.c"
#undef RINOK_THREAD
#include "C/Lzma2DecMt.c"
#undef RINOK_THREAD
#include "C/Lzma2Enc.c"

#undef kTopValue
#undef kBitModelTotal
#include "C/LzmaDec.c"
#undef kTopValue
#undef kBitModelTotal
#undef RC_NORM
#include "C/LzmaEnc.c"

#undef RINOK_THREAD
#include "C/LzmaLib.c"
#undef RINOK_THREAD
#undef kTopValue
#define ArEvent_OptCreate_And_Reset ArEvent_OptCreate_And_ResetCodec
#define ThreadFunc ThreadFuncCodec
#define FullRead FullReadCodec
#include "C/MtCoder.c"
#undef ArEvent_OptCreate_And_Reset
#undef ThreadFunc
#undef FullRead
#undef kTopValue
#undef RINOK_THREAD
#define ArEvent_OptCreate_And_Reset ArEvent_OptCreate_And_ResetDec
#define ThreadFunc ThreadFuncDec
#define FullRead FullReadDec
#undef PRF
#include "C/MtDec.c"
#undef ArEvent_OptCreate_And_Reset
#undef ThreadFunc
#undef FullRead
#undef RINOK_THREAD
#include "C/Ppmd7.c"
#undef kTopValue
#undef RC_NORM_BASE
#undef R
#undef RC_NORM
#include "C/Ppmd7Dec.c"
#undef kTopValue
#undef CASE_BRA_CONV
#undef RC_NORM_BASE
#undef R
#include "C/Ppmd7Enc.c"
#undef kTopValue
#undef RC_NORM_BASE
#undef R
#include "C/Sha256.c"
#define v128 v128_t
#include "C/Sha256Opt.c"
#undef v128
#include "C/Sort.c"
#undef PRF
#undef Print
#include "C/Threads.c"

#include "C/Xz.c"
#include "C/XzCrc64.c"
#include "C/XzCrc64Opt.c"
#undef PRF
#include "C/XzDec.c"
#undef PRF
#include "C/XzEnc.c"
#include "C/XzIn.c"

#include "common/log.hpp"
#include "common/os.hpp"
#include <regex>

#define kInputBufSize ((size_t)1 << 18)

namespace iris {
static const ISzAlloc g_Alloc = {SzAlloc, SzFree};
struct __archive7zimpl {
  __archive7zimpl(const string& in_path);
  ~__archive7zimpl();

  uint32_t numFiles() const { return db.NumFiles; }
  uint64_t getFileSize(int i) const { return SzArEx_GetFileSize(&db, i); }
  uint32_t getAttrib(int i) const { return db.Attribs.Vals[i]; }
  string getFileName(int i) const;
  bool getFileTimes(int i, archive7z::file_time& create, archive7z::file_time& modify) const;
  bool isdir(int i) const;
  bool extract(const string& dest, int i);

  CFileInStream stream;
  CLookToRead2 lookStream;
  CSzArEx db;
  WRes res;

  ISzAlloc allocImp{g_Alloc};
  ISzAlloc allocTempImp{g_Alloc};

  UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
  Byte* outBuffer = 0;            /* it must be 0 before first call for each new archive. */
  size_t outBufferSize = 0;       /* it can have any value before first call (if outBuffer = 0) */
};

archive7z::archive7z(const string& in_path) : impl_(new __archive7zimpl(in_path)) {}
archive7z::~archive7z() {
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

bool archive7z::is_open() const {
  return impl_->res == SZ_OK;
}

vector<archive7z::entry> archive7z::getFileNames() {
  vector<entry> files;
  for (int i = 0; i < impl_->db.NumFiles; i++) {
    entry e;
    e.isdir = SzArEx_IsDir(&impl_->db, i);
    size_t len = SzArEx_GetFileNameUtf16(&impl_->db, i, NULL);
    vector<UInt16> buffer(len + 1, 0);
    SzArEx_GetFileNameUtf16(&impl_->db, i, buffer.data());
    e.name = to_utf8((wchar_t*)buffer.data());
    files.push_back(e);
  }
  return files;
}

void archive7z::extract(const string& dest, const string& exclude_filter) {
  if (!path::exists(dest)) {
    path::make(dest);
  }
  unique_ptr<regex> efilter;
  if (!exclude_filter.empty()) {
    efilter = make_unique<regex>(exclude_filter);
  }
  auto count = impl_->numFiles();
  for (int i = 0; i < count; i++) {
    auto name = impl_->getFileName(i);
    bool should_extract = true;
    if (efilter) {
      smatch sm;
      std::regex_match(name, sm, *efilter);
      should_extract = sm.size() == 0;
    }
    if (should_extract) {
      auto p = path::join(dest, name);
      if (impl_->isdir(i)) {
        if (!path::exists(p)) {
          path::make(p);
        }
      } else {
        auto dir = path::file_dir(p);
        if (!path::exists(dir)) {
          path::make(dir);
        }
        impl_->extract(p, i);
      }
    }
  } // end foreach

  ISzAlloc_Free(&impl_->allocImp, impl_->outBuffer);
  impl_->outBuffer = nullptr;
  impl_->outBufferSize = 0;
}

__archive7zimpl::__archive7zimpl(const string& in_path) {
  res = InFile_Open(&stream.file, in_path.c_str());
  FileInStream_CreateVTable(&stream);
  stream.wres = 0;
  LookToRead2_CreateVTable(&lookStream, False);
  lookStream.buf = NULL;

  {
    lookStream.buf = (Byte*)ISzAlloc_Alloc(&allocImp, kInputBufSize);
    if (!lookStream.buf)
      res = SZ_ERROR_MEM;
    else {
      lookStream.bufSize = kInputBufSize;
      lookStream.realStream = &stream.vt;
      LookToRead2_Init(&lookStream);
    }
  }

  CrcGenerateTable();
  SzArEx_Init(&db);

  if (res == SZ_OK) {
    res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
  }
}
__archive7zimpl::~__archive7zimpl() {
  // SzFree(NULL, temp);
  SzArEx_Free(&db, &allocImp);
  ISzAlloc_Free(&allocImp, lookStream.buf);
  File_Close(&stream.file);
}

string __archive7zimpl::getFileName(int i) const {
  size_t len = SzArEx_GetFileNameUtf16(&db, i, NULL);
  vector<UInt16> buffer(len + 1, 0);
  SzArEx_GetFileNameUtf16(&db, i, buffer.data());
  return to_utf8((wchar_t*)buffer.data());
}

bool __archive7zimpl::getFileTimes(int i,
                                   archive7z::file_time& create,
                                   archive7z::file_time& modify) const {
  if (SzBitWithVals_Check(&db.CTime, i) && SzBitWithVals_Check(&db.MTime, i)) {
    const CNtfsFileTime* t = &db.MTime.Vals[i];
    modify.low = t->Low;
    modify.high = t->High;
    t = &db.CTime.Vals[i];
    create.low = t->Low;
    create.high = t->High;
    return true;
  }
  return false;
}

bool __archive7zimpl::isdir(int i) const {
  return SzArEx_IsDir(&db, i);
}

bool __archive7zimpl::extract(const string& dest, int index) {
  auto fileSize = getFileSize(index);
  XB_LOGI("Extracting '%s', %lld bytes..", dest.c_str(), fileSize);
#if _WIN32
  auto wpath = to_utf16(dest);
#endif

  size_t offset = 0;
  size_t outSizeProcessed = 0;
  auto res = SzArEx_Extract(&db, &lookStream.vt, index, &blockIndex, &outBuffer, &outBufferSize,
                            &offset, &outSizeProcessed, &allocImp, &allocTempImp);
  if (res == SZ_OK) {
#if _WIN32
    auto fileHandle
        = ::CreateFileW(wpath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;
    ::WriteFile(fileHandle, outBuffer + offset, outSizeProcessed, &written, NULL);

    FILETIME* pfmt = nullptr;
    FILETIME* pfct = nullptr;
    FILETIME fmt = {};
    if (SzBitWithVals_Check(&db.MTime, index)) {
      const CNtfsFileTime* t = &db.MTime.Vals[index];
      fmt.dwLowDateTime = t->Low;
      fmt.dwHighDateTime = t->High;
      pfmt = &fmt;
    }
    FILETIME fct = {};
    if (SzBitWithVals_Check(&db.CTime, index)) {
      auto t = &db.CTime.Vals[index];
      fct.dwLowDateTime = t->Low;
      fct.dwHighDateTime = t->High;
      pfct = &fct;
    }
    ::SetFileTime(fileHandle, pfct ? pfct : pfmt, pfmt, pfmt);
    ::CloseHandle(fileHandle);
#endif
  }

  if (SzBitWithVals_Check(&db.Attribs, index)) {
    UInt32 attrib = db.Attribs.Vals[index];
    /* p7zip stores posix attributes in high 16 bits and adds 0x8000 as marker.
       We remove posix bits, if we detect posix mode field */
    if ((attrib & 0xF0000000) != 0)
      attrib &= 0x7FFF;
#if _WIN32
    if (path::exists(dest)) {
      ::SetFileAttributesW(wpath.c_str(), attrib);
    }
#endif
  }

  return res == SZ_OK;
}

} // namespace iris
