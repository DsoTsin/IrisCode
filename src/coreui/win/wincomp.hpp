#pragma once

#include "../uidefs.hpp"
#include "include/core/SkRefCnt.h"
#define NOMINMAX
#include <dxgi.h>
#include <Windows.UI.Composition.h>
#include <Windows.UI.Composition.Interop.h>
#include <winrt/Windows.UI.Composition.Desktop.h>

namespace iris {
namespace ui {
class window;
namespace win {
class compositor: public SkRefCnt {
public:
  compositor();
  ~compositor();

  std::tuple<winrt::Windows::UI::Composition::CompositionTarget,
             winrt::Windows::UI::Composition::CompositionClip,
             winrt::Windows::UI::Composition::CompositionRoundedRectangleGeometry,
             winrt::Windows::UI::Composition::ContainerVisual,
             winrt::Windows::UI::Composition::SpriteVisual,
             winrt::Windows::UI::Composition::SpriteVisual>
  create_composite_window(window* wnd, IDXGISwapChain* swapchain);
  void resize(float w, float h);

private:
  void create_mica_backdrop_brush();
  void create_acrylic_backdrop_brush();
  winrt::Windows::UI::Composition::CompositionBackdropBrush create_backdrop_brush();
  winrt::com_ptr<ABI::Windows::UI::Composition::ICompositor5> compositor5_;
  winrt::com_ptr<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop> interop_;
  winrt::Windows::UI::Composition::Compositor d_{nullptr};
  winrt::Windows::UI::Composition::CompositionBrush mica_brush_{nullptr};
  winrt::Windows::UI::Composition::CompositionBrush acrylic_brush_{nullptr};
};
} // namespace win
} // namespace ui
} // namespace iris