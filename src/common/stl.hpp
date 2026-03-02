#pragma once

#include <algorithm>
#include "ext/string_piece.hpp"
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <vector>

#ifdef _MSC_VER
#pragma warning(disable : 4251)
#endif

namespace iris {
class string : public std::string {
public:
  string() : std::string() {}
  string(const char* str) : std::string(str) {}
  string(const char* str, size_t length) : std::string(str, length) {}
  string(const string& str) : std::string(str) {}
  string(const std::string& str) : std::string(str) {}
  string(size_t c, char d) : std::string(c, d) {}
  string(const std::string_view& view) : std::string(view) {}
  // string(string&& str) noexcept : std::string(str) {}

  bool endswith(const string& term) const {
    if (length() >= term.length()) {
      return (0 == compare(length() - term.length(), term.length(), term));
    } else {
      return false;
    }
  }

  bool startswith(const string& term) const {
    if (length() >= term.length()) {
      return (0 == compare(0, term.length(), term));
    } else {
      return false;
    }
  }

  bool contains(const string& substr) const { return find(substr) != npos; }

  bool equal_ignorecase(const string& r) const {
#if _WIN32
    return _strcmpi(c_str(), r.c_str()) == 0;
#else
    return strcasecmp(c_str(), r.c_str()) == 0;
#endif
  }

  string to_win_path() const;

  static string format(const char* fmt, ...);
};

} // namespace iris

namespace std {
template <>
struct hash<iris::string> {
  size_t operator()(const iris::string& str) const { return hash<std::string>()((std::string)str); }
};

} // namespace std

namespace iris {
using std::string_view;
using namespace std;
using iris::string;
struct string_set;
class string_list : public std::vector<string> {
public:
  string_list() {}
  string_list(std::initializer_list<string> list) : vector<string>(list) {}

  string_list& operator<<(const string& str) {
    if (!str.empty()) {
      push_back(str);
    }
    return *this;
  }

  string_list& operator<<(const string_list& strs) {
    for (auto& str : strs) {
      if (!str.empty()) {
        push_back(str);
      }
    }
    return *this;
  }

  string_list& operator<<(const string_set& strs);

  string join(const string& split = " ") const {
    if (size() > 0) {
      string result = at(0);
      for (size_t i = 1; i < size(); i++) {
        result += (split + at(i));
      }
      return result;
    }
    return "";
  }

  static string_list split(const string& str, const char& dem) {
    string_list ret;
    size_t last = 0;
    size_t index = str.find_first_of(dem, last);
    while (index != string::npos) {
      if (index > last) {
        ret.push_back(str.substr(last, index - last));
      }
      last = index + 1;
      index = str.find_first_of(dem, last);
    }
    if (last < str.length()) {
      ret.push_back(str.substr(last, index - last));
    }
    return ret;
  }
};

struct string_set : public std::unordered_set<string> {
public:
  static string_set from(string_list const& list) {
    string_set ret;
    for (auto& str : list) {
      ret += str;
    }
    return ret;
  }

  bool contains(const string& str) const { return find(str) != end(); }

  void operator+=(const string& str) { insert(str); }
  void operator+=(const string_list& strs) {
    for (auto str : strs)
      insert(str);
  }
  void operator+=(const string_set& strs) {
    for (auto& str : strs)
      insert(str);
  }
  void operator-=(const string& str) { erase(str); }
  operator string_list() {
    string_list ret;
    for (auto str : *this) {
      ret << str;
    }
    return ret;
  }
  string_set& operator<<(const string& str) {
    if (!str.empty()) {
      insert(str);
    }
    return *this;
  }
};

inline string_list& string_list::operator<<(const string_set& strs) {
  for (auto& str : strs)
    if (!str.empty()) {
      push_back(str);
    }
  return *this;
}

struct ip_v4_addr {
  union {
    uint32_t data;
    struct {
      uint8_t _0;
      uint8_t _1;
      uint8_t _2;
      uint8_t _3;
    };
  };
};

struct ip_v6_addr {
  union {
    uint32_t data[4];
  };
};

struct ip_addr {
  uint32_t family;
  union {
    ip_v4_addr v4;
    ip_v6_addr v6;
  };
};
} // namespace iris

#define I_PROP_RW(type, name)                                                                      \
public:                                                                                            \
  const type& name() const;                                                                        \
  void set_##name(const type& in_##name);                                                          \
                                                                                                   \
private:                                                                                           \
  type name##_;                                                                                    \
                                                                                                   \
public:

#define I_PROP_RO(type, name)                                                                      \
public:                                                                                            \
  const type& name() const { return name##_; }                                                     \
                                                                                                   \
private:                                                                                           \
  type name##_;                                                                                    \
                                                                                                   \
public:

#define I_PROP_PTR_RO(type, name)                                                                  \
public:                                                                                            \
  const type* name() const { return name##_; }                                                     \
                                                                                                   \
private:                                                                                           \
  type* name##_;                                                                                   \
                                                                                                   \
public:

#define DISALLOW_COPY_AND_ASSIGN(x)                                                                \
  x& operator=(const x&) = delete;                                                                 \
  x& operator=(const x&&) = delete;

#define DCHECK(X)
#define NOTREACHED()
