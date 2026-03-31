#include "coreui/app.hpp"
#include "coreui/view/code_editor.hpp"
#include "coreui/view/dock_view.hpp"
#include "coreui/view/label.hpp"
#include "coreui/view.hpp"
#include "coreui/window.hpp"

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <vector>

#pragma comment(linker, "/subsystem:windows")

namespace {
using namespace std::chrono_literals;

struct window_search_state {
  DWORD process_id = 0;
  std::vector<HWND>* handles = nullptr;
};

BOOL CALLBACK collect_process_windows(HWND hwnd, LPARAM l_param) {
  auto* state = reinterpret_cast<window_search_state*>(l_param);
  if (!state || !state->handles) {
    return FALSE;
  }

  DWORD process_id = 0;
  ::GetWindowThreadProcessId(hwnd, &process_id);
  if (process_id != state->process_id) {
    return TRUE;
  }
  if (!::IsWindowVisible(hwnd)) {
    return TRUE;
  }
  state->handles->push_back(hwnd);
  return TRUE;
}

std::vector<HWND> current_process_windows() {
  std::vector<HWND> handles;
  window_search_state state{::GetCurrentProcessId(), &handles};
  ::EnumWindows(&collect_process_windows, reinterpret_cast<LPARAM>(&state));
  return handles;
}

iris::ui::point rect_center(const iris::ui::rect& rect) {
  return {rect.left + rect.width * 0.5, rect.top + rect.height * 0.5};
}

bool move_cursor_to(const iris::ui::point& screen_point) {
  return ::SetCursorPos(static_cast<int>(screen_point.x), static_cast<int>(screen_point.y)) == TRUE;
}

bool send_mouse_button(DWORD flags) {
  INPUT input{};
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = flags;
  return ::SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool drag_between(const iris::ui::point& from,
                  const iris::ui::point& to,
                  int steps,
                  std::chrono::milliseconds delay) {
  if (!move_cursor_to(from)) {
    return false;
  }
  std::this_thread::sleep_for(delay);
  if (!send_mouse_button(MOUSEEVENTF_LEFTDOWN)) {
    return false;
  }

  for (int i = 1; i <= steps; ++i) {
    const double t = static_cast<double>(i) / static_cast<double>(steps);
    const iris::ui::point p{
        from.x + (to.x - from.x) * t,
        from.y + (to.y - from.y) * t,
    };
    if (!move_cursor_to(p)) {
      send_mouse_button(MOUSEEVENTF_LEFTUP);
      return false;
    }
    std::this_thread::sleep_for(delay);
  }

  std::this_thread::sleep_for(delay);
  return send_mouse_button(MOUSEEVENTF_LEFTUP);
}

bool wait_for_window_count(size_t expected, std::chrono::milliseconds timeout, std::vector<HWND>* out = nullptr) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    auto handles = current_process_windows();
    if (handles.size() == expected) {
      if (out) {
        *out = handles;
      }
      return true;
    }
    std::this_thread::sleep_for(50ms);
  }
  return false;
}

bool wait_for_window_absent(HWND hwnd, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (!hwnd || !::IsWindow(hwnd)) {
      return true;
    }
    std::this_thread::sleep_for(50ms);
  }
  return !hwnd || !::IsWindow(hwnd);
}

void request_close(HWND hwnd) {
  if (hwnd) {
    ::PostMessage(hwnd, WM_CLOSE, 0, 0);
  }
}

void log_line(const char* text) {
  std::ofstream log("dock_native_drag_smoke_test.log", std::ios::app);
  log << text << '\n';
}

bool should_exercise_close() {
  const char* value = std::getenv("IRIS_DOCK_NATIVE_SMOKE_EXERCISE_CLOSE");
  return value != nullptr && value[0] != '\0' && value[0] != '0';
}
} // namespace

#ifdef _WIN32
int WINAPI WinMain(_In_ HINSTANCE,
                   _In_opt_ HINSTANCE,
                   _In_ LPSTR,
                   _In_ int) {
#else
int main() {
#endif
  {
    std::ofstream log("dock_native_drag_smoke_test.log", std::ios::trunc);
    log << "dock_native_drag_smoke_test start" << '\n';
  }

  auto app = iris::app::shared();
  iris::ref_ptr<iris::ui::window> main_window(new iris::ui::window());

  iris::ref_ptr<iris::ui::dock_view> root(new iris::ui::dock_view());
  root->set_dock_padding(8.0f);
  root->set_default_dock_size(220.0f, 96.0f);

  iris::ref_ptr<iris::ui::view> sidebar(new iris::ui::view());
  sidebar->flex_dir(iris::ui::FlexDirection::Column)
      .justify_content(iris::ui::Justify::FlexStart)
      .align_items(iris::ui::Align::FlexStart)
      .padding(10.0f);

  iris::ref_ptr<iris::ui::label> sidebar_title(new iris::ui::label());
  sidebar_title->text("Native Drag")
      .font_size(18.0f)
      .alignment(iris::ui::text_alignment::left)
      .auto_width()
      .auto_height();
  sidebar->add_child(sidebar_title.get());

  iris::ref_ptr<iris::ui::view> splitter(new iris::ui::view());
  splitter->width(4.0f);

  iris::ref_ptr<iris::ui::code_editor> editor(new iris::ui::code_editor());
  editor->set_language(iris::ui::code_language::cpp);
  editor->set_font_size(16.0f);
  editor->set_text(
      "#include <iostream>\n"
      "\n"
      "int main() {\n"
      "  std::cout << \"native drag\" << std::endl;\n"
      "  return 0;\n"
      "}\n");

  root->dock(sidebar.get(), iris::ui::dock_position::left);
  root->dock(splitter.get(), iris::ui::dock_position::fill);
  root->dock(editor.get(), iris::ui::dock_position::fill);

  main_window->set_content_view(root.get());
  main_window->show();
  main_window->draw();

  const auto main_hwnd = reinterpret_cast<HWND>(main_window->native_handle());
  const auto editor_header = root->docked_item_header_screen_rect(editor.get());

  std::atomic<bool> drag_ok = false;
  std::atomic<bool> undock_visible = false;
  std::thread automation([&]() {
    std::this_thread::sleep_for(1200ms);
    ::SetForegroundWindow(main_hwnd);
    ::BringWindowToTop(main_hwnd);
    std::this_thread::sleep_for(250ms);

    const auto start = rect_center(editor_header);
    const iris::ui::point undock_target{start.x + 180.0, start.y + 140.0};
    drag_ok.store(drag_between(start, undock_target, 12, 25ms));
    log_line(drag_ok.load() ? "drag gesture ok" : "drag gesture failed");

    std::vector<HWND> handles;
    const bool two_windows = drag_ok.load() && wait_for_window_count(2, 4000ms, &handles);
    undock_visible.store(two_windows);
    log_line(two_windows ? "floating window appeared" : "floating window missing");

    if (two_windows) {
      std::this_thread::sleep_for(1500ms);
      log_line("floating window stable after delay");
    }

    if (!should_exercise_close()) {
      log_line("terminating process after stable drag");
      ::TerminateProcess(::GetCurrentProcess(), 0);
      return;
    }

    log_line("requesting close");
    HWND floating_hwnd = nullptr;
    for (auto hwnd : handles) {
      if (hwnd != main_hwnd) {
        floating_hwnd = hwnd;
        break;
      }
    }

    if (floating_hwnd) {
      request_close(floating_hwnd);
      if (wait_for_window_count(1, 4000ms)) {
        log_line("floating window closed");
      } else {
        log_line("floating window close timeout");
      }
      wait_for_window_absent(floating_hwnd, 1000ms);
    }

    request_close(main_hwnd);
    log_line("close requested");
  });

  app->run();
  automation.join();

  const bool passed = drag_ok.load() && undock_visible.load();
  log_line(passed ? "native drag smoke ok" : "native drag smoke failed");
  return passed ? 0 : 1;
}
