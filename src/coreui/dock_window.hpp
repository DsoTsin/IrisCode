#pragma once

#include "window.hpp"
#include "view/dock_view.hpp"

namespace iris {
namespace ui {

class UICORE_API dock_window : public window {
public:
  explicit dock_window(window* owner_window = nullptr);
  virtual ~dock_window();
  virtual void close() override;
  virtual bool activates_on_show() const override;
  virtual bool paints_synchronously_on_show() const override;
  virtual bool uses_fancy_border() const override;
  virtual bool uses_window_compositor() const override;

  bool set_hosted_view(view* child_view);
  view* hosted_view() const;
  dock_view* root_dock_view() const;

  void set_last_dock_target(dock_view* dock_target, dock_position position);
  dock_view* last_dock_target() const;
  dock_position last_dock_position() const;

  bool redock_to(dock_view* dock_target);
  bool redock_to(dock_view* dock_target, dock_position position);

  static void retain_open_window(dock_window* floating_window);
  static void release_open_window(dock_window* floating_window);

private:
  ref_ptr<dock_view> root_dock_view_;
  view* hosted_view_ = nullptr;
  dock_view* last_dock_target_ = nullptr;
  dock_position last_dock_position_ = dock_position::fill;
  bool closing_ = false;
};

} // namespace ui
} // namespace iris
