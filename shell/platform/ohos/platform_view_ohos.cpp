/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flutter/shell/platform/ohos/platform_view_ohos.h"
#include <native_image/native_image.h>
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
#include "napi_common.h"
#include "ohos_context_gl_impeller.h"
#include "ohos_external_texture_gl.h"
#include "ohos_external_texture_vulkan.h"
#include "ohos_logging.h"
#include "ohos_surface_gl_impeller.h"
#include "shell/common/platform_view.h"
#include "shell/platform/ohos/context/ohos_context.h"
#include "shell/platform/ohos/ohos_surface_vulkan_impeller.h"

namespace flutter {

// This global map's key is (PlatformViewOHOS-ptr + texture_id) because there
// may be many platformViews.
std::map<uint64_t, PlatformViewOHOS*> g_texture_platformview_map;
std::mutex g_map_mutex;

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
}

void PlatformViewOHOS::NotifyCreate(
    fml::RefPtr<OHOSNativeWindow> native_window) {
  LOGI("NotifyCreate start");
  if (ohos_surface_) {
    InstallFirstFrameCallback();
    LOGI("NotifyCreate start1");
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
  }
}

void PlatformViewOHOS::Preload(int width, int height) {
  if (ohos_surface_ && !window_is_preload_) {
    LOGI("Preload start");
    InstallFirstFrameCallback(true);
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
    PlatformView::NotifyDestroyed();
    latch.Signal();
  });
  latch.Wait();

  if (ohos_surface_) {
    // If we don't unregister external texture, PlatformViewOHOS ptr in
    // texture_platformview_map_ will bring use-after-free crash in
    // OnNativeImageFrameAvailable.
    auto temp_external_textures = all_external_texture_;
    for (const auto& [texture_id, external_texture] : temp_external_textures) {
      UnRegisterExternalTexture(texture_id);
    }
    temp_external_textures.clear();
    fml::AutoResetWaitableEvent latch;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&latch, surface = ohos_surface_.get()]() {
          surface->TeardownOnScreenContext();
          latch.Signal();
        });
    latch.Wait();
  }
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
  FML_DLOG(INFO) << "DispatchSemanticsAction (" << name << ",," << messageLenth
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
  FML_DLOG(INFO) << "DispatchEmptyPlatformMessage (" << name << "" << ","
                 << reponseId;
  fml::RefPtr<flutter::PlatformMessageResponse> response;
  response = fml::MakeRefCounted<PlatformMessageResponseOHOS>(
      reponseId, napi_facade_, task_runners_.GetPlatformTaskRunner());

  PlatformView::DispatchPlatformMessage(
      std::make_unique<flutter::PlatformMessage>(std::move(name),
                                                 std::move(response)));
}

void PlatformViewOHOS::DispatchSemanticsAction(int id,
                                               int action,
                                               void* actionData,
                                               int actionDataLenth) {
  FML_DLOG(INFO) << "DispatchSemanticsAction (" << id << "," << action << ","
                 << actionDataLenth;
  auto args_vector = fml::MallocMapping::Copy(actionData, actionDataLenth);

  PlatformView::DispatchSemanticsAction(
      id, static_cast<flutter::SemanticsAction>(action),
      std::move(args_vector));
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

// todo
void PlatformViewOHOS::UpdateSemantics(
    flutter::SemanticsNodeUpdates update,
    flutter::CustomAccessibilityActionUpdates actions) {
  FML_DLOG(INFO) << "UpdateSemantics";
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
  return std::make_unique<VsyncWaiterOHOS>(task_runners_);
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
  uint64_t context_frame_data = (uint64_t)this + (uint64_t)texture_id;
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
    std::lock_guard<std::mutex> lock(g_map_mutex);
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
  std::lock_guard<std::mutex> lock(g_map_mutex);
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

  // Note: RunNowOrPostTask may get dead lock when running in platform thread.
  platform->task_runners_.GetPlatformTaskRunner()->PostTask([ptexture_id]() {
    std::lock_guard<std::mutex> lock(g_map_mutex);
    if (g_texture_platformview_map.find(ptexture_id) ==
        g_texture_platformview_map.end()) {
      return;
    }
    PlatformViewOHOS* platform = g_texture_platformview_map[ptexture_id];
    uint64_t texture_id = ptexture_id - (uint64_t)platform;
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

  std::lock_guard<std::mutex> lock(g_map_mutex);
  g_texture_platformview_map.erase((uint64_t)this + (uint64_t)texture_id);
}

void PlatformViewOHOS::RegisterExternalTextureByPixelMap(
    int64_t texture_id,
    NativePixelMap* pixelMap) {
  auto extrenal_texture = CreateExternalTexture(texture_id);
  if (extrenal_texture != nullptr) {
    extrenal_texture->SetPixelMapAsProducer(pixelMap);
  }
}

void PlatformViewOHOS::SetExternalTextureBackGroundPixelMap(
    int64_t texture_id,
    NativePixelMap* pixelMap) {
  if (all_external_texture_.find(texture_id) != all_external_texture_.end()) {
    auto external_texture = all_external_texture_[texture_id];
    FML_LOG(INFO) << "SetExternalTextureBackGroundPixelMap " << texture_id;
    external_texture->SetPixelMapAsProducer(pixelMap);
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

}  // namespace flutter
