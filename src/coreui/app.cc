#include "app_internal.hpp"

#ifdef _WIN32
#include "platform/win/win32_app.hpp"
#endif

namespace iris {

static ref_ptr<app> g_app;

app::app(const string& name) : d(new priv::app(name)) {}

app::~app() {
  if (d) {
    delete d;
    d = nullptr;
  }
}

void app::add_window(ui::window* pwindow) {
  d->add_window(pwindow);
}

void app::remove_window(ui::window* pwindow) {
  d->remove_window(pwindow);
}

void app::request_exit() {
  d->request_exit();
}

const char* app::name() const {
  return d->name_.c_str();
}

app* app::shared() {
  if (!g_app) {
    g_app = ref_ptr<app>(new app("test"));
  }
  return g_app.get();
}

void app::set_delegate(app_delegate* in_delegate) {
  d->set_delegate(in_delegate);
}

int app::run() {
  d->loop();
  return 0;
}

namespace priv {

void app::ensure_delegate_initialized() {
  if (!delegate_ || delegate_initialized_) {
    return;
  }
  delegate_->init();
  delegate_initialized_ = true;
}

void app::sync_delegate_state() {
  if (!delegate_) {
    return;
  }
  if (run_started_) {
    ensure_delegate_initialized();
  }
  if (!delegate_initialized_) {
    return;
  }
  if (!in_background_) {
    delegate_->did_enter_foreground();
  }
  if (app_active_) {
    delegate_->did_become_active();
  }
}

app::app(const string& name) : name_(name) {
#ifdef _WIN32
  platform::win32::init_app(*this);
#endif
}

app::~app() {
#ifdef _WIN32
  platform::win32::shutdown_app(*this);
#endif
}

void app::loop() {
  mark_run_started();
#ifdef _WIN32
  platform::win32::loop_app(*this);
#endif
  notify_will_terminate();
}

void app::pump_message() {
#ifdef _WIN32
  platform::win32::pump_messages(*this);
#endif
}

void app::add_window(ui::window* w) {
#ifdef _WIN32
  platform::win32::add_window(*this, w);
#else
  (void)w;
#endif
}

void app::remove_window(ui::window* w) {
#ifdef _WIN32
  platform::win32::remove_window(*this, w);
#else
  (void)w;
#endif
}

void app::set_delegate(app_delegate* in_delegate) {
  auto* previous_delegate = delegate_;
#ifdef _WIN32
  platform::win32::set_delegate(*this, in_delegate);
#else
  delegate_ = in_delegate;
#endif
  if (previous_delegate != delegate_) {
    delegate_initialized_ = false;
  }
  sync_delegate_state();
}

void app::request_exit() {
  if (request_exit_) {
    return;
  }
  request_exit_ = true;
  notify_will_terminate();
}

void app::mark_run_started() {
  if (run_started_) {
    return;
  }
  run_started_ = true;
  ensure_delegate_initialized();
}

void app::notify_app_activation(bool active) {
  const bool was_active = app_active_;
  const bool was_in_background = in_background_;

  app_active_ = active;
  in_background_ = !active;

  if (!delegate_) {
    return;
  }

  ensure_delegate_initialized();
  if (!delegate_initialized_) {
    return;
  }

  if (active) {
    if (was_in_background) {
      delegate_->did_enter_foreground();
    }
    if (!was_active) {
      delegate_->did_become_active();
    }
    return;
  }

  if (was_active) {
    delegate_->did_resign_active();
  }
  if (!was_in_background) {
    delegate_->did_enter_background();
  }
}

void app::notify_will_terminate() {
  if (termination_notified_) {
    return;
  }
  termination_notified_ = true;
  if (!delegate_) {
    return;
  }
  ensure_delegate_initialized();
  if (delegate_initialized_) {
    delegate_->will_terminate();
  }
}

} // namespace priv
} // namespace iris
