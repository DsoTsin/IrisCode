#include "check_box.hpp"

namespace iris {
namespace ui {

check_box::check_box(view* parent) : view(parent) {}

check_box::~check_box() {}

void check_box::draw(const rect& dirty_rect) {
  view::draw(dirty_rect);
  // Check box drawing implementation
}

void check_box::set_checked(bool checked) {
  if (checked_ != checked) {
    checked_ = checked;
    if (on_changed_) {
      on_changed_(checked_);
    }
  }
}

bool check_box::checked() const {
  return checked_;
}

void check_box::set_title(const string& title) {
  title_ = title;
}

string check_box::title() const {
  return title_;
}

void check_box::set_enabled(bool enabled) {
  enabled_ = enabled;
}

bool check_box::enabled() const {
  return enabled_;
}

void check_box::set_on_changed(std::function<void(bool)> callback) {
  on_changed_ = callback;
}

} // namespace ui
} // namespace iris
