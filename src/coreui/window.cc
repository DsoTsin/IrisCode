#include "window.hpp"
#include "app.hpp"
#include "view.hpp"
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
  void draw();
  void set_content_view(ui::view* inview);
  void resize(int w, int h);
  int zone_at(int x, int y) const;
  bool on_window_pos_changed();
  bool on_window_activate(bool active);

  void draw_title();
  void draw_border();

  void set_border_width(float width) { border_width_ = width; }
  float border_width() const { return border_width_; }
  void set_shadow_distance(float dist) { shadow_distance_ = dist; }
  float shadow_distance() const { return shadow_distance_; }
  void set_corner_radius(float dist) { corner_radius_ = dist; }
  float corner_radius() const { return corner_radius_; }

  const ui::rect& frame() const { return frame_; }
  ui::rect content_bounds() const;

  iris::ui::window* wind() { return w_; }

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
  frame_.width = w;
  frame_.height = h;
  if (ctx_) {
    ctx_->resize(w, h);
  }
  if (content_view_) {
    //content_view_->mark_layout_dirty();
    content_view_->layout_if_needed();
  }
}

int window::zone_at(int x, int y) const {
#if _WIN32
  //  static const LRESULT Results[]
  //      = {HTNOWHERE, HTTOPLEFT,   HTTOP,        HTTOPRIGHT, HTLEFT,
  //         HTCLIENT,  HTRIGHT,     HTBOTTOMLEFT, HTBOTTOM,   HTBOTTOMRIGHT,
  //         HTCAPTION, HTMINBUTTON, HTMAXBUTTON,  HTCLOSE,    HTSYSMENU};
  if (x >= 10 && x <= 34 && y >= 10 && y <= 34)
    return HTTOPLEFT;
  if (x >= 34 && y >= 10 && y <= 34)
    return HTCAPTION;
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

static SkColor col_close_btn = SkColorSetARGB(0xFF, 0xFF, 0x63, 0x5C);
static SkColor col_minimize_btn = SkColorSetARGB(0xFF, 0xFE, 0xBB, 0x42);
static SkColor col_maximize_btn = SkColorSetARGB(0xFF, 0x2C, 0xC2, 0x46);
static SkColor col_disable_btn = SkColorSetARGB(0xFF, 0x5E, 0x5C, 0x5C);

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
  // ���ݰ�ť״̬��������
  float lightOffset = pressed ? -2.0f : 2.0f;
  float shadowBlur = pressed ? 1.0f : 3.0f;

  // ������Ӱ
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, shadowBlur));

  SkRect shadowRect = bounds.makeOffset(0, 3);
  canvas->drawRoundRect(shadowRect, 20, 20, shadowPaint);

  // ���ư�ť����
  SkPoint gradientPoints[2]
      = {SkPoint::Make(bounds.left(), bounds.top()), SkPoint::Make(bounds.left(), bounds.bottom())};

  SkColor gradientColors[2] = {
      SkColorSetRGB(0x1e,0x75,0xe4),      // �����߹�
      SkColorSetRGB(0x0d,0x62,0xcb)            // �ײ���Ӱ
  };

  auto buttonShader = SkGradientShader::MakeLinear(gradientPoints, gradientColors, nullptr, 2,
                                                   SkTileMode::kClamp);

  SkPaint buttonPaint;
  buttonPaint.setShader(buttonShader);
  buttonPaint.setAntiAlias(true);

  canvas->drawRoundRect(bounds, 20, 20, buttonPaint);

  // ���Ӷ����߹���
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
  canvas->clear(SK_ColorTRANSPARENT);
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
  /*draw_window_background(canvas, native_window_rect, corner_radius(), shadow_distance(),
                         SK_ColorBLACK);*/

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
  /*SkVector radii[4] = {{corner_radius(), corner_radius()},
                       {corner_radius(), corner_radius()},
                       {corner_radius(), corner_radius()},
                       {corner_radius(), corner_radius()}};

  SkRRect clipRRect;
  clipRRect.setRectRadii(windowRect, radii);*/

  canvas->clipRRect(clipRRect, true);
  
  SkPaint btn_paint;
  btn_paint.setAntiAlias(true);
  // Filled circle
  btn_paint.setColor(col_close_btn);
  btn_paint.setStyle(SkPaint::kFill_Style);
  canvas->drawCircle(50, 50, 10, btn_paint);
  btn_paint.setColor(col_minimize_btn);
  canvas->drawCircle(80, 50, 10, btn_paint);
  btn_paint.setColor(col_maximize_btn);
  canvas->drawCircle(110, 50, 10, btn_paint);

  // Define gradient parameters
  SkColor colors[] = {SkColorSetRGB(0xFF, 0xFF, 0xFF), SkColorSetRGB(0xFF, 0xFF, 0xFF),
                      SkColorSetRGB(0x1E, 0x74, 0xE3),
                      SkColorSetRGB(0x0C, 0x68, 0xD9), SkColorSetRGB(0x0B, 0x61, 0xCA)};
  SkPoint points[] = {SkPoint::Make(0, 0), 
                      SkPoint::Make(40, 40)}; // Diagonal gradient
  SkScalar positions[] = {0.0, 0.025, 0.05, 0.55, 1.0};
  
  // Create linear gradient shader
  sk_sp<SkShader> shader = SkGradientShader::MakeLinear(points, colors,
                                                        positions, // Evenly spaced
                                                        5, SkTileMode::kClamp, 0, nullptr);

  // Set up paint with the gradient
  SkPaint paint;
  paint.setShader(shader);
  paint.setAntiAlias(true);

  // Draw a rectangle with the gradient
  //canvas->drawRoundRect(SkRect::MakeXYWH(50, 100, 100, 40), cornerRadius, cornerRadius, paint);
  draw_button(canvas, SkRect::MakeXYWH(50, 100, 166, 30), 15, SkColorSetRGB(0x0, 0x88, 0xE9));
  draw3DButtonWithLighting(canvas, SkRect::MakeXYWH(50, 300, 200, 80), false);
  canvas->restore();
}

ui::rect window::content_bounds() const {
  return ui::rect{shadow_distance_, shadow_distance_, frame_.width - 2.f * shadow_distance_,
                  frame_.height - 2.f * shadow_distance_};
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
void window::draw() {
  if (content_view_) {
    ui::rect rect;
#ifdef _WIN32
    RECT rcwnd;
    ::GetWindowRect(hwnd_, &rcwnd);
    rect = ui::rect{(float)rcwnd.left, (float)rcwnd.top, (float)rcwnd.right - rcwnd.left,
                    (float)rcwnd.bottom - rcwnd.top};
#endif
    draw_border();
    draw_title();
    // layout calculation
    content_view_->layout_if_needed();
    if (ctx_) {
      auto canvas = ctx_->canvas();
      canvas->save();
      canvas->translate(shadow_distance_, shadow_distance_);
      // todo: drawIfNeeded
      content_view_->draw(rect);
      canvas->restore();
    }
    if (ctx_) {
      ctx_->flush();
      ctx_->synchronize();
    }
  }
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
  return true;
}
void window::set_content_view(ui::view* view) {
  if (view) {
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