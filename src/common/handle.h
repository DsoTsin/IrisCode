#pragma once

#include <queue>
#include <stdint.h>
#include <vector>

namespace iris {
class handle {
public:
  enum _type { process, /*E_THREAD,*/ file, pipe };
  handle(int64_t raw_val) : value_(raw_val) {}
  explicit handle(void* ptr);

  handle& operator=(handle const& rhs) {
    value_ = rhs.value_;
    return *this;
  }

  _type type() const { return (_type)type_; }
  uint32_t id() const { return id_; }
  uint32_t gen() const { return gen_; }

  static handle new_process();
  static handle new_file();
  static void free(handle const& handle);

  /// <summary>
  ///   Check raw handle if is win32 handle
  /// </summary>
  /// <param name="ptr"></param>
  /// <returns></returns>
  static bool is_handle(void* ptr);
  static uint64_t num_free_slots();

  operator int64_t() const { return int64_t(value_); }
  operator uint64_t() const { return value_; }

  /// <summary>
  /// Check handle if is emulated
  /// </summary>
  operator bool() const;

  bool operator>(const handle& h) const { return value_ > h.value_; }
  bool operator<(const handle& h) const { return value_ < h.value_; }
  bool operator==(const handle& h) const { return value_ == h.value_; }

private:
  handle(_type type);

  union {
    struct {
      uint32_t id_;
      uint16_t gen_;
      uint16_t type_ : 4;
      uint16_t magic_ : 12;
    };
    uint64_t value_;
  };

  static std::priority_queue<handle> free_slots_;
  static uint32_t* slot_flags_;
};

} // namespace iris

#include <map>

namespace std {
template <>
struct hash<iris::handle> {
  size_t operator()(iris::handle const& h) const { return (size_t)(uint64_t)h; }
};
} // namespace std
