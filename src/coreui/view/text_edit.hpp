#pragma once

#include "../view.hpp"

#include <functional>

namespace iris {
namespace ui {

class UICORE_API text_edit : public view {
public:
  text_edit(view* parent = nullptr);
  virtual ~text_edit();

  virtual void draw(const rect& dirty_rect) override;
  virtual bool mouse_down(const mouse_event& event) override;
  virtual bool key_down(const key_event& event) override;
  virtual bool key_char(const key_event& event) override;
  virtual bool can_receive_focus() const override { return editable_; }

  void set_text(const string& text);
  string text() const;

  void set_placeholder(const string& placeholder);
  string placeholder() const;

  void set_font_size(float size);
  float font_size() const;

  void set_editable(bool editable);
  bool editable() const;

  void set_word_wrap(bool wrap);
  bool word_wrap() const;

  void set_on_text_changed(function<void(const string&)> callback);

  void insert_text(const string& text);
  void delete_selection();
  void select_all();
  void move_cursor(size_t pos);
  size_t cursor_pos() const;
  void backspace();
  void delete_forward();

protected:
  virtual size on_measure(MeasureMode wm, MeasureMode hm, const size& sz) override;
  vector<string> lines() const;
  pair<size_t, size_t> cursor_line_column() const;

protected:
  string text_;
  string placeholder_;
  float font_size_ = 14.0f;
  bool editable_ = true;
  bool word_wrap_ = true;
  size_t cursor_pos_ = 0;
  size_t selection_start_ = 0;
  size_t selection_end_ = 0;
  function<void(const string&)> on_text_changed_;
};

} // namespace ui
} // namespace iris
