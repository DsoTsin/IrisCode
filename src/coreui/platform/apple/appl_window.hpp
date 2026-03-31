#pragma once

#include "../../uidefs.hpp"
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

  void clear_interaction_state();
  void set_border_width(float width) { border_width_ = width; }
  float border_width() const { return border_width_; }
  void set_shadow_distance(float dist) { shadow_distance_ = dist; }
  float shadow_distance() const { return shadow_distance_; }
  void set_corner_radius(float dist) { corner_radius_ = dist; }
  float corner_radius() const { return corner_radius_; }

  const ui::rect& frame() const { return frame_; }
  ui::rect content_bounds() const;

  bool is_maximized() const;
  void* native_handle() const { return native_handle_; }

private:
  void* native_handle_ = nullptr;
  ui::rect frame_;
  ui::view* content_view_ = nullptr;
  float shadow_distance_ = 20.f;
  float border_width_ = 1.f;
  float corner_radius_ = 12.f;
  ui::window* w_;
  ui::window* pw_;
  ref_ptr<ui::graphics_context> ctx_;
};

} // namespace priv
} // namespace iris
