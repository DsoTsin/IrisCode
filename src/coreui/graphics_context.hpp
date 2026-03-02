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
  void synchronize();
  void flush();
  canvas* canvas();
  void on_window_pos_changed();
  void on_window_activate(bool active);
  window* window() const;

private:
  priv::graphics_context* d;
};
}
} // namespace iris