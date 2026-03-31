#include "code_editor.hpp"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkPaint.h"
#ifdef _WIN32
#include "include/ports/SkTypeface_win.h"
#endif

namespace iris {
namespace ui {
namespace {
static bool is_word_char(char c) {
  return (c >= 'a' && c <= 'z')
         || (c >= 'A' && c <= 'Z')
         || (c >= '0' && c <= '9')
         || c == '_';
}
} // namespace

code_editor::code_editor(view* parent) : text_edit(parent) {
  set_word_wrap(false);
  reset_keywords();
}

code_editor::~code_editor() {}

void code_editor::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  if (!gfx_ctx_) {
    return;
  }

  auto bounds = get_layout_rect();
  auto canvas = gfx_ctx_->canvas();
  if (!canvas) {
    return;
  }
  if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
    return;
  }
  const SkRect editor_rect = SkRect::MakeXYWH(bounds.left, bounds.top, bounds.width, bounds.height);

  const float gutter_width = show_line_numbers_ ? 52.0f : 0.0f;
  const float horizontal_padding = 8.0f;
  const float vertical_padding = 6.0f;

  SkPaint bg_paint;
  bg_paint.setColor(SkColorSetARGB(0xD0, 0x1E, 0x1F, 0x23));
  bg_paint.setAntiAlias(true);
  canvas->drawRect(editor_rect, bg_paint);

  if (show_line_numbers_) {
    SkPaint gutter_paint;
    gutter_paint.setColor(SkColorSetARGB(0xFF, 0x17, 0x18, 0x1B));
    canvas->drawRect(SkRect::MakeXYWH(bounds.left, bounds.top, gutter_width, bounds.height), gutter_paint);
  }

  canvas->save();
  canvas->clipRect(editor_rect, true);

  SkPaint border_paint;
  border_paint.setColor(SkColorSetARGB(0x90, 0x55, 0x58, 0x62));
  border_paint.setStyle(SkPaint::kStroke_Style);
  border_paint.setStrokeWidth(1.0f);
  border_paint.setAntiAlias(true);

  SkFont font;
  font.setSize(font_size_);
#ifdef _WIN32
  auto fm = SkFontMgr_New_GDI();
  if (fm) {
    auto typeface = fm->legacyMakeTypeface("Consolas", SkFontStyle::Normal());
    if (!typeface) {
      typeface = fm->legacyMakeTypeface("Courier New", SkFontStyle::Normal());
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
  float line_height = metrics.fDescent - metrics.fAscent + metrics.fLeading;
  if (line_height <= 0.0f) {
    line_height = max(font_size_ * 1.35f, 10.0f);
  }

  auto rendered_lines = lines();
  if (rendered_lines.empty()) {
    rendered_lines.push_back("");
  }

  float baseline = bounds.top + vertical_padding - metrics.fAscent;
  for (size_t i = 0; i < rendered_lines.size(); ++i) {
    const auto& line = rendered_lines[i];
    if (show_line_numbers_) {
      const auto line_no = string::format("%zu", i + 1);
      SkPaint number_paint;
      number_paint.setColor(SkColorSetARGB(0xFF, 0x6E, 0x73, 0x7D));
      number_paint.setAntiAlias(true);
      canvas->drawSimpleText(line_no.c_str(), line_no.size(), SkTextEncoding::kUTF8,
                             bounds.left + 6.0f, baseline, font, number_paint);
    }

    auto spans = tokenize_line(line);
    float x = bounds.left + gutter_width + horizontal_padding;
    for (const auto& span : spans) {
      SkPaint token_paint;
      token_paint.setColor(span.color);
      token_paint.setAntiAlias(true);
      canvas->drawSimpleText(span.text.c_str(), span.text.size(), SkTextEncoding::kUTF8, x, baseline,
                             font, token_paint);
      x += font.measureText(span.text.c_str(), span.text.size(), SkTextEncoding::kUTF8);
    }

    baseline += line_height;
    if (baseline > bounds.bottom()) {
      break;
    }
  }

  if (should_draw_caret()) {
    const auto [line_index, column] = cursor_line_column();
    if (line_index < rendered_lines.size()) {
      const auto& line = rendered_lines[line_index];
      const auto prefix = line.substr(0, min(column, line.size()));
      const float cursor_x = bounds.left + gutter_width + horizontal_padding
                             + font.measureText(prefix.c_str(), prefix.size(), SkTextEncoding::kUTF8);
      const float cursor_top = bounds.top + vertical_padding + line_index * line_height;
      const float cursor_bottom = cursor_top + line_height;
      SkPaint caret_paint;
      caret_paint.setColor(SkColorSetARGB(0xFF, 0xC9, 0xD1, 0xD9));
      caret_paint.setStrokeWidth(1.2f);
      caret_paint.setAntiAlias(true);
      canvas->drawLine(cursor_x, cursor_top, cursor_x, cursor_bottom, caret_paint);
    }
  }

  canvas->restore();
  canvas->drawRect(editor_rect, border_paint);
}

void code_editor::set_language(code_language language) {
  if (language_ == language) {
    return;
  }
  language_ = language;
  reset_keywords();
}

code_language code_editor::language() const {
  return language_;
}

void code_editor::set_tab_width(size_t width) {
  tab_width_ = max<size_t>(1, width);
}

size_t code_editor::tab_width() const {
  return tab_width_;
}

void code_editor::set_show_line_numbers(bool show) {
  show_line_numbers_ = show;
}

bool code_editor::show_line_numbers() const {
  return show_line_numbers_;
}

vector<code_editor::styled_span> code_editor::tokenize_line(const string& line) const {
  vector<styled_span> spans;
  if (line.empty()) {
    spans.push_back({"", SkColorSetARGB(0xFF, 0xCF, 0xD7, 0xDF)});
    return spans;
  }

  const SkColor text_color = SkColorSetARGB(0xFF, 0xCF, 0xD7, 0xDF);
  const SkColor keyword_color = SkColorSetARGB(0xFF, 0x4F, 0xB8, 0xFF);
  const SkColor number_color = SkColorSetARGB(0xFF, 0xF7, 0xC1, 0x6C);
  const SkColor string_color = SkColorSetARGB(0xFF, 0x9E, 0xD4, 0x8B);
  const SkColor comment_color = SkColorSetARGB(0xFF, 0x78, 0x83, 0x90);

  size_t i = 0;
  while (i < line.size()) {
    if (i + 1 < line.size() && line[i] == '/' && line[i + 1] == '/') {
      spans.push_back({line.substr(i), comment_color});
      break;
    }

    if (line[i] == '"' || line[i] == '\'') {
      const char quote = line[i];
      size_t j = i + 1;
      while (j < line.size()) {
        if (line[j] == quote && line[j - 1] != '\\') {
          ++j;
          break;
        }
        ++j;
      }
      spans.push_back({line.substr(i, j - i), string_color});
      i = j;
      continue;
    }

    if (line[i] >= '0' && line[i] <= '9') {
      size_t j = i + 1;
      while (j < line.size() && ((line[j] >= '0' && line[j] <= '9') || line[j] == '.')) {
        ++j;
      }
      spans.push_back({line.substr(i, j - i), number_color});
      i = j;
      continue;
    }

    if (is_word_char(line[i])) {
      size_t j = i + 1;
      while (j < line.size() && is_word_char(line[j])) {
        ++j;
      }
      auto token = line.substr(i, j - i);
      spans.push_back({token, is_keyword(token) ? keyword_color : text_color});
      i = j;
      continue;
    }

    spans.push_back({line.substr(i, 1), text_color});
    ++i;
  }

  return spans;
}

bool code_editor::is_keyword(const string& token) const {
  return keywords_.find(token) != keywords_.end();
}

void code_editor::reset_keywords() {
  keywords_.clear();

  if (language_ == code_language::swift) {
    keywords_ = {
        "class", "deinit", "enum", "extension", "func", "import", "init", "let", "protocol",
        "return", "self", "struct", "var", "if", "else", "for", "while", "switch", "case",
        "guard", "in", "break", "continue", "public", "private", "internal",
    };
    return;
  }

  if (language_ == code_language::cpp) {
    keywords_ = {
        "alignas", "auto", "bool", "break", "case", "catch", "char", "class", "const",
        "constexpr", "continue", "default", "delete", "do", "double", "else", "enum",
        "explicit", "extern", "false", "float", "for", "friend", "if", "inline", "int",
        "long", "namespace", "new", "noexcept", "nullptr", "operator", "private", "protected",
        "public", "return", "short", "signed", "sizeof", "static", "struct", "switch",
        "template", "this", "throw", "true", "try", "typedef", "typename", "union", "unsigned",
        "using", "virtual", "void", "volatile", "while",
    };
  }
}

} // namespace ui
} // namespace iris
