#include "config.hpp"
#include "download.hpp"
#include "os.hpp"
#include <string.h>

#define Ycode_Reg_Path "reg:\\HKEY_CURRENT_USER\\Software\\Ycode\\"
#define Ycode_Selected_Path_Key "SelectedPath"

using namespace iris;

int execute_cmdline(int argc, const char* argv[]);
int print_help();

int main(int argc, const char* argv[]) {
  if (argc >= 2) {
    if (!strncmp(argv[1], "--", 2)) {
      if (!strcmp(argv[1], "--sdk")) {
        auto sdk = argv[2];
        return execute_cmdline(argc - 3, argv + 3);
      } else {
        printf("unknow option: %s", argv[1]);
        return 2;
      }
    } else {
      return execute_cmdline(argc - 1, argv + 1);
    }
  } else {
    return print_help();
  }
}

int execute_cmdline(int argc, const char* argv[]) {
  iris::string root;
#if _WIN32
  iris::ibconfig config(Ycode_Reg_Path);
  root = config[Ycode_Selected_Path_Key];
#endif

  return 0;
}

int print_help() {
  return 0;
}