#pragma once

#include "../../uidefs.hpp"

namespace iris {
namespace priv {
class graphics_context;
}

namespace ui {
class window;
}

namespace platform::apple {

priv::graphics_context* create_graphics_context(ui::window* w, bool gpu);
void destroy_graphics_context(priv::graphics_context* ctx);
void resize_graphics_context(priv::graphics_context* ctx, int width, int height);
void synchronize_graphics_context(priv::graphics_context* ctx, const ui::rect* dirty_rect);
void flush_graphics_context(priv::graphics_context* ctx);
ui::canvas* graphics_context_canvas(priv::graphics_context* ctx);
ui::rect expand_graphics_context_dirty_rect(const priv::graphics_context* ctx,
                                            const ui::rect& dirty_rect);
void on_graphics_context_window_pos_changed(priv::graphics_context* ctx);
void on_graphics_context_window_activate(priv::graphics_context* ctx, bool active);
void on_graphics_context_live_resize_changed(priv::graphics_context* ctx, bool active);
void shutdown_graphics_context(priv::graphics_context* ctx);
ui::window* window_for_graphics_context(const priv::graphics_context* ctx);

} // namespace platform::apple
} // namespace iris
