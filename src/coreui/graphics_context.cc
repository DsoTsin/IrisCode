#include "graphics_context.hpp"

#ifdef _WIN32
#include "platform/win/win32_graphics_context.hpp"
#elif defined(__APPLE__)
#include "platform/apple/appl_graphics_context.hpp"
#endif

namespace iris {
namespace ui {

graphics_context::graphics_context(ui::window* w, bool gpu)
#ifdef _WIN32
    : d(platform::win32::create_graphics_context(w, gpu)) {}
#elif defined(__APPLE__)
    : d(platform::apple::create_graphics_context(w, gpu)) {}
#else
    : d(nullptr) {
  (void)w;
  (void)gpu;
}
#endif

graphics_context::~graphics_context() {
  if (!d) {
    return;
  }
#ifdef _WIN32
  platform::win32::destroy_graphics_context(d);
#elif defined(__APPLE__)
  platform::apple::destroy_graphics_context(d);
#endif
  d = nullptr;
}

void graphics_context::resize(int width, int height) {
#ifdef _WIN32
  platform::win32::resize_graphics_context(d, width, height);
#elif defined(__APPLE__)
  platform::apple::resize_graphics_context(d, width, height);
#else
  (void)width;
  (void)height;
#endif
}

void graphics_context::synchronize(const ui::rect* dirty_rect) {
#ifdef _WIN32
  platform::win32::synchronize_graphics_context(d, dirty_rect);
#elif defined(__APPLE__)
  platform::apple::synchronize_graphics_context(d, dirty_rect);
#else
  (void)dirty_rect;
#endif
}

void graphics_context::flush() {
#ifdef _WIN32
  platform::win32::flush_graphics_context(d);
#elif defined(__APPLE__)
  platform::apple::flush_graphics_context(d);
#endif
}

canvas* graphics_context::canvas() {
#ifdef _WIN32
  return platform::win32::graphics_context_canvas(d);
#elif defined(__APPLE__)
  return platform::apple::graphics_context_canvas(d);
#else
  return nullptr;
#endif
}

ui::rect graphics_context::expand_dirty_rect(const ui::rect& dirty_rect) const {
#ifdef _WIN32
  return platform::win32::expand_graphics_context_dirty_rect(d, dirty_rect);
#elif defined(__APPLE__)
  return platform::apple::expand_graphics_context_dirty_rect(d, dirty_rect);
#else
  return dirty_rect;
#endif
}

void graphics_context::on_window_pos_changed() {
#ifdef _WIN32
  platform::win32::on_graphics_context_window_pos_changed(d);
#elif defined(__APPLE__)
  platform::apple::on_graphics_context_window_pos_changed(d);
#endif
}

void graphics_context::on_window_activate(bool active) {
#ifdef _WIN32
  platform::win32::on_graphics_context_window_activate(d, active);
#elif defined(__APPLE__)
  platform::apple::on_graphics_context_window_activate(d, active);
#else
  (void)active;
#endif
}

void graphics_context::on_live_resize_changed(bool active) {
#ifdef _WIN32
  platform::win32::on_graphics_context_live_resize_changed(d, active);
#elif defined(__APPLE__)
  platform::apple::on_graphics_context_live_resize_changed(d, active);
#else
  (void)active;
#endif
}

void graphics_context::shutdown() {
#ifdef _WIN32
  platform::win32::shutdown_graphics_context(d);
#elif defined(__APPLE__)
  platform::apple::shutdown_graphics_context(d);
#endif
}

window* graphics_context::window() const {
#ifdef _WIN32
  return platform::win32::window_for_graphics_context(d);
#elif defined(__APPLE__)
  return platform::apple::window_for_graphics_context(d);
#else
  return nullptr;
#endif
}

} // namespace ui
} // namespace iris
