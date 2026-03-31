#include "appl_graphics_context.hpp"

namespace iris {
namespace priv {

class graphics_context {
public:
  graphics_context(ui::window* w, bool gpu) : wind_(w) {
    (void)gpu;
  }

  ui::canvas* canvas() {
    return nullptr;
  }

  void resize(int width, int height) {
    (void)width;
    (void)height;
  }

  void flush() {}

  void synchronize(const ui::rect* dirty_rect) {
    (void)dirty_rect;
  }

  ui::rect expand_dirty_rect(const ui::rect& dirty_rect) const {
    return dirty_rect;
  }

  void on_window_pos_changed() {}

  void on_window_activate(bool active) {
    (void)active;
  }

  void on_live_resize_changed(bool active) {
    (void)active;
  }

  void shutdown() {}

  ui::window* window() const {
    return wind_;
  }

private:
  ui::window* wind_ = nullptr;
};

} // namespace priv

namespace platform::apple {

priv::graphics_context* create_graphics_context(ui::window* w, bool gpu) {
  return new priv::graphics_context(w, gpu);
}

void destroy_graphics_context(priv::graphics_context* ctx) {
  delete ctx;
}

void resize_graphics_context(priv::graphics_context* ctx, int width, int height) {
  if (ctx) {
    ctx->resize(width, height);
  }
}

void synchronize_graphics_context(priv::graphics_context* ctx, const ui::rect* dirty_rect) {
  if (ctx) {
    ctx->synchronize(dirty_rect);
  }
}

void flush_graphics_context(priv::graphics_context* ctx) {
  if (ctx) {
    ctx->flush();
  }
}

ui::canvas* graphics_context_canvas(priv::graphics_context* ctx) {
  return ctx ? ctx->canvas() : nullptr;
}

ui::rect expand_graphics_context_dirty_rect(const priv::graphics_context* ctx,
                                            const ui::rect& dirty_rect) {
  return ctx ? ctx->expand_dirty_rect(dirty_rect) : dirty_rect;
}

void on_graphics_context_window_pos_changed(priv::graphics_context* ctx) {
  if (ctx) {
    ctx->on_window_pos_changed();
  }
}

void on_graphics_context_window_activate(priv::graphics_context* ctx, bool active) {
  if (ctx) {
    ctx->on_window_activate(active);
  }
}

void on_graphics_context_live_resize_changed(priv::graphics_context* ctx, bool active) {
  if (ctx) {
    ctx->on_live_resize_changed(active);
  }
}

void shutdown_graphics_context(priv::graphics_context* ctx) {
  if (ctx) {
    ctx->shutdown();
  }
}

ui::window* window_for_graphics_context(const priv::graphics_context* ctx) {
  return ctx ? ctx->window() : nullptr;
}

} // namespace platform::apple
} // namespace iris
