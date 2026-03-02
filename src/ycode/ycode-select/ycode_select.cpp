#include "config.hpp"
#include "download.hpp"
#include "log.hpp"
#include "os.hpp"
#include "zip.hpp"
#include "json/json.h"
#include "json/reader.h"
#include <ext/win32.hpp>
#include <string.h>

#if _WIN32
#include <ShlObj.h>
#include <shellapi.h>
#define Ycode_Reg_Path "reg:\\HKEY_CURRENT_USER\\Software\\Ycode\\"
#define Ycode_Selected_Path_Key "SelectedPath"
#endif

void print_help();
void install_sdk(const char* repo = nullptr);
void select_sdk(const char* path);
void print_sdk_path();
void make_sdk();

int main(int argc, const char* argv[]) {
  switch (argc) {
  case 2: {
    if (!strcmp("-p", argv[1]) || !strcmp("--print-path", argv[1])) {
      print_sdk_path();
    } else if (!strcmp("--install", argv[1]) || !strcmp("-i", argv[1])) {
      install_sdk();
    } else if (!strcmp("--make_sdk", argv[1])) {
      make_sdk();
    } else if (!strcmp("-h", argv[1])) {
      print_help();
    } else {
      print_help();
    }
    break;
  }
  case 3: {
    if (!strcmp("-s", argv[1]) || !strcmp("--select-path", argv[1])) {
      const char* path = argv[2];
      select_sdk(path);
    } else if (!strcmp("--install", argv[1]) || !strcmp("-i", argv[1])) {
      install_sdk(argv[2]);
    } else {
      print_help();
    }
    break;
  }
  case 1:
  default:
    print_help();
    break;
  }
  return 0;
}

void print_help() {
  printf("ycode_select version: 0.0.1\n");
  printf("Usage:\n");
  printf("    ycode_select -h|--help, print help.\n");
  printf("    ycode_select -s|--select-path [path], select sdk path.\n");
  printf("    ycode_select -p|--print-path, print sdk path.\n");
  printf("    ycode_select -i|--install [repo_url], install sdk from repo url.\n");
}

void install_sdk(const char* repo) {
  using namespace iris;
  if (is_an_admin()) {
#if _WIN32
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath))) {
      iris::string _path = szPath;
      auto inst_path = path::join(_path, "Ycode");
      auto dl_dir = path::join(inst_path, ".download");
      if (!path::exists(dl_dir)) {
        path::make(dl_dir);
      }
      auto dld_file = path::join(inst_path, ".download/package.zip");
      download_url(repo, dld_file);
      extract_zip(dld_file, inst_path);
      select_sdk(inst_path.c_str());
    }
#endif
  } else {
    start_command_line_as_su(path::current_executable() + " -i " + repo, ".");
  }
}

/*
/Application/Xcode.app/Content/Developer/

+ Platforms
    + iPhoneOS.platform
        + Developer
            + SDKs
                + iPhoneOS.sdk
+ Toolchains
    + XcodeDefault.xctoolchain
        + usr
            + bin
                + clang
+ usr
    + bin
        + xcodebuild
        + xcrun
    + lib
    + include
*/
void select_sdk(const char* path) {
#if _WIN32
  using namespace iris;
  if (is_an_admin()) {
    iris::ibconfig config(Ycode_Reg_Path);
    // Check swift toolchain existance
    if (iris::path::exists("C:\\Library\\Developer\\Platforms")) {
      XB_LOGI("Swift toolchain on windows found.");
      auto sdk_setting_file = path::join(path, "SDKSettings.json");
      Json::Reader reader;
      Json::Value root;
      std::ifstream is;
      std::string display_name;
      std::string canonical_name;
      is.open(sdk_setting_file, std::ios::binary);
      if (reader.parse(is, root)) {
        if (!root["DisplayName"].isNull())
          display_name = root["DisplayName"].asString();
        if (!root["CanonicalName"].isNull())
          canonical_name = root["CanonicalName"].asString();
      }
      if (!display_name.empty()) {
        XB_LOGI("iOS SDK found: %s.", display_name.c_str());
        if (!path::exists("C:\\Library\\Developer\\Platforms\\iPhoneOS.sdk\\SDKSettings.json")) {
          iris::string in_path = path;
          auto files = path::list_files_by_ext(in_path, "", true);
          for (auto& file : files) {
            auto dir = path::join("C:\\Library\\Developer\\Platforms\\iPhoneOS.sdk",
                                  path::file_dir(file.substr(in_path.length())));
            if (!path::exists(dir)) {
              path::make(dir);
            }
            path::file_copy(file, path::join(dir, path::file_name(file)));
          }

          XB_LOGI("Copy iPhoneOS.sdk to Swift toolchain platform folder.");
        }
        config[Ycode_Selected_Path_Key] = "C:\\Library\\Developer\\";
      }
    } else {
      config[Ycode_Selected_Path_Key] = path;
    }
  } else {
    start_command_line_as_su(path::current_executable() + " -s " + path, ".");
  }
#endif
}

void print_sdk_path() {
#if _WIN32
  iris::ibconfig config(Ycode_Reg_Path);
  iris::string path = config[Ycode_Selected_Path_Key];
  printf("%s", path.c_str());
#endif
}

void make_sdk() {}
