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
  // Layout docked views based on their positions
}

void dock_view::dock(view* child_view, dock_position position) {
  if (child_view) {
    docked_items_.push_back({child_view, position});
    add_child(child_view);
  }
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
    }
  }
}

void dock_view::set_dock_padding(float padding) {
  padding_ = padding;
}

float dock_view::dock_padding() const {
  return padding_;
}

} // namespace ui
} // namespace iris