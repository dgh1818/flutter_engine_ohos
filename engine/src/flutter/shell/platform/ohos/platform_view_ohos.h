/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_VIEW_OHOS_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_VIEW_OHOS_H_

#include <atomic>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <multimedia/image_framework/image_mdk.h>
#include <multimedia/image_framework/image_pixel_map_mdk.h>

#include "flutter/fml/memory/weak_ptr.h"
#include "flutter/fml/task_runner.h"
#include "flutter/fml/time/time_point.h"
#include "flutter/lib/ui/window/platform_message.h"
#include "flutter/shell/common/platform_view.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_bridge.h"
#include "flutter/shell/platform/ohos/background_resource_cleanup.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_external_texture_gl.h"
#include "flutter/shell/platform/ohos/platform_message_handler_ohos.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/surface/ohos_snapshot_surface_producer.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"
#include "flutter/shell/platform/ohos/vsync_waiter_ohos.h"

namespace flutter {

enum class OhosThreadType {
  kPlatform,
  kUI,
  kRaster,
  kIO,
};

class OhosSurfaceFactoryImpl : public OhosSurfaceFactory {
 public:
  OhosSurfaceFactoryImpl(const std::shared_ptr<OHOSContext>& context);

  ~OhosSurfaceFactoryImpl() override;

  std::unique_ptr<OHOSSurface> CreateSurface() override;

 private:
  const std::shared_ptr<OHOSContext>& ohos_context_;
};

class PlatformViewOHOS final : public PlatformView {
 public:
  PlatformViewOHOS(PlatformView::Delegate& delegate,
                   const flutter::TaskRunners& task_runners,
                   const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
                   bool use_software_rendering);

  PlatformViewOHOS(PlatformView::Delegate& delegate,
                   const flutter::TaskRunners& task_runners,
                   const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
                   const std::shared_ptr<flutter::OHOSContext>& OHOS_context);

  ~PlatformViewOHOS() override;

  void NotifyCreate(fml::RefPtr<OHOSNativeWindow> native_window);

  void Preload(int width, int height);

  void NotifySurfaceWindowChanged(fml::RefPtr<OHOSNativeWindow> native_window);

  void NotifyChanged(const SkISize& size);

  /**
   * @brief Update the size of the current Flutter window. This function will
   * also synchronize the viewport size.
   *
   * @param width
   * @param height
   */
  void UpdateDisplaySize(int width, int height);

  // |PlatformView|
  void NotifyDestroyed() override;

  void SetViewportMetrics(int64_t view_id, ViewportMetrics& metrics);

  // todo
  void DispatchPlatformMessage(std::string name,
                               void* message,
                               int messageLenth,
                               int reponseId);

  void DispatchEmptyPlatformMessage(std::string name, int reponseId);

  std::shared_ptr<OHOSExternalTexture> CreateExternalTexture(
      int64_t texture_id);

  uint64_t RegisterExternalTexture(int64_t texture_id);

  void RegisterExternalTextureByPixelMap(
      int64_t texture_id,
      NativePixelMap* pixelMap,
      OH_NativeBuffer* pixelMap_native_buffer);

  void SetExternalTextureBackGroundPixelMap(
      int64_t texture_id,
      NativePixelMap* pixelMap,
      OH_NativeBuffer* pixelMap_native_buffer);

  void SetExternalTextureBackGroundColor(int64_t texture_id, uint32_t color);

  void SetTextureBufferSize(int64_t texture_id, int32_t width, int32_t height);

  void NotifyTextureResizing(int64_t texture_id, int32_t width, int32_t height);

  bool SetExternalNativeImage(int64_t texture_id, OH_NativeImage* native_image);

  void UnRegisterExternalTexture(int64_t texture_id);

  uint64_t GetExternalTextureWindowId(int64_t texture_id);

  uint64_t ResetExternalTexture(int64_t texture_id, bool need_surfaceId);

  void EnableFrameCache(bool enable) { *enable_frame_cache_ = enable; };

  // |PlatformView|
  PointerDataDispatcherMaker GetDispatcherMaker() override;

  // |PlatformView|
  void LoadDartDeferredLibrary(
      intptr_t loading_unit_id,
      std::unique_ptr<const fml::Mapping> snapshot_data,
      std::unique_ptr<const fml::Mapping> snapshot_instructions) override;

  void LoadDartDeferredLibraryError(intptr_t loading_unit_id,
                                    const std::string error_message,
                                    bool transient) override;

  // |PlatformView|
  void UpdateAssetResolverByType(
      std::unique_ptr<AssetResolver> updated_asset_resolver,
      AssetResolver::AssetResolverType type) override;

  const std::shared_ptr<OHOSContext>& GetOHOSContext() { return ohos_context_; }

  std::shared_ptr<PlatformMessageHandler> GetPlatformMessageHandler()
      const override {
    return platform_message_handler_;
  }

  void OnTouchEvent(std::shared_ptr<std::string[]> touchPacketString, int size);

  void OnMouseEvent(const std::shared_ptr<std::string[]>& mousePacketString,
                    const int& size);

  void OnAxisEvent(const std::shared_ptr<std::string[]>& axisPacketString,
                   const int& size);

  void RunTask(OhosThreadType type, const fml::closure& task, int64_t millis = 0);

  void SetSemanticsBridge(std::shared_ptr<SemanticsBridge> bridge,
                          std::shared_ptr<std::mutex> mutex);
  void AccessibilityAnnounce(std::unique_ptr<char[]>& message);
  void AccessibilityOnTap(int32_t nodeId);
  void AccessibilityOnLongPress(int32_t nodeId);
  void AccessibilityOnTooltip(std::unique_ptr<char[]>& message);
  void OnAccessibilityStateChange(bool state);
  void SetNavigation(bool isNavigation);
  void SetAccessibleNavigation(bool isAccessibleNavigation);
  void SetBoldText(double fontWeightScale);

  void SimulateTouchEvent(SemanticsNodeExtend* node);

  //--------------------------------------------------------------------------
  /// @brief  GPU Resource Reclaim Policy APIs
  //--------------------------------------------------------------------------

  /// @brief  Called when surface is created.
  void OnSurfaceCreated();

  /// @brief Called when surface is destroyed.
  void OnSurfaceDestroyed();

  /// @brief  Returns whether the frame gate is currently enabled.
  ///         When frame gate is on, MarkTextureFrameAvailable should be
  ///         blocked.
  /// Thread-safe: Can be called from any thread.
  bool IsFrameGateEnabled() const {
    return frame_gate_enabled_.load(std::memory_order_acquire);
  }

 private:
  const std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  std::shared_ptr<OHOSContext> ohos_context_;

  std::shared_ptr<OHOSSurface> ohos_surface_;
  std::shared_ptr<PlatformMessageHandlerOHOS> platform_message_handler_;

  std::shared_ptr<OhosSurfaceFactoryImpl> surface_factory_;
  std::map<int64_t, std::shared_ptr<OHOSExternalTexture>> all_external_texture_;

  std::shared_ptr<bool> enable_frame_cache_ = std::make_shared<bool>(true);

  // viewport will use this size
  int display_width_ = 0;
  int display_height_ = 0;

  ViewportMetrics viewport_metrics_;

  bool window_is_preload_ = false;

  // accessibility
  std::queue<std::pair<flutter::SemanticsNodeUpdates,
                       flutter::CustomAccessibilityActionUpdates>>
      semantics_queue_;

  std::shared_ptr<SemanticsBridge> bridge_;
  std::shared_ptr<std::mutex> bridge_mutex_;
  int32_t accessibility_feature_flags_ = 0;
  bool is_accessibility_navigation_ = false;

  //--------------------------------------------------------------------------
  /// @brief  GPU Resource Reclaim Policy state
  //--------------------------------------------------------------------------

  /// Current lifecycle state
  AppLifecycleState lifecycle_state_ = AppLifecycleState::kDetached;

  /// Whether onscreen context is valid (set false after
  /// TeardownOnScreenContext)
  std::atomic<bool> onscreen_context_valid_{true};

  /// Cached native window for rebuilding after aggressive teardown
  fml::RefPtr<OHOSNativeWindow> cached_native_window_;

  /// Current GPU reclaim level
  GpuReclaimLevel current_reclaim_level_ = GpuReclaimLevel::kRestore;

  /// Frame gate flag - when true, external texture frame updates are blocked
  /// Thread-safety: Read from callback threads, written from platform thread,
  /// use atomic.
  std::atomic<bool> frame_gate_enabled_{false};

  //--------------------------------------------------------------------------
  /// @brief  GPU Resource Reclaim Policy internal methods
  /// Architecture:
  ///   - EvaluateReclaimLevel: Decide action based on state transition
  ///   - ApplyReclaimLevel:    Execute decision
  ///   - ExecuteReclaimXxx:    Perform actual GPU resource operations
  //--------------------------------------------------------------------------

  /// @brief  Main entry point for handling application lifecycle state changes.
  /// @param  state  The new lifecycle state string from platform layer.
  void OnApplicationStateChange(const std::string& state);

  /// @brief  Handle lifecycle platform channel messages.
  /// @param  name  Platform channel name.
  /// @param  message  Raw message bytes.
  /// @param  message_length  Message size in bytes.
  void HandleLifecyclePlatformMessage(const std::string& name,
                                      const void* message,
                                      int message_length);

  /// @brief  Evaluates the reclaim decision for a state transition.
  /// @param  old_state  The previous lifecycle state.
  /// @param  new_state  The new lifecycle state.
  /// @return The decision for how to apply reclaim.
  GpuReclaimDecision EvaluateReclaimLevel(AppLifecycleState old_state,
                                          AppLifecycleState new_state) const;

  /// @brief  Applies the given reclaim decision.
  /// @param  decision  The reclaim decision to apply
  void ApplyReclaimLevel(GpuReclaimDecision decision);

  /// @brief  Executes kRestore level actions (foreground restoration).
  void ExecuteReclaimRestore();

  /// @brief  Executes kAggressive level actions (aggressive cleanup).
  void ExecuteReclaimAggressive();

  // ======================== Helper Methods ==================================

  /// @brief  Determines whether the onscreen context needs to be rebuilt.
  /// @return true if the context should be rebuilt, false if not.
  bool ShouldRebuildOnscreenContext() const;

  /// @brief  Posts tasks to rebuild the onscreen context and surface.
  void PostRebuildOnscreenContextTasks();

  /// @brief  Runs a task on the raster thread and waits for its completion.
  /// @param  task  The closure to run on the raster thread.
  void RunOnRasterAndWait(fml::closure task);

  /// @brief  Attempts to free Skia GPU resources.
  static void TryFreeSkiaGpuResources(
      const std::shared_ptr<OHOSSurface>& surface,
      const std::shared_ptr<OHOSContext>& context);

  // |PlatformView|
  void UpdateSemantics(
      int64_t view_id,
      flutter::SemanticsNodeUpdates update,
      flutter::CustomAccessibilityActionUpdates actions) override;

  // |PlatformView|
  void HandlePlatformMessage(
      std::unique_ptr<flutter::PlatformMessage> message) override;

  // |PlatformView|
  void OnPreEngineRestart() const override;

  // |PlatformView|
  std::unique_ptr<VsyncWaiter> CreateVSyncWaiter() override;

  // |PlatformView|
  std::unique_ptr<Surface> CreateRenderingSurface() override;

  // |PlatformView|
  std::shared_ptr<ExternalViewEmbedder> CreateExternalViewEmbedder() override;

  // |PlatformView|
  std::unique_ptr<SnapshotSurfaceProducer> CreateSnapshotSurfaceProducer()
      override;

  // |PlatformView|
  sk_sp<GrDirectContext> CreateResourceContext() const override;

  // |PlatformView|
  void ReleaseResourceContext() const override;

  // |PlatformView|
  std::shared_ptr<impeller::Context> GetImpellerContext() const override;

  // |PlatformView|
  std::unique_ptr<std::vector<std::string>> ComputePlatformResolvedLocales(
      const std::vector<std::string>& supported_locale_data) override;

  // |PlatformView|
  void RequestDartDeferredLibrary(intptr_t loading_unit_id) override;

  void InstallFirstFrameCallback(bool is_preload = false);

  void FireFirstFrameCallback(bool is_preload = false);

  FML_DISALLOW_COPY_AND_ASSIGN(PlatformViewOHOS);

  static void OnNativeImageFrameAvailable(void* data);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_VIEW_OHOS_H_
