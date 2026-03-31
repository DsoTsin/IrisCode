#include "dock_view.hpp"

#include "../dock_window.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkPaint.h"

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace iris {
namespace ui {
namespace {
bool should_skip_native_dock_window_show() {
  const char* value = std::getenv("IRIS_DOCK_TEST_DISABLE_NATIVE_SHOW");
  return value != nullptr && value[0] != '\0' && value[0] != '0';
}

constexpr float kDockHeaderHeight = 28.0f;
constexpr float kDockGripInsetX = 10.0f;
constexpr float kDockGripInsetY = 11.0f;
constexpr float kDockGripWidth = 18.0f;
constexpr float kDockGripSpacing = 4.0f;
constexpr float kDockDragThreshold = 6.0f;

struct dock_drop_preview_state {
  dock_view* target = nullptr;
  dock_position position = dock_position::fill;
};

dock_drop_preview_state& drop_preview_state() {
  static dock_drop_preview_state state;
  return state;
}

double point_distance_squared(const point& lhs, const point& rhs) {
  const double dx = lhs.x - rhs.x;
  const double dy = lhs.y - rhs.y;
  return dx * dx + dy * dy;
}

point window_point_to_screen(window* owner_window, const point& window_point) {
  if (!owner_window) {
    return {};
  }
  const auto frame_rect = owner_window->frame();
  return {frame_rect.left + window_point.x, frame_rect.top + window_point.y};
}

bool point_in_rect(const rect& bounds, const point& screen_point) {
  return screen_point.x >= bounds.left && screen_point.y >= bounds.top
         && screen_point.x < bounds.right() && screen_point.y < bounds.bottom();
}

rect preview_rect_for_bounds(const rect& bounds, dock_position position) {
  rect preview = bounds;
  switch (position) {
  case dock_position::left:
    preview.width *= 0.28f;
    break;
  case dock_position::right:
    preview.left = bounds.left + bounds.width * 0.72f;
    preview.width *= 0.28f;
    break;
  case dock_position::top:
    preview.height *= 0.28f;
    break;
  case dock_position::bottom:
    preview.top = bounds.top + bounds.height * 0.72f;
    preview.height *= 0.28f;
    break;
  case dock_position::fill:
    preview.left += bounds.width * 0.18f;
    preview.top += bounds.height * 0.18f;
    preview.width *= 0.64f;
    preview.height *= 0.64f;
    break;
  }
  return preview;
}

void redraw_dock_target(dock_view* target) {
  if (!target) {
    return;
  }
  if (auto* owner_window = target->owning_window()) {
#ifdef _WIN32
    if (auto* hwnd = reinterpret_cast<HWND>(owner_window->native_handle())) {
      ::InvalidateRect(hwnd, nullptr, FALSE);
    }
#else
    owner_window->draw();
#endif
  }
}

void update_drop_preview(dock_view* target, dock_position position) {
  auto& state = drop_preview_state();
  if (state.target == target && state.position == position) {
    return;
  }
  auto* previous_target = state.target;
  state.target = target;
  state.position = position;
  redraw_dock_target(previous_target);
  redraw_dock_target(target);
}

void update_drop_preview_for_window(dock_window* floating_window, const point& screen_point) {
  if (!floating_window) {
    update_drop_preview(nullptr, dock_position::fill);
    return;
  }

  auto* dock_target = floating_window->last_dock_target();
  if (!dock_target) {
    update_drop_preview(nullptr, dock_position::fill);
    return;
  }

  const auto target_rect = dock_target->screen_rect();
  if (!point_in_rect(target_rect, screen_point)) {
    update_drop_preview(nullptr, dock_position::fill);
    return;
  }

  const point local_point{screen_point.x - target_rect.left, screen_point.y - target_rect.top};
  update_drop_preview(dock_target, dock_target->drop_position_for(local_point));
}

void move_native_window(window* owner_window, const point& top_left) {
#ifdef _WIN32
  if (!owner_window) {
    return;
  }
  if (auto* hwnd = reinterpret_cast<HWND>(owner_window->native_handle())) {
    ::SetWindowPos(hwnd, nullptr, static_cast<int>(top_left.x), static_cast<int>(top_left.y), 0, 0,
                   SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
  }
#else
  (void)owner_window;
  (void)top_left;
#endif
}

class dock_item_host_view : public view {
public:
  dock_item_host_view(dock_view* owner_dock, view* content_view)
      : owner_dock_(owner_dock), content_view_(content_view) {
    if (content_view_) {
      add_child(content_view_);
    }
  }

  view* content_view() const {
    return content_view_;
  }

  virtual void draw(const rect& dirty_rect) override {
    if (gfx_ctx_) {
      if (auto* canvas = gfx_ctx_->canvas()) {
        const auto bounds = get_layout_rect();
        const float header_height = min(kDockHeaderHeight, bounds.height);
        SkPaint header_paint;
        header_paint.setAntiAlias(true);
        header_paint.setColor(SkColorSetARGB(0xFF, 0x2A, 0x2A, 0x2A));
        canvas->drawRect(SkRect::MakeXYWH(bounds.left, bounds.top, bounds.width, header_height),
                         header_paint);

        SkPaint grip_paint;
        grip_paint.setAntiAlias(true);
        grip_paint.setColor(SkColorSetARGB(0xFF, 0x8A, 0x8A, 0x8A));
        grip_paint.setStrokeWidth(2.0f);
        const float grip_left = bounds.left + kDockGripInsetX;
        const float grip_top = bounds.top + kDockGripInsetY;
        for (int i = 0; i < 3; ++i) {
          const float y = grip_top + i * kDockGripSpacing;
          canvas->drawLine(grip_left, y, grip_left + kDockGripWidth, y, grip_paint);
        }

        SkPaint border_paint;
        border_paint.setStyle(SkPaint::kStroke_Style);
        border_paint.setStrokeWidth(1.0f);
        border_paint.setAntiAlias(true);
        border_paint.setColor(SkColorSetARGB(0x80, 0x4A, 0x4A, 0x4A));
        canvas->drawRect(SkRect::MakeXYWH(bounds.left, bounds.top, bounds.width, bounds.height),
                         border_paint);
      }
    }
    view::draw(dirty_rect);
  }

  virtual void layout_if_needed() override {
    view::layout_if_needed();
    if (!content_view_) {
      return;
    }
    const auto bounds = get_layout_rect();
    auto node = content_view_->layout_node();
    YGNodeStyleSetPositionType(node, YGPositionTypeAbsolute);
    YGNodeStyleSetPosition(node, YGEdgeLeft, 0.0f);
    YGNodeStyleSetPosition(node, YGEdgeTop, min(kDockHeaderHeight, bounds.height));
    YGNodeStyleSetWidth(node, max(bounds.width, 0.0f));
    YGNodeStyleSetHeight(node, max(bounds.height - kDockHeaderHeight, 0.0f));
    content_view_->mark_layout_dirty();
    content_view_->layout_if_needed();
  }

  virtual view* hit_test(point const& p) const override {
    const auto bounds = get_layout_rect();
    if (p.x < 0.0 || p.y < 0.0 || p.x >= bounds.width || p.y >= bounds.height) {
      return nullptr;
    }
    if (p.y < min(kDockHeaderHeight, bounds.height)) {
      return const_cast<dock_item_host_view*>(this);
    }
    return view::hit_test(p);
  }

  virtual bool mouse_down(const mouse_event& event) override {
    if (event.button != mouse_button::left) {
      return false;
    }

    drag_source_window_ = owning_window();
    drag_start_screen_ = window_point_to_screen(drag_source_window_, event.location_in_window);
    drag_offset_ = dynamic_cast<dock_window*>(drag_source_window_) ? event.location_in_window
                                                                   : event.location_in_view;
    dragging_ = false;
    header_pressed_ = true;

    if (event.is_double_click) {
      if (auto* floating_owner = dynamic_cast<dock_window*>(drag_source_window_)) {
        if (auto* dock_target = floating_owner->last_dock_target()) {
          retain_internal();
          floating_owner->redock_to(dock_target);
          release_internal();
        }
        header_pressed_ = false;
        return true;
      }

      if (owner_dock_ && content_view_) {
        retain_internal();
        drag_window_ = owner_dock_->undock_to_window(content_view_);
        if (drag_window_) {
          drag_window_ = {};
        }
        release_internal();
        header_pressed_ = false;
        return true;
      }
    }

    return true;
  }

  virtual bool mouse_move(const mouse_event& event) override {
    if (!header_pressed_ || event.button != mouse_button::left || !drag_source_window_) {
      return false;
    }

    const auto current_screen = window_point_to_screen(drag_source_window_, event.location_in_window);
    if (!dragging_ && point_distance_squared(current_screen, drag_start_screen_)
                          < (kDockDragThreshold * kDockDragThreshold)) {
      return true;
    }
    dragging_ = true;

    if (auto* floating_owner = dynamic_cast<dock_window*>(drag_source_window_)) {
      move_native_window(floating_owner,
                         {current_screen.x - drag_offset_.x, current_screen.y - drag_offset_.y});
      update_drop_preview_for_window(floating_owner, current_screen);
      return true;
    }
    return true;
  }

  virtual bool mouse_up(const mouse_event& event) override {
    if (!header_pressed_ || event.button != mouse_button::left) {
      return false;
    }

    auto* preview_target = drop_preview_state().target;
    const auto preview_position = drop_preview_state().position;
    update_drop_preview(nullptr, dock_position::fill);

    if (auto* floating_owner = dynamic_cast<dock_window*>(drag_source_window_)) {
      if (dragging_ && preview_target) {
        retain_internal();
        floating_owner->redock_to(preview_target, preview_position);
        release_internal();
      }
    } else if (dragging_ && owner_dock_ && content_view_) {
      retain_internal();
      const auto release_screen = window_point_to_screen(drag_source_window_, event.location_in_window);
      auto floating_window = owner_dock_->undock_to_window(content_view_);
      if (floating_window) {
        const auto content = floating_window->content_rect();
        move_native_window(floating_window.get(),
                           {release_screen.x - content.left - event.location_in_view.x,
                            release_screen.y - content.top - event.location_in_view.y});
        if (preview_target) {
          floating_window->redock_to(preview_target, preview_position);
        }
        if (drag_source_window_) {
          drag_source_window_->cancel_interaction();
        }
      }
      release_internal();
    }

    header_pressed_ = false;
    dragging_ = false;
    drag_source_window_ = nullptr;
    drag_offset_ = {};
    return true;
  }

private:
  dock_view* owner_dock_ = nullptr;
  view* content_view_ = nullptr;
  window* drag_source_window_ = nullptr;
  point drag_start_screen_;
  point drag_offset_;
  bool header_pressed_ = false;
  bool dragging_ = false;
  ref_ptr<dock_window> drag_window_;
};
} // namespace

dock_view::dock_view(view* parent) : view(parent) {}

dock_view::~dock_view() {}

void dock_view::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);

  if (!gfx_ctx_ || drop_preview_state().target != this) {
    return;
  }
  auto* canvas = gfx_ctx_->canvas();
  if (!canvas) {
    return;
  }

  const auto preview_rect = preview_rect_for_bounds(get_layout_rect(), drop_preview_state().position);
  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setColor(SkColorSetARGB(0x60, 0x3A, 0x8D, 0xFF));
  canvas->drawRect(SkRect::MakeXYWH(preview_rect.left, preview_rect.top, preview_rect.width,
                                    preview_rect.height),
                   fill_paint);

  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setStrokeWidth(2.0f);
  stroke_paint.setColor(SkColorSetARGB(0xD0, 0x78, 0xB3, 0xFF));
  canvas->drawRect(SkRect::MakeXYWH(preview_rect.left, preview_rect.top, preview_rect.width,
                                    preview_rect.height),
                   stroke_paint);
}

void dock_view::layout_if_needed() {
  view::layout_if_needed();

  auto bounds = get_layout_rect();
  float left = padding_;
  float top = padding_;
  float right = max(bounds.width - padding_, left);
  float bottom = max(bounds.height - padding_, top);

  vector<view*> fill_items;
  fill_items.reserve(docked_items_.size());
  for (const auto& item : docked_items_) {
    auto* host = item.host_view.get();
    if (!host) {
      continue;
    }
    if (item.position == dock_position::fill) {
      fill_items.push_back(host);
      continue;
    }

    auto node = host->yoga_node();
    YGNodeStyleSetPositionType(node, YGPositionTypeAbsolute);

    switch (item.position) {
    case dock_position::left: {
      const float width = max(default_side_dock_size_, 0.0f);
      YGNodeStyleSetPosition(node, YGEdgeLeft, left);
      YGNodeStyleSetPosition(node, YGEdgeTop, top);
      YGNodeStyleSetWidth(node, min(width, max(right - left, 0.0f)));
      YGNodeStyleSetHeight(node, max(bottom - top, 0.0f));
      left += width + padding_;
      break;
    }
    case dock_position::right: {
      const float width = max(default_side_dock_size_, 0.0f);
      right -= width;
      YGNodeStyleSetPosition(node, YGEdgeLeft, max(right, left));
      YGNodeStyleSetPosition(node, YGEdgeTop, top);
      YGNodeStyleSetWidth(node, min(width, max(right - left + width, 0.0f)));
      YGNodeStyleSetHeight(node, max(bottom - top, 0.0f));
      right -= padding_;
      break;
    }
    case dock_position::top: {
      const float height = max(default_edge_dock_size_, 0.0f);
      YGNodeStyleSetPosition(node, YGEdgeLeft, left);
      YGNodeStyleSetPosition(node, YGEdgeTop, top);
      YGNodeStyleSetWidth(node, max(right - left, 0.0f));
      YGNodeStyleSetHeight(node, min(height, max(bottom - top, 0.0f)));
      top += height + padding_;
      break;
    }
    case dock_position::bottom: {
      const float height = max(default_edge_dock_size_, 0.0f);
      bottom -= height;
      YGNodeStyleSetPosition(node, YGEdgeLeft, left);
      YGNodeStyleSetPosition(node, YGEdgeTop, max(bottom, top));
      YGNodeStyleSetWidth(node, max(right - left, 0.0f));
      YGNodeStyleSetHeight(node, min(height, max(bottom - top + height, 0.0f)));
      bottom -= padding_;
      break;
    }
    case dock_position::fill:
      break;
    }
    host->mark_layout_dirty();
    host->layout_if_needed();
  }

  if (fill_items.empty()) {
    return;
  }

  const float available_width = max(right - left, 0.0f);
  vector<float> fixed_widths(fill_items.size(), -1.0f);
  float fixed_width_sum = 0.0f;
  size_t flexible_count = 0;

  for (size_t i = 0; i < fill_items.size(); ++i) {
    auto* fill = fill_items[i];
    if (!fill) {
      continue;
    }
    auto node = fill->yoga_node();
    const auto width_value = YGNodeStyleGetWidth(node);
    if (width_value.unit == YGUnitPoint) {
      const float fixed_width = max(width_value.value, 0.0f);
      fixed_widths[i] = fixed_width;
      fixed_width_sum += fixed_width;
      continue;
    }
    ++flexible_count;
  }

  const float flexible_width = max(available_width - fixed_width_sum, 0.0f);
  const float each_flexible_width = flexible_count > 0 ? (flexible_width / flexible_count) : 0.0f;
  float cursor = left;

  for (size_t i = 0; i < fill_items.size(); ++i) {
    auto* fill = fill_items[i];
    if (!fill) {
      continue;
    }
    auto node = fill->yoga_node();
    YGNodeStyleSetPositionType(node, YGPositionTypeAbsolute);
    YGNodeStyleSetPosition(node, YGEdgeLeft, cursor);
    YGNodeStyleSetPosition(node, YGEdgeTop, top);
    const float node_width = fixed_widths[i] >= 0.0f ? fixed_widths[i] : each_flexible_width;
    const float clamped_width = min(node_width, max(right - cursor, 0.0f));
    YGNodeStyleSetWidth(node, clamped_width);
    YGNodeStyleSetHeight(node, max(bottom - top, 0.0f));
    cursor += clamped_width;
    fill->mark_layout_dirty();
    fill->layout_if_needed();
  }
}

void dock_view::dock(view* child_view, dock_position position) {
  if (!child_view) {
    return;
  }
  auto it = std::find_if(docked_items_.begin(), docked_items_.end(),
                         [child_view](const docked_item& item) {
                           return item.content_view_ptr == child_view;
                         });
  if (it != docked_items_.end()) {
    it->position = position;
    mark_layout_dirty();
    return;
  }

  ref_ptr<view> host_view(new dock_item_host_view(this, child_view));
  docked_items_.push_back({child_view, host_view, position});
  add_child(host_view.get());
  mark_layout_dirty();
}

void dock_view::undock(view* child_view) {
  if (!child_view) {
    return;
  }
  auto it = std::find_if(docked_items_.begin(), docked_items_.end(),
                         [child_view](const docked_item& item) {
                           return item.content_view_ptr == child_view;
                         });
  if (it == docked_items_.end()) {
    return;
  }

  if (auto* host_view = dynamic_cast<dock_item_host_view*>(it->host_view.get())) {
    if (host_view->content_view() == child_view && child_view->parent() == host_view) {
      host_view->detach_child(child_view);
    }
  }
  remove_child(it->host_view.get());
  docked_items_.erase(it);
  mark_layout_dirty();
}

dock_position dock_view::dock_position_of(view* child_view) const {
  auto it = std::find_if(docked_items_.begin(), docked_items_.end(),
                         [child_view](const docked_item& item) {
                           return item.content_view_ptr == child_view;
                         });
  return it == docked_items_.end() ? dock_position::fill : it->position;
}

ref_ptr<dock_window> dock_view::undock_to_window(view* child_view) {
  if (!child_view) {
    return {};
  }

  const auto original_position = dock_position_of(child_view);
  auto* owner_window = owning_window();
  if (owner_window) {
    owner_window->cancel_interaction();
  }

  ref_ptr<dock_window> floating_window(new dock_window());
  floating_window->set_last_dock_target(this, original_position);
  if (!floating_window->set_hosted_view(child_view)) {
    return {};
  }

  dock_window::retain_open_window(floating_window.get());
  const bool skip_native_show = should_skip_native_dock_window_show();
  if (!skip_native_show) {
    floating_window->show();
    if (owner_window) {
#ifdef _WIN32
      if (auto* hwnd = reinterpret_cast<HWND>(owner_window->native_handle())) {
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
#endif
    }
  }
  return floating_window;
}

bool dock_view::redock_from_window(dock_window* floating_window) {
  if (!floating_window) {
    return false;
  }
  return floating_window->redock_to(this);
}

bool dock_view::redock_from_window(dock_window* floating_window, dock_position position) {
  if (!floating_window) {
    return false;
  }
  return floating_window->redock_to(this, position);
}

dock_position dock_view::drop_position_for(point const& local_point) const {
  const auto bounds = get_layout_rect();
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return dock_position::fill;
  }

  const double left_zone = bounds.width * 0.25;
  const double right_zone = bounds.width * 0.75;
  const double top_zone = bounds.height * 0.25;
  const double bottom_zone = bounds.height * 0.75;

  if (local_point.x < left_zone) {
    return dock_position::left;
  }
  if (local_point.x > right_zone) {
    return dock_position::right;
  }
  if (local_point.y < top_zone) {
    return dock_position::top;
  }
  if (local_point.y > bottom_zone) {
    return dock_position::bottom;
  }
  return dock_position::fill;
}

rect dock_view::docked_item_screen_rect(view* child_view) const {
  auto it = std::find_if(docked_items_.begin(), docked_items_.end(),
                         [child_view](const docked_item& item) {
                           return item.content_view_ptr == child_view;
                         });
  if (it == docked_items_.end() || !it->host_view) {
    return {};
  }
  return it->host_view->screen_rect();
}

rect dock_view::docked_item_header_screen_rect(view* child_view) const {
  auto bounds = docked_item_screen_rect(child_view);
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return {};
  }
  bounds.height = min(bounds.height, kDockHeaderHeight);
  return bounds;
}

void dock_view::set_dock_padding(float padding) {
  padding_ = max(padding, 0.0f);
  mark_layout_dirty();
}

float dock_view::dock_padding() const {
  return padding_;
}

void dock_view::set_default_dock_size(float side, float edge) {
  default_side_dock_size_ = max(side, 0.0f);
  default_edge_dock_size_ = max(edge, 0.0f);
  mark_layout_dirty();
}

float dock_view::default_side_dock_size() const {
  return default_side_dock_size_;
}

float dock_view::default_edge_dock_size() const {
  return default_edge_dock_size_;
}

} // namespace ui
} // namespace iris
