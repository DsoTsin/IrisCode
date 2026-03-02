#include "anisette.h"
#include <Windows.h>
#include <detours.h>

namespace iris {
struct Dir {
  std::wstring path;
  std::string patha;

  static Dir &p() {
    static Dir d;
    return d;
  }

private:
  Dir() {
    static thread_local wchar_t S_PATH[2048] = {0};
    static thread_local char S_PATHA[2048] = {0};
    DWORD len0 = GetModuleFileNameW(NULL, S_PATH, 2048);
    for (int index = len0 - 1; index > 0; index--) {
      if (S_PATH[index] == L'\\') {
        S_PATH[index + 1] = 0;
        path = S_PATH;
        break;
      }
    }

    DWORD len1 = GetModuleFileNameA(NULL, S_PATHA, 2048);
    for (int index = len1 - 1; index > 0; index--) {
      if (S_PATHA[index] == '\\') {
        S_PATHA[index + 1] = 0;
        patha = S_PATHA;
        break;
      }
    }
  }
};

HANDLE(WINAPI *origin_create_filew)
(_In_ LPCWSTR name, _In_ DWORD access, _In_ DWORD s_mode,
 _In_opt_ LPSECURITY_ATTRIBUTES sec_attribs, _In_ DWORD disp,
 _In_ DWORD flag_attribs, _In_opt_ HANDLE templ) = CreateFileW;
HANDLE WINAPI mine_create_filew(LPCWSTR name, DWORD access, DWORD s_mode,
                                LPSECURITY_ATTRIBUTES sec_attribs, DWORD disp,
                                DWORD flag_attribs, HANDLE templ) {
  std::wstring new_path = name;
  if (auto str = wcsstr(name, L"\\Apple Computer\\iTunes\\adi\\")) {
    auto new_str = str + 28;
    new_path = Dir::p().path + L"ADI\\" + new_str;
  } else if (auto str = wcsstr(name, L"\\AppData\\Roaming\\Apple Computer\\")) {
    auto new_str = str + 32;
    new_path = Dir::p().path + new_str;
  } else {
  }
  return origin_create_filew(new_path.c_str(), access, s_mode, sec_attribs,
                             disp, flag_attribs, templ);
}

DWORD(WINAPI *origin_get_file_attribsa)(LPCSTR lpFileName) = GetFileAttributesA;
DWORD WINAPI mine_get_file_attribsa(LPCSTR name) {
  std::string new_path = name;
  if (auto str = strstr(name, "\\Apple Computer\\iTunes\\adi\\")) {
    auto new_str = str + 28;
    new_path = Dir::p().patha + "ADI\\" + new_str;
  } else if (auto str = strstr(name, "\\AppData\\Roaming\\Apple Computer\\")) {
    auto new_str = str + 32;
    new_path = Dir::p().patha + new_str;
  } else {
  }
  return origin_get_file_attribsa(new_path.c_str());
}

BOOL(WINAPI *origin_set_file_attribsa)(LPCSTR, DWORD) = SetFileAttributesA;
BOOL WINAPI mine_set_file_attribsa(LPCSTR name, DWORD attribs) {
  std::string new_path = name;
  if (auto str = strstr(name, "\\Apple Computer\\iTunes\\adi\\")) {
    auto new_str = str + 28;
    new_path = Dir::p().patha + "ADI\\" + new_str;
  } else if (auto str = strstr(name, "\\AppData\\Roaming\\Apple Computer\\")) {
    auto new_str = str + 32;
    new_path = Dir::p().patha + new_str;
  } else {
  }
  return origin_set_file_attribsa(new_path.c_str(), attribs);
}

BOOL(WINAPI *origin_delete_filew)(LPCWSTR) = DeleteFileW;
BOOL WINAPI mine_delete_filew(LPCWSTR name) {
  std::wstring new_path = name;
  if (auto str = wcsstr(name, L"\\Apple Computer\\iTunes\\adi\\")) {
    auto new_str = str + 28;
    new_path = Dir::p().path + L"ADI\\" + new_str;
  } else if (auto str = wcsstr(name, L"\\AppData\\Roaming\\Apple Computer\\")) {
    auto new_str = str + 32;
    new_path = Dir::p().path + new_str;
  } else {
  }
  return origin_delete_filew(new_path.c_str());
}

void install_hooks(bool begin) {
  if (begin) {
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID &)origin_create_filew, mine_create_filew);
    DetourAttach(&(PVOID &)origin_get_file_attribsa, mine_get_file_attribsa);
    DetourAttach(&(PVOID &)origin_set_file_attribsa, mine_set_file_attribsa);

    /* auto error = */ DetourTransactionCommit();
  } else {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourDetach(&(PVOID &)origin_create_filew, mine_create_filew);
    DetourDetach(&(PVOID &)origin_get_file_attribsa, mine_get_file_attribsa);
    DetourDetach(&(PVOID &)origin_set_file_attribsa, mine_set_file_attribsa);

    /* auto error = */ DetourTransactionCommit();
  }
}

void create_dirs() {
  auto log = Dir::p().patha + "Logs";
  CreateDirectoryA(log.c_str(), NULL);
  auto pref = Dir::p().patha + "Preferences";
  CreateDirectoryA(pref.c_str(), NULL);
  auto adi = Dir::p().patha + "ADI";
  CreateDirectoryA(adi.c_str(), NULL);
}
} // namespace iris