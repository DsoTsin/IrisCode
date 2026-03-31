#include "dock_window.hpp"

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace iris {
namespace ui {
namespace {
vector<dock_window*>& open_dock_windows() {
  static vector<dock_window*> s_open_windows;
  return s_open_windows;
}

bool should_skip_native_dock_window_show() {
  const char* value = std::getenv("IRIS_DOCK_TEST_DISABLE_NATIVE_SHOW");
  return value != nullptr && value[0] != '\0' && value[0] != '0';
}
} // namespace

dock_window::dock_window(window* owner_window) : window(owner_window), root_dock_view_(new dock_view()) {
  set_content_view(root_dock_view_.get());
}

dock_window::~dock_window() {}

void dock_window::close() {
  if (closing_) {
    return;
  }
  if (hosted_view_ && last_dock_target_) {
    if (redock_to(last_dock_target_, last_dock_position_)) {
      return;
    }
  }
  closing_ = true;
  release_open_window(this);
  window::close();
}

bool dock_window::activates_on_show() const {
  return false;
}

bool dock_window::paints_synchronously_on_show() const {
  return false;
}

bool dock_window::uses_fancy_border() const {
  return false;
}

bool dock_window::uses_window_compositor() const {
  return false;
}

bool dock_window::set_hosted_view(view* child_view) {
  if (!child_view) {
    return false;
  }
  if (hosted_view_ == child_view) {
    return true;
  }
  if (hosted_view_) {
    return false;
  }

  dock_view* owner_dock = nullptr;
  for (auto* ancestor = child_view->parent(); ancestor != nullptr; ancestor = ancestor->parent()) {
    owner_dock = dynamic_cast<dock_view*>(ancestor);
    if (owner_dock) {
      break;
    }
  }

  if (owner_dock) {
    set_last_dock_target(owner_dock, owner_dock->dock_position_of(child_view));
    owner_dock->undock(child_view);
  } else if (auto* parent = child_view->parent()) {
    parent->detach_child(child_view);
  }

  root_dock_view_->dock(child_view, dock_position::fill);
  hosted_view_ = child_view;
  return true;
}

view* dock_window::hosted_view() const {
  return hosted_view_;
}

dock_view* dock_window::root_dock_view() const {
  return root_dock_view_.get();
}

void dock_window::set_last_dock_target(dock_view* dock_target, dock_position position) {
  last_dock_target_ = dock_target;
  last_dock_position_ = position;
}

dock_view* dock_window::last_dock_target() const {
  return last_dock_target_;
}

dock_position dock_window::last_dock_position() const {
  return last_dock_position_;
}

bool dock_window::redock_to(dock_view* dock_target) {
  return redock_to(dock_target ? dock_target : last_dock_target_, last_dock_position_);
}

bool dock_window::redock_to(dock_view* dock_target, dock_position position) {
  if (!dock_target || !hosted_view_) {
    return false;
  }

  cancel_interaction();
  if (auto* dock_target_window = dock_target->owning_window()) {
    dock_target_window->cancel_interaction();
  }

  auto* child_view = hosted_view_;
  root_dock_view_->undock(child_view);
  dock_target->dock(child_view, position);
  set_last_dock_target(dock_target, position);
  hosted_view_ = nullptr;
  if (should_skip_native_dock_window_show()) {
    release_open_window(this);
  } else {
    close();
  }
  dock_target->mark_layout_dirty();
  if (!should_skip_native_dock_window_show()) {
    if (auto* dock_target_window = dock_target->owning_window()) {
#ifdef _WIN32
      if (auto* hwnd = reinterpret_cast<HWND>(dock_target_window->native_handle())) {
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
#endif
    }
  }
  return true;
}

void dock_window::retain_open_window(dock_window* floating_window) {
  if (!floating_window) {
    return;
  }
  auto& windows = open_dock_windows();
  if (std::find(windows.begin(), windows.end(), floating_window) != windows.end()) {
    return;
  }
  floating_window->retain();
  windows.push_back(floating_window);
}

void dock_window::release_open_window(dock_window* floating_window) {
  if (!floating_window) {
    return;
  }
  auto& windows = open_dock_windows();
  auto it = std::find(windows.begin(), windows.end(), floating_window);
  if (it == windows.end()) {
    return;
  }
  windows.erase(it);
  floating_window->release();
}

} // namespace ui
} // namespace iris
