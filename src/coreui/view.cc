#include "view.hpp"

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

  ui::rect init_frame_;
};
} // namespace priv
namespace ui {
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
    subview->draw(dirty_rect);
  }
}
void view::layout_if_needed() {
  if (d_->is_layout_dirty()) {
    d_->layout();
  }
}
void view::add_child(view* v) {
  auto index = subviews_.size();
  subviews_.push_back(v);
  YGNodeInsertChild(d_->node_, v->d_->node_, index);
}
void view::remove_child(view* v) {
  auto iter = std::find(subviews_.begin(), subviews_.end(), v);
  subviews_.erase(iter);
  YGNodeRemoveChild(d_->node_, v->d_->node_);
}

view* view::hit_test(point const& p) const {
  return nullptr;
}

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

void view::mark_draw_dirty() {}

void view::mark_layout_dirty() {
  d_->mark_layout_dirty();
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
  return YGNodeIsDirty(node_);
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
  if (!YGNodeIsDirty(node_)) {
    YGNodeMarkDirty(node_);
  }
}
void view_impl::layout() {
  YGNodeCalculateLayout(node_, init_frame_.width, init_frame_.height, YGDirectionLTR);
}
ui::rect view_impl::get_layout_rect() const {
  return ui::rect{YGNodeLayoutGetLeft(node_), YGNodeLayoutGetTop(node_),
                  YGNodeLayoutGetWidth(node_), YGNodeLayoutGetHeight(node_)};
}
} // namespace priv
} // namespace iris