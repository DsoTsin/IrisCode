#include "dock_view.hpp"

namespace iris {
namespace ui {

dock_view::dock_view(view* parent) : view(parent) {}

dock_view::~dock_view() {}

void dock_view::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  // Dock view drawing implementation
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
    if (!item.view_ptr) {
      continue;
    }
    if (item.position == dock_position::fill) {
      fill_items.push_back(item.view_ptr);
      continue;
    }

    auto node = item.view_ptr->yoga_node();
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
    item.view_ptr->mark_layout_dirty();
    item.view_ptr->layout_if_needed();
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
                         [child_view](const docked_item& item) { return item.view_ptr == child_view; });
  if (it != docked_items_.end()) {
    it->position = position;
    mark_layout_dirty();
    return;
  }
  docked_items_.push_back({child_view, position});
  add_child(child_view);
  mark_layout_dirty();
}

void dock_view::undock(view* child_view) {
  if (child_view) {
    auto it = std::find_if(docked_items_.begin(), docked_items_.end(),
                          [child_view](const docked_item& item) {
                            return item.view_ptr == child_view;
                          });
    if (it != docked_items_.end()) {
      docked_items_.erase(it);
      remove_child(child_view);
      mark_layout_dirty();
    }
  }
}

dock_position dock_view::dock_position_of(view* child_view) const {
  auto it = std::find_if(docked_items_.begin(), docked_items_.end(),
                         [child_view](const docked_item& item) { return item.view_ptr == child_view; });
  return it == docked_items_.end() ? dock_position::fill : it->position;
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
