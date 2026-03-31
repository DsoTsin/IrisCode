#include "win32_app.hpp"

#include "../../app_internal.hpp"
#include "../../window.hpp"
#include "log.hpp"
#include "win32_window.hpp"

#define NOMINMAX
#include <Windows.h>

namespace iris {
namespace priv {
extern void init_dwm_pri_api();
HINSTANCE instance;
} // namespace priv

namespace platform::win32 {
class access {
public:
  static bool on_native_event(ui::window& window,
                              uint32_t msg,
                              uintptr_t w_param,
                              intptr_t l_param,
                              intptr_t* out_result) {
    return window.on_native_event(msg, w_param, l_param, out_result);
  }

  static bool on_window_pos_changed(ui::window& window) {
    return window.on_window_pos_changed();
  }

  static bool on_window_activate(ui::window& window, bool active) {
    return window.on_window_activate(active);
  }
};

namespace {
priv::app* g_current_app = nullptr;

enum class win_msg : UINT {
  paint = WM_PAINT,
  close = WM_CLOSE,
  destroy = WM_DESTROY,
  create = WM_CREATE,
  size = WM_SIZE,
  activate = WM_ACTIVATE,
  activate_app = WM_ACTIVATEAPP,
  mouse_activate = WM_MOUSEACTIVATE,
  nc_calc_size = WM_NCCALCSIZE,
  nc_hit_test = WM_NCHITTEST,
  nc_activate = WM_NCACTIVATE,
  nc_destroy = WM_NCDESTROY,
  nc_paint = WM_NCPAINT,
  nc_m_button_down = WM_NCMBUTTONDOWN,
  window_pos_changed = WM_WINDOWPOSCHANGED,
  get_min_max_info = WM_GETMINMAXINFO,
  sizing = WM_SIZING,
  enter_size_move = WM_ENTERSIZEMOVE,
  exit_size_move = WM_EXITSIZEMOVE,
  sys_command = WM_SYSCOMMAND,
  erase_bkgnd = WM_ERASEBKGND,
  input_lang_change_request = WM_INPUTLANGCHANGEREQUEST,
  input_lang_change = WM_INPUTLANGCHANGE,
  ime_set_context = WM_IME_SETCONTEXT,
  ime_start_composition = WM_IME_STARTCOMPOSITION,
  ime_composition = WM_IME_COMPOSITION,
  ime_end_composition = WM_IME_ENDCOMPOSITION,
  ime_char = WM_IME_CHAR,
  dwm_composition_changed = WM_DWMCOMPOSITIONCHANGED,
  quit = WM_QUIT,
  device_change = WM_DEVICECHANGE,
  display_change = WM_DISPLAYCHANGE,
  dpi_changed = WM_DPICHANGED,
};

constexpr UINT kDeferredWindowCloseMessage = WM_APP + 17;

win_msg from_uint(UINT msg) noexcept {
  return static_cast<win_msg>(msg);
}

WNDPROC app_window_proc();

void register_class(const string& name) {
  ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  WNDCLASS wc{};
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = app_window_proc();
  wc.hInstance = priv::instance;
  wc.hCursor = nullptr;
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = name.c_str();
  ::RegisterClassA(&wc);
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  auto* current = g_current_app;
  if (!current) {
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
  auto win_iter = current->windows_.find(hwnd);
  if (msg == kDeferredWindowCloseMessage) {
    if (win_iter != current->windows_.end()) {
      auto window_ref = (*win_iter).second;
      window_ref->close();
      return 0;
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }

  const auto wm = from_uint(msg);
  if (win_iter != current->windows_.end()) {
    intptr_t native_result = 0;
    if (access::on_native_event(*win_iter->second.get(), msg, static_cast<uintptr_t>(w_param),
                                static_cast<intptr_t>(l_param), &native_result)) {
      return static_cast<LRESULT>(native_result);
    }
  }

  switch (wm) {
  case win_msg::input_lang_change_request:
  case win_msg::input_lang_change:
  case win_msg::ime_set_context:
  case win_msg::ime_start_composition:
  case win_msg::ime_composition:
  case win_msg::ime_end_composition:
  case win_msg::ime_char:
    return DefWindowProc(hwnd, msg, w_param, l_param);
  case win_msg::paint: {
    PAINTSTRUCT ps = {};
    ::BeginPaint(hwnd, &ps);
    if (win_iter != current->windows_.end()) {
      const ui::rect dirty_rect{
          (float)ps.rcPaint.left,
          (float)ps.rcPaint.top,
          (float)(ps.rcPaint.right - ps.rcPaint.left),
          (float)(ps.rcPaint.bottom - ps.rcPaint.top),
      };
      win_iter->second->draw(dirty_rect);
    }
    ::EndPaint(hwnd, &ps);
    break;
  }
  case win_msg::get_min_max_info: {
    auto* mmi = reinterpret_cast<MINMAXINFO*>(l_param);
    if (win_iter != current->windows_.end()) {
      mmi->ptMinTrackSize.x = 2 * win_iter->second->shadow_distance() + 100;
      mmi->ptMinTrackSize.y = 2 * win_iter->second->shadow_distance() + 100;
    }
    return 0;
  }
#if !ENABLE_MICA
  case win_msg::nc_calc_size: {
    const bool is_max = !!::IsZoomed(hwnd);
    if (is_max) {
      WINDOWINFO wnd_info = {0};
      wnd_info.cbSize = sizeof(wnd_info);
      ::GetWindowInfo(hwnd, &wnd_info);
      auto* resizing_rect = reinterpret_cast<LPNCCALCSIZE_PARAMS>(l_param);
      resizing_rect->rgrc[0].left += wnd_info.cxWindowBorders;
      resizing_rect->rgrc[0].top += wnd_info.cxWindowBorders;
      resizing_rect->rgrc[0].right -= wnd_info.cxWindowBorders;
      resizing_rect->rgrc[0].bottom -= wnd_info.cxWindowBorders;
      resizing_rect->rgrc[1].left = resizing_rect->rgrc[0].left;
      resizing_rect->rgrc[1].top = resizing_rect->rgrc[0].top;
      resizing_rect->rgrc[1].right = resizing_rect->rgrc[0].right;
      resizing_rect->rgrc[1].bottom = resizing_rect->rgrc[0].bottom;
      resizing_rect->lppos->x += wnd_info.cxWindowBorders;
      resizing_rect->lppos->y += wnd_info.cxWindowBorders;
      resizing_rect->lppos->cx -= 2 * wnd_info.cxWindowBorders;
      resizing_rect->lppos->cy -= 2 * wnd_info.cxWindowBorders;
      return WVR_VALIDRECTS;
    }
    return 0;
  }
  case win_msg::nc_hit_test: {
    RECT wnd_rect;
    ::GetWindowRect(hwnd, &wnd_rect);
    const int local_mouse_x = (int)(short)(LOWORD(l_param)) - wnd_rect.left;
    const int local_mouse_y = (int)(short)(HIWORD(l_param)) - wnd_rect.top;
    if (win_iter != current->windows_.end()) {
      return (*win_iter).second->zone_at(local_mouse_x, local_mouse_y);
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
  case win_msg::nc_activate:
    return true;
  case win_msg::nc_destroy:
    return DefWindowProc(hwnd, msg, w_param, l_param);
  case win_msg::nc_paint:
    return 0;
#endif
  case win_msg::window_pos_changed: {
    if (win_iter != current->windows_.end()) {
      access::on_window_pos_changed(*win_iter->second.get());
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
  case win_msg::size: {
    switch (w_param) {
    case SIZE_MAXIMIZED:
      XB_LOGI("maxed");
      break;
    case SIZE_RESTORED:
      XB_LOGI("restored");
      break;
    case SIZE_MINIMIZED:
      XB_LOGI("minimized");
      break;
    }
    if (win_iter != current->windows_.end()) {
      auto nw = (int)(short)(LOWORD(l_param));
      auto nh = (int)(short)(HIWORD(l_param));
      auto window_ref = (*win_iter).second;
      if (w_param != SIZE_MINIMIZED && nw > 0 && nh > 0) {
        window_ref->resize(nw, nh);
        access::on_window_pos_changed(*window_ref.get());
        ::InvalidateRect(hwnd, nullptr, FALSE);
      } else {
        access::on_window_pos_changed(*window_ref.get());
      }
      return 0;
    }
    break;
  }
  case win_msg::nc_m_button_down:
  case win_msg::sizing:
  case win_msg::enter_size_move:
  case win_msg::exit_size_move:
  case win_msg::mouse_activate:
    return DefWindowProc(hwnd, msg, w_param, l_param);
  case win_msg::activate_app:
    current->notify_app_activation(w_param != FALSE);
    return DefWindowProc(hwnd, msg, w_param, l_param);
  case win_msg::create: {
    auto* cparam = reinterpret_cast<LPCREATESTRUCTA>(l_param);
    auto* wind = reinterpret_cast<priv::window*>(cparam->lpCreateParams);
    wind->on_native_created(reinterpret_cast<void*>(hwnd));
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wind));
    return 0;
  }
  case win_msg::destroy:
    if (win_iter != current->windows_.end()) {
      (*win_iter).second->finalize();
    }
    return 0;
  case win_msg::close:
    ::PostMessage(hwnd, kDeferredWindowCloseMessage, 0, 0);
    return 0;
  case win_msg::sys_command: {
    switch (w_param & 0xfff0) {
    case SC_RESTORE:
      if (IsIconic(hwnd)) {
        ::ShowWindow(hwnd, SW_RESTORE);
        return 0;
      }
      break;
    case SC_MAXIMIZE:
      break;
    case SC_CLOSE:
      ::PostMessage(hwnd, WM_CLOSE, 0, 0);
      return 0;
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
  case win_msg::dwm_composition_changed:
    break;
  case win_msg::erase_bkgnd:
    return 1;
  case win_msg::activate: {
    bool active = LOWORD(w_param) == WA_ACTIVE || LOWORD(w_param) == WA_CLICKACTIVE;
    if (LOWORD(w_param) == WA_INACTIVE) {
      active = false;
    }
    if (win_iter != current->windows_.end()) {
      access::on_window_activate(*win_iter->second.get(), active);
      return 0;
    }
    return 0;
  }
  case win_msg::quit:
    current->request_exit();
    break;
  case win_msg::device_change:
  case win_msg::display_change:
  case win_msg::dpi_changed:
    break;
  default:
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
  return 0;
}

WNDPROC app_window_proc() {
  return window_proc;
}

} // namespace

void init_app(priv::app& app) {
  g_current_app = &app;
  priv::init_dwm_pri_api();
  register_class(app.name_);
}

void shutdown_app(priv::app& app) {
  app.windows_.clear();
  if (g_current_app == &app) {
    g_current_app = nullptr;
  }
}

void loop_app(priv::app& app) {
  MSG msg;
  BOOL gm;
  while ((gm = ::GetMessageW(&msg, nullptr, 0, 0)) != 0 && gm != -1 && !app.request_exit_) {
    while (!app.msg_queue_.empty()) {
      auto queued = app.msg_queue_.front();
      app.msg_queue_.pop();
      switch (queued.code) {
      case WM_CREATE:
        if (queued.window) {
          queued.window->initialize();
        }
        break;
      }
    }
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
}

void pump_messages(priv::app& app) {
  (void)app;
  MSG message;
  while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }
}

void add_window(priv::app& app, ui::window* w) {
  app.windows_[w->native_handle()] = iris::weak_ptr<iris::ui::window>(w);
}

void remove_window(priv::app& app, ui::window* w) {
  app.windows_.erase(w->native_handle());
  if (app.windows_.empty()) {
    app.request_exit();
    ::PostQuitMessage(0);
  }
}

void set_delegate(priv::app& app, app_delegate* in_delegate) {
  app.delegate_ = in_delegate;
}

} // namespace platform::win32
} // namespace iris
