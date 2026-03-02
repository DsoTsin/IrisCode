#include "download.hpp"
#include "os.hpp"
#include "pe_walker.hpp"
#include "zip.hpp"
#include "meta7z.h"
#include "vm/value.h"
#if _WIN32
#include <ext/win32.hpp>
#endif
#include <cassert>

#include <chrono>

#if _WIN32
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shlwapi.lib")
#endif

using namespace std;
using namespace iris;

iris::string win_path = "c:\\";
iris::string rel_win_path = ".\\rre\\sdc/";
iris::string test_conjunction = "dscc\\";
iris::string test_conjunction2 = ".\\dscc\\";

iris::string rel_unix_path = "./rre/sdc";
iris::string rel_unix_path_parent = "../rre/sdc";

iris::string path_a1 = "C:/dfdff/ggee/tyyg/ioo.cc";
iris::string path_a2 = "C:\\dfdff\\ggee/core/";
iris::string path_a3 = "C:\\dfdff\\ggee/tyyg";
iris::string path_a4 = "C:\\dfdff\\ggee/ty";
iris::string path_a5 = "..\\fdd\\ccc";
iris::string path_a6 = "C:";

int main(int argc, char* argv[]) {
  iris::string test_url
      = "https://github.com/DsoTsin/kaleido3d/releases/download/v1.0/llvm_ir_support.7z";

  using namespace std::chrono;
  auto start = system_clock::now();
  download_url("https://github.com/DsoTsin/kaleido3d/releases/download/v1.0/llvm_ir_support.7z", "test.7z",
      [](const char*,uint64_t downloaded, uint64_t _downloaded, uint64_t total, void* data) {
        auto& time_point = *(std::chrono::system_clock::time_point*)data;
        auto end = system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - time_point);
        auto speed = 1.0 * downloaded / duration.count();
        printf("%lld, %.3f KB/s\n", downloaded, speed);
        time_point = end;
      }, &start);
  //downloader d;
  //d.download(test_url, "test.zip");
  //extract_zip("test.zip", "templ");

  assert(path::join(win_path, rel_win_path) == "c:/rre/sdc");
  assert(path::join(rel_win_path, test_conjunction2) == "rre/sdc/dscc");
  assert(path::join(rel_win_path, rel_unix_path_parent) == "rre/rre/sdc");
  assert(path::relative_to(path_a1, path_a2) == "../tyyg/ioo.cc");
  assert(path::relative_to(path_a1, path_a3) == "ioo.cc");
  assert(path::relative_to(path_a1, path_a4) == "../tyyg/ioo.cc");
  assert(path::relative_to(path_a1, path_a6) == "dfdff/ggee/tyyg/ioo.cc");

  iris::string l0 = "sdsdf";
  iris::string l1 = "dfdf/sdsdf";
  iris::string l2 = "dfdf\\sdsdf.a";
  iris::string l3 = "dfdf\\sdsdf";

  auto endswith = [](const iris::string& str, const iris::string& term) -> bool {
    return term.length() <= str.length()
           && str.find_last_of(term) == str.length() - term.length() + 1;
  };

  auto check_slib = [](const iris::string& str) -> bool {
    return str.find('/') == iris::string::npos && str.find('\\') == iris::string::npos
           && str.find_last_of(".a") == iris::string::npos;
  };
  auto ret = check_slib(l0);
  ret = check_slib(l1);
  ret = check_slib(l2);
  ret = check_slib(l3);
  auto p1 = l2.find_last_of(".a");
  ret = endswith(l2, ".a");

  iris::string p = path::join(path_a4, path_a5);
  assert(p == "C:/dfdff/ggee/fdd/ccc");

  archive7z _7z("C:\\Users\\tomic\\Desktop\\metalib-spirv\\build\\llvm_ir_support.7z");
  auto files = _7z.getFileNames();

  auto dir = get_user_dir();

#if _WIN32
  auto _dir = get_user_dir();
  win32::file_ext_reg_util util(".ib", "TsinStudio.IrisBuildFile");
  iris::string ico = util.default_icon();
  iris::string open_cmd = util.open_command();
  pe_file_list list;
  iris::string msg;
  extract_pe_dependencies("E:/bin/vc2010_portable/VC/bin/cl.exe", "", string_list(), list, &msg);
#endif
  /*string_list files;
  for (auto p : path::list_files("C:/usr/tsin/projects/xbgn/tests", false))
  {
      files.push_back(path::join("C:/usr/tsin/projects/xbgn/tests", p));
  }
  pack_zip("simple.zip", files);*/
  // depend_parser
  // p(R"(D:\proj\Cube_Native_Experiment\engine\.build\ARM\android_debug\core\core\Object.cpp.d)");
  // auto headers = p.get_headers();
  // headers.clear();


  return 0;
}
