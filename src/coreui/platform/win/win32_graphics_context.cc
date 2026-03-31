#include "win32_graphics_context.hpp"

#include "../../graphics_context.hpp"
#include "../../window.hpp"
#include "wincomp.hpp"
#include <windows.ui.composition.interop.h>

#include "log.hpp"

// skia
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include <cmath>

#include "winrt/windows.ui.composition.h"
#include <windows.ui.composition.interop.h>

#define BUFFERRED_FRAMES 2
#define ENABLE_DCOMP 0
#define ENABLE_WINCOMP 1
#define ENABLE_ACRYLIC 0

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <wrl/client.h>

#include "include/gpu/ganesh/d3d/GrD3DBackendContext.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowsapp.lib")

namespace iris {
namespace priv {
#ifdef _WIN32
enum WINDOWCOMPOSITIONATTRIB {
  WCA_UNDEFINED = 0x0,
  WCA_NCRENDERING_ENABLED = 0x1,
  WCA_NCRENDERING_POLICY = 0x2,
  WCA_TRANSITIONS_FORCEDISABLED = 0x3,
  WCA_ALLOW_NCPAINT = 0x4,
  WCA_CAPTION_BUTTON_BOUNDS = 0x5,
  WCA_NONCLIENT_RTL_LAYOUT = 0x6,
  WCA_FORCE_ICONIC_REPRESENTATION = 0x7,
  WCA_EXTENDED_FRAME_BOUNDS = 0x8,
  WCA_HAS_ICONIC_BITMAP = 0x9,
  WCA_THEME_ATTRIBUTES = 0xA,
  WCA_NCRENDERING_EXILED = 0xB,
  WCA_NCADORNMENTINFO = 0xC,
  WCA_EXCLUDED_FROM_LIVEPREVIEW = 0xD,
  WCA_VIDEO_OVERLAY_ACTIVE = 0xE,
  WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 0xF,
  WCA_DISALLOW_PEEK = 0x10,
  WCA_CLOAK = 0x11,
  WCA_CLOAKED = 0x12,
  WCA_ACCENT_POLICY = 0x13,
  WCA_FREEZE_REPRESENTATION = 0x14,
  WCA_EVER_UNCLOAKED = 0x15,
  WCA_VISUAL_OWNER = 0x16,
  WCA_HOLOGRAPHIC = 0x17,
  WCA_EXCLUDED_FROM_DDA = 0x18,
  WCA_PASSIVEUPDATEMODE = 0x19,
  WCA_LAST = 0x1A,
};
struct AccentPolicy {
  uint32_t AccentState;
  uint32_t AccentFlags;
  uint32_t GradientColor;
  uint32_t AnimationId;
};
struct WINDOWCOMPOSITIONATTRIBDATA {
  WINDOWCOMPOSITIONATTRIB Attrib;
  void* pvData;
  DWORD cbData; // size of data
};
struct Margins {
  int cxLeftWidth;
  int cxRightWidth;
  int cxTopHeight;
  int cxBottomHeight;
};
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
typedef BOOL(WINAPI* SetWindowCompositionAttribute)(HWND hwnd, WINDOWCOMPOSITIONATTRIBDATA* pwcad);
typedef HRESULT(WINAPI* DwmpCreateSharedThumbnailVisual)(
    HWND hwndDestination,
    HWND hwndSource,
    DWORD dwThumbnailFlags,
    DWM_THUMBNAIL_PROPERTIES* pThumbnailProperties,
    VOID* pDCompDevice,
    VOID** ppVisual,
    PHTHUMBNAIL phThumbnailId);
typedef HRESULT(WINAPI* DwmpCreateSharedMultiWindowVisual)(HWND hwndDestination,
                                                           VOID* pDCompDevice,
                                                           VOID** ppVisual,
                                                           PHTHUMBNAIL phThumbnailId);
typedef HRESULT(WINAPI* DwmpUpdateSharedMultiWindowVisual)(HTHUMBNAIL hThumbnailId,
                                                           HWND* phwndsInclude,
                                                           DWORD chwndsInclude,
                                                           HWND* phwndsExclude,
                                                           DWORD chwndsExclude,
                                                           RECT* prcSource,
                                                           SIZE* pDestinationSize,
                                                           DWORD dwFlags);
typedef HRESULT(WINAPI* DwmpCreateSharedVirtualDesktopVisual)(HWND hwndDestination,
                                                              VOID* pDCompDevice,
                                                              VOID** ppVisual,
                                                              PHTHUMBNAIL phThumbnailId);
typedef HRESULT(WINAPI* DwmpUpdateSharedVirtualDesktopVisual)(HTHUMBNAIL hThumbnailId,
                                                              HWND* phwndsInclude,
                                                              DWORD chwndsInclude,
                                                              HWND* phwndsExclude,
                                                              DWORD chwndsExclude,
                                                              RECT* prcSource,
                                                              SIZE* pDestinationSize);

DwmpCreateSharedThumbnailVisual DwmCreateSharedThumbnailVisual = nullptr;
DwmpCreateSharedMultiWindowVisual DwmCreateSharedMultiWindowVisual = nullptr;
DwmpUpdateSharedMultiWindowVisual DwmUpdateSharedMultiWindowVisual = nullptr;
DwmpCreateSharedVirtualDesktopVisual DwmCreateSharedVirtualDesktopVisual = nullptr;
DwmpUpdateSharedVirtualDesktopVisual DwmUpdateSharedVirtualDesktopVisual = nullptr;
SetWindowCompositionAttribute DwmSetWindowCompositionAttribute = nullptr;
RtlGetVersionPtr GetVersionInfo = nullptr;

void init_dwm_pri_api();
bool is_light_theme();
void set_app_theme(int value, HWND hwnd);
long get_os_build_ver();
void apply_win10_effect(int accent_state, int n_color, HWND hwnd);
bool apply_win11_mica(int type, HWND hwnd);
#endif
enum class backdrop_source { desktop, host_backdrop };

// associate with HWND
class dwm_compositor: public SkRefCnt {
public:
  dwm_compositor(HWND hwnd, gr_cp<IDXGISwapChain3> swapchain);

  bool set_effect(backdrop_source source);
  void exclude_from_preview(bool excl);
  bool create_backdrop(backdrop_source);

  gr_cp<IDCompositionVisual2> new_visual();

  void sync_pos(ui::window* wind);
  void commit();
  bool flush();
  void fallback();
  void restore();

private:
  bool create_fx();
  bool update_top_thumbnail();

  HWND hwnd_;

  gr_cp<IDCompositionTarget> dcomp_target_;
  gr_cp<IDCompositionVisual2> root_visual_;
  gr_cp<IDCompositionVisual2> fallback_visual_;
  gr_cp<IDXGISwapChain3> swap_chain_;

  HWND desktop_win_ = NULL;
  RECT desktop_win_rect_{};
  vector<HWND> hwnd_exclusion_list_;

  SIZE thumbnailSize{};
  RECT sourceRect{0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
  SIZE destinationSize{GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};

  HTHUMBNAIL desktop_thumb_ = NULL;
  HTHUMBNAIL top_win_thumb_ = NULL;
  DWM_THUMBNAIL_PROPERTIES thumb_props_{};
  gr_cp<IDCompositionVisual2> desktop_win_visual_;
  gr_cp<IDCompositionVisual2> top_win_visual_;

  gr_cp<IDCompositionGaussianBlurEffect> fx_blur_;
  gr_cp<IDCompositionSaturationEffect> fx_saturation_;
  gr_cp<IDCompositionTranslateTransform> fx_translate_;
  gr_cp<IDCompositionRectangleClip> fx_clip_;
};

struct gpu_device {
  gpu_device() {

    // Enable debug layer in debug builds
#if 0// defined(_DEBUG)
    gr_cp<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
      debugController->EnableDebugLayer();
    }
#endif

    UINT dxgiFactoryFlags = 0;
#if 0//defined(_DEBUG)
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    // Create DXGI Factory
    gr_cp<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)))) {
      XB_LOGE("Failed to create DXGI factory\n");
      return;
    }

    // Get adapter (you can specify which adapter to use)
    if (!getAdapter(factory.get(), &adapter)) {
      XB_LOGE("Failed to get adapter\n");
      return;
    }

    HRESULT hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    SUCCEEDED(hr);
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&command_queue));
    SUCCEEDED(hr);

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr))
      return;
    fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    GrD3DBackendContext backendContext;
    backendContext.fAdapter = adapter;
    backendContext.fDevice = device;
    backendContext.fQueue = command_queue;
    backendContext.fProtectedContext = GrProtected::kNo;

    // Create Skia GPU context
    dir_ctx = GrDirectContext::MakeDirect3D(backendContext);

#if ENABLE_DCOMP
    gr_cp<IDXGIDevice2> dxgi_dev;
    hr = device->QueryInterface(&dxgi_dev);
    if (FAILED(hr)) {
      hr = DCompositionCreateDevice3(nullptr, IID_PPV_ARGS(&dcomp_dev));
      if (FAILED(hr)) {
        return;
      }
    } else {
      hr = DCompositionCreateDevice3(dxgi_dev.get(), IID_PPV_ARGS(&dcomp_dev));
      if (FAILED(hr)) {
        return;
      }
    }
    hr = dcomp_dev->QueryInterface(&dcomp_dev3);
    if (FAILED(hr)) {
      return;
    }
#endif
  }

  ~gpu_device() {
    if (fence_event) {
      ::CloseHandle(fence_event);
      fence_event = nullptr;
    }
  }

  gr_cp<ID3D12Device> device;
  gr_cp<ID3D12CommandQueue> command_queue;
  gr_cp<ID3D12Fence> fence;
  gr_cp<IDXGIAdapter1> adapter;

  gr_cp<IDCompositionDesktopDevice> dcomp_dev;
  gr_cp<IDCompositionDevice3> dcomp_dev3;

  HANDLE fence_event = nullptr;
  sk_sp<GrDirectContext> dir_ctx;

  static gpu_device& get() {
    static gpu_device dev;
    return dev;
  }

  private:
  bool getAdapter(IDXGIFactory4* factory, IDXGIAdapter1** chosenAdapter) {
    if (SUCCEEDED(factory->EnumAdapters1(0, chosenAdapter))) {
      return true;
    }
    return findHardwareAdapter(factory, chosenAdapter);
  }

  bool findHardwareAdapter(IDXGIFactory4* factory, IDXGIAdapter1** chosenAdapter) {
    gr_cp<IDXGIAdapter1> adapter;
    *chosenAdapter = nullptr;

    for (UINT adapterIndex = 0;
         DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) {

      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);

      // Skip software adapters
      //if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      //  continue;
      //}

      // Check if the adapter supports D3D12
      if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device),
                                      nullptr))) {
        adapter->AddRef();
        *chosenAdapter = adapter.get();
        printf("Using adapter: %ls\n", desc.Description);
        return true;
      }
    }

    return false;
  }
};

class graphics_context {
public:
  graphics_context(ui::window* w, bool gpu);
  ~graphics_context();

  sk_sp<SkSurface> current();
  SkCanvas* canvas();

  void resize(int width, int height);
  void flush();
  void synchronize(const ui::rect* dirty_rect);
  ui::rect expand_dirty_rect(const ui::rect& dirty_rect) const;

  void on_window_pos_changed();
  void on_window_activate(bool active);
  void on_live_resize_changed(bool active);
  void shutdown();
  void flush_dwm_compositor();
  ui::window* window() const { return wind_; }

private:
  ui::window* wind_;
  DXGI_FORMAT pixel_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
  UINT64 fence_value_ = 0;

#if ENABLE_WINCOMP
  winrt::Windows::UI::Composition::CompositionTarget comp_target_{nullptr};
  winrt::Windows::UI::Composition::ContainerVisual root_visual_{nullptr};
  winrt::Windows::UI::Composition::SpriteVisual blur_visual_{nullptr};
  winrt::Windows::UI::Composition::SpriteVisual content_visual_{nullptr};
  winrt::Windows::UI::Composition::CompositionClip clip_{nullptr};
  winrt::Windows::UI::Composition::CompositionRoundedRectangleGeometry clip_geom_{nullptr};
  bool window_active_ = true;
  bool live_resizing_ = false;
#endif
  sk_sp<dwm_compositor> compositor_;
  sk_sp<ui::win::compositor> win_compositor_;
  bool uses_window_compositor_ = false;
  gr_cp<IDXGISwapChain3> swap_chain_;

  UINT frame_index_ = 0;
  UINT width_ = 0, height_ = 0;
  vector<sk_sp<SkSurface>> swap_surfaces_;
  RECT backbuffer_damage_[BUFFERRED_FRAMES]{};
  bool backbuffer_damage_valid_[BUFFERRED_FRAMES]{};

  friend class ui::graphics_context;
};

namespace {
constexpr float kForceFullDamageRatio = 0.85f;

RECT make_bounds_rect(UINT width, UINT height) {
  RECT r{};
  r.left = 0;
  r.top = 0;
  r.right = static_cast<LONG>(width);
  r.bottom = static_cast<LONG>(height);
  return r;
}

bool is_empty_rect(const RECT& r) {
  return r.right <= r.left || r.bottom <= r.top;
}

RECT intersect_rect(const RECT& a, const RECT& b) {
  RECT r{};
  r.left = max(a.left, b.left);
  r.top = max(a.top, b.top);
  r.right = min(a.right, b.right);
  r.bottom = min(a.bottom, b.bottom);
  if (is_empty_rect(r)) {
    return RECT{};
  }
  return r;
}

RECT union_rect(const RECT& a, const RECT& b) {
  if (is_empty_rect(a)) {
    return b;
  }
  if (is_empty_rect(b)) {
    return a;
  }
  RECT r{};
  r.left = min(a.left, b.left);
  r.top = min(a.top, b.top);
  r.right = max(a.right, b.right);
  r.bottom = max(a.bottom, b.bottom);
  return r;
}

float rect_area(const RECT& r) {
  if (is_empty_rect(r)) {
    return 0.0f;
  }
  return static_cast<float>(r.right - r.left) * static_cast<float>(r.bottom - r.top);
}

RECT sanitize_dirty_rect(const ui::rect& dirty_rect, const RECT& bounds) {
  RECT r{};
  r.left = static_cast<LONG>(floorf(dirty_rect.left));
  r.top = static_cast<LONG>(floorf(dirty_rect.top));
  r.right = static_cast<LONG>(ceilf(dirty_rect.right()));
  r.bottom = static_cast<LONG>(ceilf(dirty_rect.bottom()));
  return intersect_rect(r, bounds);
}

ui::rect to_ui_rect(const RECT& r) {
  if (is_empty_rect(r)) {
    return {};
  }
  return ui::rect{static_cast<float>(r.left), static_cast<float>(r.top),
                  static_cast<float>(r.right - r.left), static_cast<float>(r.bottom - r.top)};
}
} // namespace
} // namespace priv

namespace priv {
graphics_context::graphics_context(ui::window* w, bool gpu) : wind_(w) {
  if (gpu) {
    auto& gpu_dev = gpu_device::get();
    uses_window_compositor_ = w->uses_window_compositor();
    auto frame = w->frame();
    width_ = static_cast<UINT>(max(frame.width, 0.0f));
    height_ = static_cast<UINT>(max(frame.height, 0.0f));
    for (UINT i = 0; i < BUFFERRED_FRAMES; ++i) {
      backbuffer_damage_[i] = RECT{};
      backbuffer_damage_valid_[i] = false;
    }
    gr_cp<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    if (uses_window_compositor_) {
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
      swapChainDesc1.BufferCount = BUFFERRED_FRAMES;
      swapChainDesc1.Width = frame.width;
      swapChainDesc1.Height = frame.height;
      swapChainDesc1.Format = pixel_format_;
      swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      swapChainDesc1.SampleDesc.Count = 1;
      swapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
      gr_cp<IDXGISwapChain1> swapChain;
      auto hr = factory->CreateSwapChainForComposition(gpu_dev.command_queue.get(), &swapChainDesc1,
                                                       nullptr, &swapChain);
      if (FAILED(hr))
        return;
      swap_chain_.reset();
      swapChain->QueryInterface(&swap_chain_);

#if ENABLE_DCOMP
      compositor_.reset(new dwm_compositor((HWND)wind_->native_handle(), swap_chain_));
      if (w->is_main()) {
        compositor_->set_effect(backdrop_source::host_backdrop);
      }
#else
      win_compositor_.reset(new ui::win::compositor);
      auto [comp_target, clip, geometry, root_visual, blur_visual, content_visual]
          = win_compositor_->create_composite_window(wind_, swap_chain_.get());
      comp_target_ = comp_target;
      clip_ = clip;
      clip_geom_ = geometry;
      root_visual_ = root_visual;
      blur_visual_ = blur_visual;
      content_visual_ = content_visual;
#endif
    } else {
      DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
      swapChainDesc.BufferCount = BUFFERRED_FRAMES;
      swapChainDesc.BufferDesc.Width = frame.width;
      swapChainDesc.BufferDesc.Height = frame.height;
      swapChainDesc.BufferDesc.Format = pixel_format_;
      swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
      swapChainDesc.OutputWindow = (HWND)w->native_handle();
      swapChainDesc.SampleDesc.Count = 1;
      swapChainDesc.Windowed = TRUE;
      gr_cp<IDXGISwapChain> tempSwapChain;
      auto hr = factory->CreateSwapChain(gpu_dev.command_queue.get(), &swapChainDesc, &tempSwapChain);
      if (FAILED(hr))
        return;
      tempSwapChain->QueryInterface(&swap_chain_);
    }

    // This sample does not support fullscreen transitions.
    factory->MakeWindowAssociation((HWND)w->native_handle(), DXGI_MWA_NO_ALT_ENTER);

    swap_surfaces_.resize(BUFFERRED_FRAMES);
    SkSurfaceProps props = SkSurfaceProps();
    for (UINT i = 0; i < BUFFERRED_FRAMES; i++) {
      ID3D12Resource* render_target = nullptr;
      HRESULT hr = swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_target));
      if (FAILED(hr))
        return;
      GrD3DTextureResourceInfo info;
      info.fResource.reset(render_target);
      info.fResourceState = D3D12_RESOURCE_STATE_PRESENT;
      info.fFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
      info.fSampleCount = 1;
      info.fLevelCount = 1;
      GrBackendRenderTarget backendRT(frame.width, frame.height, info);
      swap_surfaces_[i]
          = SkSurfaces::WrapBackendRenderTarget(gpu_dev.dir_ctx.get(), backendRT,
                                                kTopLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
                                                nullptr, &props);
    }
    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
  }
}

graphics_context::~graphics_context() {
  shutdown();
}

sk_sp<SkSurface> graphics_context::current() {
  return swap_surfaces_[frame_index_];
}

void graphics_context::resize(int width, int height) {
  if (swap_chain_) {
    auto& gpu_dev = gpu_device::get();
    swap_surfaces_.clear();
    swap_surfaces_.resize(BUFFERRED_FRAMES);
    width_ = static_cast<UINT>(max(width, 0));
    height_ = static_cast<UINT>(max(height, 0));
    for (UINT i = 0; i < BUFFERRED_FRAMES; ++i) {
      backbuffer_damage_[i] = RECT{};
      backbuffer_damage_valid_[i] = false;
    }

    // need sync CPU
    GrFlushInfo flushInfo;
    gpu_dev.dir_ctx->flush();
    gpu_dev.dir_ctx->submit(GrSyncCpu::kYes);

    {
      const UINT64 currentFenceValue = fence_value_;
      gpu_dev.command_queue->Signal(gpu_dev.fence.get(), currentFenceValue);

      if (gpu_dev.fence->GetCompletedValue() < currentFenceValue) {
        gpu_dev.fence->SetEventOnCompletion(currentFenceValue, gpu_dev.fence_event);
        ::WaitForSingleObject(gpu_dev.fence_event, INFINITE);
      }
      fence_value_++;
    }
    HRESULT hr = swap_chain_->ResizeBuffers(BUFFERRED_FRAMES, width, height, pixel_format_,
                                            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (FAILED(hr)) {
      XB_LOGI("failed to resizeBuffers");
      return;
    }

    for (UINT i = 0; i < BUFFERRED_FRAMES; i++) {
      ID3D12Resource* render_target = nullptr;
      hr = swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_target));
      if (FAILED(hr))
        return;

      GrD3DTextureResourceInfo info;
      info.fResource.reset(render_target);
      info.fResourceState = D3D12_RESOURCE_STATE_PRESENT;
      info.fFormat = pixel_format_;
      info.fSampleCount = 1;
      info.fLevelCount = 1;
      
      GrBackendRenderTarget backendRT(width, height, info);
      swap_surfaces_[i]
          = SkSurfaces::WrapBackendRenderTarget(gpu_dev.dir_ctx.get(), backendRT,
                                                kTopLeft_GrSurfaceOrigin,
                                                kRGBA_8888_SkColorType, nullptr, nullptr);
    }

    {
      const UINT64 currentFenceValue = fence_value_;
      gpu_dev.command_queue->Signal(gpu_dev.fence.get(), currentFenceValue);

      if (gpu_dev.fence->GetCompletedValue() < currentFenceValue) {
        gpu_dev.fence->SetEventOnCompletion(currentFenceValue, gpu_dev.fence_event);
        WaitForSingleObject(gpu_dev.fence_event, INFINITE);
      }
      fence_value_++;
    }

    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
    if (uses_window_compositor_ && root_visual_ && clip_geom_) {
      root_visual_.Size({(float)width, (float)height});
      winrt::Windows::Foundation::Numerics::float2 content_size{width - 2 * wind_->shadow_distance(),
                                                                height
                                                                    - 2 * wind_->shadow_distance()};
      if (content_size.x < 0.f) {
        content_size.x = 1.f;
      }
      if (content_size.y < 0.f) {
        content_size.y = 1.f;
      }
      clip_geom_.Size(content_size);
      //clip_geom_.Offset();
      clip_geom_.CornerRadius({0, 0});
    }

    //::PostMessage((HWND)wind_->native_handle(), WM_PAINT, 0, 0);

    XB_LOGI("resize %dx%d", width, height);
  }
}

ui::rect graphics_context::expand_dirty_rect(const ui::rect& dirty_rect) const {
  const RECT bounds = make_bounds_rect(width_, height_);
  RECT expanded = sanitize_dirty_rect(dirty_rect, bounds);
  if (is_empty_rect(expanded)) {
    return {};
  }
  if (!backbuffer_damage_valid_[frame_index_]) {
    return to_ui_rect(bounds);
  }

  for (UINT i = 0; i < BUFFERRED_FRAMES; ++i) {
    if (i == frame_index_ || !backbuffer_damage_valid_[i]) {
      continue;
    }
    expanded = union_rect(expanded, intersect_rect(backbuffer_damage_[i], bounds));
  }

  const float total_area = rect_area(bounds);
  if (total_area > 0.0f && rect_area(expanded) >= total_area * kForceFullDamageRatio) {
    expanded = bounds;
  }
  return to_ui_rect(expanded);
}

void graphics_context::synchronize(const ui::rect* dirty_rect) {
  if (swap_chain_) {
    auto& gpu_dev = gpu_device::get();
    const RECT bounds = make_bounds_rect(width_, height_);
    const bool force_full_present = (dirty_rect == nullptr) || !backbuffer_damage_valid_[frame_index_];
    RECT present_damage = bounds;
    if (!force_full_present) {
      present_damage = sanitize_dirty_rect(*dirty_rect, bounds);
      if (is_empty_rect(present_damage)) {
        return;
      }
    }

    HRESULT present_hr = S_OK;
    if (!force_full_present) {
      DXGI_PRESENT_PARAMETERS present_params{};
      present_params.DirtyRectsCount = 1;
      present_params.pDirtyRects = &present_damage;
      present_hr = swap_chain_->Present1(1, 0, &present_params);
      if (FAILED(present_hr)) {
        present_hr = swap_chain_->Present(1, 0);
      }
    } else {
      present_hr = swap_chain_->Present(1, 0);
    }
    if (FAILED(present_hr)) {
      return;
    }

    backbuffer_damage_[frame_index_] = present_damage;
    backbuffer_damage_valid_[frame_index_] = true;

    const UINT64 currentFenceValue = fence_value_;

    gpu_dev.command_queue->Signal(gpu_dev.fence.get(), currentFenceValue);

    if (gpu_dev.fence->GetCompletedValue() < currentFenceValue) {
      gpu_dev.fence->SetEventOnCompletion(currentFenceValue, gpu_dev.fence_event);
      WaitForSingleObject(gpu_dev.fence_event, INFINITE);
    }
    fence_value_++;

    frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
    //XB_LOGI("Present");
  }
}

void graphics_context::flush_dwm_compositor() {

}
void graphics_context::flush() {
  auto current_s = current();
  auto& gpu_dev = gpu_device::get();

  GrFlushInfo flushInfo;
  gpu_dev.dir_ctx->flush(current_s.get(), SkSurfaces::BackendSurfaceAccess::kPresent, flushInfo);
  gpu_dev.dir_ctx->submit();
  //XB_LOGI("Flush");
}
SkCanvas* graphics_context::canvas() {
  auto current_s = current();
  return current_s->getCanvas();
}

void graphics_context::on_window_pos_changed() {
  //XB_LOGI("window pos change");
  bool is_max = !!::IsZoomed((HWND)wind_->native_handle());
#if ENABLE_WINCOMP
  if (uses_window_compositor_ && blur_visual_) {
    blur_visual_.IsVisible(!is_max && window_active_ /*&& !live_resizing_*/);
  }
  //clip_geom_.CornerRadius({0.f,0.f});
#else
  if (compositor_) {
    if (is_max) {
      compositor_->fallback();
      compositor_->flush();
    } else {
      compositor_->restore();
      compositor_->sync_pos(wind_);
    }
  }
#endif
}
void graphics_context::on_window_activate(bool active) {
  XB_LOGI("activate : %d", active? 1: 0);
#if ENABLE_WINCOMP
  window_active_ = active;
  if (!active || !uses_window_compositor_ || !blur_visual_) {
    return;
  }
  bool is_max = !!::IsZoomed((HWND)wind_->native_handle());
  blur_visual_.IsVisible(!is_max && window_active_ && !live_resizing_);
#else
  // flush compositor
  if (compositor_) {
    if (active && !is_max) {
      compositor_->restore();
      compositor_->flush();
    } else {
      compositor_->fallback();
    }
  }
#endif
}

void graphics_context::on_live_resize_changed(bool active) {
#if ENABLE_WINCOMP
  live_resizing_ = active;
  if (!uses_window_compositor_ || !blur_visual_) {
    return;
  }
  bool is_max = !!::IsZoomed((HWND)wind_->native_handle());
  blur_visual_.IsVisible(!is_max && window_active_/* && !live_resizing_*/);
#else
  (void)active;
#endif
}

void graphics_context::shutdown() {
  if (!swap_chain_) {
    return;
  }

  auto& gpu_dev = gpu_device::get();
  if (gpu_dev.dir_ctx) {
    gpu_dev.dir_ctx->flush();
    gpu_dev.dir_ctx->submit(GrSyncCpu::kYes);
  }

  if (gpu_dev.command_queue && gpu_dev.fence) {
    const UINT64 currentFenceValue = fence_value_;
    gpu_dev.command_queue->Signal(gpu_dev.fence.get(), currentFenceValue);
    if (gpu_dev.fence->GetCompletedValue() < currentFenceValue) {
      gpu_dev.fence->SetEventOnCompletion(currentFenceValue, gpu_dev.fence_event);
      ::WaitForSingleObject(gpu_dev.fence_event, INFINITE);
    }
    fence_value_++;
  }

  swap_surfaces_.clear();
  compositor_.reset();
  win_compositor_.reset();
#if ENABLE_WINCOMP
  content_visual_ = nullptr;
  blur_visual_ = nullptr;
  root_visual_ = nullptr;
  clip_ = nullptr;
  clip_geom_ = nullptr;
  comp_target_ = nullptr;
#endif
  swap_chain_.reset();
}
dwm_compositor::dwm_compositor(HWND hwnd, gr_cp<IDXGISwapChain3> swapchain)
  : hwnd_(hwnd), swap_chain_(swapchain) {
  auto& gpu_dev = gpu_device::get();
  dcomp_target_.reset();
  auto& dcomp_dev = gpu_dev.dcomp_dev;
  dcomp_dev->CreateTargetForHwnd(hwnd, true, &dcomp_target_);
  root_visual_ = new_visual();
  root_visual_->SetContent(swapchain.get());
  dcomp_target_->SetRoot(root_visual_.get());
  commit();
#if ENABLE_ACRYLIC
  create_fx();
#endif
}
#define DWM_TNP_FREEZE 0x100000
#define DWM_TNP_ENABLE3D 0x4000000
#define DWM_TNP_DISABLE3D 0x8000000
#define DWM_TNP_FORCECVI 0x40000000
#define DWM_TNP_DISABLEFORCECVI 0x80000000
bool dwm_compositor::create_fx() {
  auto& gpu_dev = gpu_device::get();
  auto& dcomp_dev3 = gpu_dev.dcomp_dev3;
  if (dcomp_dev3->CreateGaussianBlurEffect(&fx_blur_) != S_OK) {
    return false;
  }
  if (dcomp_dev3->CreateSaturationEffect(&fx_saturation_) != S_OK) {
    return false;
  }
  if (dcomp_dev3->CreateTranslateTransform(&fx_translate_) != S_OK) {
    return false;
  }
  if (dcomp_dev3->CreateRectangleClip(&fx_clip_) != S_OK) {
    return false;
  }
  return true;
}
bool dwm_compositor::update_top_thumbnail() {
  HRESULT hr = S_OK;
  if (get_os_build_ver() >= 20000) {
    hr = DwmUpdateSharedMultiWindowVisual(top_win_thumb_, NULL, 0, hwnd_exclusion_list_.data(),
                                          hwnd_exclusion_list_.size(),
                                          &sourceRect, &destinationSize, 1);
  } else {
    hr = DwmUpdateSharedVirtualDesktopVisual(top_win_thumb_, NULL, 0, hwnd_exclusion_list_.data(),
                                             hwnd_exclusion_list_.size(), &sourceRect,
                                             &destinationSize);
  }
  return hr == S_OK;
}
bool dwm_compositor::set_effect(backdrop_source source) {
#if ENABLE_ACRYLIC
  if (source == backdrop_source::host_backdrop) {
    exclude_from_preview(true);
  }
  create_backdrop(source);
  fallback_visual_ = new_visual();
  fallback_visual_->SetContent(swap_chain_.get());

  root_visual_->RemoveAllVisuals();
  switch (source) {
  case backdrop_source::desktop:
    root_visual_->AddVisual(desktop_win_visual_.get(), false, NULL);
    root_visual_->AddVisual(fallback_visual_.get(), true, desktop_win_visual_.get());
    break;
  case backdrop_source::host_backdrop:
    root_visual_->AddVisual(desktop_win_visual_.get(), false, NULL);
    root_visual_->AddVisual(top_win_visual_.get(), true, desktop_win_visual_.get());
    root_visual_->AddVisual(fallback_visual_.get(), true, top_win_visual_.get());
    break;
  default:
    root_visual_->RemoveAllVisuals();
    break;
  }
  fx_saturation_->SetSaturation(2 /*params.saturationAmount*/);
  fx_blur_->SetBorderMode(D2D1_BORDER_MODE_HARD);
  fx_blur_->SetInput(0, fx_saturation_.get(), 0);
  fx_blur_->SetStandardDeviation(40 /*params.blurAmount*/);

  desktop_win_visual_->SetEffect(fx_blur_.get());
  top_win_visual_->SetEffect(fx_blur_.get());
  dcomp_target_->SetRoot(root_visual_.get());
  commit();
  //sync_pos(wind_);
#endif
  return false;
}
void dwm_compositor::exclude_from_preview(bool excl) {
  BOOL enable = excl ? TRUE : FALSE;
  WINDOWCOMPOSITIONATTRIBDATA composition_attribute{};
  composition_attribute.Attrib = WCA_EXCLUDED_FROM_LIVEPREVIEW;
  composition_attribute.pvData = &enable;
  composition_attribute.cbData = sizeof(BOOL);
  DwmSetWindowCompositionAttribute(hwnd_, &composition_attribute);
}
bool dwm_compositor::create_backdrop(backdrop_source src) {
#if ENABLE_ACRYLIC
  auto& gpu_dev = gpu_device::get();
  auto& dcomp_dev_desktop_ = gpu_dev.dcomp_dev;
  switch (src) {
  case backdrop_source::desktop:
    desktop_win_ = (HWND)FindWindowW(L"Progman", NULL);
    ::GetWindowRect(desktop_win_, &desktop_win_rect_);
    thumbnailSize.cx = (desktop_win_rect_.right - desktop_win_rect_.left);
    thumbnailSize.cy = (desktop_win_rect_.bottom - desktop_win_rect_.top);
    thumb_props_.dwFlags = DWM_TNP_SOURCECLIENTAREAONLY | DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION
                        | DWM_TNP_RECTSOURCE | DWM_TNP_OPACITY | DWM_TNP_ENABLE3D;
    thumb_props_.opacity = 255;
    thumb_props_.fVisible = TRUE;
    thumb_props_.fSourceClientAreaOnly = FALSE;
    thumb_props_.rcDestination = RECT{0, 0, thumbnailSize.cx, thumbnailSize.cy};
    thumb_props_.rcSource = RECT{0, 0, thumbnailSize.cx, thumbnailSize.cy};
    if (DwmCreateSharedThumbnailVisual(hwnd_, desktop_win_, 2, &thumb_props_,
                                       dcomp_dev_desktop_.get(),
                                       (void**)&desktop_win_visual_, &desktop_thumb_)
        != S_OK) {
      return false;
    }
    break;
  case backdrop_source::host_backdrop: {
    HRESULT hr = S_OK;
    if (get_os_build_ver() >= 20000) {
      hr = DwmCreateSharedMultiWindowVisual(hwnd_, dcomp_dev_desktop_.get(),
                                            (void**)&top_win_visual_, &top_win_thumb_);
    } else {
      hr = DwmCreateSharedVirtualDesktopVisual(hwnd_, dcomp_dev_desktop_.get(),
                                               (void**)&top_win_visual_, &top_win_thumb_);
    }
    if (!create_backdrop(backdrop_source::desktop) || hr != S_OK) {
      return false;
    }
    hwnd_exclusion_list_.resize(1);
    hwnd_exclusion_list_[0] = (HWND)hwnd_;
    if (!update_top_thumbnail()) {
      return false;
    }
    break;
  }
  }
#endif
  return true;
}
gr_cp<IDCompositionVisual2> dwm_compositor::new_visual() {
  auto& gpu_dev = gpu_device::get();
  auto& dcomp_dev = gpu_dev.dcomp_dev;
  gr_cp<IDCompositionVisual2> visual;
  dcomp_dev->CreateVisual(&visual);
  return visual;
}
void dwm_compositor::sync_pos(ui::window* wind) {
#if ENABLE_ACRYLIC
  if (!top_win_visual_ || !desktop_win_visual_)
    return;
  bool is_max = !!::IsZoomed((HWND)wind->native_handle());
  RECT hostWindowRect{};
  ::GetWindowRect((HWND)wind->native_handle(), &hostWindowRect);
  fx_clip_->SetLeft((float)hostWindowRect.left + (is_max ? 0 : wind->shadow_distance()));
  fx_clip_->SetRight((float)hostWindowRect.right - (is_max ? 0 : (wind->shadow_distance()/*2*/)));
  fx_clip_->SetTop((float)hostWindowRect.top + (is_max ? 0 : wind->shadow_distance()));
  fx_clip_->SetBottom((float)hostWindowRect.bottom - (is_max ? 0 : (wind->shadow_distance() /*2*/)));

  fx_clip_->SetTopLeftRadiusX(wind->corner_radius());
  fx_clip_->SetTopLeftRadiusY(wind->corner_radius());

  fx_clip_->SetTopRightRadiusX(wind->corner_radius());
  fx_clip_->SetTopRightRadiusY(wind->corner_radius());

  fx_clip_->SetBottomLeftRadiusX(wind->corner_radius());
  fx_clip_->SetBottomLeftRadiusY(wind->corner_radius());

  fx_clip_->SetBottomRightRadiusX(wind->corner_radius());
  fx_clip_->SetBottomRightRadiusY(wind->corner_radius());

  top_win_visual_->SetClip(fx_clip_.get());
  desktop_win_visual_->SetClip(fx_clip_.get());
  fx_translate_->SetOffsetX(
      -1 * (float)hostWindowRect.left);
  fx_translate_->SetOffsetY(-1 * (float)hostWindowRect.top);
  top_win_visual_->SetTransform(fx_translate_.get());
  desktop_win_visual_->SetTransform(fx_translate_.get());
  commit();
  DwmFlush();
#endif
}
void dwm_compositor::commit() {
  auto& gpu_dev = gpu_device::get();
  auto& dcomp_dev = gpu_dev.dcomp_dev;
  dcomp_dev->Commit();
}

bool dwm_compositor::flush() {
  if (!top_win_visual_)
    return false;
#if ENABLE_ACRYLIC
  if (top_win_thumb_ != NULL) {
    update_top_thumbnail();
    DwmFlush();
  }
#endif
  return true;
}

void dwm_compositor::fallback() {
  if (!fallback_visual_)
    return;
#if ENABLE_ACRYLIC
  root_visual_->RemoveAllVisuals();
  root_visual_->AddVisual(fallback_visual_.get(), false, NULL);
  commit();
#endif
}

void dwm_compositor::restore() {
  if (!fallback_visual_ || !desktop_win_visual_ || !top_win_visual_)
    return;
#if ENABLE_ACRYLIC
  root_visual_->RemoveAllVisuals();
  root_visual_->AddVisual(desktop_win_visual_.get(), false, NULL);
  root_visual_->AddVisual(top_win_visual_.get(), true, desktop_win_visual_.get());
  root_visual_->AddVisual(fallback_visual_.get(), true, top_win_visual_.get());
  commit();
#endif
}

void init_dwm_pri_api() {
    using namespace winrt::Windows::UI::Composition;
  winrt::init_apartment();

  auto dwmapi = LoadLibraryW(L"dwmapi.dll");
  auto user32 = LoadLibraryW(L"user32.dll");
  auto ntdll = GetModuleHandleW(L"ntdll.dll");
  if (!dwmapi || !user32 || !ntdll) {
    return;
  }

  GetVersionInfo = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
  DwmSetWindowCompositionAttribute
      = (SetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");
  DwmCreateSharedThumbnailVisual
      = (DwmpCreateSharedThumbnailVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(147));
  DwmCreateSharedMultiWindowVisual
      = (DwmpCreateSharedMultiWindowVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(163));
  DwmUpdateSharedMultiWindowVisual
      = (DwmpUpdateSharedMultiWindowVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(164));
  DwmCreateSharedVirtualDesktopVisual
      = (DwmpCreateSharedVirtualDesktopVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(163));
  DwmUpdateSharedVirtualDesktopVisual
      = (DwmpUpdateSharedVirtualDesktopVisual)GetProcAddress(dwmapi, MAKEINTRESOURCEA(164));
}

long get_os_build_ver() {
  RTL_OSVERSIONINFOW versionInfo = {0};
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (GetVersionInfo(&versionInfo) == 0x00000000) {
    return versionInfo.dwBuildNumber;
  }
  return 0;
}

bool is_light_theme() {
  // based on
  // https://stackoverflow.com/questions/51334674/how-to-detect-windows-10-light-dark-mode-in-win32-application

  // The value is expected to be a REG_DWORD, which is a signed 32-bit little-endian
  auto buffer = std::vector<char>(4);
  auto cb_data = static_cast<DWORD>(buffer.size() * sizeof(char));
  auto res = RegGetValueW(HKEY_CURRENT_USER,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                          L"AppsUseLightTheme",
                          RRF_RT_REG_DWORD, // expected value type
                          nullptr, buffer.data(), &cb_data);
  if (res != ERROR_SUCCESS) {
    return false;
  }
  // convert bytes written to our buffer to an int, assuming little-endian
  auto i = int(buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0]);
  return i == 1;
}

void set_app_theme(int value, HWND hwnd) {
  int use_dark_theme = 0x00;
  // if dark mod, apply dark effect
  if (value == 1 /* DARK */ || (value == 5 /* AUTO */ && !is_light_theme()))
    use_dark_theme = 0x01;
  ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark_theme, sizeof(int));
}
/*
    TRANSPARENT: 2,
    BLURBEHIND: 3,
    ACRYLIC: 4
*/
void apply_win10_effect(int accent_state, int n_color, HWND hwnd) {
  struct ACCENTPOLICY {
    int nAccentState;
    int nFlags;
    int nColor;
    int nAnimationId;
  };
  struct WINCOMATTRPDATA {
    int nAttribute;
    PVOID pData;
    ULONG ulDataSize;
  };
  ACCENTPOLICY policy;
  policy.nAccentState = accent_state;
  policy.nFlags = 2;
  policy.nColor = n_color;
  policy.nAnimationId = 0;

  WINCOMATTRPDATA data;
  data.nAttribute = 19;
  data.pData = &policy;
  data.ulDataSize = sizeof(policy);

  DwmSetWindowCompositionAttribute(hwnd, (WINDOWCOMPOSITIONATTRIBDATA*)&data);
}

#define DWMWA_MICA_EFFECT DWORD(1029)
#define DWMWA_SYSTEMBACKDROP_TYPE DWORD(38)
#define DWMWA_USE_IMMERSIVE_DARK_MODE DWORD(20)
#define DWMWA_WINDOW_CORNER_PREFERENCE DWORD(33)
#define DWMWA_BORDER_COLOR DWORD(34)
#define DWMWA_CAPTION_COLOR DWORD(35)
#define DWMWA_TEXT_COLOR DWORD(36)
/*
        AUTO: 0,
        NONE: 1,
        ACRYLIC: 3,         // Acrylic
        MICA: 2,            // Mica
        TABBED_MICA: 4      // Mica tabbed
*/
bool apply_win11_mica(int type, HWND hwnd) {
  long ver = get_os_build_ver();
  if (ver >= 22000) {
    if (ver < 22523)
      ::DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(int));
    else if (type > 2)
      return false;
    else {
      int micaEnable = 0x00;
      if (type == 1)
        micaEnable = 0x02;
      else if (type == 2)
        micaEnable = 0x01;
      ::DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &micaEnable, sizeof(int));
    }
  }
  return true;
}

} // namespace priv

namespace platform::win32 {

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

} // namespace platform::win32
} // namespace iris
