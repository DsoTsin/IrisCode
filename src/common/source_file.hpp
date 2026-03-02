#pragma once
#ifndef __SOURCE_FILE_HPP_20180607__
#define __SOURCE_FILE_HPP_20180607__

#include "stl.hpp"
#include <ext/string_piece.hpp>

#if _WIN32
#pragma comment(lib, "Shlwapi.lib")
#endif

namespace iris {
using std::ext::string_piece;

enum class source_file_type : uint32_t {
  objc = 2,
  objcxx,
  c,
  cpp,
  header,            // c headers
  module_interfacce, // ixx c++20
  assembly,
  swift,
  ispc,
  rc,         // windows resource file
  rust,       // rust
  csharp,     // csharp
  py,         // python
  go,         // go
  metal,      // metal
  hlsl,       // hlsl
  glsl,       // glsl
  pssl,       // pssl
  spv,        // spirv
  cl,         // opencl
  cuda,         // cuda
  hip,        // hip, AMD ROCm  
  cg,         // cg
  qtui,       // qt ui
  java,       // java source
  javaclass,  // java class
  jar,        // java library
  proto,      // proto
  xml,        // xml
  yml,        // yml
  json,       // json
  shader,     // unity shaderlab
  static_lib, // static lib
  dyn_lib,
  object,      // compiled object (elf/coef)
  ib,
  unknown
};
class file_base {
protected:
  string m_path;
  bool m_is_absolute;

public:
  explicit file_base(string const& path = "", string const& basedir = "");
  virtual ~file_base();
  bool exists() const;
  bool is_absolute() const { return m_is_absolute; }
  string abspath() const;
  string get_shortpath() const;
  string get_relpath(string const& abs_path) const;
};

class source_dir {
public:
  source_dir() : m_isabs(false) {}
  explicit source_dir(string const& in_path,
                      string const& original_path = "",
                      string const& basedir = "");

  string original() const { return m_original; }
  string abspath() const { return m_abspath; }
  bool isabs() const { return m_isabs; }

  bool operator==(const source_dir& other) const {
    return m_isabs == other.m_isabs && m_abspath == other.m_abspath
           && m_original == other.m_original;
  }

protected:
  bool m_isabs;
  string m_abspath;
  string m_original;
};

class source_file : public source_dir {
public:
  source_file();
  explicit source_file(string const& in_path,
                       string const& original_path = "",
                       string const& basedir = "");

  source_file_type type() const;
  // resolve absolute path
  string resolve();

  // intermediate file extersion
  string int_ext() const;
  bool is_c_source_file() const;

  string base_name() const;
  const string& original() const { return m_original; }
  const string& abspath() const { return m_abspath; }

  static source_file_type get_type(string_piece const& path);

private:
  source_file_type m_type;
};
using source_list = vector<source_file>;
} // namespace iris

namespace std {
template <>
struct hash<iris::source_dir> {
  size_t operator()(const iris::source_dir& dir) const {
    return hash<string>()(dir.isabs() ? dir.abspath() : dir.original());
  }
};
template <>
struct hash<iris::source_file> {
  size_t operator()(const iris::source_file& dir) const {
    return hash<string>()(dir.isabs() ? dir.abspath() : dir.original());
  }
};
} // namespace std

namespace iris {
struct source_dirs : public unordered_set<source_dir> {
public:
  void operator+=(const source_dir& str) { insert(str); }
  void operator+=(const source_dirs& dirs) {
    for (auto& d : dirs) {
      insert(d);
    }
  }
  void operator-=(const source_dir& str) { erase(str); }
};

struct source_files : public unordered_set<source_file> {
  void operator+=(const source_file& file) { insert(file); }
  void operator+=(const source_files& dirs) {
    for (auto& d : dirs) {
      insert(d);
    }
  }
  void operator-=(const source_file& file) { erase(file); }
};
} // namespace iris

#endif