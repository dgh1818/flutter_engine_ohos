/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_EXTERNAL_VIEW_EMBEDDER_EXTERNAL_VIEW_EMBEDDER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_EXTERNAL_VIEW_EMBEDDER_EXTERNAL_VIEW_EMBEDDER_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "flutter/common/task_runners.h"
#include "flutter/display_list/dl_builder.h"
#include "flutter/flow/embedded_views.h"
#include "flutter/flow/surface.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"

namespace flutter {

/// Allows to embed OpenHarmony native views into a Flutter application, in the
/// "Hybrid Composition++" (HCPP) fashion.
///
/// The native platform view corresponding to |flutter::PlatformViewLayer| is
/// hosted by ArkUI as an independent, system-composited layer
/// (BuilderNode RENDER_TYPE_DISPLAY). The Flutter content below the lowest
/// platform view is rendered to the main XComponent surface (the "background"
/// slice), and the Flutter content above platform views is composited to a
/// single transparent overlay XComponent surface (the "overlay" slice).
///
/// This mirrors the semantics of Android's |AndroidExternalViewEmbedder2|:
/// - SurfaceControl              -> ArkUI RENDER_TYPE_DISPLAY node
/// - overlay SurfaceControl      -> transparent overlay XComponent
/// - SurfaceFlinger              -> ArkUI RenderService
/// - JNI (onDisplayPlatformView) -> NAPI (OnDisplayPlatformViewHybrid)
///
class OHOSExternalViewEmbedder final : public ExternalViewEmbedder {
 public:
  OHOSExternalViewEmbedder(
      const std::shared_ptr<OHOSContext>& ohos_context,
      const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
      const std::shared_ptr<OhosSurfaceFactory>& surface_factory,
      const TaskRunners& task_runners);

  ~OHOSExternalViewEmbedder() override;

  void PrerollCompositeEmbeddedView(
      int64_t view_id,
      std::unique_ptr<flutter::EmbeddedViewParams> params) override;

  DlCanvas* CompositeEmbeddedView(int64_t view_id) override;

  void SubmitFlutterView(
      int64_t flutter_view_id,
      GrDirectContext* context,
      const std::shared_ptr<impeller::AiksContext>& aiks_context,
      std::unique_ptr<SurfaceFrame> frame) override;

  PostPrerollResult PostPrerollAction(
      const fml::RefPtr<fml::RasterThreadMerger>& raster_thread_merger)
      override;

  DlCanvas* GetRootCanvas() override;

  void BeginFrame(GrDirectContext* context,
                  const fml::RefPtr<fml::RasterThreadMerger>&
                      raster_thread_merger) override;

  void PrepareFlutterView(DlISize frame_size,
                          double device_pixel_ratio) override;

  void CancelFrame() override;

  void EndFrame(bool should_resubmit_frame,
                const fml::RefPtr<fml::RasterThreadMerger>&
                    raster_thread_merger) override;

  bool SupportsDynamicThreadMerging() override;

  void Teardown() override;

  // Registers/updates the overlay XComponent's native window. Called from the
  // NAPI layer once ArkUI has created the overlay surface. Passing nullptr
  // clears it (overlay disabled -> graceful degrade to no-overlay).
  void SetOverlayWindow(fml::RefPtr<OHOSNativeWindow> overlay_window);

  // Clears the registered overlay window and destroys the cached overlay
  // surfaces — everything that touches the overlay native window. Intended to
  // run on the raster thread (and be synchronously waited for by the caller)
  // when the overlay XComponent is being destroyed, so that the underlying
  // OHNativeWindow can be unreferenced right after without racing in-flight
  // raster uses.
  void TearDownOverlayWindow();

  // Gets the on-screen rect of a platform view, taking the mutator stack into
  // account, in physical pixels relative to the Flutter view.
  static DlRect GetViewRect(
      int64_t view_id,
      const std::unordered_map<int64_t, EmbeddedViewParams>& view_params);

  struct FoldedMutators {
    DlMatrix final_matrix;
    std::vector<DlRect> clip_rects;
    std::vector<DlRect> clip_rrect_bounds;
    std::vector<float> clip_rrect_radii;
    std::vector<DlRect> clip_path_bounds;
    std::vector<std::vector<double>> clip_path_commands;
    float opacity = 1.0f;
  };
  static FoldedMutators FoldMutatorsToFinal(
      const EmbeddedViewParams& params);

 private:
  const std::shared_ptr<OHOSContext> ohos_context_;

  // Allows to call methods in ArkTS via NAPI.
  const std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;

  // Allows to create GPU surfaces for the overlay layer.
  const std::shared_ptr<OhosSurfaceFactory> surface_factory_;

  const TaskRunners task_runners_;
  std::mutex overlay_mutex_;
  fml::RefPtr<OHOSNativeWindow> overlay_window_;
  std::atomic_bool overlay_window_dirty_{false};
  fml::RefPtr<OHOSNativeWindow> applied_overlay_window_;
  std::unique_ptr<OHOSSurface> overlay_ohos_surface_;
  std::unique_ptr<Surface> overlay_gpu_surface_;
  // Shared so PostTask lambdas can read/update it without capturing raw |this|.
  std::shared_ptr<std::atomic_bool> overlay_layer_is_shown_ =
      std::make_shared<std::atomic_bool>(false);
  bool overlay_had_content_last_frame_ = false;
  // Set while overlay regions are being dropped due to a missing overlay
  // surface; used to log the error once per drop window instead of per frame.
  bool overlay_drop_logged_ = false;

  // The size of the root canvas (physical pixels).
  DlISize frame_size_;

  // The device pixel ratio used to size platform view layers.
  double device_pixel_ratio_ = 1.0;

  // The order of composition. Each entry is a unique platform view id.
  std::vector<int64_t> composition_order_;

  // The EmbedderViewSlice keyed by platform view id.
  std::unordered_map<int64_t, std::unique_ptr<EmbedderViewSlice>> slices_;

  // The params (size, position, mutator stack) keyed by platform view id.
  std::unordered_map<int64_t, EmbeddedViewParams> view_params_;

  // Platform views visible in the last frame.
  std::unordered_set<int64_t> views_visible_last_frame_;

  // Resets per-frame state.
  void Reset();

  // Whether the layer tree in the current frame has platform layers.
  bool FrameHasPlatformLayers();

  Surface* EnsureOverlaySurface(GrDirectContext* context);

  // Destroys the overlay GPU surface (platform thread).
  void DestroyOverlaySurface();

  void ShowOverlayLayerIfNeeded();

  void HideOverlayLayerIfNeeded();

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalViewEmbedder);
};

// ============================================================================
// Multi-window (Windowing) embedder — one of two runtime-selected
// embedders (see PlatformViewOHOS::CreateExternalViewEmbedder).
// ============================================================================

// The always-valid root surface installed as the rasterizer's implicit
// surface. It only satisfies the stock rasterizer's root-surface contract
// (shared GPU context): AcquireFrame returns a token frame whose canvas
// is never the raster canvas (see Rasterizer::DrawToSurfaceUnsafe) and
// whose submit is a no-op.
class OhosEmbedderRootSurface : public Surface {
 public:
  explicit OhosEmbedderRootSurface(
      std::shared_ptr<impeller::Context> impeller_context);

  ~OhosEmbedderRootSurface() override = default;

  // |Surface|
  bool IsValid() override { return true; }

  // |Surface|
  std::unique_ptr<SurfaceFrame> AcquireFrame(const DlISize& size) override;

  // |Surface| (this backend does not support root surface transformations.)
  DlMatrix GetRootTransformation() const override { return {}; }

  // |Surface| (Impeller != Skia.)
  GrDirectContext* GetContext() override { return nullptr; }

  // |Surface|
  // Lazily created over the shared impeller context; per-view frames carry
  // their own contexts and never render through this one.
  std::shared_ptr<impeller::AiksContext> GetAiksContext() const override;

  // |Surface| (mirror GPUSurfaceVulkanImpeller: no raster cache on this
  // path.)
  bool EnableRasterCache() const override { return false; }

 private:
  std::shared_ptr<impeller::Context> impeller_context_;
  mutable std::shared_ptr<impeller::AiksContext> aiks_context_;
};

// OHOS multi-window ExternalViewEmbedder: owns one on-screen surface per
// window (including the implicit view's main window) and presents each view
// to its own window.
//
// The rasterizer records each view's layer tree onto the GetRootCanvas()
// DisplayListBuilder; SubmitFlutterView replays it onto that view's
// swapchain frame — one display-list indirection, not a new GPU pipeline.
//
// PrepareFlutterView receives frame size and DPR, but the view id only at
// SubmitFlutterView, so per-view recording state lives between the two
// calls.
//
// Threading: every override and the surface registry run on the raster task
// runner only.
class OHOSWindowingViewEmbedder : public ExternalViewEmbedder {
 public:
  OHOSWindowingViewEmbedder();

  ~OHOSWindowingViewEmbedder() override;

  // Registers the on-screen render surface for a view; a null surface
  // unregisters. Raster task runner only.
  void RegisterViewSurface(int64_t view_id, std::unique_ptr<Surface> surface);

  // Unregisters (and destroys) a view's surface. Raster task runner only;
  // must happen before the owning OHOSSurface is torn down.
  void UnregisterViewSurface(int64_t view_id);

  // |ExternalViewEmbedder|
  // The engine is discarding this view; drop its surface too.
  void CollectView(int64_t view_id) override;

  // |ExternalViewEmbedder|
  DlCanvas* GetRootCanvas() override;

  // |ExternalViewEmbedder|
  void CancelFrame() override;

  // |ExternalViewEmbedder|
  void BeginFrame(GrDirectContext* context,
                  const fml::RefPtr<fml::RasterThreadMerger>&
                      raster_thread_merger) override;

  // |ExternalViewEmbedder|
  // OHOS windows are pure Flutter content; no embedded platform views.
  void PrerollCompositeEmbeddedView(
      int64_t platform_view_id,
      std::unique_ptr<EmbeddedViewParams> params) override {}

  // |ExternalViewEmbedder|
  DlCanvas* CompositeEmbeddedView(int64_t platform_view_id) override {
    return nullptr;
  }

  // |ExternalViewEmbedder|
  void PrepareFlutterView(DlISize frame_size,
                          double device_pixel_ratio) override;

  // |ExternalViewEmbedder|
  void SubmitFlutterView(
      int64_t flutter_view_id,
      GrDirectContext* context,
      const std::shared_ptr<impeller::AiksContext>& aiks_context,
      std::unique_ptr<SurfaceFrame> frame) override;

  // |ExternalViewEmbedder|
  void EndFrame(bool should_resubmit_frame,
                const fml::RefPtr<fml::RasterThreadMerger>&
                    raster_thread_merger) override;

  // |ExternalViewEmbedder|
  void Teardown() override;

 private:
  // Per-view on-screen surfaces keyed by view id.
  std::unordered_map<int64_t, std::unique_ptr<Surface>> view_surfaces_;

  // Per-view recording state; reset in BeginFrame/EndFrame/CancelFrame.
  std::unique_ptr<DisplayListBuilder> pending_builder_;
  DlISize pending_size_ = {0, 0};
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_EXTERNAL_VIEW_EMBEDDER_EXTERNAL_VIEW_EMBEDDER_H_