#pragma once

namespace iris {
class app_delegate;
namespace ui {
class window;
}
namespace priv {
class app;
}
namespace platform::win32 {

void init_app(priv::app& app);
void shutdown_app(priv::app& app);
void loop_app(priv::app& app);
void pump_messages(priv::app& app);
void add_window(priv::app& app, ui::window* w);
void remove_window(priv::app& app, ui::window* w);
void set_delegate(priv::app& app, app_delegate* in_delegate);

} // namespace platform::win32
} // namespace iris
