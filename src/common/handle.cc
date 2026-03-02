#include "handle.h"
#if _WIN32
#include <Windows.h>
#endif
namespace iris {
static volatile uint32_t s_initial_value = 0;
static uint32_t s_magic = 0xfad;
std::priority_queue<handle> handle::free_slots_;
uint32_t* handle::slot_flags_ = nullptr;
handle handle::new_file() {
  return handle(handle::file);
}
handle::handle(void* ptr) {
  value_ = (uint64_t)ptr;
}
handle handle::new_process() {
  return handle(handle::process);
}
handle::handle(_type type) : value_(0) {
  magic_ = s_magic;
  type_ = type;
  gen_ = 0;
  if (free_slots_.empty()) {
#if _WIN32
    id_ = _InterlockedIncrement(&s_initial_value);
#endif
  } else {
    handle top = free_slots_.top();
    free_slots_.pop();
    operator=(top);
    gen_++;
  }
}
void handle::free(handle const& inhandle) {
  free_slots_.push(inhandle);
  // slot_flags_.resize(inhandle.id_ + 1);
  // inhandle.id_;
}
bool handle::is_handle(void* ptr) {
  handle h(ptr);
  return h;
}
uint64_t handle::num_free_slots() {
  return (uint64_t)free_slots_.size();
}
handle::operator bool() const {
  return magic_ == s_magic;
}
} // namespace iris
