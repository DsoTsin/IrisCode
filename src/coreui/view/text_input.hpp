#pragma once

#include "../view.hpp"
#include <functional>

namespace iris {
namespace ui {

class UICORE_API text_input : public view {
public:
  text_input(view* parent = nullptr);
  virtual ~text_input();

  virtual void draw(const rect& dirty_rect) override;

  void set_text(const string& text);
  string text() const;

  void set_placeholder(const string& placeholder);
  string placeholder() const;

  void set_font_size(float size);
  float font_size() const;

  void set_enabled(bool enabled);
  bool enabled() const;

  void set_password_mode(bool password);
  bool password_mode() const;

  void set_max_length(size_t length);
  size_t max_length() const;

  void set_on_text_changed(function<void(const string&)> callback);
  void set_on_return_pressed(function<void()> callback);

protected:
  string text_;
  string placeholder_;
  float font_size_ = 14.0f;
  bool enabled_ = true;
  bool password_mode_ = false;
  size_t max_length_ = 0; // 0 means unlimited
  function<void(const string&)> on_text_changed_;
  function<void()> on_return_pressed_;
};

} // namespace ui
} // namespace iris