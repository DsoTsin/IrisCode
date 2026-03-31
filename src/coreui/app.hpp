#pragma once

#include "uidefs.hpp"

namespace iris {
namespace platform::win32 {
class access;
}
namespace priv {
#ifdef _WIN32
// #if __has_include("Windows.h")
// extern HINSTANCE instance;
// #endif
#endif
class app;
class window;
} // namespace priv

namespace ui {
class window;
class window_controller;
} // namespace ui

class app_delegate {
public:
  virtual ~app_delegate() {}

  virtual void init() {}
  virtual void will_terminate() {}

  virtual void did_become_active() {}
  virtual void did_enter_background() {}
  virtual void did_enter_foreground() {}
  // lose focus
  virtual void did_resign_active() {}
};

class UICORE_API app : public ref_counted {
public:
  static app* shared();

  void set_delegate(app_delegate* in_delegate);
  int run();

  const char* name() const;

protected:
  explicit app(const string& name);
  virtual ~app();

  virtual void add_window(ui::window* pwindow);
  virtual void remove_window(ui::window* pwindow);

  void request_exit();

private:
  friend class ui::window;
  friend class priv::window;
  friend class priv::app;
  friend class platform::win32::access;

  priv::app* d;
};

} // namespace iris
