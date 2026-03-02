#include "text_input.hpp"

namespace iris {
namespace ui {

text_input::text_input(view* parent) : view(parent) {}

text_input::~text_input() {}

void text_input::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  // Text input drawing implementation
}

void text_input::set_text(const string& text) {
  string new_text = text;
  if (max_length_ > 0 && new_text.length() > max_length_) {
    new_text = new_text.substr(0, max_length_);
  }
  
  if (text_ != new_text) {
    text_ = new_text;
    if (on_text_changed_) {
      on_text_changed_(text_);
    }
  }
}

string text_input::text() const {
  return text_;
}

void text_input::set_placeholder(const string& placeholder) {
  placeholder_ = placeholder;
}

string text_input::placeholder() const {
  return placeholder_;
}

void text_input::set_font_size(float size) {
  font_size_ = size;
}

float text_input::font_size() const {
  return font_size_;
}

void text_input::set_enabled(bool enabled) {
  enabled_ = enabled;
}

bool text_input::enabled() const {
  return enabled_;
}

void text_input::set_password_mode(bool password) {
  password_mode_ = password;
}

bool text_input::password_mode() const {
  return password_mode_;
}

void text_input::set_max_length(size_t length) {
  max_length_ = length;
  if (max_length_ > 0 && text_.length() > max_length_) {
    text_ = text_.substr(0, max_length_);
  }
}

size_t text_input::max_length() const {
  return max_length_;
}

void text_input::set_on_text_changed(function<void(const string&)> callback) {
  on_text_changed_ = callback;
}

void text_input::set_on_return_pressed(function<void()> callback) {
  on_return_pressed_ = callback;
}

} // namespace ui
} // namespace iris
