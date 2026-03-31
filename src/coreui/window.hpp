#pragma once

#include "uidefs.hpp"
#include <functional>

#define ENABLE_MICA 0

namespace iris {
namespace platform::win32 {
class access;
}
namespace priv {
class app;
class window;
class surface;
class graphics_context;
}
namespace ui {
class window;
class view_controller;
class window_controller {
public:
  virtual ~window_controller() {}

  virtual void did_load() {}
  virtual void will_load() {}

  void close() {}
  void show() {}

protected:
  window* window_;
  view_controller* view_controller_;
};
class view;


class surface : public ref_counted
{
public:
  surface() {}
  virtual ~surface() {}

private:
  priv::surface* d;
};

class UICORE_API window : public ref_counted {
  public:
    window(window* in_window = nullptr);
  
    virtual void initialize();
    virtual void finalize();
  
    virtual void set_content_view(view* inview);
    virtual void set_delegate();
  
    virtual void show();
    virtual void close();
    virtual bool activates_on_show() const;
    virtual bool paints_synchronously_on_show() const;
    virtual bool uses_fancy_border() const;
    virtual bool uses_window_compositor() const;
    virtual void cancel_interaction();
    virtual void request_redraw();
    virtual void request_redraw(const ui::rect& dirty_rect);
    virtual void draw();
    virtual void draw(const ui::rect& dirty_rect);
    virtual uint64_t start_timer(uint32_t interval_ms, function<void()> callback);
    virtual void stop_timer(uint64_t timer_id);

    const ui::rect& frame() const;
    ui::rect content_rect() const;

    void set_border_width(float width);
    float border_width() const;
    void set_shadow_distance(float dist);
    float shadow_distance() const;
    void set_corner_radius(float radius);
    float corner_radius() const;
    virtual bool is_main() const { return true; }
  
    void* native_handle() const;
    virtual void resize(int width, int height);

    virtual int zone_at(int x, int y);

  protected:
    friend class iris::priv::app;
    friend class iris::platform::win32::access;
    virtual bool on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result);
    virtual bool on_window_pos_changed();
    virtual bool on_window_activate(bool active);
    virtual ~window();
  
  private:
    priv::window* d;
  };
} // namespace ui
} // namespace iris
