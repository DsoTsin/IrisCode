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
  virtual bool mouse_down(const mouse_event& event) override;
  virtual bool mouse_up(const mouse_event& event) override;
  virtual bool mouse_enter(const mouse_event& event) override;
  virtual bool mouse_leave(const mouse_event& event) override;

  button& text(const string& title);
  string text() const;

  button& enabled(bool enabled);
  bool enabled() const;

  button& on_click(function<void()> callback);

protected:
  void request_redraw();
  string text_;
  bool enabled_ = true;
  bool hovered_ = false;
  bool pressed_ = false;
  button_style style_;
  color color_;
  function<void()> on_click_;
};

class UICORE_API title_button : public view {
public:
  title_button(view* parent = nullptr);
  virtual ~title_button();

  virtual void draw(const rect& dirty_rect) override;
  virtual bool mouse_down(const mouse_event& event) override;
  virtual bool mouse_up(const mouse_event& event) override;
  virtual bool mouse_enter(const mouse_event& event) override;
  virtual bool mouse_move(const mouse_event& event) override;
  virtual bool mouse_leave(const mouse_event& event) override;
  bool is_control_button_hit(const point& local_point) const;

private:
  int hit_circle_index(const point& local_point) const;
  void perform_action(int circle_index);

private:
  int hovered_circle_ = -1;
  int pressed_circle_ = -1;
};

class UICORE_API vsplitter : public view {
public:
  vsplitter(view* parent = nullptr);
  virtual ~vsplitter();

  virtual void draw(const rect& dirty_rect) override;
};

} // namespace ui
} // namespace iris
