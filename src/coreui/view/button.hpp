#pragma once

#include "../view.hpp"

#include <functional>

namespace iris {
namespace ui {

class UICORE_API button : public view {
public:
  button(view* parent = nullptr);
  virtual ~button();

  virtual void draw(const rect& dirty_rect) override;

  button& text(const string& title);
  string text() const;

  button& enabled(bool enabled);
  bool enabled() const;

  button& on_click(function<void()> callback);

protected:
  string text_;
  bool enabled_ = true;
  button_style style_;
  color color_;
  function<void()> on_click_;
};

} // namespace ui
} // namespace iris