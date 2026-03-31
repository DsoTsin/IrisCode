#include "window.hpp"

#include "app.hpp"

#ifdef _WIN32
#include "platform/win/win32_window.hpp"
#elif defined(__APPLE__)
#include "platform/apple/appl_window.hpp"
#endif

namespace iris {
namespace ui {

window::window(window* in_window) : d(new priv::window(this, in_window)) {
  d->create_native_window(in_window ? in_window->native_handle() : nullptr);
  app::shared()->add_window(this);
}

bool window::on_window_pos_changed() {
  return d->on_window_pos_changed();
}

bool window::on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result) {
  return d->on_native_event(msg, w_param, l_param, out_result);
}

bool window::on_window_activate(bool active) {
  return d->on_window_activate(active);
}

window::~window() {
  if (d) {
    app::shared()->remove_window(this);
    delete d;
    finalize();
    d = nullptr;
  }
}

void window::initialize() {
  d->init();
}

void window::finalize() {}

void window::set_content_view(view* inview) {
  d->set_content_view(inview);
}

void window::set_delegate() {}

void window::show() {
  d->show();
}

bool window::activates_on_show() const {
  return true;
}

bool window::paints_synchronously_on_show() const {
  return true;
}

bool window::uses_fancy_border() const {
  return true;
}

bool window::uses_window_compositor() const {
  return false;
}

void window::close() {
  d->close();
}

void window::cancel_interaction() {
  d->clear_interaction_state();
}

void window::request_redraw() {
  d->request_redraw(nullptr);
}

void window::request_redraw(const ui::rect& dirty_rect) {
  d->request_redraw(&dirty_rect);
}

void window::draw() {
  d->draw();
}

void window::draw(const ui::rect& dirty_rect) {
  d->draw(&dirty_rect);
}

uint64_t window::start_timer(uint32_t interval_ms, function<void()> callback) {
  return d->start_timer(interval_ms, std::move(callback));
}

void window::stop_timer(uint64_t timer_id) {
  d->stop_timer(timer_id);
}

const ui::rect& window::frame() const {
  return d->frame();
}

ui::rect window::content_rect() const {
  return d->content_bounds();
}

void* window::native_handle() const {
  return d->native_handle();
}

void window::set_border_width(float width) {
  d->set_border_width(width);
}

float window::border_width() const {
  return d->border_width();
}

void window::set_shadow_distance(float dist) {
  d->set_shadow_distance(dist);
}

float window::shadow_distance() const {
  return d->shadow_distance();
}

void window::set_corner_radius(float radius) {
  d->set_corner_radius(radius);
}

float window::corner_radius() const {
  return d->corner_radius();
}

void window::resize(int w, int h) {
  d->resize(w, h);
}

int window::zone_at(int x, int y) {
  return d->zone_at(x, y);
}

} // namespace ui
} // namespace iris
