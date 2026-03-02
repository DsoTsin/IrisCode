#include "source_file.hpp"
#include "os.hpp"
#if _WIN32
#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#endif

namespace iris {
using namespace std;
using namespace std::ext;

file_base::file_base(string const& path, string const& basedir)
  : m_path(path), m_is_absolute(path::is_absolute(m_path)) {
  if (!is_absolute() && basedir.empty()) {
    if (basedir.empty()) {
      m_path = abspath();
    } else {
      if (!path::is_absolute(m_path))
        m_path = path::join(basedir, m_path);
    }
  }
}
file_base::~file_base() {}

bool file_base::exists() const {
#if _WIN32
  return GetFileAttributesA(m_path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
  return false;
#endif
}
string file_base::abspath() const {
  if (!is_absolute()) {
    char buffer[2048] = {};
#if _WIN32
    DWORD sz = GetFullPathNameA(m_path.c_str(), 2048, buffer, NULL);
#else
    realpath(m_path.c_str(), buffer);
#endif
    return string(buffer);
  } else {
    return m_path;
  }
}
string file_base::get_shortpath() const {
#if _WIN32
  if (!is_absolute()) {
    return m_path;
  } else {
    char buffer[2048] = {};
    GetShortPathNameA(m_path.c_str(), buffer, 2048);
    return string(buffer);
  }
#else
  return string();
#endif
}

string file_base::get_relpath(string const& abs_path) const {
#if _WIN32
  char rel_path[MAX_PATH] = "";
  PathRelativePathToA(rel_path, abs_path.c_str(), FILE_ATTRIBUTE_DIRECTORY, m_path.c_str(),
                      FILE_ATTRIBUTE_NORMAL);
  return string(rel_path);
#else
  return string();
#endif
}

source_dir::source_dir(string const& path, string const& origin, string const& basedir)
  : m_isabs(path::is_absolute(path)), m_abspath(path), m_original(origin.empty() ? path : origin) {
  // need combine path
  if (!basedir.empty() && path::is_absolute(basedir) && !m_isabs) {
    m_abspath = path::join(basedir, path);
  } else {
    if (!m_isabs) {
      m_abspath = path::join(path::current_dir(), m_abspath);
    }
  }
}

source_file::source_file() : m_type(source_file_type::unknown) {}

source_file::source_file(string const& path, string const& origin, string const& basedir)
  : source_dir(path, origin, basedir),
    m_type(get_type(string_piece(path.c_str(), path.length()))) {}

source_file_type source_file::type() const {
  return m_type;
}

string source_file::resolve() {
  return string();
}

string source_file::int_ext() const {
  switch (m_type) {
  case source_file_type::objc:
  case source_file_type::objcxx:
  case source_file_type::c:
  case source_file_type::cpp:
  case source_file_type::swift:
  case source_file_type::assembly:
    return "o";
  }
  return "x";
}

bool source_file::is_c_source_file() const {
  switch (m_type) {
  case source_file_type::objc:
  case source_file_type::objcxx:
  case source_file_type::c:
  case source_file_type::cpp:
    return true;
  }
  return false;
}

string source_file::base_name() const {
  return path::file_basename(m_original);
}

source_file_type source_file::get_type(string_piece const& path) {
  if (path.end_with(".cpp") || path.end_with(".cc")) {
    return source_file_type::cpp;
  } else if (path.end_with(".c")) {
    return source_file_type::c;
  } else if (path.end_with(".m")) {
    return source_file_type::objc;
  } else if (path.end_with(".mm")) {
    return source_file_type::objcxx;
  } else if (path.end_with(".S") || path.end_with(".s") || path.end_with(".asm")) {
    return source_file_type::assembly;
  } else if (path.end_with(".ispc")) {
    return source_file_type::ispc;
  } else if (path.end_with(".swift")) {
    return source_file_type::swift;
  } else if (path.end_with(".hpp") || path.end_with(".h")) {
    return source_file_type::header;
  } else if (path.end_with(".py")) {
    return source_file_type::py;
  } else if (path.end_with(".rs")) {
    return source_file_type::rust;
  } else if (path.end_with(".cs")) {
    return source_file_type::csharp;
  } else if (path.end_with(".cu")) {
    return source_file_type::cuda;
  } else if (path.end_with(".proto")) {
    return source_file_type::proto;
  } else if (path.end_with(".hip")) {
    return source_file_type::hip;
  } else if (path.end_with(".cg")) {
    return source_file_type::cg;
  } else if (path.end_with(".rc")) {
    return source_file_type::rc;
  } else if (path.end_with(".java")) {
    return source_file_type::java;
  } else if (path.end_with(".ui")) {
    return source_file_type::qtui;
  } else if (path.end_with(".shader")) {
    return source_file_type::shader;
  } else if (path.end_with(".hlsl")) {
    return source_file_type::hlsl;
  } else if (path.end_with(".glsl")) {
    return source_file_type::glsl;
  } else if (path.end_with(".pssl")) {
    return source_file_type::pssl;
  } else if (path.end_with(".metal")) {
    return source_file_type::metal;
  } else if (path.end_with(".spv")) {
    return source_file_type::spv;
  } else if (path.end_with(".a") || path.end_with(".lib")) {
    return source_file_type::static_lib;
  } else if (path.end_with(".dll") || path.end_with(".dylib")) {
    return source_file_type::dyn_lib;
  } else if (path.end_with(".obj") || path.end_with(".o")) {
    return source_file_type::object;
  } else if (path.end_with(".ib")) {
    return source_file_type::ib;
  }
  return source_file_type::unknown;
}
} // namespace iris
