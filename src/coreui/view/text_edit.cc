#include "text_edit.hpp"

namespace iris {
namespace ui {

text_edit::text_edit(view* parent) : view(parent) {}

text_edit::~text_edit() {}

void text_edit::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  // Text edit drawing implementation
}

void text_edit::set_text(const string& text) {
  if (text_ != text) {
    text_ = text;
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
  font_size_ = size;
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
  text_ += text;
  if (on_text_changed_) {
    on_text_changed_(text_);
  }
}

void text_edit::delete_selection() {
  // Selection deletion implementation
}

void text_edit::select_all() {
  // Select all implementation
}

} // namespace ui
} // namespace iris
