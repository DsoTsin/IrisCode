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

protected:
  string text_;
  string placeholder_;
  float font_size_ = 14.0f;
  bool editable_ = true;
  bool word_wrap_ = true;
  function<void(const string&)> on_text_changed_;
};

} // namespace ui
} // namespace iris