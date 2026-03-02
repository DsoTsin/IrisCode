#include "view.hpp"

#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkCanvas.h"
#include "yoga/node/Node.h"
#include "yoga/algorithm/CalculateLayout.h"

namespace iris {
namespace priv {
class view_impl {
public:
  view_impl(ui::view* inview);
  ~view_impl() {
    if (node_) {
      YGNodeFree(node_);
      node_ = nullptr;
    }
  }
  void init_with_frame(ui::rect const& r);

  bool is_layout_dirty() const;

  void enable_measure();
  void mark_layout_dirty();

  void layout();
  ui::rect get_layout_rect() const;
  // todo:
  ui::size size() const { return {};  }

private:
  friend class ui::view;

  ui::view* v_;
  YGNodeRef node_;
  bool manual_layout_dirty_ = true;

  ui::rect init_frame_;
};
} // namespace priv
namespace ui {
namespace {
static bool rect_intersects(const rect& a, const rect& b) {
  return a.left < b.right() && a.right() > b.left && a.top < b.bottom() && a.bottom() > b.top;
}

static rect rect_intersection(const rect& a, const rect& b) {
  const float left = max(a.left, b.left);
  const float top = max(a.top, b.top);
  const float right = min(a.right(), b.right());
  const float bottom = min(a.bottom(), b.bottom());
  if (right <= left || bottom <= top) {
    return {};
  }
  return {left, top, right - left, bottom - top};
}
} // namespace
view::view(view* in_view) : parent_(in_view), d_(nullptr) {
  d_ = new priv::view_impl(this);
  //d_->setOwner(parent_ ? parent_->d_ : nullptr);
}
view::~view() {
  if (d_) {
    delete d_;
    d_ = nullptr;
  }
}

void view::init(const rect& frame) {
  d_->init_with_frame(frame);
  for (auto& subview : subviews_) {
    //subview->init(frame);
  }
}
void view::set_gfx_context(graphics_context* ctx) {
  gfx_ctx_ = weak_ptr<graphics_context>(ctx);
  for (auto& subview : subviews_) {
    subview->set_gfx_context(ctx);
  }
}
void view::draw(const rect& dirty_rect) {
  //d_->isDirty();
  for (auto& subview : subviews_) {
    const auto child_layout = subview->get_layout_rect();
    if (!rect_intersects(dirty_rect, child_layout)) {
      continue;
    }
    subview->draw(rect_intersection(dirty_rect, child_layout));
  }
#if 0//defined(_DEBUG) debug drawing
  if (gfx_ctx_) {
    if (auto* c = gfx_ctx_->canvas()) {
      const auto r = get_layout_rect();
      SkPaint layout_paint;
      layout_paint.setStyle(SkPaint::kStroke_Style);
      layout_paint.setStrokeWidth(1.0f);
      layout_paint.setColor(SkColorSetARGB(160, 64, 196, 255));
      c->drawRect(SkRect::MakeXYWH(r.left, r.top, r.width, r.height), layout_paint);

      SkPaint dirty_paint;
      dirty_paint.setStyle(SkPaint::kStroke_Style);
      dirty_paint.setStrokeWidth(1.0f);
      dirty_paint.setColor(SkColorSetARGB(120, 255, 168, 0));
      c->drawRect(SkRect::MakeXYWH(dirty_rect.left, dirty_rect.top, dirty_rect.width, dirty_rect.height),
                  dirty_paint);
    }
  }
#endif
}
void view::layout_if_needed() {
  if (d_->is_layout_dirty()) {
    d_->layout();
  }
}
void view::add_child(view* v) {
  if (!v) {
    return;
  }
  if (std::find(subviews_.begin(), subviews_.end(), v) != subviews_.end()) {
    return;
  }
  if (v->parent_ && v->parent_ != this) {
    v->parent_->remove_child(v);
  }
  v->parent_ = this;
  auto index = subviews_.size();
  subviews_.push_back(v);
  YGNodeInsertChild(d_->node_, v->d_->node_, index);
}
void view::remove_child(view* v) {
  if (!v) {
    return;
  }
  auto iter = std::find(subviews_.begin(), subviews_.end(), v);
  if (iter == subviews_.end()) {
    return;
  }
  subviews_.erase(iter);
  YGNodeRemoveChild(d_->node_, v->d_->node_);
  v->parent_ = nullptr;
}

view* view::hit_test(point const& p) const {
  const auto bounds = get_layout_rect();
  if (p.x < 0.0 || p.y < 0.0 || p.x >= bounds.width || p.y >= bounds.height) {
    return nullptr;
  }

  for (size_t i = subviews_.size(); i > 0; --i) {
    auto* child = subviews_[i - 1];
    const auto child_rect = child->get_layout_rect();
    const double local_x = p.x - child_rect.left;
    const double local_y = p.y - child_rect.top;
    if (local_x < 0.0 || local_y < 0.0 || local_x >= child_rect.width || local_y >= child_rect.height) {
      continue;
    }
    if (auto* hit = child->hit_test({local_x, local_y})) {
      return hit;
    }
  }

  return const_cast<view*>(this);
}

bool view::mouse_down(const mouse_event&) {
  return false;
}

bool view::mouse_up(const mouse_event&) {
  return false;
}

bool view::mouse_move(const mouse_event&) {
  return false;
}

bool view::mouse_enter(const mouse_event&) {
  return false;
}

bool view::mouse_leave(const mouse_event&) {
  return false;
}

bool view::mouse_wheel(const mouse_event&) {
  return false;
}

bool view::key_down(const key_event&) {
  return false;
}

bool view::key_up(const key_event&) {
  return false;
}

bool view::key_char(const key_event&) {
  return false;
}

void view::on_focus_changed(bool) {}

float view::width() const {
  return YGNodeLayoutGetWidth(d_->node_);
}

view& view::width(float width) {
  YGNodeStyleSetWidth(d_->node_, width);
  return *this;
}

view& view::auto_width() {
  YGNodeStyleSetWidthAuto(d_->node_);
  return *this;
}

float view::height() const {
  return YGNodeLayoutGetHeight(d_->node_);
}

view& view::fill_height(float percent) {
  YGNodeStyleSetHeightPercent(d_->node_, percent);
  return *this;
}

view& view::height(float height) {
  YGNodeStyleSetHeight(d_->node_, height);
  return *this;
}

view& view::auto_height() {
  YGNodeStyleSetHeightAuto(d_->node_);
  return *this;
}

view& view::flex_dir(FlexDirection dir) {
  YGNodeStyleSetFlexDirection(d_->node_, unscopedEnum(dir));
  return *this;
}

view& view::justify_content(Justify justify) {
  YGNodeStyleSetJustifyContent(d_->node_, unscopedEnum(justify));
  return *this;
}

view& view::align_items(Align align) {
  YGNodeStyleSetAlignItems(d_->node_, unscopedEnum(align));
  return *this;
}

view& view::padding(float margin) {
  YGNodeStyleSetPadding(d_->node_, YGEdgeTop, margin);
  YGNodeStyleSetPadding(d_->node_, YGEdgeLeft, margin);
  YGNodeStyleSetPadding(d_->node_, YGEdgeBottom, margin);
  YGNodeStyleSetPadding(d_->node_, YGEdgeRight, margin);
  return *this;
}

view& view::padding(float horizontal, float vertical) {
  YGNodeStyleSetPadding(d_->node_, YGEdgeLeft, horizontal);
  YGNodeStyleSetPadding(d_->node_, YGEdgeRight, horizontal);
  YGNodeStyleSetPadding(d_->node_, YGEdgeTop, vertical);
  YGNodeStyleSetPadding(d_->node_, YGEdgeBottom, vertical);
  return *this;
}

view& view::padding(float left, float top, float right, float bottom) {
  YGNodeStyleSetPadding(d_->node_, YGEdgeLeft, left);
  YGNodeStyleSetPadding(d_->node_, YGEdgeTop, top);
  YGNodeStyleSetPadding(d_->node_, YGEdgeRight, right);
  YGNodeStyleSetPadding(d_->node_, YGEdgeBottom, bottom);
  return *this;
}

rect view::get_layout_rect() const {
  return d_->get_layout_rect();
}

void view::enable_measure() {
  d_->enable_measure();
}

size view::on_measure(MeasureMode, MeasureMode, const ui::size& sz) {
  return sz;
}

YGNodeRef view::yoga_node() const {
  return d_->node_;
}

void view::mark_draw_dirty() {}

void view::mark_layout_dirty() {
  d_->mark_layout_dirty();
  if (parent_) {
    parent_->mark_layout_dirty();
  }
}

void view::measure_children() {

}

} // namespace ui

namespace priv {
view_impl::view_impl(ui::view* inview) : v_(inview) {
  // when width/height auto enabled, measure function should be called
  //setMeasureFunc([](YGNodeConstRef node, float width, YGMeasureMode widthMode, float height,
  //                  YGMeasureMode heightMode) -> YGSize {
  //        return {0,0};
  //    });
  node_ = YGNodeNew();
  YGNodeSetContext(node_, v_);
}
void view_impl::init_with_frame(ui::rect const& r) {
  init_frame_ = r;
  YGNodeStyleSetWidth(node_, r.width);
  YGNodeStyleSetHeight(node_, r.height);
}
bool view_impl::is_layout_dirty() const {
  return manual_layout_dirty_ || YGNodeIsDirty(node_);
}
void view_impl::enable_measure() {
  // when width/height auto enabled, measure function should be called
  YGNodeSetMeasureFunc(node_,
                       [](YGNodeConstRef node, float width, YGMeasureMode widthMode, float height,
                          YGMeasureMode heightMode) -> YGSize {
                         ui::view* v = (ui::view*)YGNodeGetContext(node);
                         auto sz = v->on_measure(ui::scopedEnum(widthMode), ui::scopedEnum(heightMode),
                                       {width, height});
                         return {sz.width, sz.height};
                       });
}
void view_impl::mark_layout_dirty() {
  manual_layout_dirty_ = true;
  const bool has_measure = YGNodeHasMeasureFunc(node_) == true;
  const bool is_leaf = YGNodeGetChildCount(node_) == 0;
  if (has_measure && is_leaf && !YGNodeIsDirty(node_)) {
    YGNodeMarkDirty(node_);
  }
}
void view_impl::layout() {
  YGNodeCalculateLayout(node_, init_frame_.width, init_frame_.height, YGDirectionLTR);
  manual_layout_dirty_ = false;
}
ui::rect view_impl::get_layout_rect() const {
  return ui::rect{YGNodeLayoutGetLeft(node_), YGNodeLayoutGetTop(node_),
                  YGNodeLayoutGetWidth(node_), YGNodeLayoutGetHeight(node_)};
}
} // namespace priv
} // namespace iris
