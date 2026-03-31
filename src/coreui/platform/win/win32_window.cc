#include "win32_window.hpp"

#include "../../app.hpp"
#include "../../graphics_context.hpp"
#include "../../view.hpp"
#include "../../view/button.hpp"
#include "../../window.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSurface.h"
#include "include/effects/SkGradientShader.h"
#include "include/effects/SkImageFilters.h"
#include "include/effects/SkRuntimeEffect.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <cmath>

#pragma comment(lib, "dwmapi.lib")

static_assert(sizeof(iris::ui::color) == sizeof(SkColor), "inconsistent color type size");

namespace iris {
namespace priv {

extern void set_app_theme(int value, HWND hwnd);
extern bool apply_win11_mica(int type, HWND hwnd);
extern HINSTANCE instance;

namespace {

bool is_win11_or_greater() {
  OSVERSIONINFOEX osvi = {};
  DWORDLONG dwlConditionMask = 0;

  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  osvi.dwMajorVersion = 10;
  osvi.dwMinorVersion = 0;
  osvi.dwBuildNumber = 22000;

  VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

  return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
                           dwlConditionMask)
         != TRUE;
}

constexpr uint8_t kMouseLeftMask = 1 << 0;
constexpr uint8_t kMouseRightMask = 1 << 1;
constexpr uint8_t kMouseMiddleMask = 1 << 2;
constexpr uint8_t kMouseX1Mask = 1 << 3;
constexpr uint8_t kMouseX2Mask = 1 << 4;

HWND as_hwnd(void* hwnd) {
  return reinterpret_cast<HWND>(hwnd);
}

SkColor& as_sk_color(uint32_t& value) {
  return reinterpret_cast<SkColor&>(value);
}

UINT query_window_dpi(HWND hwnd) {
  using get_dpi_for_window_fn = UINT(WINAPI*)(HWND);
  static const get_dpi_for_window_fn get_dpi_for_window
      = reinterpret_cast<get_dpi_for_window_fn>(
          ::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
  if (get_dpi_for_window && hwnd) {
    return get_dpi_for_window(hwnd);
  }
  return USER_DEFAULT_SCREEN_DPI;
}

uint8_t query_input_modifiers() {
  uint8_t modifiers = 0;
  if ((::GetKeyState(VK_SHIFT) & 0x8000) != 0) {
    modifiers |= ui::modifier_mask(ui::input_modifier::shift);
  }
  if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0) {
    modifiers |= ui::modifier_mask(ui::input_modifier::control);
  }
  if ((::GetKeyState(VK_MENU) & 0x8000) != 0) {
    modifiers |= ui::modifier_mask(ui::input_modifier::alt);
  }
  return modifiers;
}

uint8_t mouse_button_mask(ui::mouse_button button) {
  switch (button) {
  case ui::mouse_button::left:
    return kMouseLeftMask;
  case ui::mouse_button::right:
    return kMouseRightMask;
  case ui::mouse_button::middle:
    return kMouseMiddleMask;
  case ui::mouse_button::x1:
    return kMouseX1Mask;
  case ui::mouse_button::x2:
    return kMouseX2Mask;
  default:
    return 0;
  }
}

ui::mouse_button mouse_button_from_message(UINT msg, WPARAM w_param) {
  switch (msg) {
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_LBUTTONDBLCLK:
    return ui::mouse_button::left;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
  case WM_RBUTTONDBLCLK:
    return ui::mouse_button::right;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
  case WM_MBUTTONDBLCLK:
    return ui::mouse_button::middle;
  case WM_XBUTTONDOWN:
  case WM_XBUTTONUP:
  case WM_XBUTTONDBLCLK:
    return ((HIWORD(w_param) & XBUTTON1) != 0) ? ui::mouse_button::x1 : ui::mouse_button::x2;
  default:
    return ui::mouse_button::none;
  }
}

ui::event_type mouse_down_type(ui::mouse_button button) {
  switch (button) {
  case ui::mouse_button::left:
    return ui::event_type::left_mouse_down;
  case ui::mouse_button::right:
    return ui::event_type::right_mouse_down;
  default:
    return ui::event_type::other_mouse_down;
  }
}

ui::event_type mouse_up_type(ui::mouse_button button) {
  switch (button) {
  case ui::mouse_button::left:
    return ui::event_type::left_mouse_up;
  case ui::mouse_button::right:
    return ui::event_type::right_mouse_up;
  default:
    return ui::event_type::other_mouse_up;
  }
}

ui::event_type mouse_drag_type(ui::mouse_button button) {
  switch (button) {
  case ui::mouse_button::left:
    return ui::event_type::left_mouse_dragged;
  case ui::mouse_button::right:
    return ui::event_type::right_mouse_dragged;
  default:
    return ui::event_type::mouse_moved;
  }
}

ui::mouse_button primary_pressed_button(uint8_t down_mask) {
  if ((down_mask & kMouseLeftMask) != 0) {
    return ui::mouse_button::left;
  }
  if ((down_mask & kMouseRightMask) != 0) {
    return ui::mouse_button::right;
  }
  if ((down_mask & kMouseMiddleMask) != 0) {
    return ui::mouse_button::middle;
  }
  if ((down_mask & kMouseX1Mask) != 0) {
    return ui::mouse_button::x1;
  }
  if ((down_mask & kMouseX2Mask) != 0) {
    return ui::mouse_button::x2;
  }
  return ui::mouse_button::none;
}

ui::point message_point(HWND hwnd, UINT msg, LPARAM l_param) {
  POINT cursor_pt{};
  if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
    cursor_pt.x = GET_X_LPARAM(l_param);
    cursor_pt.y = GET_Y_LPARAM(l_param);
    ::ScreenToClient(hwnd, &cursor_pt);
  } else {
    cursor_pt.x = GET_X_LPARAM(l_param);
    cursor_pt.y = GET_Y_LPARAM(l_param);
  }
  return {static_cast<double>(cursor_pt.x), static_cast<double>(cursor_pt.y)};
}

} // namespace

window::window(iris::ui::window* in_window, iris::ui::window* parent)
    : w_(in_window), pw_(parent) {
  window_color_ = SkColorSetARGB(0x80, 0x46, 0x44, 0x40);
}

window::~window() {
  stop_all_timers();
  clear_capture();
  hover_view_ = nullptr;
  focus_view_ = nullptr;
  if (content_view_) {
    content_view_->release_internal();
    content_view_ = nullptr;
  }
  if (hwnd_) {
    ::DestroyWindow(as_hwnd(hwnd_));
    hwnd_ = nullptr;
  }
}

void window::init() {
  ctx_ = ref_ptr<ui::graphics_context>(new ui::graphics_context(w_, true));
}

void window::create_native_window(void* parent_handle) {
#if !ENABLE_MICA
  UINT window_ex_style = WS_EX_WINDOWEDGE | WS_EX_COMPOSITED;
#else
  UINT window_ex_style = WS_EX_LAYERED;
#endif
  if (w_->activates_on_show()) {
    window_ex_style |= WS_EX_APPWINDOW;
  } else {
    window_ex_style |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
  }

  const UINT window_style = WS_OVERLAPPEDWINDOW;
  hwnd_ = reinterpret_cast<void*>(::CreateWindowExA(window_ex_style,
                                                    iris::app::shared()->name(),
                                                    "",
                                                    window_style,
                                                    CW_USEDEFAULT,
                                                    CW_USEDEFAULT,
                                                    800,
                                                    600,
                                                    as_hwnd(parent_handle),
                                                    nullptr,
                                                    instance,
                                                    this));
}

void window::on_native_created(void* native_handle) {
  hwnd_ = native_handle;
  RECT w_rect;
  ::GetWindowRect(as_hwnd(hwnd_), &w_rect);
  frame_ = {(float)w_rect.left, (float)w_rect.top, (float)w_rect.right - (float)w_rect.left,
            (float)w_rect.bottom - (float)w_rect.top};
  init();

  MARGINS margins = {-1};
  ::DwmExtendFrameIntoClientArea(as_hwnd(hwnd_), &margins);
#if !ENABLE_MICA
  {
    const DWMNCRENDERINGPOLICY rendering_policy = DWMNCRP_DISABLED;
    SUCCEEDED(DwmSetWindowAttribute(as_hwnd(hwnd_), DWMWA_NCRENDERING_POLICY, &rendering_policy,
                                    sizeof(rendering_policy)));

    const BOOL enable_nc_paint = false;
    SUCCEEDED(DwmSetWindowAttribute(as_hwnd(hwnd_), DWMWA_ALLOW_NCPAINT, &enable_nc_paint,
                                    sizeof(enable_nc_paint)));
  }
#else
  ::SetLayeredWindowAttributes(as_hwnd(hwnd_), RGB(255, 255, 255), 0, LWA_COLORKEY);
#endif
  ::SetWindowTextA(as_hwnd(hwnd_), "IrisWindow");
  if (is_win11_or_greater()) {
    const BOOL enable_host_backdrop = 1;
    DwmSetWindowAttribute(as_hwnd(hwnd_), DWMWA_USE_HOSTBACKDROPBRUSH, &enable_host_backdrop,
                          sizeof(BOOL));
  }
}

void window::resize(int w, int h) {
  if (w <= 0 || h <= 0) {
    return;
  }
  if (frame_.width == w && frame_.height == h) {
    return;
  }
  frame_.width = w;
  frame_.height = h;
  if (ctx_) {
    ctx_->resize(w, h);
  }
  if (content_view_) {
    content_view_->init(content_bounds());
    content_view_->mark_layout_dirty();
    content_view_->layout_if_needed();
  }
}

ui::point window::window_to_content(const ui::point& window_point) const {
  const auto content = content_bounds();
  return {window_point.x - content.left, window_point.y - content.top};
}

ui::point window::content_to_view(ui::view* target, const ui::point& content_point) const {
  if (!target) {
    return {};
  }
  double x = content_point.x;
  double y = content_point.y;
  for (ui::view* node = target; node && node != content_view_; node = node->parent_) {
    const auto bounds = node->get_layout_rect();
    x -= bounds.left;
    y -= bounds.top;
  }
  return {x, y};
}

ui::view* window::hit_test_content(const ui::point& window_point) const {
  if (!content_view_) {
    return nullptr;
  }
  const auto content = content_bounds();
  const auto content_point = window_to_content(window_point);
  if (content_point.x < 0.0 || content_point.y < 0.0 || content_point.x >= content.width
      || content_point.y >= content.height) {
    return nullptr;
  }
  return content_view_->hit_test(content_point);
}

bool window::route_mouse_event(ui::view* target, const ui::mouse_event& event, ui::view** handled_by) {
  if (handled_by) {
    *handled_by = nullptr;
  }
  if (!target) {
    return false;
  }

  const auto content_point = window_to_content(event.location_in_window);
  for (ui::view* node = target; node != nullptr; node = node->parent_) {
    ui::mouse_event routed = event;
    routed.location_in_view = content_to_view(node, content_point);
    bool handled = false;
    switch (event.type) {
    case ui::event_type::left_mouse_down:
    case ui::event_type::right_mouse_down:
    case ui::event_type::other_mouse_down:
      handled = node->mouse_down(routed);
      break;
    case ui::event_type::left_mouse_up:
    case ui::event_type::right_mouse_up:
    case ui::event_type::other_mouse_up:
      handled = node->mouse_up(routed);
      break;
    case ui::event_type::left_mouse_dragged:
    case ui::event_type::right_mouse_dragged:
    case ui::event_type::mouse_moved:
      handled = node->mouse_move(routed);
      break;
    case ui::event_type::mouse_entered:
      handled = node->mouse_enter(routed);
      break;
    case ui::event_type::mouse_exited:
      handled = node->mouse_leave(routed);
      break;
    case ui::event_type::scroll_wheel:
      handled = node->mouse_wheel(routed);
      break;
    default:
      break;
    }
    if (handled) {
      if (handled_by) {
        *handled_by = node;
      }
      return true;
    }
  }
  return false;
}

bool window::route_key_event(ui::view* target,
                             const ui::key_event& event,
                             bool is_char_message,
                             ui::view** handled_by) {
  if (handled_by) {
    *handled_by = nullptr;
  }
  if (!target) {
    return false;
  }

  for (ui::view* node = target; node != nullptr; node = node->parent_) {
    bool handled = false;
    if (is_char_message) {
      handled = node->key_char(event);
    } else if (event.type == ui::event_type::key_up) {
      handled = node->key_up(event);
    } else {
      handled = node->key_down(event);
    }
    if (handled) {
      if (handled_by) {
        *handled_by = node;
      }
      return true;
    }
  }
  return false;
}

void window::update_hover_view(ui::view* next_hover, const ui::point& window_point, uint8_t modifiers) {
  if (next_hover == hover_view_) {
    return;
  }

  ui::mouse_event hover_event;
  hover_event.button = ui::mouse_button::none;
  hover_event.location_in_window = window_point;
  hover_event.modifiers = modifiers;

  if (hover_view_) {
    hover_event.type = ui::event_type::mouse_exited;
    route_mouse_event(hover_view_, hover_event);
  }

  hover_view_ = next_hover;
  if (hover_view_) {
    hover_event.type = ui::event_type::mouse_entered;
    route_mouse_event(hover_view_, hover_event);
  }
}

void window::set_focus_view(ui::view* next_focus) {
  if (next_focus && !next_focus->can_receive_focus()) {
    next_focus = nullptr;
  }
  if (focus_view_ == next_focus) {
    return;
  }
  if (focus_view_) {
    focus_view_->on_focus_changed(false);
  }
  focus_view_ = next_focus;
  if (focus_view_) {
    focus_view_->on_focus_changed(true);
  }
}

void window::clear_capture() {
  mouse_buttons_down_ = 0;
  capture_view_ = nullptr;
  if (hwnd_ && ::GetCapture() == as_hwnd(hwnd_)) {
    ::ReleaseCapture();
  }
}

void window::clear_interaction_state() {
  clear_capture();
  hover_view_ = nullptr;
  focus_view_ = nullptr;
}

bool window::on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result) {
  if (!content_view_) {
    return false;
  }

  const UINT wm = static_cast<UINT>(msg);
  const auto modifiers = query_input_modifiers();
  const auto hwnd = as_hwnd(hwnd_);
  const auto window_point = message_point(hwnd, wm, static_cast<LPARAM>(l_param));
  auto* hit_view = hit_test_content(window_point);

  switch (wm) {
  case WM_SETFOCUS:
    return false;
  case WM_KILLFOCUS:
    clear_capture();
    set_focus_view(nullptr);
    update_hover_view(nullptr, window_point, modifiers);
    return false;
  case WM_CAPTURECHANGED:
    if (reinterpret_cast<HWND>(l_param) != hwnd) {
      clear_capture();
    }
    return false;
  case WM_MOUSELEAVE:
    tracking_mouse_leave_ = false;
    update_hover_view(nullptr, window_point, modifiers);
    if (out_result) {
      *out_result = 0;
    }
    return true;
  case WM_MOUSEMOVE: {
    if (!tracking_mouse_leave_) {
      TRACKMOUSEEVENT tracking{};
      tracking.cbSize = sizeof(tracking);
      tracking.dwFlags = TME_LEAVE;
      tracking.hwndTrack = hwnd;
      if (::TrackMouseEvent(&tracking)) {
        tracking_mouse_leave_ = true;
      }
    }

    if (!capture_view_) {
      update_hover_view(hit_view, window_point, modifiers);
    }

    auto* target = capture_view_ ? capture_view_ : hit_view;
    if (!target) {
      return false;
    }
    const auto drag_button = primary_pressed_button(mouse_buttons_down_);
    ui::mouse_event event;
    event.type = (drag_button == ui::mouse_button::none) ? ui::event_type::mouse_moved
                                                          : mouse_drag_type(drag_button);
    event.button = drag_button;
    event.location_in_window = window_point;
    event.modifiers = modifiers;
    const bool handled = route_mouse_event(target, event);
    if (handled && out_result) {
      *out_result = 0;
    }
    return handled;
  }
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_XBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_XBUTTONDBLCLK: {
    const auto button = mouse_button_from_message(wm, static_cast<WPARAM>(w_param));
    if (button == ui::mouse_button::none) {
      return false;
    }
    update_hover_view(hit_view, window_point, modifiers);
    auto* target = hit_view;
    if (!target) {
      return false;
    }

    ui::mouse_event event;
    event.type = mouse_down_type(button);
    event.button = button;
    event.location_in_window = window_point;
    event.modifiers = modifiers;
    event.is_double_click = (wm == WM_LBUTTONDBLCLK || wm == WM_RBUTTONDBLCLK
                             || wm == WM_MBUTTONDBLCLK || wm == WM_XBUTTONDBLCLK);
    ui::view* handled_by = nullptr;
    const bool handled = route_mouse_event(target, event, &handled_by);
    if (handled) {
      capture_view_ = handled_by;
      mouse_buttons_down_ |= mouse_button_mask(button);
      set_focus_view(handled_by);
      ::SetCapture(hwnd);
      if (out_result) {
        *out_result = 0;
      }
    }
    return handled;
  }
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  case WM_MBUTTONUP:
  case WM_XBUTTONUP: {
    const auto button = mouse_button_from_message(wm, static_cast<WPARAM>(w_param));
    if (button == ui::mouse_button::none) {
      return false;
    }

    auto* target = capture_view_ ? capture_view_ : hit_view;
    bool handled = false;
    if (target) {
      ui::mouse_event event;
      event.type = mouse_up_type(button);
      event.button = button;
      event.location_in_window = window_point;
      event.modifiers = modifiers;
      handled = route_mouse_event(target, event);
      if (handled && out_result) {
        *out_result = 0;
      }
    }

    mouse_buttons_down_ &= ~mouse_button_mask(button);
    if (mouse_buttons_down_ == 0) {
      clear_capture();
    }
    hit_view = hit_test_content(window_point);
    update_hover_view(hit_view, window_point, modifiers);
    return handled;
  }
  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL: {
    auto* target = hit_view ? hit_view : content_view_;
    if (!target) {
      return false;
    }
    ui::mouse_event event;
    event.type = ui::event_type::scroll_wheel;
    event.button = ui::mouse_button::none;
    event.location_in_window = window_point;
    event.modifiers = modifiers;
    const SHORT wheel_delta = GET_WHEEL_DELTA_WPARAM(static_cast<WPARAM>(w_param));
    event.wheel_delta = static_cast<float>(wheel_delta) / 120.0f;
    const bool handled = route_mouse_event(target, event);
    if (handled && out_result) {
      *out_result = 0;
    }
    return handled;
  }
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP:
  case WM_CHAR: {
    ui::view* target = focus_view_ ? focus_view_ : content_view_;
    if (!target) {
      return false;
    }

    ui::key_event event;
    event.type = (wm == WM_KEYUP || wm == WM_SYSKEYUP) ? ui::event_type::key_up : ui::event_type::key_down;
    event.key_code = static_cast<uint32_t>(w_param);
    event.scan_code = static_cast<uint32_t>((l_param >> 16) & 0xFF);
    event.character = (wm == WM_CHAR) ? static_cast<uint32_t>(w_param) : 0;
    event.is_repeat = (l_param & 0x40000000) != 0;
    event.is_system = (wm == WM_SYSKEYDOWN || wm == WM_SYSKEYUP);
    event.modifiers = modifiers;

    const bool is_char_message = (wm == WM_CHAR);
    const bool handled = route_key_event(target, event, is_char_message);
    if (out_result && (handled || is_char_message)) {
      *out_result = 0;
    }
    return handled || is_char_message;
  }
  case WM_TIMER: {
    const auto timer_id = static_cast<uint64_t>(w_param);
    auto iter = timers_.find(timer_id);
    if (iter == timers_.end()) {
      return false;
    }

    auto callback = iter->second;
    callback();
    if (out_result) {
      *out_result = 0;
    }
    return true;
  }
  case WM_ENTERSIZEMOVE:
    live_resizing_ = true;
    if (ctx_) {
      ctx_->on_live_resize_changed(true);
    }
    return false;
  case WM_EXITSIZEMOVE:
    live_resizing_ = false;
    if (ctx_) {
      ctx_->on_live_resize_changed(false);
    }
    if (hwnd_) {
      ::InvalidateRect(hwnd, nullptr, FALSE);
    }
    return false;
  default:
    return false;
  }
}

int window::zone_at(int x, int y) const {
  if (is_maximized()) {
    return HTCLIENT;
  }
  const UINT dpi = query_window_dpi(as_hwnd(hwnd_));
  const float dpi_scale = static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
  const int resize_border = static_cast<int>(2.f * dpi_scale + 0.5f);

  const ui::rect content = content_bounds();
  const int left_edge = static_cast<int>(content.left);
  const int top_edge = static_cast<int>(content.top);
  const int right_edge = static_cast<int>(content.right());
  const int bottom_edge = static_cast<int>(content.bottom());

  const bool in_horizontal_range = x >= left_edge - resize_border && x < right_edge + resize_border;
  const bool in_vertical_range = y >= top_edge - resize_border && y < bottom_edge + resize_border;
  const bool left = in_vertical_range && x >= left_edge - resize_border && x < left_edge + resize_border;
  const bool right = in_vertical_range && x >= right_edge - resize_border && x < right_edge + resize_border;
  const bool top = in_horizontal_range && y >= top_edge - resize_border && y < top_edge + resize_border;
  const bool bottom = in_horizontal_range && y >= bottom_edge - resize_border && y < bottom_edge + resize_border;

  if (top && left) {
    return HTTOPLEFT;
  }
  if (top && right) {
    return HTTOPRIGHT;
  }
  if (bottom && left) {
    return HTBOTTOMLEFT;
  }
  if (bottom && right) {
    return HTBOTTOMRIGHT;
  }
  if (top) {
    return HTTOP;
  }
  if (bottom) {
    return HTBOTTOM;
  }
  if (left) {
    return HTLEFT;
  }
  if (right) {
    return HTRIGHT;
  }
  if (content_view_) {
    const double content_x = static_cast<double>(x) - static_cast<double>(content.left);
    const double content_y = static_cast<double>(y) - static_cast<double>(content.top);
    if (content_x >= 0.0 && content_y >= 0.0 && content_x < content.width && content_y < content.height) {
      const ui::point content_point{content_x, content_y};
      if (ui::view* hit = content_view_->hit_test(content_point)) {
        if (auto* title = dynamic_cast<ui::title_button*>(hit)) {
          const ui::point local_point = content_to_view(title, content_point);
          return title->is_control_button_hit(local_point) ? HTCLIENT : HTCAPTION;
        }
        return HTCLIENT;
      }
    }
  }
  return HTCAPTION;
}

void window::draw_title() {
  if (!ctx_) {
    return;
  }
}

void window::draw_border() {
  if (!ctx_) {
    return;
  }
  const bool is_max = !!::IsZoomed(as_hwnd(hwnd_));
  const int shadow_blur = is_max ? 0 : static_cast<int>(shadow_distance());
  const int border_width = border_width_ > 1.f ? static_cast<int>(border_width_) : 1;
  const int corner_radius = is_max ? 0 : static_cast<int>(corner_radius_);
  const SkColor border_color = SK_ColorGRAY;
  const SkColor window_color = as_sk_color(window_color_);

  SkRect window_rect = SkRect::MakeXYWH(shadow_blur, shadow_blur, frame_.width - 2 * shadow_blur,
                                        frame_.height - 2 * shadow_blur);
  SkRect window_outer_rect = SkRect::MakeXYWH(shadow_blur - 1, shadow_blur - 1,
                                              frame_.width - 2 * (shadow_blur - 1),
                                              frame_.height - 2 * (shadow_blur - 1));
  SkRect native_window_rect{0, 0, frame_.width, frame_.height};
  auto* canvas = ctx_->canvas();
  if (!canvas) {
    return;
  }

  SkPaint clear_paint;
  clear_paint.setColor(SK_ColorTRANSPARENT);
  clear_paint.setBlendMode(SkBlendMode::kSrc);
  canvas->drawRect(native_window_rect, clear_paint);

  if (!w_->uses_fancy_border()) {
    SkPaint fill_paint;
    fill_paint.setColor(SkColorSetARGB(0xF4, 0x2B, 0x2C, 0x30));
    fill_paint.setAntiAlias(true);
    canvas->drawRoundRect(window_rect, corner_radius, corner_radius, fill_paint);

    SkPaint border_paint;
    border_paint.setColor(SkColorSetARGB(0xC0, 0x62, 0x66, 0x70));
    border_paint.setStyle(SkPaint::kStroke_Style);
    border_paint.setStrokeWidth(border_width);
    border_paint.setAntiAlias(true);
    canvas->drawRoundRect(window_rect, corner_radius, corner_radius, border_paint);
    return;
  }

  static const SkColor shadow_color = SkColorSetARGB(0x80, 0x00, 0x00, 0x00);

  SkPaint shadow_paint;
  shadow_paint.setImageFilter(SkImageFilters::DropShadowOnly(0, 0, shadow_blur / 2.0f,
                                                             shadow_blur / 2.0f, shadow_color, nullptr));
  canvas->save();
  canvas->drawRoundRect(window_rect, corner_radius, corner_radius, shadow_paint);

  SkVector radii[4] = {{(SkScalar)corner_radius, (SkScalar)corner_radius},
                       {(SkScalar)corner_radius, (SkScalar)corner_radius},
                       {(SkScalar)corner_radius, (SkScalar)corner_radius},
                       {(SkScalar)corner_radius, (SkScalar)corner_radius}};
  SkRRect clip_rrect;
  clip_rrect.setRectRadii(window_rect, radii);
  canvas->clipRRect(clip_rrect, true);

  SkPaint window_paint;
  window_paint.setColor(window_color);
  window_paint.setAntiAlias(true);
  canvas->drawRoundRect(window_rect, corner_radius, corner_radius, window_paint);

  SkRect panel_rect = SkRect::MakeXYWH(300, shadow_blur, frame_.width - shadow_blur - 300,
                                       frame_.height - 2 * shadow_blur);
  window_paint.setColor(SkColorSetARGB(0xFF, 0x23, 0x22, 0x20));
  canvas->drawRect(panel_rect, window_paint);
  canvas->restore();

  SkPaint border_paint;
  border_paint.setColor(border_color);
  border_paint.setStyle(SkPaint::kStroke_Style);
  border_paint.setStrokeWidth((SkScalar)border_width);
  border_paint.setAntiAlias(true);
  canvas->drawRoundRect(window_rect, corner_radius, corner_radius, border_paint);
  border_paint.setColor(SK_ColorBLACK);
  canvas->drawRoundRect(window_outer_rect, corner_radius + 1, corner_radius + 1, border_paint);
}

ui::rect window::content_bounds() const {
  const bool is_max = is_maximized();
  return ui::rect{is_max ? 0 : shadow_distance_, is_max ? 0 : shadow_distance_,
                  frame_.width - (is_max ? 0 : 2.f) * shadow_distance_,
                  frame_.height - (is_max ? 0 : 2.f) * shadow_distance_};
}

bool window::is_maximized() const {
  return !!::IsZoomed(reinterpret_cast<HWND>(w_->native_handle()));
}

bool window::on_window_pos_changed() {
  const bool is_max = !!::IsZoomed(as_hwnd(hwnd_));
  as_sk_color(window_color_)
      = !is_max ? SkColorSetARGB(0x80, 0x46, 0x44, 0x40) : SkColorSetARGB(0xFF, 0x46, 0x44, 0x40);
  if (ctx_) {
    ctx_->on_window_pos_changed();
  }
  return true;
}

void window::show() {
  ::ShowWindow(as_hwnd(hwnd_), w_->activates_on_show() ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);
  if (w_->paints_synchronously_on_show()) {
    ::UpdateWindow(as_hwnd(hwnd_));
  } else {
    ::InvalidateRect(as_hwnd(hwnd_), nullptr, FALSE);
  }
}

void window::request_redraw(const ui::rect* dirty_rect) {
  if (!hwnd_) {
    return;
  }
  if (!dirty_rect) {
    ::InvalidateRect(as_hwnd(hwnd_), nullptr, FALSE);
    return;
  }

  RECT native_dirty{};
  native_dirty.left = static_cast<LONG>(floorf(dirty_rect->left));
  native_dirty.top = static_cast<LONG>(floorf(dirty_rect->top));
  native_dirty.right = static_cast<LONG>(ceilf(dirty_rect->right()));
  native_dirty.bottom = static_cast<LONG>(ceilf(dirty_rect->bottom()));
  ::InvalidateRect(as_hwnd(hwnd_), &native_dirty, FALSE);
}

uint64_t window::start_timer(uint32_t interval_ms, function<void()> callback) {
  if (!hwnd_ || !callback) {
    return 0;
  }

  const uint64_t timer_id = next_timer_id_++;
  const UINT interval = max<UINT>(1, interval_ms);
  if (::SetTimer(as_hwnd(hwnd_), static_cast<UINT_PTR>(timer_id), interval, nullptr) == 0) {
    return 0;
  }

  timers_[timer_id] = std::move(callback);
  return timer_id;
}

void window::stop_timer(uint64_t timer_id) {
  if (timer_id == 0) {
    return;
  }

  if (hwnd_) {
    ::KillTimer(as_hwnd(hwnd_), static_cast<UINT_PTR>(timer_id));
  }
  timers_.erase(timer_id);
}

void window::stop_all_timers() {
  if (hwnd_) {
    for (const auto& [timer_id, callback] : timers_) {
      (void)callback;
      ::KillTimer(as_hwnd(hwnd_), static_cast<UINT_PTR>(timer_id));
    }
  }
  timers_.clear();
}

void window::close() {
  if (!hwnd_) {
    return;
  }
  stop_all_timers();
  clear_interaction_state();
  weak_ptr<ui::graphics_context> ctx_guard;
  if (ctx_) {
    ctx_guard = weak_ptr<ui::graphics_context>(ctx_.get());
  }
  if (content_view_) {
    content_view_->set_gfx_context(nullptr);
  }
  if (ctx_guard) {
    ctx_guard->shutdown();
  }
  ctx_ = ref_ptr<ui::graphics_context>();
  iris::app::shared()->remove_window(w_);
  HWND hwnd = as_hwnd(hwnd_);
  hwnd_ = nullptr;
  ::DestroyWindow(hwnd);
}

void window::draw(const ui::rect* dirty_rect) {
  if (!content_view_ || !ctx_ || !ctx_->canvas()) {
    return;
  }
  const ui::rect* effective_dirty_rect = live_resizing_ ? nullptr : dirty_rect;

  ui::rect present_dirty_rect{0.0f, 0.0f, max(frame_.width, 0.0f), max(frame_.height, 0.0f)};
  if (effective_dirty_rect) {
    const float clipped_left = max(effective_dirty_rect->left, 0.0f);
    const float clipped_top = max(effective_dirty_rect->top, 0.0f);
    const float clipped_right = min(effective_dirty_rect->right(), frame_.width);
    const float clipped_bottom = min(effective_dirty_rect->bottom(), frame_.height);
    present_dirty_rect.left = clipped_left;
    present_dirty_rect.top = clipped_top;
    present_dirty_rect.width = max(0.0f, clipped_right - clipped_left);
    present_dirty_rect.height = max(0.0f, clipped_bottom - clipped_top);
    if (present_dirty_rect.width <= 0.0f || present_dirty_rect.height <= 0.0f) {
      return;
    }
  }

  const ui::rect draw_dirty_rect = ctx_->expand_dirty_rect(present_dirty_rect);
  if (draw_dirty_rect.width <= 0.0f || draw_dirty_rect.height <= 0.0f) {
    return;
  }

  const auto content = content_bounds();
  ui::rect content_dirty_rect{};
  {
    const float clipped_left = max(draw_dirty_rect.left, content.left);
    const float clipped_top = max(draw_dirty_rect.top, content.top);
    const float clipped_right = min(draw_dirty_rect.right(), content.right());
    const float clipped_bottom = min(draw_dirty_rect.bottom(), content.bottom());
    content_dirty_rect.left = max(0.0f, clipped_left - content.left);
    content_dirty_rect.top = max(0.0f, clipped_top - content.top);
    content_dirty_rect.width = max(0.0f, clipped_right - clipped_left);
    content_dirty_rect.height = max(0.0f, clipped_bottom - clipped_top);
  }

  auto* canvas = ctx_->canvas();
  canvas->save();
  canvas->clipRect(
      SkRect::MakeXYWH(draw_dirty_rect.left, draw_dirty_rect.top, draw_dirty_rect.width, draw_dirty_rect.height),
      true);

  draw_border();
  draw_title();
  content_view_->layout_if_needed();

  canvas->save();
  if (!is_maximized()) {
    canvas->translate(shadow_distance_, shadow_distance_);
  }
  if (content_dirty_rect.width > 0.0f && content_dirty_rect.height > 0.0f) {
    content_view_->draw(content_dirty_rect);
  }
  canvas->restore();
  canvas->restore();

  ctx_->flush();
  ctx_->synchronize(effective_dirty_rect ? &draw_dirty_rect : nullptr);
}

bool window::on_window_activate(bool active) {
#if ENABLE_MICA
  if (active) {
    MARGINS margins = {-1};
    ::DwmExtendFrameIntoClientArea(as_hwnd(hwnd_), &margins);
    set_app_theme(1, as_hwnd(hwnd_));
    apply_win11_mica(2, as_hwnd(hwnd_));
  }
#endif
  const bool is_max = !!IsZoomed(as_hwnd(hwnd_));
  as_sk_color(window_color_)
      = (active && !is_max) ? SkColorSetARGB(0x80, 0x46, 0x44, 0x40)
                            : SkColorSetARGB(0xFF, 0x46, 0x44, 0x40);

  if (!active) {
    clear_capture();
    update_hover_view(nullptr, {}, 0);
    set_focus_view(nullptr);
  }
  return true;
}

void window::set_content_view(ui::view* view) {
  if (!view) {
    return;
  }
  clear_capture();
  hover_view_ = nullptr;
  set_focus_view(nullptr);
  if (content_view_) {
    content_view_->release_internal();
  }
  content_view_ = view;
  content_view_->init(content_bounds());
  content_view_->set_gfx_context(ctx_.get());
  content_view_->retain_internal();
}

} // namespace priv
} // namespace iris
