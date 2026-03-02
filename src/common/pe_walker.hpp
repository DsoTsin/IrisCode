#pragma once
#include "stl.hpp"

#if _WIN32
namespace iris {
struct pe_file {
  string name;
  string abs_path;
  uint8_t is_32_bit : 1;
  uint8_t is_debuggable : 1;
  // 0, Current dir; 1, EXE dir; 2, system32; 3, PATH;
  uint8_t dir_type : 2;

  bool operator==(iris::pe_file const& rhs) const {
#if _WIN32
    return !stricmp(name.c_str(), rhs.name.c_str());
#else
    return !strcasecmp(name.c_str(), rhs.name.c_str());
#endif
  }
};
using pe_file_list = vector<pe_file>;
extern bool read_pe_executable(string const& exe, pe_file_list& out_pes, string* err_msg = nullptr);
/*
 * Get All DLLs for Current PE file
 * @param exe absolute or simple name
 * @param cur_dir current directory
 * @param out_pes output dlls
 * @param err_msg
 */
extern bool extract_pe_dependencies(string const& exe,
                                    string const& cur_dir,
                                    string_list const& dll_search_paths,
                                    pe_file_list& out_pes,
                                    string* err_msg = nullptr);
} // namespace iris
#endif
