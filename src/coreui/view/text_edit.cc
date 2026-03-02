#include "text_edit.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkPaint.h"

namespace iris {
namespace ui {

namespace {
static bool has_selection(size_t start, size_t end) {
  return start != end;
}

static size_t clamp_index(size_t value, size_t upper) {
  return value > upper ? upper : value;
}

constexpr uint32_t kKeyBackspace = 0x08;
constexpr uint32_t kKeyTab = 0x09;
constexpr uint32_t kKeyEnter = 0x0D;
constexpr uint32_t kKeyHome = 0x24;
constexpr uint32_t kKeyLeft = 0x25;
constexpr uint32_t kKeyRight = 0x27;
constexpr uint32_t kKeyEnd = 0x23;
constexpr uint32_t kKeyDelete = 0x2E;
constexpr uint32_t kKeyA = 0x41;
} // namespace

text_edit::text_edit(view* parent) : view(parent) {
  enable_measure();
}

text_edit::~text_edit() {}

void text_edit::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  if (!gfx_ctx_) {
    return;
  }

  auto bounds = get_layout_rect();
  auto canvas = gfx_ctx_->canvas();
  if (!canvas) {
    return;
  }

  const float horizontal_padding = 8.0f;
  const float vertical_padding = 6.0f;

  SkPaint bg_paint;
  bg_paint.setColor(SkColorSetARGB(0x30, 0xFF, 0xFF, 0xFF));
  bg_paint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeXYWH(bounds.left, bounds.top, bounds.width, bounds.height), bg_paint);

  SkPaint border_paint;
  border_paint.setColor(SkColorSetARGB(0x80, 0x70, 0x70, 0x70));
  border_paint.setStyle(SkPaint::kStroke_Style);
  border_paint.setStrokeWidth(1.0f);
  border_paint.setAntiAlias(true);
  canvas->drawRect(SkRect::MakeXYWH(bounds.left, bounds.top, bounds.width, bounds.height), border_paint);

  SkFont font;
  font.setSize(font_size_);

  SkFontMetrics metrics;
  font.getMetrics(&metrics);
  const float line_height = metrics.fDescent - metrics.fAscent + metrics.fLeading;
  float baseline = bounds.top + vertical_padding - metrics.fAscent;

  auto rendered_lines = lines();
  if (rendered_lines.empty()) {
    rendered_lines.push_back("");
  }

  SkPaint text_paint;
  text_paint.setColor(SkColorSetARGB(0xFF, 0xE8, 0xE8, 0xE8));
  text_paint.setAntiAlias(true);

  if (text_.empty() && !placeholder_.empty()) {
    SkPaint placeholder_paint = text_paint;
    placeholder_paint.setColor(SkColorSetARGB(0xFF, 0x90, 0x90, 0x90));
    canvas->drawSimpleText(placeholder_.c_str(), placeholder_.size(), SkTextEncoding::kUTF8,
                           bounds.left + horizontal_padding, baseline, font, placeholder_paint);
  } else {
    for (const auto& line : rendered_lines) {
      canvas->drawSimpleText(line.c_str(), line.size(), SkTextEncoding::kUTF8,
                             bounds.left + horizontal_padding, baseline, font, text_paint);
      baseline += line_height;
      if (baseline > bounds.bottom()) {
        break;
      }
    }
  }

  if (editable_) {
    const auto [line_index, column] = cursor_line_column();
    if (line_index < rendered_lines.size()) {
      const auto& line = rendered_lines[line_index];
      const auto prefix = line.substr(0, min(column, line.size()));
      const float cursor_x = bounds.left + horizontal_padding
                             + font.measureText(prefix.c_str(), prefix.size(), SkTextEncoding::kUTF8);
      const float cursor_top = bounds.top + vertical_padding + line_index * line_height;
      const float cursor_bottom = cursor_top + line_height;

      SkPaint cursor_paint;
      cursor_paint.setColor(SkColorSetARGB(0xFF, 0x90, 0xC6, 0xFF));
      cursor_paint.setStrokeWidth(1.25f);
      cursor_paint.setAntiAlias(true);
      canvas->drawLine(cursor_x, cursor_top, cursor_x, cursor_bottom, cursor_paint);
    }
  }
}

bool text_edit::mouse_down(const mouse_event& event) {
  if (event.button != mouse_button::left) {
    return false;
  }
  move_cursor(text_.size());
  return true;
}

bool text_edit::key_down(const key_event& event) {
  if (!editable_) {
    return false;
  }

  if (has_modifier(event.modifiers, input_modifier::control) && event.key_code == kKeyA) {
    select_all();
    return true;
  }

  switch (event.key_code) {
  case kKeyBackspace:
    backspace();
    return true;
  case kKeyDelete:
    delete_forward();
    return true;
  case kKeyLeft:
    if (cursor_pos_ > 0) {
      move_cursor(cursor_pos_ - 1);
    }
    return true;
  case kKeyRight:
    if (cursor_pos_ < text_.size()) {
      move_cursor(cursor_pos_ + 1);
    }
    return true;
  case kKeyHome:
    move_cursor(0);
    return true;
  case kKeyEnd:
    move_cursor(text_.size());
    return true;
  case kKeyEnter:
    insert_text("\n");
    return true;
  case kKeyTab:
    insert_text("\t");
    return true;
  default:
    return false;
  }
}

bool text_edit::key_char(const key_event& event) {
  if (!editable_) {
    return false;
  }
  if (event.character < 0x20 || event.character > 0x7E) {
    return false;
  }
  char ascii[2] = {static_cast<char>(event.character), '\0'};
  insert_text(ascii);
  return true;
}

void text_edit::set_text(const string& text) {
  if (text_ != text) {
    text_ = text;
    cursor_pos_ = text_.size();
    selection_start_ = cursor_pos_;
    selection_end_ = cursor_pos_;
    if (on_text_changed_) {
      on_text_changed_(text_);
    }
  }
}

string text_edit::text() const {
  return text_;
}

void text_edit::set_placeholder(const string& placeholder) {
  placeholder_ = placeholder;
}

string text_edit::placeholder() const {
  return placeholder_;
}

void text_edit::set_font_size(float size) {
  font_size_ = size > 6.0f ? size : 6.0f;
}

float text_edit::font_size() const {
  return font_size_;
}

void text_edit::set_editable(bool editable) {
  editable_ = editable;
}

bool text_edit::editable() const {
  return editable_;
}

void text_edit::set_word_wrap(bool wrap) {
  word_wrap_ = wrap;
}

bool text_edit::word_wrap() const {
  return word_wrap_;
}

void text_edit::set_on_text_changed(std::function<void(const string&)> callback) {
  on_text_changed_ = callback;
}

void text_edit::insert_text(const string& text) {
  if (!editable_ || text.empty()) {
    return;
  }
  if (has_selection(selection_start_, selection_end_)) {
    delete_selection();
  }
  text_.insert(cursor_pos_, text);
  cursor_pos_ += text.size();
  selection_start_ = cursor_pos_;
  selection_end_ = cursor_pos_;
  if (on_text_changed_) {
    on_text_changed_(text_);
  }
}

void text_edit::delete_selection() {
  if (!editable_ || !has_selection(selection_start_, selection_end_)) {
    return;
  }
  const auto start = min(selection_start_, selection_end_);
  const auto end = max(selection_start_, selection_end_);
  text_.erase(start, end - start);
  cursor_pos_ = start;
  selection_start_ = start;
  selection_end_ = start;
  if (on_text_changed_) {
    on_text_changed_(text_);
  }
}

void text_edit::select_all() {
  selection_start_ = 0;
  selection_end_ = text_.size();
  cursor_pos_ = selection_end_;
}

void text_edit::move_cursor(size_t pos) {
  cursor_pos_ = clamp_index(pos, text_.size());
  selection_start_ = cursor_pos_;
  selection_end_ = cursor_pos_;
}

size_t text_edit::cursor_pos() const {
  return cursor_pos_;
}

void text_edit::backspace() {
  if (!editable_) {
    return;
  }
  if (has_selection(selection_start_, selection_end_)) {
    delete_selection();
    return;
  }
  if (cursor_pos_ == 0 || text_.empty()) {
    return;
  }
  text_.erase(cursor_pos_ - 1, 1);
  --cursor_pos_;
  selection_start_ = cursor_pos_;
  selection_end_ = cursor_pos_;
  if (on_text_changed_) {
    on_text_changed_(text_);
  }
}

void text_edit::delete_forward() {
  if (!editable_) {
    return;
  }
  if (has_selection(selection_start_, selection_end_)) {
    delete_selection();
    return;
  }
  if (cursor_pos_ >= text_.size()) {
    return;
  }
  text_.erase(cursor_pos_, 1);
  selection_start_ = cursor_pos_;
  selection_end_ = cursor_pos_;
  if (on_text_changed_) {
    on_text_changed_(text_);
  }
}

size text_edit::on_measure(MeasureMode, MeasureMode, const size& sz) {
  if (!word_wrap_) {
    return sz;
  }

  auto rendered_lines = lines();
  if (rendered_lines.empty()) {
    rendered_lines.push_back("");
  }

  SkFont font;
  font.setSize(font_size_);
  SkFontMetrics metrics;
  font.getMetrics(&metrics);
  const float line_height = metrics.fDescent - metrics.fAscent + metrics.fLeading;

  float measured_width = 0.0f;
  for (const auto& line : rendered_lines) {
    measured_width = max(
        measured_width,
        font.measureText(line.c_str(), line.size(), SkTextEncoding::kUTF8));
  }

  return {
      measured_width + 16.0f,
      line_height * rendered_lines.size() + 12.0f,
  };
}

vector<string> text_edit::lines() const {
  vector<string> output;
  if (text_.empty()) {
    return output;
  }
  size_t start = 0;
  while (start <= text_.size()) {
    const auto end = text_.find('\n', start);
    if (end == string::npos) {
      output.push_back(text_.substr(start));
      break;
    }
    output.push_back(text_.substr(start, end - start));
    start = end + 1;
    if (start == text_.size()) {
      output.push_back("");
      break;
    }
  }
  return output;
}

pair<size_t, size_t> text_edit::cursor_line_column() const {
  size_t line = 0;
  size_t column = 0;
  const auto clamped = clamp_index(cursor_pos_, text_.size());
  for (size_t i = 0; i < clamped; ++i) {
    if (text_[i] == '\n') {
      ++line;
      column = 0;
    } else {
      ++column;
    }
  }
  return {line, column};
}

} // namespace ui
} // namespace iris
