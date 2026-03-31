#include "appl_window.hpp"

#include "../../graphics_context.hpp"
#include "../../view.hpp"
#include "../../window.hpp"

namespace iris {
namespace priv {

window::window(ui::window* in_window, ui::window* parent) : w_(in_window), pw_(parent) {}

window::~window() = default;

void window::init() {
  ctx_ = ref_ptr<ui::graphics_context>(new ui::graphics_context(w_, true));
}

void window::create_native_window(void* parent_handle) {
  (void)parent_handle;
}

void window::on_native_created(void* native_handle) {
  native_handle_ = native_handle;
}

void window::show() {}

void window::close() {}

void window::draw(const ui::rect* dirty_rect) {
  (void)dirty_rect;
}

void window::request_redraw(const ui::rect* dirty_rect) {
  draw(dirty_rect);
}

uint64_t window::start_timer(uint32_t interval_ms, function<void()> callback) {
  (void)interval_ms;
  (void)callback;
  return 0;
}

void window::stop_timer(uint64_t timer_id) {
  (void)timer_id;
}

void window::set_content_view(ui::view* inview) {
  content_view_ = inview;
}

void window::resize(int w, int h) {
  frame_.width = static_cast<float>(w);
  frame_.height = static_cast<float>(h);
}

int window::zone_at(int x, int y) const {
  (void)x;
  (void)y;
  return 0;
}

bool window::on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result) {
  (void)msg;
  (void)w_param;
  (void)l_param;
  (void)out_result;
  return false;
}

bool window::on_window_pos_changed() {
  return true;
}

bool window::on_window_activate(bool active) {
  (void)active;
  return true;
}

void window::clear_interaction_state() {}

ui::rect window::content_bounds() const {
  return frame_;
}

bool window::is_maximized() const {
  return false;
}

} // namespace priv
} // namespace iris
