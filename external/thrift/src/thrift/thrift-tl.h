#pragma once

#include <string>
#include <vector>

namespace apache {
namespace thrift {
class noncopyable {
protected:
  constexpr noncopyable() = default;
  ~noncopyable() = default;
  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;
};

template <class T>
class scoped_array {
public:
  scoped_array(T* ptr = nullptr)
      : ptr_(nullptr) {
      reset(ptr);
  }

  ~scoped_array() {
    if (ptr_)
      delete[] ptr_;
  }

  void reset(T* ptr = nullptr) {
    if (ptr_)
      delete[] ptr_;
    ptr_ = ptr;
  }

  T* get() const { return ptr_; }

private:
  T* ptr_;
};

bool istarts_with(const char* a, const char* b);
bool iends_with(const char* a, const char* b);
void split_string(const std::string& s, std::vector<std::string>& v, const std::string& c);
std::string utf16_to_utf8(std::vector<uint16_t> const& data);
} // namespace thrift
} // namespace apache
