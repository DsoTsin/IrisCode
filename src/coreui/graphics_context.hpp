#pragma once

#include "uidefs.hpp"

namespace iris {
namespace priv {
class graphics_context;
} // namespace priv
namespace ui {

class window;
class UICORE_API graphics_context : public ref_counted {
public:
  graphics_context(ui::window* w, bool gpu = false);
  virtual ~graphics_context();

  void resize(int width, int height);
  void synchronize(const ui::rect* dirty_rect = nullptr);
  void flush();
  canvas* canvas();
  ui::rect expand_dirty_rect(const ui::rect& dirty_rect) const;
  void on_window_pos_changed();
  void on_window_activate(bool active);
  void on_live_resize_changed(bool active);
  window* window() const;

private:
  priv::graphics_context* d;
};
}
} // namespace iris
