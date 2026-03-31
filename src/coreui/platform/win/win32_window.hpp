#pragma once

#include "../../uidefs.hpp"
#include "../../view.hpp"
#include <functional>

namespace iris {
namespace ui {
class window;
class view;
class graphics_context;
}

namespace priv {

class window {
public:
  window(ui::window* in_window, ui::window* parent = nullptr);
  ~window();

  void init();
  void create_native_window(void* parent_handle);
  void on_native_created(void* native_handle);
  void show();
  void close();
  void draw(const ui::rect* dirty_rect = nullptr);
  void request_redraw(const ui::rect* dirty_rect = nullptr);
  uint64_t start_timer(uint32_t interval_ms, function<void()> callback);
  void stop_timer(uint64_t timer_id);
  void set_content_view(ui::view* inview);
  void resize(int w, int h);
  int zone_at(int x, int y) const;
  bool on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result);
  bool on_window_pos_changed();
  bool on_window_activate(bool active);

  void draw_title();
  void draw_border();
  ui::point window_to_content(const ui::point& window_point) const;
  ui::point content_to_view(ui::view* target, const ui::point& content_point) const;
  ui::view* hit_test_content(const ui::point& window_point) const;
  bool route_mouse_event(ui::view* target, const ui::mouse_event& event, ui::view** handled_by = nullptr);
  bool route_key_event(ui::view* target,
                       const ui::key_event& event,
                       bool is_char_message,
                       ui::view** handled_by = nullptr);
  void update_hover_view(ui::view* next_hover, const ui::point& window_point, uint8_t modifiers);
  void set_focus_view(ui::view* next_focus);
  void clear_capture();
  void clear_interaction_state();

  void set_border_width(float width) { border_width_ = width; }
  float border_width() const { return border_width_; }
  void set_shadow_distance(float dist) { shadow_distance_ = dist; }
  float shadow_distance() const { return shadow_distance_; }
  void set_corner_radius(float dist) { corner_radius_ = dist; }
  float corner_radius() const { return corner_radius_; }

  const ui::rect& frame() const { return frame_; }
  ui::rect content_bounds() const;

  ui::window* wind() { return w_; }
  bool is_maximized() const;
  void* native_handle() const { return hwnd_; }

private:
  void stop_all_timers();

  void* hwnd_ = nullptr;
  ui::rect frame_;
  ui::view* content_view_ = nullptr;
  float shadow_distance_ = 20.f;
  float border_width_ = 1.f;
  float corner_radius_ = 12.f;
  uint32_t window_color_ = 0;
  ui::window* w_;
  ui::window* pw_;
  ref_ptr<ui::graphics_context> ctx_;
  ui::view* hover_view_ = nullptr;
  ui::view* capture_view_ = nullptr;
  ui::view* focus_view_ = nullptr;
  uint8_t mouse_buttons_down_ = 0;
  bool tracking_mouse_leave_ = false;
  bool live_resizing_ = false;
  uint64_t next_timer_id_ = 1;
  unordered_map<uint64_t, function<void()>> timers_;
};

} // namespace priv
} // namespace iris
