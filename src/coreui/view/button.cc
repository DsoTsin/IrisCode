#include "button.hpp"

#include "include/effects/SkRuntimeEffect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkBlurTypes.h"

namespace iris {
namespace ui {

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

static void draw_button(SkCanvas* canvas, const SkRect& rect, float radius, const SkColor& color) {
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
  SkPaint buttonPaint;
  buttonPaint.setShader(get_button_fx()->makeShader(uniforms, nullptr, 0, &SkMatrix::I()));
  buttonPaint.setAntiAlias(true);
  canvas->drawRect(rect, /* radius, radius,*/ buttonPaint);
}

void button::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  // Button drawing implementation
  auto rect = get_layout_rect();
  SkRect sk_rect{rect.left, rect.top, rect.left+ rect.width, rect.top+rect.height};
  switch (style_) {
  case button_style::capsule:
    draw_button(gfx_ctx_->canvas(), sk_rect, 10.f, SkColorSetRGB(0x0, 0x88, 0xE9));
    break;
  case button_style::flat:
    break;
  case button_style::gradient:
    break;
  }
}

button& button::text(const string& title) {
  text_ = title;
  return *this;
}

string button::text() const {
  return text_;
}

button& button::enabled(bool enabled) {
  enabled_ = enabled;
  return *this;
}

bool button::enabled() const {
  return enabled_;
}

button& button::on_click(std::function<void()> callback) {
  on_click_ = callback;
  return *this;
}

} // namespace ui
} // namespace iris
