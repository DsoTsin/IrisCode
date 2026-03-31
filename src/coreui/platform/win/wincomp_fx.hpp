#pragma once


#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime

#include <d2d1_1.h>
#include <d2d1effects_2.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Web.Syndication.h>
#include <winrt/windows.system.diagnostics.h>
#include <winrt/windows.system.h>
#include <winrt/windows.system.threading.h>

#include <winrt/windows.storage.h>
#include <winrt/windows.storage.pickers.h>
#include <winrt/windows.storage.streams.h>

#include <winrt/windows.graphics.directx.h>
#include <winrt/windows.graphics.effects.h>
#include <winrt/windows.graphics.h>

#include <winrt/Windows.Foundation.h>

#include <winrt/Windows.Graphics.Effects.h>
#include <winrt/Windows.Graphics.h>

#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Composition.effects.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.interactions.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.h>

#pragma pop_macro("GetCurrentTime")

namespace iris {
namespace ui {
struct NamedProperty {
  LPCWSTR Name; // Compile-time constant
  UINT Index;   // Property index
  using GRAPHICS_EFFECT_PROPERTY_MAPPING
      = ABI::Windows::Graphics::Effects::GRAPHICS_EFFECT_PROPERTY_MAPPING;
  GRAPHICS_EFFECT_PROPERTY_MAPPING Mapping;
};
class CanvasEffect
  : public winrt::implements<CanvasEffect,
                             winrt::Windows::Graphics::Effects::IGraphicsEffect,
                             winrt::Windows::Graphics::Effects::IGraphicsEffectSource,
                             ABI::Windows::Graphics::Effects::IGraphicsEffectD2D1Interop> {
  CLSID m_effectId{};
  winrt::hstring m_name{};
  std::unordered_map<size_t, winrt::Windows::Foundation::IPropertyValue> m_properties{};
  std::unordered_map<size_t, winrt::Windows::Graphics::Effects::IGraphicsEffectSource>
      m_effectSources{};

protected:
  std::vector<NamedProperty> m_namedProperties{};

public:
  CanvasEffect(REFCLSID effectId) : m_effectId{effectId} {}
  virtual ~CanvasEffect() = default;

  winrt::hstring Name() { return m_name; }
  void Name(const winrt::hstring& string) { m_name = string; }

  IFACEMETHOD(GetEffectId)(CLSID* id) override {
    if (id) {
      *id = m_effectId;
      return S_OK;
    }

    return E_POINTER;
  }
  IFACEMETHOD(GetSourceCount)(UINT* count) override {
    if (count) {
      *count = static_cast<UINT>(m_effectSources.size());
      return S_OK;
    }

    return E_POINTER;
  }
  IFACEMETHOD(GetSource)(UINT index,
                         ABI::Windows::Graphics::Effects::IGraphicsEffectSource** source) {
    if (!source) {
      return E_POINTER;
    }

    m_effectSources.at(index).as<ABI::Windows::Graphics::Effects::IGraphicsEffectSource>().copy_to(
        source);
    return S_OK;
  }
  IFACEMETHOD(GetPropertyCount)(UINT* count) override {
    if (count) {
      *count = static_cast<UINT>(m_effectSources.size());
      return S_OK;
    }

    return E_POINTER;
  }
  IFACEMETHOD(GetProperty)(UINT index, ABI::Windows::Foundation::IPropertyValue** value) override {
    if (!value) {
      return E_POINTER;
    }

    *value = m_properties.at(index).as<ABI::Windows::Foundation::IPropertyValue>().detach();
    return S_OK;
  }
  IFACEMETHOD(GetNamedPropertyMapping)(
      LPCWSTR name,
      UINT* index,
      NamedProperty::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) override {
    for (UINT i = 0; i < m_namedProperties.size(); ++i) {
      const auto& prop = m_namedProperties[i];
      if (!_wcsicmp(name, prop.Name)) {
        *index = prop.Index;
        *mapping = prop.Mapping;
        return S_OK;
      }
    }
    return E_INVALIDARG;
  }

public:
  template <typename T>
  static winrt::com_ptr<T> construct_from_abi(T* from) {
    winrt::com_ptr<T> to{nullptr};
    to.copy_from(from);

    return to;
  };
  void SetName(const winrt::hstring& name) { Name(name); }
  void SetInput(UINT index,
                const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source) {
    m_effectSources.insert_or_assign(index, source);
  }
  void SetInput(const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source) {
    SetInput(0, source);
  }
  void SetInput(const winrt::Windows::UI::Composition::CompositionEffectSourceParameter& source) {
    SetInput(0, source.as<winrt::Windows::Graphics::Effects::IGraphicsEffectSource>());
  }

  void SetInput(const std::vector<winrt::Windows::Graphics::Effects::IGraphicsEffectSource>&
                    effectSourceList) {
    for (UINT i = 0; i < effectSourceList.size(); i++) {
      SetInput(i, effectSourceList[i]);
    }
  }

protected:
  void SetProperty(UINT index, const winrt::Windows::Foundation::IPropertyValue& value) {
    m_properties.insert_or_assign(index, value);
  }
  template <typename T>
  auto BoxValue(T value) {
    return winrt::Windows::Foundation::PropertyValue::CreateUInt32(value)
        .as<winrt::Windows::Foundation::IPropertyValue>();
  }
  auto BoxValue(bool value) {
    return winrt::Windows::Foundation::PropertyValue::CreateBoolean(value)
        .as<winrt::Windows::Foundation::IPropertyValue>();
  }
  auto BoxValue(float value) {
    return winrt::Windows::Foundation::PropertyValue::CreateSingle(value)
        .as<winrt::Windows::Foundation::IPropertyValue>();
  }
  auto BoxValue(UINT32 value) {
    return winrt::Windows::Foundation::PropertyValue::CreateUInt32(value)
        .as<winrt::Windows::Foundation::IPropertyValue>();
  }
  auto BoxValue(const D2D1_MATRIX_5X4_F& value) {
    return winrt::Windows::Foundation::PropertyValue::CreateSingleArray(value.m[0])
        .as<winrt::Windows::Foundation::IPropertyValue>();
  }
  template <size_t size>
  auto BoxValue(float (&value)[size]) {
    return winrt::Windows::Foundation::PropertyValue::CreateSingleArray(value)
        .as<winrt::Windows::Foundation::IPropertyValue>();
  }
};

class BlendEffect : public CanvasEffect {
public:
  BlendEffect() : CanvasEffect{CLSID_D2D1Blend} { SetBlendMode(); }
  virtual ~BlendEffect() = default;

  void SetBlendMode(D2D1_BLEND_MODE blendMode = D2D1_BLEND_MODE_MULTIPLY) {
    SetProperty(D2D1_BLEND_PROP_MODE, BoxValue(blendMode));
  }
  void SetBackground(const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source) {
    SetInput(0, source);
  }
  void SetBackground(
      const winrt::Windows::UI::Composition::CompositionEffectSourceParameter& source) {
    SetInput(0, source);
  }

  void SetForeground(const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source) {
    SetInput(1, source);
  }
  void SetForeground(
      const winrt::Windows::UI::Composition::CompositionEffectSourceParameter& source) {
    SetInput(1, source);
  }
};

class BorderEffect : public CanvasEffect {
public:
  BorderEffect() : CanvasEffect{CLSID_D2D1Border} {
    SetExtendX();
    SetExtendY();
  }
  virtual ~BorderEffect() = default;

  void SetExtendX(D2D1_BORDER_EDGE_MODE extendX = D2D1_BORDER_EDGE_MODE_CLAMP) {
    SetProperty(D2D1_BORDER_PROP_EDGE_MODE_X, BoxValue(extendX));
  }
  void SetExtendY(D2D1_BORDER_EDGE_MODE extendY = D2D1_BORDER_EDGE_MODE_CLAMP) {
    SetProperty(D2D1_BORDER_PROP_EDGE_MODE_Y, BoxValue(extendY));
  }
};

class CompositeEffect : public CanvasEffect {
public:
  CompositeEffect() : CanvasEffect{CLSID_D2D1Composite} { SetCompositeMode(); }
  virtual ~CompositeEffect() = default;

  void SetCompositeMode(D2D1_COMPOSITE_MODE compositeMode = D2D1_COMPOSITE_MODE_SOURCE_OVER) {
    SetProperty(D2D1_COMPOSITE_PROP_MODE, BoxValue(compositeMode));
  }
};

class CompositeStepEffect : public CanvasEffect {
public:
  CompositeStepEffect() : CanvasEffect{CLSID_D2D1Composite} { SetCompositeMode(); }
  virtual ~CompositeStepEffect() = default;

  void SetCompositeMode(D2D1_COMPOSITE_MODE compositeMode = D2D1_COMPOSITE_MODE_SOURCE_OVER) {
    SetProperty(D2D1_COMPOSITE_PROP_MODE, BoxValue(compositeMode));
  }
  void SetDestination(const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source) {
    SetInput(0, source);
  }
  void SetDestination(
      const winrt::Windows::UI::Composition::CompositionEffectSourceParameter& source) {
    SetInput(0, source);
  }
  void SetSource(const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source) {
    SetInput(1, source);
  }
  void SetSource(const winrt::Windows::UI::Composition::CompositionEffectSourceParameter& source) {
    SetInput(1, source);
  }
};

class ColorSourceEffect : public CanvasEffect {
public:
  ColorSourceEffect() : CanvasEffect{CLSID_D2D1Flood} { SetColor(); }
  virtual ~ColorSourceEffect() = default;

  void SetColor(const D2D1_COLOR_F& color = {0.f, 0.f, 0.f, 1.f}) {
    float value[]{color.r, color.g, color.b, color.a};
    SetProperty(D2D1_FLOOD_PROP_COLOR, BoxValue(value));
  }
  void SetColor(const winrt::Windows::UI::Color& color) {
    SetColor(D2D1_COLOR_F{static_cast<float>(color.R) / 255.f, static_cast<float>(color.G) / 255.f,
                          static_cast<float>(color.B) / 255.f,
                          static_cast<float>(color.A) / 255.f});
  }
};

class GaussianBlurEffect : public CanvasEffect {
public:
  GaussianBlurEffect() : CanvasEffect{CLSID_D2D1GaussianBlur} {
    SetBlurAmount();
    SetOptimizationMode();
    SetBorderMode();
  }
  virtual ~GaussianBlurEffect() = default;

  void SetBlurAmount(float blurAmount = 3.f) {
    SetProperty(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, BoxValue(blurAmount));
  }
  void SetOptimizationMode(D2D1_GAUSSIANBLUR_OPTIMIZATION optimizationMode
                           = D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED) {
    SetProperty(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, BoxValue(optimizationMode));
  }
  void SetBorderMode(D2D1_BORDER_MODE borderMode = D2D1_BORDER_MODE_SOFT) {
    SetProperty(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, BoxValue(borderMode));
  }
};

class OpacityEffect : public CanvasEffect {
public:
  OpacityEffect() : CanvasEffect{CLSID_D2D1Opacity} { SetOpacity(); }
  virtual ~OpacityEffect() = default;

  void SetOpacity(float opacity = 1.0f) {
    SetProperty(D2D1_OPACITY_PROP_OPACITY, BoxValue(opacity));
  }
};

class SaturationEffect : public CanvasEffect {
public:
  SaturationEffect() : CanvasEffect{CLSID_D2D1Saturation} { SetSaturation(); }
  virtual ~SaturationEffect() = default;

  void SetSaturation(float saturation = 0.5f) {
    SetProperty(D2D1_SATURATION_PROP_SATURATION, BoxValue(saturation));
  }
};

class TintEffect : public CanvasEffect {
public:
  TintEffect() : CanvasEffect{CLSID_D2D1Tint} {
    SetClamp();
    SetColor();
  }
  virtual ~TintEffect() = default;

  void SetColor(const D2D1_COLOR_F& color = {0.f, 0.f, 0.f, 1.f}) {
    float value[]{color.r, color.g, color.b, color.a};
    SetProperty(D2D1_TINT_PROP_COLOR, BoxValue(value));
  }
  void SetColor(const winrt::Windows::UI::Color& color) {
    SetColor(D2D1_COLOR_F{static_cast<float>(color.R) / 255.f, static_cast<float>(color.G) / 255.f,
                          static_cast<float>(color.B) / 255.f,
                          static_cast<float>(color.A) / 255.f});
  }
  void SetClamp(bool clamp = false) { SetProperty(D2D1_TINT_PROP_CLAMP_OUTPUT, BoxValue(clamp)); }
};
}
} // namespace iris