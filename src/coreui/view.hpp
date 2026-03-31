#pragma once

#include "uidefs.hpp"

#include "yoga/enums/Align.h"
#include "yoga/enums/Direction.h"
#include "yoga/enums/FlexDirection.h"
#include "yoga/enums/Justify.h"
#include "yoga/enums/MeasureMode.h"
#include "yoga/Yoga.h"
#include "yoga/node/Node.h"
#include "graphics_context.hpp"

namespace iris {
namespace priv {
class view_impl;
class window;
}

namespace ui {
class view;
class dock_view;
class dock_window;

enum class mouse_button : uint8_t {
  none,
  left,
  right,
  middle,
  x1,
  x2,
  other,
};

enum class input_modifier : uint8_t {
  shift = 1 << 0,
  control = 1 << 1,
  alt = 1 << 2,
};

constexpr uint8_t modifier_mask(input_modifier modifier) {
  return static_cast<uint8_t>(modifier);
}

inline bool has_modifier(uint8_t modifiers, input_modifier modifier) {
  return (modifiers & modifier_mask(modifier)) != 0;
}

struct mouse_event {
  event_type type = event_type::mouse_moved;
  mouse_button button = mouse_button::none;
  point location_in_window;
  point location_in_view;
  float wheel_delta = 0.0f;
  bool is_double_click = false;
  uint8_t modifiers = 0;
};

struct key_event {
  event_type type = event_type::key_down;
  uint32_t key_code = 0;
  uint32_t scan_code = 0;
  uint32_t character = 0;
  bool is_repeat = false;
  bool is_system = false;
  uint8_t modifiers = 0;
};

class view_controller {
public:
  virtual ~view_controller() {}
  virtual bool is_appearing(bool animate) const { return false; }
  virtual void did_load() {}
  virtual void will_appear(bool animate) {}
  virtual void will_layout_subviews() {}
  virtual void did_layout_subviews() {}
  virtual void did_appear(bool animate) {}
  virtual void will_disappear(bool animate) {}
  virtual void did_disappear(bool animate) {}

protected:
  view* view_;
  view_controller* parent_;
  vector<view_controller*> child_;
};

class UICORE_API view : public ref_counted {
public:
  view(view* in_view = nullptr);
  virtual ~view();

  // initWithFrame
  virtual void init(const rect& frame);
  virtual void set_gfx_context(graphics_context* ctx);
  virtual void draw(const rect& dirty_rect);
  virtual void layout_if_needed();
  virtual void update_layer() {}

  #pragma region filter
  virtual void set_filter() {}
  #pragma endregion

  virtual void add_child(view* v);
  virtual void remove_child(view* v);
  virtual void detach_child(view* v);

  virtual view* hit_test(point const& p) const;
  view* parent() const { return parent_; }
  window* owning_window() const;
  rect screen_rect() const;
  YGNodeRef layout_node() const;

  virtual bool mouse_down(const mouse_event& event);
  virtual bool mouse_up(const mouse_event& event);
  virtual bool mouse_move(const mouse_event& event);
  virtual bool mouse_enter(const mouse_event& event);
  virtual bool mouse_leave(const mouse_event& event);
  virtual bool mouse_wheel(const mouse_event& event);
  virtual bool key_down(const key_event& event);
  virtual bool key_up(const key_event& event);
  virtual bool key_char(const key_event& event);
  virtual bool can_receive_focus() const { return true; }
  virtual void on_focus_changed(bool focused);
  virtual rect dirty_rect() const;

  float width() const;
  view& width(float width);
  view& auto_width();
  float height() const;
  view& fill_height(float percent);

  view& height(float height);
  view& auto_height();
  view& flex_dir(FlexDirection dir);
  view& justify_content(Justify justify);
  view& align_items(Align align);
  view& padding(float margin);
  view& padding(float horizontal, float vertical);
  view& padding(float left, float top, float right, float bottom);

  rect get_layout_rect() const;
  void mark_layout_dirty();

protected:
  void enable_measure();
  virtual size on_measure(MeasureMode mw, MeasureMode mh, const ui::size& sz);
  YGNodeRef yoga_node() const;

  void mark_draw_dirty();

private:
  void measure_children();

protected:
  view* parent_ = nullptr;
  vector<view*> subviews_;
  weak_ptr<graphics_context> gfx_ctx_;

private:
  friend class priv::view_impl;
  friend class priv::window;
  friend class dock_view;

  priv::view_impl* d_;
};
} // namespace ui
} // namespace iris
