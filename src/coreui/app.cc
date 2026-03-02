#include "app.hpp"
#include "window.hpp"
#include "log.hpp"
#include <magic_enum/magic_enum_all.hpp>

#define USE_REALTIME_LOOP 0

#ifdef _WIN32
#include <Windows.h>

enum class win_msg : UINT {
  // ========== System Messages ==========
  
  /// No operation message
  null = WM_NULL,
  
  /// Window creation message
  create = WM_CREATE,
  
  /// Window destruction message
  destroy = WM_DESTROY,
  
  /// Window move message
  move = WM_MOVE,
  
  /// Window size change message
  size = WM_SIZE,
  
  /// Window activation/deactivation message
  activate = WM_ACTIVATE,
  
  /// Set keyboard focus message
  set_focus = WM_SETFOCUS,
  
  /// Kill keyboard focus message
  kill_focus = WM_KILLFOCUS,
  
  /// Enable/disable window message
  enable = WM_ENABLE,
  
  /// Set/clear redraw flag message
  set_redraw = WM_SETREDRAW,
  
  /// Set window text message
  set_text = WM_SETTEXT,
  
  /// Get window text message
  get_text = WM_GETTEXT,
  
  /// Get window text length message
  get_text_length = WM_GETTEXTLENGTH,
  
  /// Paint window message
  paint = WM_PAINT,
  
  /// Close window message
  close = WM_CLOSE,
  
  /// Quit application message
  quit = WM_QUIT,
  
  /// Query end session message
  query_end_session = WM_QUERYENDSESSION,
  
  /// End session message
  end_session = WM_ENDSESSION,
  
  /// Erase background message
  erase_bkgnd = WM_ERASEBKGND,
  
  /// System color change message
  sys_color_change = WM_SYSCOLORCHANGE,
  
  /// Show/hide window message
  show_window = WM_SHOWWINDOW,
  
  /// Windows INI change message (deprecated)
  win_ini_change = WM_WININICHANGE,
  
  /// Settings change message
  setting_change = WM_SETTINGCHANGE,
  
  /// Device mode change message
  dev_mode_change = WM_DEVMODECHANGE,
  
  /// Application activation message
  activate_app = WM_ACTIVATEAPP,
  
  /// Font change message
  font_change = WM_FONTCHANGE,
  
  /// Time change message
  time_change = WM_TIMECHANGE,
  
  /// Cancel mode message
  cancel_mode = WM_CANCELMODE,
  
  /// Set cursor message
  set_cursor = WM_SETCURSOR,
  
  /// Mouse activate message
  mouse_activate = WM_MOUSEACTIVATE,
  
  /// Child activate message
  child_activate = WM_CHILDACTIVATE,
  
  /// Queue sync message
  queue_sync = WM_QUEUESYNC,
  
  /// Get min/max info message
  get_min_max_info = WM_GETMINMAXINFO,
  
  /// Paint icon message
  paint_icon = WM_PAINTICON,
  
  /// Icon erase background message
  icon_erase_bkgnd = WM_ICONERASEBKGND,
  
  /// Next dialog control message
  next_dlg_ctl = WM_NEXTDLGCTL,
  
  /// Spooler status message
  spooler_status = WM_SPOOLERSTATUS,
  
  /// Draw item message
  draw_item = WM_DRAWITEM,
  
  /// Measure item message
  measure_item = WM_MEASUREITEM,
  
  /// Delete item message
  delete_item = WM_DELETEITEM,
  
  /// Virtual key to item message
  v_key_to_item = WM_VKEYTOITEM,
  
  /// Character to item message
  char_to_item = WM_CHARTOITEM,
  
  /// Set font message
  set_font = WM_SETFONT,
  
  /// Get font message
  get_font = WM_GETFONT,
  
  /// Set hotkey message
  set_hotkey = WM_SETHOTKEY,
  
  /// Get hotkey message
  get_hotkey = WM_GETHOTKEY,
  
  /// Query drag icon message
  query_drag_icon = WM_QUERYDRAGICON,
  
  /// Compare item message
  compare_item = WM_COMPAREITEM,
  
  /// Get object message
  get_object = WM_GETOBJECT,
  
  /// Compacting message
  compacting = WM_COMPACTING,
  
  /// Communication notification message (obsolete)
  comm_notify = WM_COMMNOTIFY,
  
  /// Window position changing message
  window_pos_changing = WM_WINDOWPOSCHANGING,
  
  /// Window position changed message
  window_pos_changed = WM_WINDOWPOSCHANGED,
  
  /// Power management message
  power = WM_POWER,
  
  /// Copy data message
  copy_data = WM_COPYDATA,
  
  /// Cancel journal message
  cancel_journal = WM_CANCELJOURNAL,
  
  /// Notify message
  notify = WM_NOTIFY,
  
  /// Input language change request message
  input_lang_change_request = WM_INPUTLANGCHANGEREQUEST,
  
  /// Input language change message
  input_lang_change = WM_INPUTLANGCHANGE,
  
  /// Training card message
  t_card = WM_TCARD,
  
  /// Help message
  help = WM_HELP,
  
  /// User changed message
  user_changed = WM_USERCHANGED,
  
  /// Notify format message
  notify_format = WM_NOTIFYFORMAT,
  
  /// Context menu message
  context_menu = WM_CONTEXTMENU,
  
  /// Style changing message
  style_changing = WM_STYLECHANGING,
  
  /// Style changed message
  style_changed = WM_STYLECHANGED,
  
  /// Display change message
  display_change = WM_DISPLAYCHANGE,
  
  /// Get icon message
  get_icon = WM_GETICON,
  
  /// Set icon message
  set_icon = WM_SETICON,
  
  // ========== Non-Client Area Messages ==========
  
  /// Non-client create message
  nc_create = WM_NCCREATE,
  
  /// Non-client destroy message
  nc_destroy = WM_NCDESTROY,
  
  /// Non-client calculate size message
  nc_calc_size = WM_NCCALCSIZE,
  
  /// Non-client hit test message
  nc_hit_test = WM_NCHITTEST,
  
  /// Non-client paint message
  nc_paint = WM_NCPAINT,
  
  /// Non-client activate message
  nc_activate = WM_NCACTIVATE,
  
  /// Get dialog code message
  get_dlg_code = WM_GETDLGCODE,
  
  /// Synchronous paint message
  sync_paint = WM_SYNCPAINT,
  
  /// Non-client mouse move message
  nc_mouse_move = WM_NCMOUSEMOVE,
  
  /// Non-client left button down message
  nc_l_button_down = WM_NCLBUTTONDOWN,
  
  /// Non-client left button up message
  nc_l_button_up = WM_NCLBUTTONUP,
  
  /// Non-client left button double-click message
  nc_l_button_dbl_clk = WM_NCLBUTTONDBLCLK,
  
  /// Non-client right button down message
  nc_r_button_down = WM_NCRBUTTONDOWN,
  
  /// Non-client right button up message
  nc_r_button_up = WM_NCRBUTTONUP,
  
  /// Non-client right button double-click message
  nc_r_button_dbl_clk = WM_NCRBUTTONDBLCLK,
  
  /// Non-client middle button down message
  nc_m_button_down = WM_NCMBUTTONDOWN,
  
  /// Non-client middle button up message
  nc_m_button_up = WM_NCMBUTTONUP,
  
  /// Non-client middle button double-click message
  nc_m_button_dbl_clk = WM_NCMBUTTONDBLCLK,
  
  /// Non-client X button down message
  nc_x_button_down = WM_NCXBUTTONDOWN,
  
  /// Non-client X button up message
  nc_x_button_up = WM_NCXBUTTONUP,
  
  /// Non-client X button double-click message
  nc_x_button_dbl_clk = WM_NCXBUTTONDBLCLK,
  
  // ========== Input Messages ==========
  
  /// Input device change message
  input_device_change = WM_INPUT_DEVICE_CHANGE,
  
  /// Raw input message
  input = WM_INPUT,
  
  /// Key down message
  key_down = WM_KEYDOWN,
  
  /// Key up message
  key_up = WM_KEYUP,
  
  /// Character message
  char_msg = WM_CHAR,
  
  /// Dead character message
  dead_char = WM_DEADCHAR,
  
  /// System key down message
  sys_key_down = WM_SYSKEYDOWN,
  
  /// System key up message
  sys_key_up = WM_SYSKEYUP,
  
  /// System character message
  sys_char = WM_SYSCHAR,
  
  /// System dead character message
  sys_dead_char = WM_SYSDEADCHAR,
  
  /// Unicode character message
  uni_char = WM_UNICHAR,

  // ========== Dialog Messages ==========

  /// Initialize dialog message
  init_dialog = WM_INITDIALOG,

  /// Command message
  command = WM_COMMAND,

  /// System command message
  sys_command = WM_SYSCOMMAND,

  /// Timer message
  timer = WM_TIMER,
  
  // ========== IME Messages ==========
  
  /// IME set context message
  ime_set_context = WM_IME_SETCONTEXT,
  
  /// IME notify message
  ime_notify = WM_IME_NOTIFY,
  
  /// IME control message
  ime_control = WM_IME_CONTROL,
  
  /// IME composition full message
  ime_composition_full = WM_IME_COMPOSITIONFULL,
  
  /// IME select message
  ime_select = WM_IME_SELECT,
  
  /// IME character message
  ime_char = WM_IME_CHAR,
  
  /// IME request message
  ime_request = WM_IME_REQUEST,
  
  /// IME keydown message
  ime_key_down = WM_IME_KEYDOWN,
  
  /// IME keyup message
  ime_key_up = WM_IME_KEYUP,
  
  /// IME start composition message
  ime_start_composition = WM_IME_STARTCOMPOSITION,
  
  /// IME end composition message
  ime_end_composition = WM_IME_ENDCOMPOSITION,
  
  /// IME composition message
  ime_composition = WM_IME_COMPOSITION,
  
  // ========== Scroll Messages ==========
  
  /// Horizontal scroll message
  h_scroll = WM_HSCROLL,
  
  /// Vertical scroll message
  v_scroll = WM_VSCROLL,
  
  // ========== Menu Messages ==========
  
  /// Initialize menu message
  init_menu = WM_INITMENU,
  
  /// Initialize menu popup message
  init_menu_popup = WM_INITMENUPOPUP,
  
  /// Gesture message
  gesture = WM_GESTURE,
  
  /// Gesture notify message
  gesture_notify = WM_GESTURENOTIFY,
  
  /// Menu select message
  menu_select = WM_MENUSELECT,
  
  /// Menu character message
  menu_char = WM_MENUCHAR,
  
  /// Enter idle message
  enter_idle = WM_ENTERIDLE,
  
  /// Menu right button up message
  menu_r_button_up = WM_MENURBUTTONUP,
  
  /// Menu drag message
  menu_drag = WM_MENUDRAG,
  
  /// Menu get object message
  menu_get_object = WM_MENUGETOBJECT,
  
  /// Uninitialize menu popup message
  un_init_menu_popup = WM_UNINITMENUPOPUP,
  
  /// Menu command message
  menu_command = WM_MENUCOMMAND,
  
  /// Change UI state message
  change_ui_state = WM_CHANGEUISTATE,
  
  /// Update UI state message
  update_ui_state = WM_UPDATEUISTATE,
  
  /// Query UI state message
  query_ui_state = WM_QUERYUISTATE,
  
  // ========== Control Color Messages ==========
  
  /// Control color message box message
  ctl_color_msg_box = WM_CTLCOLORMSGBOX,
  
  /// Control color edit message
  ctl_color_edit = WM_CTLCOLOREDIT,
  
  /// Control color list box message
  ctl_color_list_box = WM_CTLCOLORLISTBOX,
  
  /// Control color button message
  ctl_color_btn = WM_CTLCOLORBTN,
  
  /// Control color dialog message
  ctl_color_dlg = WM_CTLCOLORDLG,
  
  /// Control color scrollbar message
  ctl_color_scroll_bar = WM_CTLCOLORSCROLLBAR,
  
  /// Control color static message
  ctl_color_static = WM_CTLCOLORSTATIC,
  
  // ========== Mouse Messages ==========
  
  /// Mouse move message
  mouse_move = WM_MOUSEMOVE,
  
  /// Left button down message
  l_button_down = WM_LBUTTONDOWN,
  
  /// Left button up message
  l_button_up = WM_LBUTTONUP,
  
  /// Left button double-click message
  l_button_dbl_clk = WM_LBUTTONDBLCLK,
  
  /// Right button down message
  r_button_down = WM_RBUTTONDOWN,
  
  /// Right button up message
  r_button_up = WM_RBUTTONUP,
  
  /// Right button double-click message
  r_button_dbl_clk = WM_RBUTTONDBLCLK,
  
  /// Middle button down message
  m_button_down = WM_MBUTTONDOWN,
  
  /// Middle button up message
  m_button_up = WM_MBUTTONUP,
  
  /// Middle button double-click message
  m_button_dbl_clk = WM_MBUTTONDBLCLK,
  
  /// Mouse wheel message
  mouse_wheel = WM_MOUSEWHEEL,
  
  /// X button down message
  x_button_down = WM_XBUTTONDOWN,
  
  /// X button up message
  x_button_up = WM_XBUTTONUP,
  
  /// X button double-click message
  x_button_dbl_clk = WM_XBUTTONDBLCLK,
  
  /// Mouse horizontal wheel message
  mouse_h_wheel = WM_MOUSEHWHEEL,
  
  // ========== Parent Notification Messages ==========
  
  /// Parent notify message
  parent_notify = WM_PARENTNOTIFY,
  
  /// Enter menu loop message
  enter_menu_loop = WM_ENTERMENULOOP,
  
  /// Exit menu loop message
  exit_menu_loop = WM_EXITMENULOOP,
  
  /// Next menu message
  next_menu = WM_NEXTMENU,
  
  /// Sizing message
  sizing = WM_SIZING,
  
  /// Capture changed message
  capture_changed = WM_CAPTURECHANGED,
  
  /// Moving message
  moving = WM_MOVING,
  
  /// Power broadcast message
  power_broadcast = WM_POWERBROADCAST,
  
  /// Device change message
  device_change = WM_DEVICECHANGE,
  
  // ========== MDI Messages ==========
  
  /// MDI create message
  mdi_create = WM_MDICREATE,
  
  /// MDI destroy message
  mdi_destroy = WM_MDIDESTROY,
  
  /// MDI activate message
  mdi_activate = WM_MDIACTIVATE,
  
  /// MDI restore message
  mdi_restore = WM_MDIRESTORE,
  
  /// MDI next message
  mdi_next = WM_MDINEXT,
  
  /// MDI maximize message
  mdi_maximize = WM_MDIMAXIMIZE,
  
  /// MDI tile message
  mdi_tile = WM_MDITILE,
  
  /// MDI cascade message
  mdi_cascade = WM_MDICASCADE,
  
  /// MDI icon arrange message
  mdi_icon_arrange = WM_MDIICONARRANGE,
  
  /// MDI get active message
  mdi_get_active = WM_MDIGETACTIVE,
  
  /// MDI set menu message
  mdi_set_menu = WM_MDISETMENU,
  
  /// Enter size move message
  enter_size_move = WM_ENTERSIZEMOVE,
  
  /// Exit size move message
  exit_size_move = WM_EXITSIZEMOVE,
  
  /// Drop files message
  drop_files = WM_DROPFILES,
  
  /// MDI refresh menu message
  mdi_refresh_menu = WM_MDIREFRESHMENU,
  
  // ========== Pointer and Touch Messages ==========
  
  /// Pointer device change message
  pointer_device_change = WM_POINTERDEVICECHANGE,
  
  /// Pointer device in range message
  pointer_device_in_range = WM_POINTERDEVICEINRANGE,
  
  /// Pointer device out of range message
  pointer_device_out_of_range = WM_POINTERDEVICEOUTOFRANGE,
  
  /// Touch message
  touch = WM_TOUCH,
  
  /// Non-client pointer update message
  nc_pointer_update = WM_NCPOINTERUPDATE,
  
  /// Non-client pointer down message
  nc_pointer_down = WM_NCPOINTERDOWN,
  
  /// Non-client pointer up message
  nc_pointer_up = WM_NCPOINTERUP,
  
  /// Pointer update message
  pointer_update = WM_POINTERUPDATE,
  
  /// Pointer down message
  pointer_down = WM_POINTERDOWN,
  
  /// Pointer up message
  pointer_up = WM_POINTERUP,
  
  /// Pointer enter message
  pointer_enter = WM_POINTERENTER,
  
  /// Pointer leave message
  pointer_leave = WM_POINTERLEAVE,
  
  /// Pointer activate message
  pointer_activate = WM_POINTERACTIVATE,
  
  /// Pointer capture changed message
  pointer_capture_changed = WM_POINTERCAPTURECHANGED,
  
  /// Touch hit testing message
  touch_hit_testing = WM_TOUCHHITTESTING,
  
  /// Pointer wheel message
  pointer_wheel = WM_POINTERWHEEL,
  
  /// Pointer horizontal wheel message
  pointer_h_wheel = WM_POINTERHWHEEL,
  
  /// Pointer routed to message
  pointer_routed_to = WM_POINTERROUTEDTO,
  
  /// Pointer routed away message
  pointer_routed_away = WM_POINTERROUTEDAWAY,
  
  /// Pointer routed released message
  pointer_routed_released = WM_POINTERROUTEDRELEASED,
  
  // ========== DPI and Display Messages ==========
  
  /// DPI changed message
  dpi_changed = WM_DPICHANGED,
  
  /// DPI changed before parent message
  dpi_changed_before_parent = WM_DPICHANGED_BEFOREPARENT,
  
  /// DPI changed after parent message
  dpi_changed_after_parent = WM_DPICHANGED_AFTERPARENT,
  
  /// Get DPI scaled size message
  get_dpi_scaled_size = WM_GETDPISCALEDSIZE,
  
  // ========== Desktop Window Manager Messages ==========
  
#if WINVER >= 0x0600
  /// DWM composition changed message
  dwm_composition_changed = WM_DWMCOMPOSITIONCHANGED,
  
  /// DWM NC rendering changed message
  dwm_nc_rendering_changed = WM_DWMNCRENDERINGCHANGED,
  
  /// DWM colorization color changed message
  dwm_colorization_color_changed = WM_DWMCOLORIZATIONCOLORCHANGED,
  
  /// DWM window maximized change message
  dwm_window_maximized_change = WM_DWMWINDOWMAXIMIZEDCHANGE,
  
  /// DWM send iconic thumbnail message
  dwm_send_iconic_thumbnail = WM_DWMSENDICONICTHUMBNAIL,
  
  /// DWM send iconic live preview bitmap message
  dwm_send_iconic_live_preview_bitmap = WM_DWMSENDICONICLIVEPREVIEWBITMAP,
#endif
  
  // ========== User Defined Messages ==========
  
  /// First user-defined message
  user = WM_USER,
  
  /// Application-defined message base
  app = WM_APP
};


constexpr UINT ToUInt(win_msg msg) noexcept {
  return static_cast<UINT>(msg);
}

/**
* @brief Convert UINT to win_msg enum
* @param msg The UINT message value
* @return The corresponding win_msg enum value
*/
constexpr win_msg FromUInt(UINT msg) noexcept {
  return static_cast<win_msg>(msg);
}
#endif

namespace iris {
namespace priv {
#ifdef _WIN32
extern void init_dwm_pri_api();
HINSTANCE instance;
struct msg {
  UINT code;
  union {
    struct create_window {
      ui::window* wind;
    } win_create;
  };
};
extern void init_priv_win(priv::window* wind, HWND hwnd);
#endif

class app {
public:
  app(const string& name);
  ~app();

  void set_delegate(app_delegate* in_delegate);
#ifdef _WIN32
  void register_class(const string& name);

  static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  // int process_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

  void loop();

  void pump_message();

  void add_window(iris::ui::window* w);
  void remove_window(iris::ui::window* w);

  void request_exit() { request_exit_ = true; }
#ifdef _WIN32
  void enqueue_msg(const msg& msg) { msg_queue_.push(msg); }
  bool pop_msg(msg& msg) {
    if (!msg_queue_.empty()) {
      msg = msg_queue_.front();
      msg_queue_.pop();
      return true;
    } else {
      return false;
    }
  }
#endif
private:
  friend class iris::app;
  app_delegate* delegate_;
#ifdef _WIN32
  unordered_map<HWND, iris::weak_ptr<iris::ui::window>> windows;
  queue<msg> msg_queue_;
#endif
  string name_;
  bool request_exit_ = false;
};
} // namespace priv

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
app::app(const string& name) : delegate_(nullptr), name_(name) {
#ifdef _WIN32
  init_dwm_pri_api();
  register_class(name);
#endif
} // app::app

app::~app() {
#ifdef _WIN32
  windows.clear();
#endif
}

#ifdef _WIN32
void app::register_class(const string& name) {
  // support HDPI
  ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  WNDCLASS wc;
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_DBLCLKS; // We want to receive double clicks
  wc.lpfnWndProc = window_proc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = instance;
  // wc.hIcon = HIcon;
  wc.hCursor = NULL;       // We manage the cursor ourselves
  wc.hbrBackground = NULL; // Transparent
  wc.lpszMenuName = NULL;
  wc.lpszClassName = name.c_str();

  if (!::RegisterClassA(&wc)) {
    // ShowLastError();

    // return false;
  }
}

LRESULT app::window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  iris::app* app = iris::app::shared();
  auto win_iter = app->d->windows.find(hwnd);
  auto wm = FromUInt(msg);
  if (win_iter != app->d->windows.end()) {
    intptr_t native_result = 0;
    if (win_iter->second->on_native_event(msg, static_cast<uintptr_t>(w_param),
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
    break;
  case win_msg::paint: {
    PAINTSTRUCT ps = {};
    ::BeginPaint(hwnd, &ps);
    if (win_iter != app->d->windows.end()) {
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
    MINMAXINFO* mmi = (MINMAXINFO*)l_param;
    if (win_iter != app->d->windows.end()) {
      mmi->ptMinTrackSize.x = 2 * win_iter->second->shadow_distance() + 100;
      mmi->ptMinTrackSize.y = 2 * win_iter->second->shadow_distance() + 100;
    }
    return 0;
  }
#if !ENABLE_MICA
  // Non-client area drawing
  case win_msg::nc_calc_size: {
	bool is_max = !!::IsZoomed(hwnd);
    if (is_max) {
      WINDOWINFO wnd_info = {0};
      wnd_info.cbSize = sizeof(wnd_info);
      ::GetWindowInfo(hwnd, &wnd_info);
      LPNCCALCSIZE_PARAMS resizing_rect = (LPNCCALCSIZE_PARAMS)l_param;
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
    //return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  }
  // Non-client area hit testing
  case win_msg::nc_hit_test: {
    RECT wnd_rect;
    ::GetWindowRect(hwnd, &wnd_rect);
    const int local_mouse_x = (int)(short)(LOWORD(l_param))-wnd_rect.left;
    const int local_mouse_y = (int)(short)(HIWORD(l_param))-wnd_rect.top;
    //if (CurrentNativeEventWindow->IsRegularWindow()) {
    //  EWindowZone::Type Zone;

    if (win_iter != app->d->windows.end()) {
      return (*win_iter).second->zone_at(local_mouse_x, local_mouse_y);
    }
    //  if (MessageHandler->ShouldProcessUserInputMessages(CurrentNativeEventWindowPtr)) {
    //    // Assumes this is not allowed to leave Slate or touch rendering
    //    Zone = MessageHandler->GetWindowZoneForPoint(CurrentNativeEventWindow, LocalMouseX,
    //                                                 LocalMouseY);
    //  } else {
    //    // Default to client area so that we are able to see the feedback effect when attempting to
    //    // click on a non-modal window when a modal window is active Any other window zones could
    //    // have side effects and NotInWindow prevents the feedback effect.
    //    Zone = EWindowZone::ClientArea;
    //  }

    //  static const LRESULT Results[]
    //      = {HTNOWHERE, HTTOPLEFT,   HTTOP,        HTTOPRIGHT, HTLEFT,
    //         HTCLIENT,  HTRIGHT,     HTBOTTOMLEFT, HTBOTTOM,   HTBOTTOMRIGHT,
    //         HTCAPTION, HTMINBUTTON, HTMAXBUTTON,  HTCLOSE,    HTSYSMENU};

    //  return Results[Zone];
    //}
    return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  }
  case win_msg::nc_activate: {
    return true;
    break;
  }
  case win_msg::nc_destroy:
    return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  case win_msg::nc_paint: // default
    return 0;
#endif // !ENABLE_MICA
  case win_msg::window_pos_changed: {
    WINDOWPOS* wnd_pos = (WINDOWPOS*)l_param;
    if (win_iter != app->d->windows.end()) {
      //wnd_pos->x;
      if ((*win_iter).second->on_window_pos_changed()) {
      }
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
    if (win_iter != app->d->windows.end()) {
      auto nw = (int)(short)(LOWORD(l_param));
      auto nh = (int)(short)(HIWORD(l_param));
      auto window_ref = (*win_iter).second;
      if (w_param != SIZE_MINIMIZED && nw > 0 && nh > 0) {
        window_ref->resize(nw, nh);
        window_ref->on_window_pos_changed();
        // Defer repaint to WM_PAINT queue to avoid synchronous resize-time flicker.
        ::InvalidateRect(hwnd, nullptr, FALSE);
      } else {
        window_ref->on_window_pos_changed();
      }
      return 0;
    }
    break;
  }
  case win_msg::nc_m_button_down: {
    switch (w_param) {
    case HTMINBUTTON: {
      /*if (!MessageHandler->OnWindowAction(CurrentNativeEventWindow,
                                          EWindowAction::ClickedNonClientArea)) {
        return 1;
      }*/
    } break;
    case HTMAXBUTTON: {
      /*if (!MessageHandler->OnWindowAction(CurrentNativeEventWindow,
                                          EWindowAction::ClickedNonClientArea)) {
        return 1;
      }*/
    } break;
    case HTCLOSE: {
      //if (!MessageHandler->OnWindowAction(CurrentNativeEventWindow,
      //                                    EWindowAction::ClickedNonClientArea)) {
      //  return 1;
      //}
    } break;
    case HTCAPTION: {
      /*if (!MessageHandler->OnWindowAction(CurrentNativeEventWindow,
                                          EWindowAction::ClickedNonClientArea)) {
        return 1;
      }*/
    } break;
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
  } break;
  // Resize transition
  case win_msg::sizing: {
    return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  }
  case win_msg::enter_size_move: {
    return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  }
  case win_msg::exit_size_move:
    return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  case win_msg::create:
  {
    LPCREATESTRUCTA cparam = (LPCREATESTRUCTA)l_param;
    auto wind = (priv::window*)cparam->lpCreateParams;
    init_priv_win(wind, hwnd);
    //priv::msg msg;
    //msg.code = WM_CREATE;
    //msg.win_create.wind = wind;
    ////cparam->cx;
    ////cparam->cy;
    ////cparam->x;
    ////cparam->y;
    //app->d->enqueue_msg(msg);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(wind));
    return 0;
  }
  case win_msg::destroy:
    if (win_iter != app->d->windows.end()) {
      (*win_iter).second->finalize();
    }
    //app->d->windows.erase(hwnd);
    return 0;
  case win_msg::close:
    break;
  case win_msg::sys_command: {
    switch (w_param & 0xfff0) {
    case SC_RESTORE:
      // Checks to see if the window is minimized.
      if (IsIconic(hwnd)) {
        // This is required for restoring a minimized fullscreen window
        ::ShowWindow(hwnd, SW_RESTORE);
        return 0;
      } else {
        // if (!MessageHandler->OnWindowAction(CurrentNativeEventWindow, EWindowAction::Restore)) {
        //   return 1;
        // }
      }
      break;
    case SC_MAXIMIZE: {
      // if (!MessageHandler->OnWindowAction(CurrentNativeEventWindow, EWindowAction::Maximize)) {
      //   return 1;
      // }
    } break;
    case SC_CLOSE: {
      // do not allow Alt-f4 during slow tasks.  This causes entry into the shutdown sequence at
      // abonrmal times which causes crashes.
      /*if (!GIsSlowTask) {
        DeferMessage(CurrentNativeEventWindowPtr, hwnd, WM_CLOSE, 0, 0);
      }*/
      app->request_exit();
      return 1;
    } break;
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
  } break;
#if WINVER > 0x502
  case win_msg::dwm_composition_changed: {
    
  } break;
#endif
  case win_msg::erase_bkgnd: {
    // Intercept background erasing to eliminate flicker.
    // Return non-zero to indicate that we'll handle the erasing ourselves
    return 1;
  } break;
  // Window focus and activation
  case win_msg::activate: {
    bool active = false;
    if (LOWORD(w_param) == WA_ACTIVE || LOWORD(w_param) == WA_CLICKACTIVE) {
      active = true;
    } else if (LOWORD(w_param) == WA_INACTIVE) {
      active = false;
    }
    if (win_iter != app->d->windows.end()) {
      (*win_iter).second->on_window_activate(active);
      //return 0;
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  }
  case win_msg::activate_app: {
    return DefWindowProc(hwnd, msg, w_param, l_param);
    break;
  }
  // Window focus and activation
  case win_msg::mouse_activate: {
    // If the mouse activate isn't in the client area we'll force the WM_ACTIVATE to be
    // EWindowActivation::ActivateByMouse This ensures that clicking menu buttons on the header
    // doesn't generate a WM_ACTIVATE with EWindowActivation::Activate which may cause mouse capture
    // to be taken because is not differentiable from Alt-Tabbing back to the application.
    //return 0;
    return DefWindowProc(hwnd, msg, w_param, l_param);
  } break;
  case win_msg::quit:
    app->request_exit();
    break;
  case win_msg::device_change: // Device
    break;
  case win_msg::display_change:
    break;
  case win_msg::dpi_changed: // DPI
    break;
  default:
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
  return 0;
  //DefWindowProc(hwnd, msg, w_param, l_param);
}
#endif

void app::loop() {
#ifdef _WIN32
#if USE_REALTIME_LOOP
  while (!request_exit_) {
    pump_message();
    priv::msg msg;
    while (pop_msg(msg)) {
      switch (msg.code) {
      case WM_CREATE:
        msg.win_create.wind->initialize();
        break;
      }
    }
    ::SwitchToThread();
  }

#else  // USE_REALTIME_LOOP
  MSG Msg;
  BOOL Gm;
  while ((Gm = ::GetMessageW(&Msg, NULL, 0, 0)) != 0 && Gm != -1 && !request_exit_) {
    priv::msg msg;
    while (pop_msg(msg)) {
      switch (msg.code) {
      case WM_CREATE:
        msg.win_create.wind->initialize();
        break;
      }
    }
    ::TranslateMessage(&Msg);
    ::DispatchMessage(&Msg);
  }
#endif // USE_REALTIME_LOOP
#endif
}

void app::pump_message() {
#ifdef _WIN32
  MSG message;

  // standard Windows message handling
  while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }
#endif
}

void app::add_window(iris::ui::window* w) {
#ifdef _WIN32
  windows[(HWND)w->native_handle()] = iris::weak_ptr<iris::ui::window>(w);
#endif
}

void app::remove_window(iris::ui::window* w) {
#ifdef _WIN32
  windows.erase((HWND)(w->native_handle()));
#endif
}

void app::set_delegate(app_delegate* in_delegate) {}

} // namespace priv
} // namespace iris
