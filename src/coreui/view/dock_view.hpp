#pragma once

#include "../view.hpp"

namespace iris {
namespace ui {

enum class dock_position { left, right, top, bottom, fill };

class UICORE_API dock_view : public view {
public:
  dock_view(view* parent = nullptr);
  virtual ~dock_view();

  virtual void draw(const rect& dirty_rect) override;
  virtual void layout_if_needed() override;

  void dock(view* child_view, dock_position position);
  void undock(view* child_view);
  dock_position dock_position_of(view* child_view) const;

  void set_dock_padding(float padding);
  float dock_padding() const;
  void set_default_dock_size(float side, float edge);
  float default_side_dock_size() const;
  float default_edge_dock_size() const;

protected:
  struct docked_item {
    view* view_ptr;
    dock_position position;
  };

  vector<docked_item> docked_items_;
  float padding_ = 0.0f;
  float default_side_dock_size_ = 220.0f;
  float default_edge_dock_size_ = 120.0f;
};

} // namespace ui
} // namespace iris
