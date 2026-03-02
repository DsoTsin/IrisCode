#include "button.hpp"

#include "../window.hpp"
#include "include/effects/SkRuntimeEffect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkBlurTypes.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkPaint.h"
#ifdef _WIN32
#include <windows.h>
#include "include/ports/SkTypeface_win.h"
#endif

namespace iris {
namespace ui {

namespace {
static uint8_t adjust_channel(uint8_t value, float factor) {
  const float clamped_factor = max(-1.0f, min(1.0f, factor));
  if (clamped_factor >= 0.0f) {
    return static_cast<uint8_t>(value + (255.0f - value) * clamped_factor);
  }
  return static_cast<uint8_t>(value * (1.0f + clamped_factor));
}

static SkColor adjust_color_brightness(SkColor color, float factor) {
  return SkColorSetARGB(
      SkColorGetA(color),
      adjust_channel(SkColorGetR(color), factor),
      adjust_channel(SkColorGetG(color), factor),
      adjust_channel(SkColorGetB(color), factor));
}
} // namespace

static sk_sp<SkRuntimeEffect> s_button_fx;
sk_sp<SkRuntimeEffect> get_button_fx() {
  if (!s_button_fx) {
  static const char* s_button_shader = R"(
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

    auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(s_button_shader));
    if (!effect) {
      SkDebugf("Failed to create fx: %s", error.c_str());
      return s_button_fx;
    }
    s_button_fx = effect;
  }
  return s_button_fx;
}

button::button(view* parent) : view(parent), style_(button_style::capsule) {}

button::~button() {}

void button::request_redraw() {
  auto* ctx = gfx_ctx_.get();
  if (!ctx) {
    return;
  }
  auto* wnd = ctx->window();
  if (!wnd) {
    return;
  }
  wnd->draw();
}

static void draw_button(SkCanvas* canvas, const SkRect& rect, float radius, const SkColor& color) {
  if (!canvas) {
    return;
  }
  struct Param {
    SkRect rect;
    SkColor4f color; // .a is radius
  };
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

  auto fx = get_button_fx();
  if (!fx) {
    SkPaint fallback_paint;
    fallback_paint.setColor(color);
    fallback_paint.setAntiAlias(true);
    canvas->drawRoundRect(rect, radius, radius, fallback_paint);
    return;
  }

  SkPaint buttonPaint;
  buttonPaint.setShader(fx->makeShader(uniforms, nullptr, 0, &SkMatrix::I()));
  buttonPaint.setAntiAlias(true);
  canvas->drawRect(rect, /* radius, radius,*/ buttonPaint);
}

void button::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  if (!gfx_ctx_) {
    return;
  }
  auto canvas = gfx_ctx_->canvas();
  if (!canvas) {
    return;
  }
  // Button drawing implementation
  auto rect = get_layout_rect();
  SkRect sk_rect{rect.left, rect.top, rect.left+ rect.width, rect.top+rect.height};
  SkColor button_color = SkColorSetRGB(0x00, 0x88, 0xE9);
  if (!enabled_) {
    button_color = adjust_color_brightness(button_color, -0.35f);
  } else if (pressed_) {
    button_color = adjust_color_brightness(button_color, -0.10f);
  } else if (hovered_) {
    button_color = adjust_color_brightness(button_color, 0.18f);
  }
  switch (style_) {
  case button_style::capsule:
    draw_button(canvas, sk_rect, 10.f, button_color);
    break;
  case button_style::flat:
    break;
  case button_style::gradient:
    break;
  }

  if (!text_.empty()) {
    SkFont font;
    font.setSize(14.0f);
#ifdef _WIN32
    auto fm = SkFontMgr_New_GDI();
    if (fm) {
      auto typeface = fm->legacyMakeTypeface("Microsoft YaHei UI", SkFontStyle::Normal());
      if (!typeface) {
        typeface = fm->legacyMakeTypeface("Microsoft YaHei", SkFontStyle::Normal());
      }
      if (!typeface) {
        typeface = fm->legacyMakeTypeface("Segoe UI", SkFontStyle::Normal());
      }
      if (!typeface) {
        typeface = fm->legacyMakeTypeface(nullptr, SkFontStyle::Normal());
      }
      if (typeface) {
        font.setTypeface(typeface);
      }
    }
#endif
    font.setSubpixel(true);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    const float baseline = rect.top + (rect.height - (metrics.fDescent - metrics.fAscent)) * 0.5f
                           - metrics.fAscent;
    SkPaint text_paint;
    text_paint.setColor(SK_ColorWHITE);
    text_paint.setAntiAlias(true);
    canvas->drawSimpleText(text_.c_str(), text_.size(), SkTextEncoding::kUTF8,
                           rect.left + 12.0f, baseline, font, text_paint);
  }
}

bool button::mouse_down(const mouse_event& event) {
  if (!enabled_ || event.button != mouse_button::left) {
    return false;
  }
  pressed_ = true;
  request_redraw();
  return true;
}

bool button::mouse_up(const mouse_event& event) {
  if (!enabled_ || event.button != mouse_button::left) {
    return false;
  }
  const bool was_pressed = pressed_;
  pressed_ = false;
  if (was_pressed) {
    request_redraw();
  }
  if (was_pressed && on_click_) {
    on_click_();
  }
  return was_pressed;
}

bool button::mouse_enter(const mouse_event&) {
  if (!enabled_) {
    return false;
  }
  if (!hovered_) {
    hovered_ = true;
    request_redraw();
  }
  return false;
}

bool button::mouse_leave(const mouse_event&) {
  const bool was_pressed = pressed_;
  const bool was_hovered = hovered_;
  pressed_ = false;
  hovered_ = false;
  if (was_pressed || was_hovered) {
    request_redraw();
  }
  return false;
}

button& button::text(const string& title) {
  text_ = title;
  return *this;
}

string button::text() const {
  return text_;
}

button& button::enabled(bool enabled) {
  if (enabled_ == enabled) {
    return *this;
  }
  enabled_ = enabled;
  if (!enabled_) {
    hovered_ = false;
    pressed_ = false;
  }
  request_redraw();
  return *this;
}

bool button::enabled() const {
  return enabled_;
}

button& button::on_click(std::function<void()> callback) {
  on_click_ = callback;
  return *this;
}

title_button::title_button(view* parent) : view(parent) {}
title_button::~title_button() {}

static SkColor col_close_btn = SkColorSetARGB(0xFF, 0xFF, 0x63, 0x5C);
static SkColor col_minimize_btn = SkColorSetARGB(0xFF, 0xFE, 0xBB, 0x42);
static SkColor col_maximize_btn = SkColorSetARGB(0xFF, 0x2C, 0xC2, 0x46);
static SkColor col_disable_btn = SkColorSetARGB(0xFF, 0x5E, 0x5C, 0x5C);

namespace {
constexpr float kTitleCircleRadius = 7.0f;
constexpr float kTitleCircleSpacing = 12.0f;
constexpr float kTitleCircleMarginX = 12.0f;
constexpr float kTitleCircleMarginY = 12.0f;

static SkColor circle_draw_color(SkColor base, bool hovered, bool pressed) {
  if (pressed) {
    return adjust_color_brightness(base, -0.15f);
  }
  if (hovered) {
    return adjust_color_brightness(base, 0.18f);
  }
  return base;
}
} // namespace

void title_button::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  if (!gfx_ctx_) {
    return;
  }
  auto canvas = gfx_ctx_->canvas();
  if (!canvas) {
    return;
  }
  auto rect = get_layout_rect();
  const float center_y = rect.top + kTitleCircleMarginY + kTitleCircleRadius;
  const float first_center_x = rect.left + kTitleCircleMarginX + kTitleCircleRadius;
  const float step = kTitleCircleRadius * 2.0f + kTitleCircleSpacing;
  const SkColor colors[3] = {col_close_btn, col_minimize_btn, col_maximize_btn};

  SkPaint btn_paint;
  btn_paint.setAntiAlias(true);
  btn_paint.setStyle(SkPaint::kFill_Style);
  for (int i = 0; i < 3; ++i) {
    const float cx = first_center_x + i * step;
    const bool hovered = (hovered_circle_ == i);
    const bool pressed = (pressed_circle_ == i);
    btn_paint.setColor(circle_draw_color(colors[i], hovered, pressed));
    canvas->drawCircle(cx, center_y, kTitleCircleRadius, btn_paint);
  }
}

int title_button::hit_circle_index(const point& local_point) const {
  const auto rect = get_layout_rect();
  const float local_x = static_cast<float>(local_point.x);
  const float local_y = static_cast<float>(local_point.y);
  const float center_y = kTitleCircleMarginY; // codex: must not change
  const float first_center_x = kTitleCircleMarginX; // codex: must not change
  const float step = kTitleCircleRadius * 2.0f + kTitleCircleSpacing;

  auto hit_index = [&](float x, float y) -> int {
    const float hit_radius = kTitleCircleRadius + 1.5f;
    for (int i = 0; i < 3; ++i) {
      const float cx = first_center_x + i * step;
      const float dx = x - cx;
      const float dy = y - center_y;
      if (dx * dx + dy * dy <= hit_radius * hit_radius) {
        return i;
      }
    }
    return -1;
  };

  // Primary path: local coordinates from event routing.
  int hit = hit_index(local_x, local_y);
  if (hit >= 0) {
    return hit;
  }
  // Fallback: tolerate absolute coordinates from callers like non-client hit-testing.
  return hit_index(local_x - rect.left, local_y - rect.top);
}

bool title_button::is_control_button_hit(const point& local_point) const {
  return hit_circle_index(local_point) >= 0;
}

void title_button::perform_action(int circle_index) {
  if (circle_index < 0 || circle_index > 2) {
    return;
  }
  auto* ctx = gfx_ctx_.get();
  if (!ctx) {
    return;
  }
  auto* wnd = ctx->window();
  if (!wnd) {
    return;
  }
#ifdef _WIN32
  auto* hwnd = reinterpret_cast<HWND>(wnd->native_handle());
  if (!hwnd) {
    return;
  }
  switch (circle_index) {
  case 0:
    ::PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    break;
  case 1:
    ::ShowWindow(hwnd, SW_MINIMIZE);
    break;
  case 2:
    ::ShowWindow(hwnd, ::IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
    break;
  default:
    break;
  }
#endif
}

bool title_button::mouse_down(const mouse_event& event) {
  if (event.button != mouse_button::left) {
    return false;
  }
  const int hit = hit_circle_index(event.location_in_view);
  if (hit < 0) {
    return false;
  }
  pressed_circle_ = hit;
  hovered_circle_ = hit;
  auto* ctx = gfx_ctx_.get();
  if (ctx && ctx->window()) {
    ctx->window()->draw();
  }
  return true;
}

bool title_button::mouse_up(const mouse_event& event) {
  if (event.button != mouse_button::left) {
    return false;
  }
  const int hit = hit_circle_index(event.location_in_view);
  const int pressed = pressed_circle_;
  pressed_circle_ = -1;
  hovered_circle_ = hit;
  auto* ctx = gfx_ctx_.get();
  if (ctx && ctx->window()) {
    ctx->window()->draw();
  }
  if (pressed >= 0 && pressed == hit) {
    perform_action(hit);
    return true;
  }
  return pressed >= 0;
}

bool title_button::mouse_enter(const mouse_event& event) {
  const int hit = hit_circle_index(event.location_in_view);
  if (hovered_circle_ == hit) {
    return false;
  }
  hovered_circle_ = hit;
  auto* ctx = gfx_ctx_.get();
  if (ctx && ctx->window()) {
    ctx->window()->draw();
  }
  return hit >= 0;
}

bool title_button::mouse_move(const mouse_event& event) {
  const int hit = hit_circle_index(event.location_in_view);
  if (hovered_circle_ == hit) {
    return false;
  }
  hovered_circle_ = hit;
  auto* ctx = gfx_ctx_.get();
  if (ctx && ctx->window()) {
    ctx->window()->draw();
  }
  return hit >= 0;
}

bool title_button::mouse_leave(const mouse_event&) {
  const bool changed = hovered_circle_ >= 0 || pressed_circle_ >= 0;
  hovered_circle_ = -1;
  pressed_circle_ = -1;
  if (changed) {
    auto* ctx = gfx_ctx_.get();
    if (ctx && ctx->window()) {
      ctx->window()->draw();
    }
  }
  return false;
}

vsplitter::vsplitter(view* parent): view(parent) {}
vsplitter::~vsplitter() {}
void vsplitter::draw(const rect& dirty_rect) {
  if (!gfx_ctx_) {
    return;
  }
  auto canvas = gfx_ctx_->canvas();
  if (!canvas) {
    return;
  }
  view::draw(dirty_rect);
  auto rect = get_layout_rect();
  SkRect sk_rect{rect.left, rect.top - 10, rect.left + rect.width, rect.top + rect.height + 10};

  SkPaint btn_paint;
  btn_paint.setAntiAlias(true);
  btn_paint.setColor(SkColorSetARGB(0xFF, 0x00, 0x00, 0x00));

  canvas->drawRect(sk_rect, btn_paint);
}
} // namespace ui
} // namespace iris
