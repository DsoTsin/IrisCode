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
}

namespace ui {
class view;
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

  virtual view* hit_test(point const& p) const;

  float width() const;
  view& width(float width);
  view& auto_width();
  float height() const;
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

  void mark_draw_dirty();

private:
  void measure_children();

protected:
  view* parent_ = nullptr;
  vector<view*> subviews_;
  weak_ptr<graphics_context> gfx_ctx_;

private:
  friend class priv::view_impl;

  priv::view_impl* d_;
};
} // namespace ui
} // namespace iris