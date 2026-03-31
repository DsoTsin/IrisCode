#pragma once

#include "app.hpp"
#include "window.hpp"

namespace iris {
namespace priv {

struct app_msg {
  uint32_t code = 0;
  ui::window* window = nullptr;
};

class app {
public:
  app(const string& name);
  ~app();

  void set_delegate(app_delegate* in_delegate);
  void loop();
  void pump_message();
  void add_window(ui::window* w);
  void remove_window(ui::window* w);
  void request_exit();
  void mark_run_started();
  void notify_app_activation(bool active);
  void notify_will_terminate();

  app_delegate* delegate_ = nullptr;
  string name_;
  bool request_exit_ = false;
  bool run_started_ = false;
  bool delegate_initialized_ = false;
  bool app_active_ = false;
  bool in_background_ = true;
  bool termination_notified_ = false;

#ifdef _WIN32
  unordered_map<void*, iris::weak_ptr<iris::ui::window>> windows_;
  queue<app_msg> msg_queue_;
#endif

private:
  void ensure_delegate_initialized();
  void sync_delegate_state();
};

} // namespace priv
} // namespace iris
