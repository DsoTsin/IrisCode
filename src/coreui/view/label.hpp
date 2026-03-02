#pragma once

#include "../view.hpp"


namespace iris {
namespace ui {

enum class text_alignment {
  left,
  center,
  right
};

class UICORE_API label : public view {
public:
  label(view* parent = nullptr);
  virtual ~label();

  virtual void draw(const rect& dirty_rect) override;

  label& text(const string& text);
  string text() const;

  label& font_size(float size);
  float font_size() const;

  label& alignment(text_alignment alignment);
  text_alignment alignment() const;

  label& text_color(const color& col);
  color text_color() const;

protected:
  virtual size on_measure(MeasureMode wm, MeasureMode hm, const size& sz) override;

  string text_;
  float font_size_ = 14.0f;
  text_alignment alignment_ = text_alignment::left;
  color text_color_/* = 0xFF000000*/; // Black
  font* font_;
};

} // namespace ui
} // namespace iris