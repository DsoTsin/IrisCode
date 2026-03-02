#include "window.hpp"
#include "app.hpp"
#include "view.hpp"
#include "view/button.hpp"
#include "graphics_context.hpp"

#ifdef _WIN32
#include "win/wincomp.hpp"
#endif

#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

#include "include/core/SkRRect.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/effects/SkImageFilters.h"
#include "include/effects/SkRuntimeEffect.h"
#include "include/effects/SkGradientShader.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkBlurTypes.h"

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

static_assert(sizeof(iris::ui::color) == sizeof(SkColor), "inconsistent color type size");

namespace iris {
namespace priv {
#ifdef _WIN32
extern void set_app_theme(int value, HWND hwnd);
extern long get_os_build_ver();
extern void apply_win10_effect(int accent_state, int n_color, HWND hwnd);
extern bool apply_win11_mica(int type, HWND hwnd);
class window;
void init_priv_win(window* wind, HWND hwnd);
extern HINSTANCE instance;
#endif
class window {
public:
  window(iris::ui::window* in_window, iris::ui::window* parent = nullptr);
  ~window();

  void init();
  void show();
  void draw(const ui::rect* dirty_rect = nullptr);
  void set_content_view(ui::view* inview);
  void resize(int w, int h);
  int zone_at(int x, int y) const;
  bool on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result);
  bool on_window_pos_changed();
  bool on_window_activate(bool active);

  void draw_title();
  void draw_border();
  ui::point window_to_content(const ui::point& window_point) const;
  ui::point content_to_view(ui::view* target, const ui::point& content_point) const;
  ui::view* hit_test_content(const ui::point& window_point) const;
  bool route_mouse_event(ui::view* target, const ui::mouse_event& event, ui::view** handled_by = nullptr);
  bool route_key_event(ui::view* target,
                       const ui::key_event& event,
                       bool is_char_message,
                       ui::view** handled_by = nullptr);
  void update_hover_view(ui::view* next_hover, const ui::point& window_point, uint8_t modifiers);
  void set_focus_view(ui::view* next_focus);
  void clear_capture();

  void set_border_width(float width) { border_width_ = width; }
  float border_width() const { return border_width_; }
  void set_shadow_distance(float dist) { shadow_distance_ = dist; }
  float shadow_distance() const { return shadow_distance_; }
  void set_corner_radius(float dist) { corner_radius_ = dist; }
  float corner_radius() const { return corner_radius_; }

  const ui::rect& frame() const { return frame_; }
  ui::rect content_bounds() const;

  iris::ui::window* wind() { return w_; }
  bool is_maximized() const;

private:
  friend class iris::ui::window;
#ifdef _WIN32
  friend void init_priv_win(window* wind, HWND hwnd);

  void _init(HWND parent_handle);

  HWND hwnd_;
#endif
  ui::rect frame_;
  ui::view* content_view_;
  float shadow_distance_ = 20.f;
  float border_width_ = 1.f;
  float corner_radius_ = 12.f;
  SkColor window_color = SkColorSetARGB(0x80, 0x46, 0x44, 0x40);
  iris::ui::window* w_;
  iris::ui::window* pw_;
  ref_ptr<iris::ui::graphics_context> ctx_;
  ui::view* hover_view_ = nullptr;
  ui::view* capture_view_ = nullptr;
  ui::view* focus_view_ = nullptr;
  uint8_t mouse_buttons_down_ = 0;
  bool tracking_mouse_leave_ = false;
  bool live_resizing_ = false;
};
#ifdef _WIN32
void init_priv_win(window* wind, HWND hwnd) {
  wind->hwnd_ = hwnd;
  RECT w_rect;
  ::GetWindowRect(hwnd, &w_rect);
  wind->frame_ = {(float)w_rect.left, (float)w_rect.top, (float)w_rect.right - (float)w_rect.left,
                  (float)w_rect.bottom - (float)w_rect.top};
  wind->init();
}
#endif
} // namespace priv

namespace ui {
window::window(window* in_window) : d(new priv::window(this, in_window)) {
#ifdef _WIN32
  d->_init(in_window ? (HWND)in_window->native_handle() : NULL);
#endif
  app::shared()->add_window(this);
}
bool window::on_window_pos_changed() {
  return d->on_window_pos_changed();
}
bool window::on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result) {
  return d->on_native_event(msg, w_param, l_param, out_result);
}
bool window::on_window_activate(bool active) {
  return d->on_window_activate(active);
}
window::~window() {
  if (d) {
    app::shared()->remove_window(this);
    delete d;
    finalize();
    d = nullptr;
  }
}

void window::initialize() {
  d->init();
}

void window::finalize() {}

void window::set_content_view(view* inview) {
  d->set_content_view(inview);
}

void window::set_delegate() {}

void window::show() {
  d->show();
}
void window::draw() {
  d->draw();
}
void window::draw(const ui::rect& dirty_rect) {
  d->draw(&dirty_rect);
}
const ui::rect& window::frame() const {
  return d->frame();
}
void* window::native_handle() const {
#ifdef _WIN32
  return d->hwnd_;
#else
  return nullptr;
#endif
}

/*
        public PixelPoint Position
        {
            get
            {
                GetWindowRect(_hwnd, out var rc);
                return new PixelPoint(rc.left, rc.top);
            }
            set
            {
                SetWindowPos(
                    Handle.Handle,
                    IntPtr.Zero,
                    value.X,
                    value.Y,
                    0,
                    0,
                    SetWindowPosFlags.SWP_NOSIZE | SetWindowPosFlags.SWP_NOACTIVATE | SetWindowPosFlags.SWP_NOZORDER);
                
                if (ShCoreAvailable && Win32Platform.WindowsVersion >= PlatformConstants.Windows8_1)
                {
                    var monitor = MonitorFromWindow(Handle.Handle, MONITOR.MONITOR_DEFAULTTONEAREST);

                    if (GetDpiForMonitor(
                            monitor,
                            MONITOR_DPI_TYPE.MDT_EFFECTIVE_DPI,
                            out _dpi,
                            out _) == 0)
                    {
                    // public const double StandardDpi = 96;
                        _scaling = _dpi / StandardDpi;
                    }
                }
            }
        }*/

void window::set_border_width(float width) {
  d->set_border_width(width);
}
float window::border_width() const {
  return d->border_width();
}
void window::set_shadow_distance(float dist) {
  d->set_shadow_distance(dist);
}
float window::shadow_distance() const {
  return d->shadow_distance();
}
void window::set_corner_radius(float radius) {
  d->set_corner_radius(radius);
}
float window::corner_radius() const {
  return d->corner_radius();
}
void window::resize(int w, int h) {
  d->resize(w, h);
}
int window::zone_at(int x, int y) {
  return d->zone_at(x, y);
}
} // namespace ui
namespace priv {
#ifdef _WIN32
bool is_win11_or_greater() {
  OSVERSIONINFOEX osvi = {};
  DWORDLONG dwlConditionMask = 0;

  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  osvi.dwMajorVersion = 10;
  osvi.dwMinorVersion = 0;
  osvi.dwBuildNumber = 22000; // Windows 11 build number

  VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

  return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
                           dwlConditionMask)
         != TRUE;
}
#endif
enum class BackdropType { None = 0, Mica = 1, Acrylic = 3, Tabbed = 2 };

#ifdef _WIN32
namespace {
constexpr uint8_t kMouseLeftMask = 1 << 0;
constexpr uint8_t kMouseRightMask = 1 << 1;
constexpr uint8_t kMouseMiddleMask = 1 << 2;
constexpr uint8_t kMouseX1Mask = 1 << 3;
constexpr uint8_t kMouseX2Mask = 1 << 4;

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
#endif

window::window(iris::ui::window* in_window, iris::ui::window* parent) :
#ifdef _WIN32
    hwnd_(NULL), 
#endif
    content_view_(nullptr)
    , w_(in_window)
    , pw_(parent)
    , ctx_(nullptr) {
}
window::~window() {
  clear_capture();
  hover_view_ = nullptr;
  focus_view_ = nullptr;
  if (content_view_) {
    content_view_->release_internal();
    content_view_ = nullptr;
  }
#ifdef _WIN32
  if (hwnd_) {
    ::DestroyWindow(hwnd_);
    hwnd_ = NULL;
  }
#endif
}
void window::init() {
  ctx_ = ref_ptr<ui::graphics_context>(new ui::graphics_context(w_, true));
}
void window::resize(int w, int h) {
  if (w <= 0 || h <= 0) {
    return;
  }
  frame_.width = w;
  frame_.height = h;
  if (ctx_) {
    ctx_->resize(w, h);
  }
  if (content_view_) {
    // Keep root view frame in sync with the current content area so
    // children (e.g. dock fill/code_editor) can relayout on resize.
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
#ifdef _WIN32
  if (hwnd_ && ::GetCapture() == hwnd_) {
    ::ReleaseCapture();
  }
#endif
}

bool window::on_native_event(uint32_t msg, uintptr_t w_param, intptr_t l_param, intptr_t* out_result) {
#ifndef _WIN32
  (void)msg;
  (void)w_param;
  (void)l_param;
  (void)out_result;
  return false;
#else
  if (!content_view_) {
    return false;
  }

  const UINT wm = static_cast<UINT>(msg);
  const auto modifiers = query_input_modifiers();
  const auto window_point = message_point(hwnd_, wm, static_cast<LPARAM>(l_param));
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
    if (reinterpret_cast<HWND>(l_param) != hwnd_) {
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
      tracking.hwndTrack = hwnd_;
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
    event.is_double_click = (wm == WM_LBUTTONDBLCLK || wm == WM_RBUTTONDBLCLK || wm == WM_MBUTTONDBLCLK
                             || wm == WM_XBUTTONDBLCLK);
    ui::view* handled_by = nullptr;
    const bool handled = route_mouse_event(target, event, &handled_by);
    if (handled) {
      capture_view_ = handled_by;
      mouse_buttons_down_ |= mouse_button_mask(button);
      set_focus_view(handled_by);
      ::SetCapture(hwnd_);
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
      ::InvalidateRect(hwnd_, nullptr, FALSE);
    }
    return false;
  default:
    return false;
  }
#endif
}

int window::zone_at(int x, int y) const {
#if _WIN32
  if (is_maximized()) {
    return HTCLIENT;
  }
  const UINT dpi = query_window_dpi(hwnd_);
  const float dpi_scale = static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);

  float resize_border_dip = 2.f;
  const int resize_border = static_cast<int>(resize_border_dip * dpi_scale + 0.5f);

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
  const bool bottom
      = in_horizontal_range && y >= bottom_edge - resize_border && y < bottom_edge + resize_border;

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
#else
  return 0;
#endif
}
void window::draw_title() {
  if (!ctx_)
    return;
  // todo:
}

static const char* window_border_fx = R"(
uniform float width;
uniform float height;
uniform float cornerRadius;
uniform float blurRadius;
uniform vec4 borderColor;
uniform float borderWidth;

float sdRoundRect( in vec2 p, in vec2 b, in float r ) {
    vec2 q = abs(p)-b+r;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r;
}

vec4 normalBlend(vec4 src, vec4 dst) {
    float finalAlpha = src.a + dst.a * (1.0 - src.a);
    return vec4(
        (src.rgb * src.a + dst.rgb * dst.a * (1.0 - src.a)) / finalAlpha,
        finalAlpha
    );
}

float sigmoid(float t) {
    return 1.0 / (1.0 + exp(-t));
}

vec4 main( vec2 fragCoord )
{
    vec2 center = vec2(width, height) * 0.5;
    vec2 hsize = center - vec2(blurRadius);
    vec2 position = fragCoord - center;
	float distShadow = clamp(sigmoid(
                            sdRoundRect(position, hsize, cornerRadius + blurRadius) 
                            / blurRadius), 0.0, 1.0);
        
    float distRect = clamp(sdRoundRect(position, hsize, cornerRadius), 0.0, 1.0);
    float distBorder = clamp(sdRoundRect(position, hsize - vec2(borderWidth), cornerRadius - borderWidth), 0.0, 1.0);
    //float distBorderInner = clamp(sdRoundRect(position, hsize - vec2(borderWidth*2), cornerRadius - borderWidth*2), 0.0, 1.0);
    // border outline
    float outline = step(distBorder, distRect);
    float alpha = smoothstep(1.0-distBorder, outline, 1.0-distShadow);
    return vec4(borderColor.rgb*alpha, alpha);
}
)";

static void draw_window_background(SkCanvas* canvas,
                                   const SkRect& rect,
                                   float corner_radius,
                                   float blur_radius,
                                   const SkColor& color) {
  struct Param {
    float width;
    float height;
    float cornerRadius;
    float blurRadius;
    SkColor4f color;
    float borderWidth;
  };
  Param p{rect.width(), rect.height(), corner_radius, blur_radius};
  p.color = SkColor4f::FromColor(color);
  p.borderWidth = 4.0f;
  auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(window_border_fx));
  if (!effect) {
    SkDebugf("Failed to create fx: %s", error.c_str());
    return;
  }
  auto bg_color = SkColor4f::FromColor(color);
  auto uniforms = SkData::MakeWithCopy(&p, sizeof(p));
  SkPaint window_paint;
  window_paint.setShader(effect->makeShader(uniforms, nullptr, 0, &SkMatrix::I()));
  window_paint.setAntiAlias(true);
  window_paint.setBlendMode(SkBlendMode::kScreen);
  canvas->drawRect(rect, window_paint);
}

static void draw_button(SkCanvas* canvas, const SkRect& rect, float radius, const SkColor& color) {
  struct Param {
    SkRect rect;
    SkColor4f color; // .a is radius
  };

  static const char* lightingSource = R"(
        uniform float left;
        uniform float top;

        uniform float right;
        uniform float bottom;
        uniform vec3 bcolor;
        uniform float radius;

        float sdRoundedBox( in vec2 p, in vec2 b, in float r ) {
            vec2 q = abs(p)-b+r;
            return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r;
        }

        float invlerp(float x, float a, float b)
        {
            return clamp((x - a) / (b - a), 0., 1.);
        }

        float calcdepth(float dist, vec2 centeroff)
        {
            float a = 10.0;
            return pow(1.-pow(1.-invlerp(dist, 0., -25.),a),1./a);
        }

        vec4 dist2color(float dist)
        {
            if (dist > 0.)
                return vec4(0.);
            return smoothstep(dist, 0., -1.) * vec4(bcolor, 1.0);
        }

        float G1V(float dotNV, float k)
        {
	        return 1.0/(dotNV*(1.0-k)+k);
        }

        float GGX(vec3 N, float roughness, vec3 L)
        {
            N = normalize(N);
            vec3 V = vec3(0.,0.,1.);
            vec3 H = normalize(V+L);
            float dotNL = max(0., dot(N, L));
            float dotNV = max(0., dot(N,V));
            float dotNH = max(0., dot(N, H));
            float alpha = roughness * roughness;
            float alphaSqr = alpha*alpha;
	        float pi = 3.14159;
	        float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0;
	        float D = alphaSqr/(pi * denom * denom);
            float F = 0.04;
            float k = alpha/2.0;
	        float vis = G1V(dotNL,k)*G1V(dotNV,k);
            return D * dotNL * F * pi;
        }

        vec3 nrm2speclight(vec3 nrm, float roughness)
        {
            vec3 ret = vec3(0.);
            ret += GGX(nrm, roughness, vec3(0.0,0.4,0.9));
            ret += GGX(nrm, roughness, vec3(0.0,-0.2,0.2));
            return ret;
        }

        vec4 main(vec2 coord) {
            vec2 p = coord;
            vec2 bsize = vec2(right-left, bottom-top)*0.5; // button size
            vec2 center = bsize+vec2(left,top);
            float bround = radius;
            vec2 o = p - center; // button position
            float dist = sdRoundedBox(o, bsize, bround);
            float d = calcdepth(dist, o);
            vec4 color = dist2color(dist);

            vec2 oL = o+vec2(-0.1,0.);
            vec2 oR = o+vec2(0.1,0.);
            vec2 oB = o+vec2(0.,0.1);
            vec2 oT = o+vec2(0.,-0.1);
            float dL = calcdepth(sdRoundedBox(oL, bsize, bround), oL);
            float dR = calcdepth(sdRoundedBox(oR, bsize, bround), oR);
            float ddX = (dR - dL) * 25.;
            float dB = calcdepth(sdRoundedBox(oB, bsize, bround), oB);
            float dT = calcdepth(sdRoundedBox(oT, bsize, bround), oT);
            float ddY = (dT - dB) * 25.;
            float ddZ = sqrt(1. - min(1.,ddX * ddX + ddY * ddY));
            vec3 normal = vec3(ddX, -ddY, ddZ);
            float rough = 0.55;
            vec3 speclight = nrm2speclight(normal, rough) * (d > 0.1 ? 1. : 0.);
            vec3 final = color.rgb * bcolor + speclight;
            vec4 fragColor = vec4(final,color.a);
            return fragColor;
        }
    )";

  auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(lightingSource));
  if (!effect) {
    SkDebugf("Failed to create fx: %s", error.c_str());
    return;
  }
  auto bcolor = SkColor4f::FromColor(color);
  bcolor.fA = radius;
  Param param{rect, bcolor};
  auto uniforms = SkData::MakeWithCopy(&param, sizeof(param));

  
  float shadowBlur = /* pressed ? 1.0f : */ 1.0f;
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, shadowBlur));

  SkRect shadowRect = rect.makeOffset(0, 1);
  canvas->drawRoundRect(shadowRect, radius, radius, shadowPaint);


  SkPaint buttonPaint;
  buttonPaint.setShader(effect->makeShader(uniforms, nullptr, 0, &SkMatrix::I()));
  buttonPaint.setAntiAlias(true);

  canvas->drawRect(rect, /* radius, radius,*/ buttonPaint);
}

void draw3DButtonWithLighting(SkCanvas* canvas, const SkRect& bounds, bool pressed) {
  float lightOffset = pressed ? -2.0f : 2.0f;
  float shadowBlur = pressed ? 1.0f : 3.0f;
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, shadowBlur));

  SkRect shadowRect = bounds.makeOffset(0, 3);
  canvas->drawRoundRect(shadowRect, 20, 20, shadowPaint);

  // ���ư�ť����
  SkPoint gradientPoints[2]
      = {SkPoint::Make(bounds.left(), bounds.top()), SkPoint::Make(bounds.left(), bounds.bottom())};

  SkColor gradientColors[2] = {
      SkColorSetRGB(0x1e,0x75,0xe4),
      SkColorSetRGB(0x0d,0x62,0xcb)
  };

  auto buttonShader = SkGradientShader::MakeLinear(gradientPoints, gradientColors, nullptr, 2,
                                                   SkTileMode::kClamp);

  SkPaint buttonPaint;
  buttonPaint.setShader(buttonShader);
  buttonPaint.setAntiAlias(true);

  canvas->drawRoundRect(bounds, 20, 20, buttonPaint);
  SkPaint highlightPaint;
  highlightPaint.setColor(SkColorSetARGB(0xFF, 0x5b,0x99,0xeb));
  highlightPaint.setStyle(SkPaint::kStroke_Style);
  highlightPaint.setStrokeWidth(2.0f);
  highlightPaint.setAntiAlias(true);

  SkPath highlightPath;
  SkRect highlightRect = bounds.makeInset(2, 2);
  SkScalar r[2] = {18, 18};
  highlightPath.addRoundRect(highlightRect, 18,18);
  canvas->drawPath(highlightPath, highlightPaint);
}

void window::draw_border() {
  if (!ctx_)
    return;
#ifdef _WIN32
  bool is_max = !!::IsZoomed(hwnd_);
#else
  bool is_max = false;
#endif
  // Shadow parameters
  const int shadow_blur = is_max ? 0 : shadow_distance();
  const int shadow_offset_x = 0;
  const int shadow_offset_y = 0;
  const int borderWidth = border_width();
  const int cornerRadius = is_max ? 0 : corner_radius();
  SkColor border_color = SK_ColorGRAY;
  static const SkColor shadowColor = SkColorSetARGB(0x80, 0x00, 0x00, 0x00);
  // Window rectangle (smaller than canvas to accommodate shadow)
  SkRect windowRect = SkRect::MakeXYWH(shadow_blur, shadow_blur, frame_.width - 2 * shadow_blur,
                                       frame_.height - 2 * shadow_blur);
  SkRect window_outter_rect = SkRect::MakeXYWH(shadow_blur - 1, shadow_blur - 1, frame_.width - 2 * (shadow_blur-1),
                         frame_.height - 2 * (shadow_blur - 1));

  // Draw shadow using image filter
  SkPaint shadowPaint;
  shadowPaint.setImageFilter(SkImageFilters::DropShadowOnly(shadow_offset_x, shadow_offset_y,
                                                        shadow_blur / 2.0f, shadow_blur / 2.0f,
                                                            shadowColor, nullptr));
  SkRect native_window_rect{0, 0, frame_.width, frame_.height};
  auto canvas = ctx_->canvas();
  SkPaint clear_paint;
  clear_paint.setColor(SK_ColorTRANSPARENT);
  clear_paint.setBlendMode(SkBlendMode::kSrc);
  canvas->drawRect(native_window_rect, clear_paint);
  // Draw the shadow
  canvas->save();

  canvas->drawRoundRect(windowRect, cornerRadius, cornerRadius, shadowPaint);
  SkVector radii[4] = {{cornerRadius, cornerRadius},
                       {cornerRadius, cornerRadius},
                       {cornerRadius, cornerRadius},
                       {cornerRadius, cornerRadius}};
  SkRRect clipRRect;
  clipRRect.setRectRadii(windowRect, radii);
  canvas->clipRRect(clipRRect, true);
  // Draw window background
  SkPaint windowPaint;
  windowPaint.setColor(window_color);
  windowPaint.setAntiAlias(true);
  canvas->drawRoundRect(windowRect, cornerRadius, cornerRadius, windowPaint);

  SkRect panel_rect = SkRect::MakeXYWH(300, shadow_blur, frame_.width - shadow_blur - 300,
                                       frame_.height - 2 * shadow_blur);
  windowPaint.setColor(SkColorSetARGB(0xFF, 0x23, 0x22, 0x20));
  windowPaint.setAntiAlias(true);
  canvas->drawRect(panel_rect, windowPaint);

  canvas->restore();

  // Draw border
  SkPaint borderPaint;
  borderPaint.setColor(border_color);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(borderWidth);
  borderPaint.setAntiAlias(true);
  canvas->drawRoundRect(windowRect, cornerRadius, cornerRadius, borderPaint);
  borderPaint.setColor(SK_ColorBLACK);
  canvas->drawRoundRect(window_outter_rect, cornerRadius + 1, cornerRadius + 1, borderPaint);

  canvas->save();

  canvas->clipRRect(clipRRect, true);
  
  // Close buttons
  //SkPaint btn_paint;
  //btn_paint.setAntiAlias(true);
  // Filled circle
  //btn_paint.setColor(col_close_btn);
  //btn_paint.setStyle(SkPaint::kFill_Style);
  //canvas->drawCircle(50, 50, 10, btn_paint);
  //btn_paint.setColor(col_minimize_btn);
  //canvas->drawCircle(80, 50, 10, btn_paint);
  //btn_paint.setColor(col_maximize_btn);
  //canvas->drawCircle(110, 50, 10, btn_paint);

  canvas->restore();
}

ui::rect window::content_bounds() const {
  bool is_max = is_maximized();
  return ui::rect{is_max ? 0 : shadow_distance_, is_max ? 0 : shadow_distance_,
                  frame_.width - (is_max ? 0 : 2.f) * shadow_distance_,
                  frame_.height - (is_max ? 0 : 2.f) * shadow_distance_};
}

bool window::is_maximized() const {
#if _WIN32
  return !!IsZoomed((HWND)w_->native_handle());
#else
  return false;
#endif
}

#ifdef _WIN32
void window::_init(HWND parent_handle) {
#if !ENABLE_MICA
  UINT window_ex_style = WS_EX_WINDOWEDGE | WS_EX_COMPOSITED | WS_EX_APPWINDOW /* | WS_EX_LAYERED*/;
#else
  UINT window_ex_style = WS_EX_LAYERED;
#endif
  // UINT window_ex_style = WS_EX_WINDOWEDGE | WS_EX_COMPOSITED | WS_EX_APPWINDOW;
  UINT window_style = WS_OVERLAPPEDWINDOW;
  hwnd_ = ::CreateWindowExA(window_ex_style,
                            /*class name*/ iris::app::shared()->name(),
                            /*window name*/ "",
                            /*style*/ window_style, CW_USEDEFAULT, CW_USEDEFAULT,
                            /*width*/ 800, /*height*/ 600,
                            (parent_handle ? (HWND)parent_handle : NULL),
                            /*hMenu*/ NULL,
                            /*hInstance*/ instance,
                            /*lpParam*/ this);

  // Extend window frame into client area for better visual integration
  MARGINS margins = {-1};
  ::DwmExtendFrameIntoClientArea(hwnd_, &margins);
#if !ENABLE_MICA
  {
    // NC Rendering Policy
    const DWMNCRENDERINGPOLICY RenderingPolicy = DWMNCRP_DISABLED;
    SUCCEEDED(DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_POLICY, &RenderingPolicy,
                                    sizeof(RenderingPolicy)));

    // NC_PAINT
    const BOOL enable_nc_paint = false;
    SUCCEEDED(DwmSetWindowAttribute(hwnd_, DWMWA_ALLOW_NCPAINT, &enable_nc_paint,
                                    sizeof(enable_nc_paint)));
  }
#else
  ::SetLayeredWindowAttributes(hwnd_, RGB(255, 255, 255), 0, LWA_COLORKEY);
#endif
  ::SetWindowTextA(hwnd_, "IrisWindow");
  if (!is_win11_or_greater()) {
    return;
  }

  {
    const BOOL enable_host_backdrop = 1;
    DwmSetWindowAttribute(hwnd_, DWMWA_USE_HOSTBACKDROPBRUSH, &enable_host_backdrop, sizeof(BOOL));
  }

  RECT w_rect;
  ::GetWindowRect(hwnd_, &w_rect);
  frame_ = {(float)w_rect.left, (float)w_rect.top, (float)w_rect.right - (float)w_rect.left,
            (float)w_rect.bottom - (float)w_rect.top};
}
#endif

bool window::on_window_pos_changed() {
#ifdef _WIN32
  bool is_max = !!::IsZoomed(hwnd_);
#else
  bool is_max = false;
#endif
  window_color
      = !is_max ? SkColorSetARGB(0x80, 0x46, 0x44, 0x40) : SkColorSetARGB(0xFF, 0x46, 0x44, 0x40); 
  if (ctx_) {
    ctx_->on_window_pos_changed(); 
  }
  return true;
}

void window::show() {
#ifdef _WIN32
  ::ShowWindow(hwnd_, SW_SHOWNORMAL);
  ::UpdateWindow(hwnd_);
#endif
}
void window::draw(const ui::rect* dirty_rect) {
  if (!content_view_ || !ctx_) {
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
  // Present dirty region must match what we actually redraw (expanded damage),
  // otherwise flip-model partial present can leave stale content on screen.
  ctx_->synchronize(effective_dirty_rect ? &draw_dirty_rect : nullptr);
}
bool window::on_window_activate(bool active) {
#if ENABLE_MICA
  if (active) {
    MARGINS margins = {-1};
    ::DwmExtendFrameIntoClientArea(hwnd_, &margins);
    set_app_theme(1, hwnd_);
    // apply_win10_effect(4, 0xcccccc, hwnd_);
    apply_win11_mica(2, hwnd_);
  }
#endif
  bool is_max = !!IsZoomed(hwnd_);
  window_color
      = (active && !is_max) ? SkColorSetARGB(0x80, 0x46, 0x44, 0x40) : SkColorSetARGB(0xFF, 0x46, 0x44, 0x40);
  
  if (ctx_) {
    ctx_->on_window_activate(active);
  }
  if (!active) {
    clear_capture();
    update_hover_view(nullptr, {}, 0);
    set_focus_view(nullptr);
  }
  return true;
}
void window::set_content_view(ui::view* view) {
  if (view) {
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
}

} // namespace priv
} // namespace iris
