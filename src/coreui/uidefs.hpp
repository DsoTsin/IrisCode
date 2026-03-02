#pragma once

#include "ref.hpp"
#include "stl.hpp"

#ifndef UICORE_API
#ifdef BUILD_SHARED
#ifdef BUILD_UICORE
#define UICORE_API __declspec(dllexport)
#else // BUILD_UICORE
#define UICORE_API __declspec(dllimport)
#endif
#else // BUILD_SHARED
#define UICORE_API  
#endif
#endif // UICORE_API

class SkFont;
class SkCanvas;

namespace iris {
namespace ui {
//@see
// https://github.com/phracker/MacOSX-SDKs/blob/master/MacOSX11.3.sdk/System/Library/Frameworks/AppKit.framework/Versions/C/Headers/NSEvent.h
enum class event_type : uint8_t {
  left_mouse_down,    // The user pressed the left mouse button.
  left_mouse_dragged, // The user moved the mouse while holding down the left mouse button.
  left_mouse_up,      // The user released the left mouse button.
  right_mouse_down,   // The user pressed the right mouse button.

  right_mouse_up,      // The user released the right mouse button.
  right_mouse_dragged, // The user moved the mouse while holding down the right mouse button.
  other_mouse_down,    // The user pressed a tertiary mouse button.
  other_mouse_up,      // The user released a tertiary mouse button.
  mouse_moved,         // The user moved the mouse in a way that caused the cursor to move onscreen.
  mouse_entered,       // The cursor entered a well-defined area, such as a view.
  mouse_exited,        // The cursor exited a well-defined area, such as a view.

  /// Getting Keyboard Event Types
  key_down, // The user pressed a key on the keyboard.
  key_up,   // The user released a key on the keyboard.

  /// Getting Touch-Based Events
  begin_gesture,    // An event marking the beginning of a gesture.
  end_gesture,      // An event that marks the end of a gesture.
  magnify,          // The user performed a pinch-open or pinch-close gesture.
  smart_magnify,    // The user performed a smart-zoom gesture.
  swipe,            // The user performed a swipe gesture.
  rotate,           // The user performed a rotate gesture.
  gesture,          // The user performed a nonspecific type of gesture.
  direct_touch,     // The user touched a portion of the touch bar.
  tablet_point,     // The user touched a point on a tablet.
  tablet_proximity, // A pointing device is near, but not touching, the associated tablet.
  pressure,         // An event that reports a change in pressure on a pressure-sensitive device.

  /// Getting Other Input Types

  scroll_wheel, // The scroll wheel position changed.
  change_mode,  // The user changed the mode of a connected device.

  /// Getting System Event Types
  appkit_defined,      // An AppKit-related event occurred.
  application_defined, // An app-defined event occurred.
  cursor_update,       // An event that updates the cursor.
  flags_changed,       // The event flags changed.
  periodic,            // An event that provides execution time to periodic tasks.
  quick_look,          // An event that initiates a Quick Look request.
  system_defined,      // A system-related event occurred.
};

struct size {
  float width = 0;
  float height = 0;
};

struct rect {
  float left = 0;
  float top = 0;
  float width = 0;
  float height = 0;
  float right() const { return left + width; }
  float bottom() const { return top + height; }
};

struct point {
  double x = 0.0;
  double y = 0.0;
};

struct transform {

};

struct color {
  union {
    struct {
      uint8_t a;
      uint8_t r;
      uint8_t g;
      uint8_t b;
    };
    uint32_t v;
  };

  color() : v(0) {}
  color(uint8_t ia, uint8_t ir, uint8_t ig, uint8_t ib) : a(ia), r(ir), g(ig), b(ib) {}
  color(uint8_t ir, uint8_t ig, uint8_t ib) : a(255), r(ir), g(ig), b(ib) {}
};

enum class button_style : uint8_t { flat, capsule, gradient };
enum class button_state : uint8_t {
  undetermined,
  hovered,
  pressed,
  disabled,
};

typedef SkFont font;
typedef SkCanvas canvas;

} // namespace ui
} // namespace iris