/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "flutter/shell/platform/ohos/platform_view_ohos.h"
#include <GLES2/gl2ext.h>
#include <arkui/native_interface_accessibility.h>
#include <native_image/native_image.h>
#include <memory>
#include <optional>
#include <string>
#include "flutter/common/constants.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/impeller/renderer/backend/vulkan/context_vk.h"
#include "flutter/lib/ui/window/viewport_metrics.h"
#include "flutter/shell/common/shell_io_manager.h"
#include "flutter/shell/platform/ohos/ohos_context_gl_skia.h"
#include "flutter/shell/platform/ohos/ohos_surface_gl_skia.h"
#include "flutter/shell/platform/ohos/ohos_surface_software.h"
#include "flutter/shell/platform/ohos/platform_message_response_ohos.h"
#include "fml/trace_event.h"
#include "lib/ui/semantics/semantics_node.h"
#include "napi_common.h"
#include "ohos_context_gl_impeller.h"
#include "ohos_external_texture_gl.h"
#include "ohos_external_texture_vulkan.h"
#include "ohos_logging.h"
#include "ohos_surface_gl_impeller.h"
#include "shell/common/platform_view.h"
#include "shell/platform/ohos/accessibility/ohos_semantics_node.h"
#include "shell/platform/ohos/context/ohos_context.h"
#include "shell/platform/ohos/ohos_surface_vulkan_impeller.h"

namespace flutter {

// This global map's key is (texture_id)
std::map<uint64_t, PlatformViewOHOS*> g_texture_platformview_map;
std::recursive_mutex g_map_mutex;

OhosSurfaceFactoryImpl::OhosSurfaceFactoryImpl(
    const std::shared_ptr<OHOSContext>& context)
    : ohos_context_(context) {}

OhosSurfaceFactoryImpl::~OhosSurfaceFactoryImpl() = default;

std::unique_ptr<OHOSSurface> OhosSurfaceFactoryImpl::CreateSurface() {
  switch (ohos_context_->RenderingApi()) {
    case OHOSRenderingAPI::kSoftware:
      FML_LOG(INFO) << "OhosSurfaceFactoryImpl::CreateSurface use software";
      return std::make_unique<OHOSSurfaceSoftware>(ohos_context_);
    case OHOSRenderingAPI::kOpenGLES:
      FML_LOG(INFO) << "OhosSurfaceFactoryImpl::CreateSurface use skia-gl";
      return std::make_unique<OhosSurfaceGLSkia>(ohos_context_);
    case flutter::OHOSRenderingAPI::kImpellerVulkan:
      FML_LOG(INFO)
          << "OhosSurfaceFactoryImpl::CreateSurface use impeller-vulkan";
      return std::make_unique<OHOSSurfaceVulkanImpeller>(ohos_context_);
    default:
      FML_DCHECK(false);
      return nullptr;
  }
}

std::unique_ptr<OHOSContext> CreateOHOSContext(
    const flutter::TaskRunners& task_runners,
    uint8_t msaa_samples,
    OHOSRenderingAPI rendering_api,
    bool enable_vulkan_validation,
    bool enable_opengl_gpu_tracing,
    bool enable_vulkan_gpu_tracing) {
  TRACE_EVENT0("flutter", "CreateOHOSContext");
  switch (rendering_api) {
    case OHOSRenderingAPI::kSoftware:
      return std::make_unique<OHOSContext>(OHOSRenderingAPI::kSoftware);
    case OHOSRenderingAPI::kOpenGLES:
      return std::make_unique<OhosContextGLSkia>(OHOSRenderingAPI::kOpenGLES,
                                                 task_runners, msaa_samples);
    case OHOSRenderingAPI::kImpellerVulkan:
      return std::make_unique<OHOSContextVulkanImpeller>(
          enable_vulkan_validation, enable_vulkan_gpu_tracing);
    default:
      FML_DCHECK(false);
      return nullptr;
  }
}

PlatformViewOHOS::PlatformViewOHOS(
    PlatformView::Delegate& delegate,
    const flutter::TaskRunners& task_runners,
    const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
    bool use_software_rendering,
    uint8_t msaa_samples)
    : PlatformViewOHOS(
          delegate,
          task_runners,
          napi_facade,
          CreateOHOSContext(
              task_runners,
              msaa_samples,
              delegate.OnPlatformViewGetSettings().ohos_rendering_api,
              delegate.OnPlatformViewGetSettings().enable_vulkan_validation,
              delegate.OnPlatformViewGetSettings().enable_opengl_gpu_tracing,
              delegate.OnPlatformViewGetSettings().enable_vulkan_gpu_tracing)) {
}

PlatformViewOHOS::PlatformViewOHOS(
    PlatformView::Delegate& delegate,
    const flutter::TaskRunners& task_runners,
    const std::shared_ptr<PlatformViewOHOSNapi>& napi_facade,
    const std::shared_ptr<flutter::OHOSContext>& ohos_context)
    : PlatformView(delegate, task_runners),
      napi_facade_(napi_facade),
      ohos_context_(ohos_context),
      platform_message_handler_(new PlatformMessageHandlerOHOS(
          napi_facade,
          task_runners_.GetPlatformTaskRunner())) {
  if (ohos_context_) {
    FML_CHECK(ohos_context_->IsValid())
        << "Could not create surface from invalid HarmonyOS context.";
    surface_factory_ = std::make_shared<OhosSurfaceFactoryImpl>(ohos_context_);
    ohos_surface_ = surface_factory_->CreateSurface();

    // PrepareGpuSurface preloads the GPUSurface, which in turn preloads the
    // Vulkan rendering pipeline. This helps reduce the time between application
    // launch and the rendering of the first frame. The 1ms delay ensures that
    // subsequent raster tasks can run first, as it can block the platform
    // thread.
    auto task_delay = fml::TimeDelta::FromMicroseconds(1000);
    task_runners_.GetRasterTaskRunner()->PostDelayedTask(
        [surface = ohos_surface_]() { surface->PrepareGpuSurface(); },
        task_delay);
    FML_CHECK(ohos_surface_ && ohos_surface_->IsValid())
        << "Could not create an OpenGL, Vulkan or Software surface to set "
           "up "
           "rendering.";
  }
}

PlatformViewOHOS::~PlatformViewOHOS() {
  FML_LOG(INFO) << "PlatformViewOHOS::~PlatformViewOHOS";
  // The UnregisterTexture cannot be called here because it depends on
  // rasterizer_, and rasterizer_ may be null at this time.
}

void PlatformViewOHOS::NotifyCreate(
    fml::RefPtr<OHOSNativeWindow> native_window) {
  LOGI("NotifyCreate start");
  if (ohos_surface_) {
    InstallFirstFrameCallback();
    LOGI("NotifyCreate start1");
    // We register these external textures with the engine again to ensure that
    // the screen is normal in the scenario of page jump and return (when there
    // is a detachEngine operation during page jump, there will be a
    // NotifyDestroy call, which will bring unregister texture).
    for (auto [texture_id, external_texture] : all_external_texture_) {
      // registerTexture must be called before PlatformView::NotifyCreated,
      // because the onGrContextCreate method of the external texture will be
      // called in PlatformView::NotifyCreated.
      RegisterTexture(external_texture);
      std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
      g_texture_platformview_map[(uint64_t)texture_id] = this;
    }

    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&, surface = ohos_surface_.get(),
         native_window = std::move(native_window)]() {
          LOGI("NotifyCreate start4");
          surface->SetDisplayWindow(native_window);
          // Note that NotifyDestroyed will wait raster task, so platformview is
          // not deleted here.
          if (!window_is_preload_) {
            PlatformView::NotifyCreated();
          } else if (surface->NeedNewFrame()) {
            PlatformView::ScheduleFrame();
          } else {
            fml::TaskRunner::RunNowOrPostTask(
                task_runners_.GetPlatformTaskRunner(),
                [&] { PlatformViewOHOS::FireFirstFrameCallback(); });
          }
        });

    {
      std::lock_guard<std::mutex> lock(*bridge_mutex_);
      while (!semantics_queue_.empty()) {
        auto semantics = semantics_queue_.front();
        semantics_queue_.pop();
        bridge_->UpdateNodeTree(semantics.first);
      }
    }
  }
}

void PlatformViewOHOS::Preload(int width, int height) {
  if (ohos_surface_ && !window_is_preload_) {
    LOGI("Preload start");
    InstallFirstFrameCallback(true);

    for (auto [texture_id, external_texture] : all_external_texture_) {
      RegisterTexture(external_texture);
      std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
      g_texture_platformview_map[(uint64_t)texture_id] = this;
    }

    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&, surface = ohos_surface_.get(), width, height]() {
          TRACE_EVENT0("flutter", "surface:Preload");
          LOGI("Preload PlatformViewOHOS");
          if (!window_is_preload_) {
            bool ret = surface->PrepareOffscreenWindow(width, height);
            if (ret) {
              // Note that NotifyDestroyed will wait raster task, so
              // platformview is not deleted here.
              PlatformView::NotifyCreated();
              window_is_preload_ = true;
            }
          }
        });
  }
}

void PlatformViewOHOS::NotifySurfaceWindowChanged(
    fml::RefPtr<OHOSNativeWindow> native_window) {
  LOGI("PlatformViewOHOS NotifySurfaceWindowChanged enter");
  TRACE_EVENT0("flutter", "NotifySurfaceWindowChanged");
  if (ohos_surface_) {
    fml::AutoResetWaitableEvent latch;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&latch, width = display_width_, height = display_height_,
         surface = ohos_surface_.get(),
         native_window = std::move(native_window)]() {
          if (native_window) {
            // Reset the window size here to prevent the window size from being
            // unsynchronized when the XComponent size changes.
            // Note: Setting the window size in the platform thread may not take
            // effect because Vulkan might request the buffer using the
            // previously configured size before raster reaches this point,
            // causing the window size to revert to its original value during
            // the process.
            native_window->SetSize(width, height);
            surface->SetDisplayWindow(native_window);
          }
          latch.Signal();
        });
    latch.Wait();
  }
}

void PlatformViewOHOS::NotifyChanged(const SkISize& size) {
  LOGI("PlatformViewOHOS NotifyChanged enter");
  if (ohos_surface_) {
    fml::AutoResetWaitableEvent latch;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),  //
        [&latch, surface = ohos_surface_.get(), size]() {
          surface->OnScreenSurfaceResize(size);
          latch.Signal();
        });
    latch.Wait();
  }
}

void PlatformViewOHOS::UpdateDisplaySize(int width, int height) {
  if (display_width_ != width || display_height_ != height) {
    display_width_ = width;
    display_height_ = height;
    // Here, we update the viewport to ensure that the size of the window buffer
    // matches the size of the viewport. This prevents stretching or
    // compression, which can occur if the physical size of the viewport differs
    // from the window size.
    SetViewportMetrics(kFlutterImplicitViewId, viewport_metrics_);
  }
}

// |PlatformView|
void PlatformViewOHOS::NotifyDestroyed() {
  LOGI("PlatformViewOHOS NotifyDestroyed enter");

  // Note: NotifyCreate is invoked in raster thread. So we post NotifyDestroyed
  // to raster to avoid latent conflic.
  fml::AutoResetWaitableEvent latch;
  fml::TaskRunner::RunNowOrPostTask(task_runners_.GetRasterTaskRunner(), [&]() {
    window_is_preload_ = false;
    // This function will internally call the GrContextDestroy of the external
    // texture, and within this callback, the graphic resources occupied by the
    // external texture will be released.
    PlatformView::NotifyDestroyed();
    latch.Signal();
  });
  latch.Wait();

  if (ohos_surface_) {
    // If we don't remove ptr in g_texture_platformview_map, PlatformViewOHOS
    // ptr in g_texture_platformview_map_ will bring use-after-free crash in
    // OnNativeImageFrameAvailable.
    for (const auto& [texture_id, external_texture] : all_external_texture_) {
      // Here we only remove the external textures maintained internally by the
      // engine, but do not actually destroy them. Without actively calling
      // unregisterExternalTexture, their actual destruction will occur after
      // ~PlatformViewOHOS.
      UnregisterTexture(texture_id);
      std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
      g_texture_platformview_map.erase((uint64_t)texture_id);
    }
    fml::AutoResetWaitableEvent latch;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&latch, surface = ohos_surface_.get()]() {
          surface->TeardownOnScreenContext();
          latch.Signal();
        });
    latch.Wait();
  }
  SetSemanticsEnabled(false);
}

void PlatformViewOHOS::SetViewportMetrics(int64_t view_id,
                                          ViewportMetrics& metrics) {
  if (display_width_ != 0 && display_height_ != 0) {
    // Note: Size change notifications from ArkUI are sent tens of milliseconds
    // after the window size changes. Using them for updates may cause visual
    // anomalies.
    // We use the previously set window size as the physical_size instead of the
    // provided one to ensure that the viewport size matches the buffer size
    // (avoiding screen stretching). As a result, size updates from the ArkUI
    // layer will not take effect.
    metrics.physical_width = display_width_;
    metrics.physical_height = display_height_;
  }
  FML_LOG(INFO) << "SetViewportMetrics physical size: "
                << metrics.physical_width << "," << metrics.physical_height
                << " display size: " << display_width_ << ","
                << display_height_;
  viewport_metrics_ = metrics;
  PlatformView::SetViewportMetrics(view_id, metrics);
}

// todo

void PlatformViewOHOS::DispatchPlatformMessage(std::string name,
                                               void* message,
                                               int messageLenth,
                                               int reponseId) {
  FML_DLOG(INFO) << "DispatchPlatformMessage（" << name << "," << messageLenth
                 << "," << reponseId;
  fml::MallocMapping mapMessage =
      fml::MallocMapping::Copy(message, messageLenth);

  fml::RefPtr<flutter::PlatformMessageResponse> response;
  response = fml::MakeRefCounted<PlatformMessageResponseOHOS>(
      reponseId, napi_facade_, task_runners_.GetPlatformTaskRunner());

  PlatformView::DispatchPlatformMessage(
      std::make_unique<flutter::PlatformMessage>(
          std::move(name), std::move(mapMessage), std::move(response)));
}

void PlatformViewOHOS::DispatchEmptyPlatformMessage(std::string name,
                                                    int reponseId) {
  FML_DLOG(INFO) << "DispatchEmptyPlatformMessage (" << name << ","
                 << reponseId;
  fml::RefPtr<flutter::PlatformMessageResponse> response;
  response = fml::MakeRefCounted<PlatformMessageResponseOHOS>(
      reponseId, napi_facade_, task_runners_.GetPlatformTaskRunner());

  PlatformView::DispatchPlatformMessage(
      std::make_unique<flutter::PlatformMessage>(std::move(name),
                                                 std::move(response)));
}

// |PlatformView|
void PlatformViewOHOS::LoadDartDeferredLibrary(
    intptr_t loading_unit_id,
    std::unique_ptr<const fml::Mapping> snapshot_data,
    std::unique_ptr<const fml::Mapping> snapshot_instructions) {
  FML_DLOG(INFO) << "LoadDartDeferredLibrary:" << loading_unit_id;
  delegate_.LoadDartDeferredLibrary(loading_unit_id, std::move(snapshot_data),
                                    std::move(snapshot_instructions));
}

void PlatformViewOHOS::LoadDartDeferredLibraryError(
    intptr_t loading_unit_id,
    const std::string error_message,
    bool transient) {
  FML_DLOG(INFO) << "LoadDartDeferredLibraryError:" << loading_unit_id << ":"
                 << error_message;
  delegate_.LoadDartDeferredLibraryError(loading_unit_id, error_message,
                                         transient);
}

// |PlatformView|
void PlatformViewOHOS::UpdateAssetResolverByType(
    std::unique_ptr<AssetResolver> updated_asset_resolver,
    AssetResolver::AssetResolverType type) {
  FML_DLOG(INFO) << "UpdateAssetResolverByType";
  delegate_.UpdateAssetResolverByType(std::move(updated_asset_resolver), type);
}

// ohos_accessbility_bridge
void PlatformViewOHOS::UpdateSemantics(
    flutter::SemanticsNodeUpdates update,
    flutter::CustomAccessibilityActionUpdates actions) {
  TRACE_EVENT0("flutter", "UpdateSemantics");
  if (bridge_->provider_ohos_ == nullptr) {
    semantics_queue_.push(std::make_pair(update, actions));
    FML_DLOG(INFO) << "PlatformViewOHOS::UpdateSemantics is called when "
                      "bridge_.provider_ohos_ is nullptr ";
    return;
  } else if (!semantics_queue_.empty()) {
    FML_DLOG(WARNING)
        << "PlatformViewOHOS::UpdateSemantics has unhandled calls";
  }
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  bridge_->UpdateNodeTree(update);
}

// |PlatformView|
void PlatformViewOHOS::HandlePlatformMessage(
    std::unique_ptr<flutter::PlatformMessage> message) {
  FML_DLOG(INFO) << "HandlePlatformMessage";
  platform_message_handler_->HandlePlatformMessage(std::move(message));
}

// |PlatformView|
void PlatformViewOHOS::OnPreEngineRestart() const {
  FML_DLOG(INFO) << "OnPreEngineRestart";
  task_runners_.GetPlatformTaskRunner()->PostTask(
      fml::MakeCopyable([napi_facede = napi_facade_]() mutable {
        napi_facede->FlutterViewOnPreEngineRestart();
      }));
}

// |PlatformView|
std::unique_ptr<VsyncWaiter> PlatformViewOHOS::CreateVSyncWaiter() {
  FML_DLOG(INFO) << "CreateVSyncWaiter";
  return std::make_unique<VsyncWaiterOHOS>(task_runners_, enable_frame_cache_);
}

// |PlatformView|
std::unique_ptr<Surface> PlatformViewOHOS::CreateRenderingSurface() {
  FML_DLOG(INFO) << "CreateRenderingSurface";
  if (ohos_surface_ == nullptr) {
    FML_DLOG(ERROR) << "CreateRenderingSurface Failed.ohos_surface_ is null ";
    return nullptr;
  }

  LOGD("return CreateGPUSurface");
  return ohos_surface_->CreateGPUSurface(
      ohos_context_->GetMainSkiaContext().get());
}

// |PlatformView|
std::shared_ptr<ExternalViewEmbedder>
PlatformViewOHOS::CreateExternalViewEmbedder() {
  FML_DLOG(INFO) << "CreateExternalViewEmbedder";
  return nullptr;
}

// |PlatformView|
std::unique_ptr<SnapshotSurfaceProducer>
PlatformViewOHOS::CreateSnapshotSurfaceProducer() {
  FML_DLOG(INFO) << "CreateSnapshotSurfaceProducer";
  return std::make_unique<OHOSSnapshotSurfaceProducer>(*(ohos_surface_.get()));
}

// |PlatformView|
sk_sp<GrDirectContext> PlatformViewOHOS::CreateResourceContext() const {
  FML_DLOG(INFO) << "CreateResourceContext";
  if (!ohos_surface_) {
    return nullptr;
  }
  sk_sp<GrDirectContext> resource_context;
  if (ohos_surface_->ResourceContextMakeCurrent()) {
    // TODO(chinmaygarde): Currently, this code depends on the fact that only
    // the OpenGL surface will be able to make a resource context current. If
    // this changes, this assumption breaks. Handle the same.
    resource_context = ShellIOManager::CreateCompatibleResourceLoadingContext(
        GrBackend::kOpenGL_GrBackend,
        GPUSurfaceGLDelegate::GetDefaultPlatformGLInterface());
  } else {
    FML_DLOG(ERROR) << "Could not make the resource context current.";
  }

  return resource_context;
}

// |PlatformView|
void PlatformViewOHOS::ReleaseResourceContext() const {
  LOGI("PlatformViewOHOS::ReleaseResourceContext");
  // IO thread will invoke glGetError() when exit.
  // It will bring lots of "Call To OpenGL ES API With No Current Context"
  // without gl context. So we don't clear current.
  // if (ohos_surface_) {
  //   ohos_surface_->ResourceContextClearCurrent();
  // }
}

// |PlatformView|
std::shared_ptr<impeller::Context> PlatformViewOHOS::GetImpellerContext()
    const {
  FML_DLOG(INFO) << "GetImpellerContext";
  if (ohos_surface_) {
    return ohos_surface_->GetImpellerContext();
  }
  return nullptr;
}

// |PlatformView|
std::unique_ptr<std::vector<std::string>>
PlatformViewOHOS::ComputePlatformResolvedLocales(
    const std::vector<std::string>& supported_locale_data) {
  FML_DLOG(INFO) << "ComputePlatformResolvedLocales";
  return napi_facade_->FlutterViewComputePlatformResolvedLocales(
      supported_locale_data);
}

// |PlatformView|
void PlatformViewOHOS::RequestDartDeferredLibrary(intptr_t loading_unit_id) {
  FML_DLOG(INFO) << "RequestDartDeferredLibrary:" << loading_unit_id;
  return;
}

void PlatformViewOHOS::InstallFirstFrameCallback(bool is_preload) {
  FML_DLOG(INFO) << "InstallFirstFrameCallback";
  SetNextFrameCallback(
      [platform_view = GetWeakPtr(),
       platform_task_runner = task_runners_.GetPlatformTaskRunner(),
       is_preload]() {
        platform_task_runner->PostTask([platform_view, is_preload]() {
          // Back on Platform Task Runner.
          FML_DLOG(INFO) << "install InstallFirstFrameCallback ";
          if (platform_view) {
            reinterpret_cast<PlatformViewOHOS*>(platform_view.get())
                ->FireFirstFrameCallback(is_preload);
          }
        });
      });
}

void PlatformViewOHOS::FireFirstFrameCallback(bool is_preload) {
  FML_DLOG(INFO) << "FlutterViewOnFirstFrame";
  napi_facade_->FlutterViewOnFirstFrame(is_preload);
}

PointerDataDispatcherMaker PlatformViewOHOS::GetDispatcherMaker() {
  return [](DefaultPointerDataDispatcher::Delegate& delegate) {
    return std::make_unique<SmoothPointerDataDispatcher>(delegate);
  };
}

std::shared_ptr<OHOSExternalTexture> PlatformViewOHOS::CreateExternalTexture(
    int64_t texture_id) {
  uint64_t context_frame_data = (uint64_t)texture_id;
  OH_OnFrameAvailableListener listener;
  listener.context = (void*)context_frame_data;
  listener.onFrameAvailable = &PlatformViewOHOS::OnNativeImageFrameAvailable;
  std::shared_ptr<OHOSExternalTexture> extrenal_texture = nullptr;
  FML_LOG(INFO) << " RegisterExternalTexture api type "
                << int(ohos_context_->RenderingApi()) << " texture_id "
                << texture_id;
  if (ohos_context_->RenderingApi() == OHOSRenderingAPI::kOpenGLES) {
    extrenal_texture =
        std::make_shared<OHOSExternalTextureGL>(texture_id, listener);
  } else if (ohos_context_->RenderingApi() ==
             OHOSRenderingAPI::kImpellerVulkan) {
    extrenal_texture = std::make_shared<OHOSExternalTextureVulkan>(
        std::static_pointer_cast<impeller::ContextVK>(
            ohos_context_->GetImpellerContext()),
        texture_id, listener);
  }
  if (extrenal_texture && extrenal_texture->GetProducerSurfaceId() != 0 &&
      extrenal_texture->GetProducerWindowId() != 0) {
    std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
    g_texture_platformview_map[context_frame_data] = this;
    all_external_texture_[texture_id] = extrenal_texture;
    RegisterTexture(extrenal_texture);
  }
  return extrenal_texture;
}

uint64_t PlatformViewOHOS::RegisterExternalTexture(int64_t texture_id) {
  auto extrenal_texture = CreateExternalTexture(texture_id);
  if (extrenal_texture == nullptr) {
    return 0;
  } else {
    return extrenal_texture->GetProducerSurfaceId();
  }
  return 0;
}

uint64_t PlatformViewOHOS::GetExternalTextureWindowId(int64_t texture_id) {
  if (all_external_texture_.find(texture_id) != all_external_texture_.end()) {
    auto external_texture = all_external_texture_[texture_id];
    return external_texture->GetProducerWindowId();
  }
  return 0;
}

void PlatformViewOHOS::OnNativeImageFrameAvailable(void* data) {
  uint64_t ptexture_id = (uint64_t)data;
  std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
  if (g_texture_platformview_map.find(ptexture_id) ==
      g_texture_platformview_map.end()) {
    return;
  }
  PlatformViewOHOS* platform = g_texture_platformview_map[ptexture_id];

  if (platform == nullptr || platform->ohos_surface_ == nullptr) {
    FML_LOG(ERROR) << "OnNativeImageFrameAvailable NotifyDstroyed, will not "
                      "MarkTextureFrameAvailable";
    return;
  }

  // Note: PostTask may lead to a deadlock if a render task (which might acquire
  // the buffer) is dispatched earlier and scheduled to run before this task.
  // So we use recursive_mutex to safely invoke OnNativeImageFrameAvailable from
  // the platform thread, allowing nested lock acquisition without deadlock.
  fml::TaskRunner::RunNowOrPostTask(
      platform->task_runners_.GetPlatformTaskRunner(), [ptexture_id]() {
        std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
        if (g_texture_platformview_map.find(ptexture_id) ==
            g_texture_platformview_map.end()) {
          return;
        }
        PlatformViewOHOS* platform = g_texture_platformview_map[ptexture_id];
        uint64_t texture_id = ptexture_id;
        platform->MarkTextureFrameAvailable(texture_id);
      });
}

void PlatformViewOHOS::UnRegisterExternalTexture(int64_t texture_id) {
  all_external_texture_.erase(texture_id);
  FML_LOG(INFO) << "UnRegisterExternalTexture " << texture_id;
  // Note that external_texture will be destroy after UnregisterTexture.
  UnregisterTexture(texture_id);

  // Wait to prevent potential conflicts with SetExternalNativeImage(use same
  // NativeImage) being called from another raster thread.
  fml::AutoResetWaitableEvent latch;
  fml::TaskRunner::RunNowOrPostTask(task_runners_.GetRasterTaskRunner(),
                                    [&latch]() { latch.Signal(); });
  latch.Wait();

  std::lock_guard<std::recursive_mutex> lock(g_map_mutex);
  g_texture_platformview_map.erase((uint64_t)texture_id);
}

void PlatformViewOHOS::RegisterExternalTextureByPixelMap(
    int64_t texture_id,
    NativePixelMap* pixelMap,
    OH_NativeBuffer* pixelMap_native_buffer) {
  auto external_texture = CreateExternalTexture(texture_id);
  if (external_texture == nullptr) {
    return;
  }

  bool ret =
      external_texture->SetPixelMapAsProducer(pixelMap, pixelMap_native_buffer);
  if (ret) {
    PlatformView::ScheduleFrame();
  }
}

void PlatformViewOHOS::SetExternalTextureBackGroundPixelMap(
    int64_t texture_id,
    NativePixelMap* pixelMap,
    OH_NativeBuffer* pixelMap_native_buffer) {
  if (all_external_texture_.find(texture_id) == all_external_texture_.end()) {
    return;
  }

  auto external_texture = all_external_texture_[texture_id];
  FML_LOG(INFO) << "SetExternalTextureBackGroundPixelMap " << texture_id;
  bool ret =
      external_texture->SetPixelMapAsProducer(pixelMap, pixelMap_native_buffer);
  if (ret) {
    PlatformView::ScheduleFrame();
  }
}

void PlatformViewOHOS::SetExternalTextureBackGroundColor(int64_t texture_id,
                                                         uint32_t color) {
  if (all_external_texture_.find(texture_id) != all_external_texture_.end()) {
    auto external_texture = all_external_texture_[texture_id];
    FML_LOG(INFO) << "SetExternalTextureBackGroundColor " << texture_id
                  << " color " << color;
    external_texture->SetBackGroundColor(color);
    PlatformView::ScheduleFrame();
  }
}

void PlatformViewOHOS::SetTextureBufferSize(int64_t texture_id,
                                            int32_t width,
                                            int32_t height) {
  if (all_external_texture_.find(texture_id) != all_external_texture_.end()) {
    auto external_texture = all_external_texture_[texture_id];
    external_texture->SetProducerWindowSize(width, height);
  }
}

void PlatformViewOHOS::NotifyTextureResizing(int64_t texture_id,
                                             int32_t width,
                                             int32_t height) {
  if (all_external_texture_.find(texture_id) != all_external_texture_.end()) {
    auto external_texture = all_external_texture_[texture_id];
    external_texture->NotifyResizing(width, height);
  }
}

bool PlatformViewOHOS::SetExternalNativeImage(int64_t texture_id,
                                              OH_NativeImage* native_image) {
  if (all_external_texture_.find(texture_id) != all_external_texture_.end()) {
    auto external_texture = all_external_texture_[texture_id];
    fml::AutoResetWaitableEvent latch;
    bool result = false;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&external_texture, &latch, &result, native_image]() {
          result = external_texture->SetExternalNativeImage(native_image);
          latch.Signal();
        });
    latch.Wait();
    return result;
  } else {
    return false;
  }
}

uint64_t PlatformViewOHOS::ResetExternalTexture(int64_t texture_id,
                                                bool need_surfaceId) {
  if (all_external_texture_.find(texture_id) != all_external_texture_.end()) {
    FML_LOG(INFO) << "ResetExternalTexture " << texture_id;

    auto external_texture = all_external_texture_[texture_id];
    fml::AutoResetWaitableEvent latch;
    uint64_t surface_id = 0;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&external_texture, &latch, &surface_id, need_surfaceId]() {
          surface_id = external_texture->Reset(need_surfaceId);
          latch.Signal();
        });
    latch.Wait();
    return surface_id;
  } else {
    return 0;
  }
}

void PlatformViewOHOS::OnTouchEvent(
    const std::shared_ptr<std::string[]> touchPacketString,
    int size) {
  return napi_facade_->FlutterViewOnTouchEvent(touchPacketString, size);
}

void PlatformViewOHOS::RunTask(OhosThreadType type, const fml::closure& task) {
  fml::RefPtr<fml::TaskRunner> TaskRunnerPtr = nullptr;
  switch (type) {
    case OhosThreadType::kPlatform:
      TaskRunnerPtr = task_runners_.GetPlatformTaskRunner();
      break;
    case OhosThreadType::kUI:
      TaskRunnerPtr = task_runners_.GetUITaskRunner();
      break;
    case OhosThreadType::kRaster:
      TaskRunnerPtr = task_runners_.GetRasterTaskRunner();
      break;
    case OhosThreadType::kIO:
      TaskRunnerPtr = task_runners_.GetIOTaskRunner();
      break;
    default:
      break;
  }

  if (!TaskRunnerPtr) {
    return;
  }

  fml::TaskRunner::RunNowOrPostTask(TaskRunnerPtr, task);
}

void PlatformViewOHOS::SetSemanticsBridge(
    std::shared_ptr<SemanticsBridge> bridge,
    std::shared_ptr<std::mutex> mutex) {
  bridge_ = std::move(bridge);
  bridge_mutex_ = std::move(mutex);
}

void PlatformViewOHOS::AccessibilityAnnounce(std::unique_ptr<char[]>& message) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  bridge_->Announce(message);
}

void PlatformViewOHOS::AccessibilityOnTap(int32_t nodeId) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  bridge_->OnTap(nodeId);
}

void PlatformViewOHOS::AccessibilityOnLongPress(int32_t nodeId) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  bridge_->OnLongPress(nodeId);
}

void PlatformViewOHOS::AccessibilityOnTooltip(
    std::unique_ptr<char[]>& message) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  bridge_->OnTooltip(message);
}

void PlatformViewOHOS::OnAccessibilityStateChange(bool state) {
  if (state) {
    SetSemanticsEnabled(true);
    SetAccessibleNavigation(true);
    std::lock_guard<std::mutex> lock(*bridge_mutex_);
    bridge_->OnAccessibilityStateChange(state);
  } else {
    SetAccessibleNavigation(false);
    SetSemanticsEnabled(false);
  }
}

// This navigation behavior belongs to page routing operations in screen reader
// (such as jumping/back to the next/last flutter page)
void PlatformViewOHOS::SetNavigation(bool isNavigation) {
  std::lock_guard<std::mutex> lock(*bridge_mutex_);
  bridge_->OnAccessibilityNavigation(isNavigation);
}

// This navigation behavior aims to determine whether the accessibility service
// (such as screen reading, automated testing) is on or off.
// The dart side can use the API of "MediaQuery.of().accessibleNavigation" to
// get the accessibility service status is true/false.
void PlatformViewOHOS::SetAccessibleNavigation(bool isAccessibleNavigation) {
  if (is_accessibility_navigation_ == isAccessibleNavigation) {
    return;
  }
  is_accessibility_navigation_ = isAccessibleNavigation;
  if (is_accessibility_navigation_) {
    accessibility_feature_flags_ |=
        static_cast<int32_t>(AccessibilityFeatureFlag::kAccessibleNavigation);
    FML_DLOG(INFO) << "SetAccessibleNavigation -> accessibilityFeatureFlags: "
                   << accessibility_feature_flags_;
  } else {
    accessibility_feature_flags_ &=
        ~static_cast<int32_t>(AccessibilityFeatureFlag::kAccessibleNavigation);
  }
  SetAccessibilityFeatures(accessibility_feature_flags_);
}

void PlatformViewOHOS::SetBoldText(double fontWeightScale) {
  bool shouldBold = fontWeightScale > 1.0;
  if (shouldBold) {
    accessibility_feature_flags_ |=
        static_cast<int32_t>(AccessibilityFeatureFlag::kBoldText);
    FML_DLOG(INFO) << "SetBoldText -> accessibilityFeatureFlags: "
                   << accessibility_feature_flags_;
  } else {
    accessibility_feature_flags_ &=
        ~static_cast<int32_t>(AccessibilityFeatureFlag::kBoldText);
  }
  SetAccessibilityFeatures(accessibility_feature_flags_);
}

void PlatformViewOHOS::SimulateTouchEvent(SemanticsNodeExtend* node) {
  const int numTouchPoints = 1;
  const float simulatePressure = 0.05;
  PointerData pointerData;

  pointerData.Clear();
  pointerData.embedder_id = 0;
  pointerData.change = PointerData::Change::kDown;
  pointerData.physical_y =
      (node->absoluteRect.fTop + node->absoluteRect.fBottom) / 2;
  pointerData.physical_x =
      (node->absoluteRect.fLeft + node->absoluteRect.fRight) / 2;
  pointerData.physical_delta_x = 0.0;
  pointerData.physical_delta_y = 0.0;
  pointerData.device = 0;
  pointerData.pointer_identifier = 0;
  pointerData.signal_kind = PointerData::SignalKind::kNone;
  pointerData.scroll_delta_x = 0.0;
  pointerData.scroll_delta_y = 0.0;
  pointerData.pressure = simulatePressure;
  pointerData.pressure_max = 1.0;
  pointerData.pressure_min = 0.0;
  pointerData.kind = PointerData::DeviceKind::kTouch;
  pointerData.buttons = kPointerButtonTouchContact;
  pointerData.pan_x = 0.0;
  pointerData.pan_y = 0.0;
  pointerData.pan_delta_x = 0.0;
  pointerData.pan_delta_y = 0.0;
  pointerData.size = 0;
  pointerData.scale = 1.0;
  pointerData.rotation = 0.0;

  std::unique_ptr<flutter::PointerDataPacket> downPacket =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  downPacket->SetPointerData(0, pointerData);
  DispatchPointerDataPacket(std::move(downPacket));
  std::unique_ptr<flutter::PointerDataPacket> upPacket =
      std::make_unique<flutter::PointerDataPacket>(numTouchPoints);
  pointerData.change = PointerData::Change::kUp;
  pointerData.buttons = 0;
  upPacket->SetPointerData(0, pointerData);
  DispatchPointerDataPacket(std::move(upPacket));
}

}  // namespace flutter
