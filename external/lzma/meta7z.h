#ifndef __META7Z__
#define __META7Z__
#pragma once
#include "common/stl.hpp"

namespace iris {
struct __archive7zimpl;

class archive7z {
public:
  struct file_time {
    uint32_t low;
    uint32_t high;
  };

  struct entry {
    string name;
    bool isdir;
  };

  archive7z(const string& in_path);
  virtual ~archive7z();

  bool is_open() const;
  vector<entry> getFileNames();
  void extract(const string& dest,
               const string& exclude_filter = "");

private:
  __archive7zimpl* impl_;
};
} // namespace iris
#endif
