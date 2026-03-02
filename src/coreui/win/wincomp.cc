#define INITGUID
#define NOMINMAX
#include <unknwn.h>

#include "wincomp.hpp"
#include "../window.hpp"

#include <DispatcherQueue.h>
#include <windows.ui.composition.h>
#include <windows.graphics.effects.interop.h>
#include "wincomp_fx.hpp"

#pragma comment(lib, "windowsapp.lib")

namespace iris {
namespace priv {
long get_os_build_ver();
/*
    public static readonly Version MinWinCompositionVersion = new(10, 0, 17134);
    public static readonly Version MinAcrylicVersion = new(10, 0, 15063);
    public static readonly Version MinHostBackdropVersion = new(10, 0, 22000);
    */
}
namespace ui {
namespace win {
struct initializer {
  initializer() { ensure_dispatcher_queue(); }
  ~initializer() {}

private:
  void ensure_dispatcher_queue() {
    namespace abi = ABI::Windows::System;
    namespace rt = winrt::Windows::System;
    if (m_dispatcherQueueController == nullptr) {
      DispatcherQueueOptions options{
          sizeof(DispatcherQueueOptions), /* dwSize */
          DQTYPE_THREAD_CURRENT,          /* threadType */
          DQTAT_COM_ASTA                  /* apartmentType */
      };
      rt::DispatcherQueueController controller{nullptr};
      winrt::check_hresult(
          CreateDispatcherQueueController(options,
                                          reinterpret_cast<abi::IDispatcherQueueController**>(
                                              winrt::put_abi(controller))));
      m_dispatcherQueueController = controller;
    }
  }

  winrt::Windows::System::DispatcherQueueController m_dispatcherQueueController{nullptr};
};

using namespace ABI::Windows::Graphics::Effects;
namespace wuc = winrt::Windows::UI::Composition;
namespace wu = winrt::Windows::UI;
namespace wge = winrt::Windows::Graphics::Effects;
namespace abi = ABI::Windows::UI::Composition;

compositor::compositor() : d_(nullptr) {
  d_ = winrt::Windows::UI::Composition::Compositor();

  compositor5_ = d_.as<abi::ICompositor5>();
  interop_ = d_.as<abi::Desktop::ICompositorDesktopInterop>();
  //commit async c5->RequestCommitAsync();
  // @see https://github.com/Maplespe/DWMBlurGlass

  create_mica_backdrop_brush();
  create_acrylic_backdrop_brush();
}
compositor::~compositor() {}

std::tuple<wuc::CompositionTarget,
           wuc::CompositionClip,
           wuc::CompositionRoundedRectangleGeometry,
           wuc::ContainerVisual,
           wuc::SpriteVisual,
           wuc::SpriteVisual>
compositor::create_composite_window(ui::window* wnd, IDXGISwapChain* swapchain) {
  abi::Desktop::IDesktopWindowTarget* window = nullptr;
  winrt::check_hresult(
      interop_->CreateDesktopWindowTarget((HWND)wnd->native_handle(), TRUE, &window));
  wuc::Desktop::DesktopWindowTarget win_target(window, winrt::take_ownership_from_abi);
  auto target = win_target.as<wuc::CompositionTarget>();
  auto root_visual = d_.CreateContainerVisual();
  root_visual.BorderMode(wuc::CompositionBorderMode::Hard);
  auto content_visual = d_.CreateSpriteVisual();

  auto rounded_geom = d_.CreateRoundedRectangleGeometry();
  rounded_geom.CornerRadius({wnd->corner_radius(), wnd->corner_radius()});
  rounded_geom.Offset({wnd->shadow_distance(), wnd->shadow_distance()});

  auto rect_geom = d_.CreateRectangleClip();
  rect_geom.Offset({0, 0});

  auto geom_clip = d_.CreateGeometricClip(rounded_geom);

  // create blur visual
  auto create_blur_visual = [this](const wuc::CompositionBrush& brush) -> wuc::SpriteVisual {
    auto visual = d_.CreateSpriteVisual();
    visual.IsVisible(true);
    visual.RelativeSizeAdjustment({1.f, 1.f});
    visual.Brush(brush);
    return visual;
  };

  auto acrylic_visual = create_blur_visual(acrylic_brush_);
  acrylic_visual.Clip(geom_clip);

  auto mica_visual = create_blur_visual(mica_brush_);
  mica_visual.Clip(geom_clip);

  auto ci = d_.as<abi::ICompositorInterop>();
  abi::ICompositionSurface* surface = nullptr;
  winrt::check_hresult(ci->CreateCompositionSurfaceForSwapChain(swapchain, &surface));
  wuc::ICompositionSurface csurf(surface, winrt::take_ownership_from_abi);
  auto surface_brush = d_.CreateSurfaceBrush();
  surface_brush.Surface(csurf);
  content_visual.RelativeSizeAdjustment({1.f, 1.f});

  auto frame = wnd->frame();
  root_visual.Size({frame.width, frame.height});
  rounded_geom.Size(
      {frame.width - 2 * wnd->shadow_distance(), frame.height - 2 * wnd->shadow_distance()});

  content_visual.Brush(surface_brush);
  root_visual.Children().InsertAtTop(acrylic_visual);
  //root_visual.Children().InsertAtTop(mica_visual);
  root_visual.Children().InsertAtTop(content_visual);
  target.Root(root_visual);
  d_.RequestCommitAsync();
  return {target, geom_clip, rounded_geom, root_visual, acrylic_visual, content_visual};
}

void compositor::resize(float w, float h) {

}

static wuc::CompositionBrush create_brush(const wuc::Compositor& compositor,
                                          const wu::Color& tintColor,
                                          float tintOpacity,
                                          float blurAmount,
                                          bool hostBackdrop) try {
  if (static_cast<float>(tintColor.A) * tintOpacity == 255.f) {
    return compositor.CreateColorBrush(tintColor);
  }
  auto tintColorEffect{winrt::make_self<ColorSourceEffect>()};
  tintColorEffect->SetName(L"TintColor");
  tintColorEffect->SetColor(tintColor);
  auto tintOpacityEffect{winrt::make_self<OpacityEffect>()};
  tintOpacityEffect->SetName(L"TintOpacity");
  tintOpacityEffect->SetOpacity(tintOpacity);
  tintOpacityEffect->SetInput(*tintColorEffect);
  auto colorBlendEffect{winrt::make_self<BlendEffect>()};
  colorBlendEffect->SetBlendMode(D2D1_BLEND_MODE_MULTIPLY);
  if (hostBackdrop) {
    colorBlendEffect->SetBackground(wuc::CompositionEffectSourceParameter{L"Backdrop"});
  } else {
    auto gaussianBlurEffect{winrt::make_self<GaussianBlurEffect>()};
    gaussianBlurEffect->SetName(L"Blur");
    gaussianBlurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
    gaussianBlurEffect->SetBlurAmount(blurAmount);
    gaussianBlurEffect->SetInput(wuc::CompositionEffectSourceParameter{L"Backdrop"});
    colorBlendEffect->SetBackground(*gaussianBlurEffect);
  }
  colorBlendEffect->SetForeground(*tintOpacityEffect);
  auto effectBrush{compositor.CreateEffectFactory(*colorBlendEffect).CreateBrush()};
  if (hostBackdrop) {
    effectBrush.SetSourceParameter(L"Backdrop", compositor.CreateHostBackdropBrush());
  } else {
    effectBrush.SetSourceParameter(L"Backdrop", compositor.CreateBackdropBrush());
  }
  return effectBrush;
} catch (...) {
  return nullptr;
}

FORCEINLINE wge::IGraphicsEffectSource create_blurred_backdrop(float blurAmount) {
  if (!blurAmount) {
    return wuc::CompositionEffectSourceParameter{L"Backdrop"};
  }

  auto gaussianBlurEffect{winrt::make_self<GaussianBlurEffect>()};
  gaussianBlurEffect->SetName(L"Blur");
  gaussianBlurEffect->SetBorderMode(D2D1_BORDER_MODE_HARD);
  gaussianBlurEffect->SetBlurAmount(blurAmount);
  gaussianBlurEffect->SetOptimizationMode(D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED);
  gaussianBlurEffect->SetInput(wuc::CompositionEffectSourceParameter{L"Backdrop"});
  return *gaussianBlurEffect;
}

wuc::CompositionBrush create_brush(const wuc::Compositor& compositor,
                                  const wuc::CompositionSurfaceBrush& noiceBrush,
                                  const wu::Color& tintColor,
                                  const wu::Color& luminosityColor,
                                  float blurAmount,
                                  float noiceAmount) {
  if (tintColor.A == 255) {
    return compositor.CreateColorBrush(tintColor);
  }

  auto tintColorEffect{winrt::make_self<ColorSourceEffect>()};
  tintColorEffect->SetName(L"TintColor");
  tintColorEffect->SetColor(tintColor);

  auto luminosityColorEffect{winrt::make_self<ColorSourceEffect>()};
  luminosityColorEffect->SetName(L"LuminosityColor");
  luminosityColorEffect->SetColor(luminosityColor);

  auto luminosityBlendEffect{winrt::make_self<BlendEffect>()};
  // NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and
  // BlendEffectMode::Color are flipped-> This should be changed to Luminosity when/if the bug is
  // fixed->
  luminosityBlendEffect->SetBlendMode(D2D1_BLEND_MODE_COLOR);
  luminosityBlendEffect->SetBackground(create_blurred_backdrop(blurAmount));
  luminosityBlendEffect->SetForeground(*luminosityColorEffect);

  auto colorBlendEffect{winrt::make_self<BlendEffect>()};
  // NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and
  // BlendEffectMode::Color are flipped-> This should be changed to Color when/if the bug is fixed->
  colorBlendEffect->SetBlendMode(D2D1_BLEND_MODE_LUMINOSITY);
  colorBlendEffect->SetBackground(*luminosityBlendEffect);
  colorBlendEffect->SetForeground(*tintColorEffect);

  auto noiseBorderEffect{winrt::make_self<BorderEffect>()};
  noiseBorderEffect->SetExtendX(D2D1_BORDER_EDGE_MODE_WRAP);
  noiseBorderEffect->SetExtendY(D2D1_BORDER_EDGE_MODE_WRAP);
  wuc::CompositionEffectSourceParameter noiseEffectSourceParameter{L"Noise"};
  noiseBorderEffect->SetInput(noiseEffectSourceParameter);

  auto noiseOpacityEffect{winrt::make_self<OpacityEffect>()};
  noiseOpacityEffect->SetName(L"NoiseOpacity");
  noiseOpacityEffect->SetOpacity(noiceAmount);
  noiseOpacityEffect->SetInput(*noiseBorderEffect);

  auto blendEffectOuter{winrt::make_self<BlendEffect>()};
  blendEffectOuter->SetBlendMode(D2D1_BLEND_MODE_MULTIPLY);
  blendEffectOuter->SetBackground(*colorBlendEffect);
  blendEffectOuter->SetForeground(*noiseOpacityEffect);

  auto effectBrush{compositor.CreateEffectFactory(*blendEffectOuter).CreateBrush()};
  effectBrush.SetSourceParameter(L"Noise", noiceBrush);
  effectBrush.SetSourceParameter(L"Backdrop", compositor.CreateBackdropBrush());

  return effectBrush;
}

void compositor::create_mica_backdrop_brush() {
  if (priv::get_os_build_ver() < 22000)
    return;

  float opacity = 1.0f;
  winrt::Windows::UI::Color color;

  auto fx_tint_col = winrt::make_self<ColorSourceEffect>();
  fx_tint_col->SetColor(color);
  auto fx_tint_opacity = winrt::make_self<OpacityEffect>();
  fx_tint_opacity->SetOpacity(1);
  fx_tint_opacity->SetInput(0, *fx_tint_col);
  auto tint_opacity_brush = d_.CreateEffectFactory(*fx_tint_opacity).CreateBrush();
  auto fx_luminosity_col = winrt::make_self<ColorSourceEffect>();
  fx_luminosity_col->SetColor(color);
  auto fx_luminosity_opacity = winrt::make_self<OpacityEffect>();
  fx_luminosity_opacity->SetOpacity(opacity);
  fx_luminosity_opacity->SetInput(0, *fx_luminosity_col);
  auto luminosity_opacity_brush = d_.CreateEffectFactory(*fx_luminosity_opacity).CreateBrush();
  auto mica_backdrop_brush = d_.as<wuc::ICompositorWithBlurredWallpaperBackdropBrush>()
                                 .TryCreateBlurredWallpaperBackdropBrush();
  auto fx_luminosity_blend = winrt::make_self<BlendEffect>();
  fx_luminosity_blend->SetBlendMode(D2D1_BLEND_MODE_LUMINOSITY);
  fx_luminosity_blend->SetBackground(wuc::CompositionEffectSourceParameter(L"Background"));
  fx_luminosity_blend->SetForeground(wuc::CompositionEffectSourceParameter(L"Foreground"));
  auto luminosity_blend_brush = d_.CreateEffectFactory(*fx_luminosity_blend).CreateBrush();
  luminosity_blend_brush.SetSourceParameter(L"Background", mica_backdrop_brush);
  luminosity_blend_brush.SetSourceParameter(L"Foreground", luminosity_opacity_brush);
  auto fx_blend = winrt::make_self<BlendEffect>();
  fx_blend->SetBlendMode(D2D1_BLEND_MODE_COLOR);
  fx_blend->SetBackground(wuc::CompositionEffectSourceParameter(L"Background"));
  fx_blend->SetForeground(wuc::CompositionEffectSourceParameter(L"Foreground"));
  auto blend_brush = d_.CreateEffectFactory(*fx_blend).CreateBrush();
  blend_brush.SetSourceParameter(L"Foreground", tint_opacity_brush);
  blend_brush.SetSourceParameter(L"Background", luminosity_blend_brush);
  mica_brush_ = blend_brush.as<wuc::CompositionBrush>();
}

void compositor::create_acrylic_backdrop_brush() {
  auto backdrop_brush = create_brush(d_,
                                     {
                                         0,
                                         178,
                                         200,
                                         177,
                                     },
                                     0.8f, 2.0f, priv::get_os_build_ver() >= 22000);

  //auto fx_sat = winrt::make_self<iris::ui::SaturationEffect>();
  //fx_sat->SetSaturation(0.f);
  //fx_sat->SetInput(fx_blur);
  //auto fx_sat_factory = d_.CreateEffectFactory(*fx_sat);
  //auto sat_brush = fx_sat_factory.CreateBrush();
  acrylic_brush_ = backdrop_brush.as<wuc::CompositionBrush>();
}

wuc::CompositionBackdropBrush compositor::
    create_backdrop_brush() {
  abi::ICompositionBackdropBrush* brush = nullptr;
  if (priv::get_os_build_ver() >= 22000) {
    auto c3 = d_.as<abi::ICompositor3>();
    c3->CreateHostBackdropBrush(&brush);
  } else {
    auto c2 = d_.as<abi::ICompositor2>();
    c2->CreateBackdropBrush(&brush);
  }
  return wuc::CompositionBackdropBrush(brush, winrt::take_ownership_from_abi);
}
static initializer g_initializer;
}
} // namespace ui
} // namespace iris