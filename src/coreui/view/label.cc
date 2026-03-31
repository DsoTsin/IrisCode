#include "label.hpp"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontTypes.h"
#include "include/ports/SkTypeface_win.h"

namespace iris {
namespace ui {

label::label(view* parent) : view(parent), font_(new SkFont) {
  enable_measure();
}

label::~label() {
  if (font_) {
    delete font_;
    font_ = nullptr;
  }
}

void label::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  if (!gfx_ctx_) {
    return;
  }
  auto* canvas = gfx_ctx_->canvas();
  if (!canvas) {
    return;
  }

  const auto bounds = get_layout_rect();
  SkFontMetrics metrics;
  font_->getMetrics(&metrics);
  const SkScalar baseline = bounds.top - metrics.fAscent;

  SkPaint paint;
  paint.setColor(SkColorSetARGB(text_color_.a, text_color_.r, text_color_.g, text_color_.b));
  paint.setAntiAlias(true);
  canvas->drawSimpleText(text_.c_str(), text_.size(), SkTextEncoding::kUTF8, bounds.left, baseline,
                         *font_, paint);
}

label& label::text(const string& text) {
  text_ = text;
  return *this;
}

string label::text() const {
  return text_;
}

label& label::font_size(float size) {
  font_size_ = size;
  return *this;
}

float label::font_size() const {
  return font_size_;
}

label& label::alignment(text_alignment alignment) {
  alignment_ = alignment;
  return *this;
}

text_alignment label::alignment() const {
  return alignment_;
}

label& label::text_color(const color& color) {
  text_color_ = color;
  return *this;
}

color label::text_color() const {
  return text_color_;
}

size label::on_measure(MeasureMode wm, MeasureMode hm, const size& sz) {
  (void)wm;
  (void)hm;
  (void)sz;

  auto fm = SkFontMgr_New_GDI();
  auto typeface = fm ? fm->legacyMakeTypeface("Segoe UI", SkFontStyle::Normal()) : nullptr;
  font_->setSize(font_size_);
  if (typeface) {
    font_->setTypeface(typeface);
  }

  SkRect bounds;
  font_->measureText(text_.c_str(), text_.size(), SkTextEncoding::kUTF8, &bounds, nullptr);
  return {bounds.width(), bounds.height()};
}

} // namespace ui
} // namespace iris
