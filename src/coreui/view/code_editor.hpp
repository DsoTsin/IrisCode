#pragma once

#include "text_edit.hpp"
#include "include/core/SkColor.h"

namespace iris {
namespace ui {

enum class code_language : uint8_t {
  plain_text,
  cpp,
  swift,
};

class UICORE_API code_editor : public text_edit {
public:
  code_editor(view* parent = nullptr);
  virtual ~code_editor();

  virtual void draw(const rect& dirty_rect) override;

  void set_language(code_language language);
  code_language language() const;

  void set_tab_width(size_t width);
  size_t tab_width() const;

  void set_show_line_numbers(bool show);
  bool show_line_numbers() const;

private:
  struct styled_span {
    string text;
    SkColor color;
  };

  vector<styled_span> tokenize_line(const string& line) const;
  bool is_keyword(const string& token) const;
  void reset_keywords();

private:
  code_language language_ = code_language::cpp;
  size_t tab_width_ = 2;
  bool show_line_numbers_ = true;
  unordered_set<string> keywords_;
};

} // namespace ui
} // namespace iris
