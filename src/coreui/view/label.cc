#include "label.hpp"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkFontMgr.h"
#include "include/ports/SkTypeface_win.h"
#include "include/core/SkCanvas.h"

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
  // Label text drawing implementation
  auto rect = get_layout_rect();

  
  SkFontMetrics metrics;
  font_->getMetrics(&metrics);

  // 计算基线位置（考虑 ascent 和 descent）
  SkScalar textHeight = metrics.fDescent - metrics.fAscent;
  SkScalar baseline = rect.top - /*(*/metrics.fAscent/* + metrics.fDescent) / 2*/;

  // 计算起始 x 坐标（水平居中）
  //SkScalar startX = centerX - textWidth / 2;

  SkPaint paint;
  paint.setColor(SK_ColorWHITE);
  paint.setAntiAlias(true);
  gfx_ctx_->canvas()->drawSimpleText(text_.c_str(), text_.size(), SkTextEncoding::kUTF8, rect.left,
                                     baseline, *font_, paint);

  
  //  SkRect bounds;
  //font.measureText(text, strlen(text), SkTextEncoding::kUTF8, &bounds);
  //bounds.offset(x, y);
  //SkPaint linePaint;
  //linePaint.setColor(SK_ColorBLUE);
  //linePaint.setStyle(SkPaint::kStroke_Style);
  //gfx_ctx_->canvas()->drawRect(SkRect{rect.left, rect.top, rect.left + rect.width,
  //                                    rect.top+rect.height},
  //                             linePaint);
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
  auto fm = SkFontMgr_New_GDI();
  auto nf = fm->countFamilies();
  auto tyf = fm->legacyMakeTypeface("Segoe UI", SkFontStyle::Normal());
  font_->setSize(font_size_);
  font_->setTypeface(tyf);

  SkFontMetrics metrics;
  font_->getMetrics(&metrics);
  SkScalar ascent = metrics.fAscent;
  SkScalar descent = metrics.fDescent;
  SkScalar leading = metrics.fLeading;
  SkScalar xHeight = metrics.fXHeight;
  SkScalar lineHeight = descent - ascent + leading;

  SkRect bounds;
  SkScalar width
      = font_->measureText(text_.c_str(), text_.size(), SkTextEncoding::kUTF8, &bounds, nullptr);

  SkDebugf("Text width: %f, Bounds: [%f, %f, %f, %f]\n", width, bounds.left(), bounds.top(),
           bounds.right(), bounds.bottom());
  return {bounds.width(), bounds.height()};
  }

} // namespace ui
} // namespace iris
