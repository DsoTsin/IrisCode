#pragma once

#include "../view.hpp"

#include <functional>

namespace iris {
namespace ui {

class UICORE_API check_box : public view {
public:
  check_box(view* parent = nullptr);
  virtual ~check_box();

  virtual void draw(const rect& dirty_rect) override;

  void set_checked(bool checked);
  bool checked() const;

  void set_title(const string& title);
  string title() const;

  void set_enabled(bool enabled);
  bool enabled() const;

  void set_on_changed(function<void(bool)> callback);

protected:
  bool checked_ = false;
  bool enabled_ = true;
  string title_;
  function<void(bool)> on_changed_;
};

} // namespace ui
} // namespace iris
