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
#include "flutter/fml/logging.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/lib/ui/window/viewport_metrics.h"
#include "flutter/shell/common/shell_io_manager.h"
#include "flutter/shell/platform/ohos/ohos_context_gl_skia.h"
#include "flutter/shell/platform/ohos/ohos_surface_gl_skia.h"
#include "flutter/shell/platform/ohos/ohos_surface_software.h"
#include "flutter/shell/platform/ohos/platform_message_response_ohos.h"
#include "napi_common.h"
#include "ohos_context_gl_impeller.h"
#include "ohos_external_texture_gl.h"
#include "ohos_surface_gl_impeller.h"
#include "shell/platform/ohos/context/ohos_context.h"
#include "shell/platform/ohos/ohos_surface_vulkan_impeller.h"

#include <GLES2/gl2ext.h>

namespace flutter {

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
  switch (rendering_api) {
    case OHOSRenderingAPI::kSoftware:
      return std::make_unique<OHOSContext>(OHOSRenderingAPI::kSoftware);
    case OHOSRenderingAPI::kOpenGLES:
      return std::make_unique<OhosContextGLSkia>(
          OHOSRenderingAPI::kOpenGLES, fml::MakeRefCounted<OhosEnvironmentGL>(),
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
        << "Could not create surface from invalid Android context.";
    LOGI("ohos_surface_ end 1");
    surface_factory_ = std::make_shared<OhosSurfaceFactoryImpl>(ohos_context_);
    LOGI("ohos_surface_ end 2");
    ohos_surface_ = surface_factory_->CreateSurface();
    LOGI("ohos_surface_ end 3");
    FML_CHECK(ohos_surface_ && ohos_surface_->IsValid())
        << "Could not create an OpenGL, Vulkan or Software surface to set "
           "up "
           "rendering.";
  }
}

PlatformViewOHOS::~PlatformViewOHOS() {}

void PlatformViewOHOS::NotifyCreate(
    fml::RefPtr<OHOSNativeWindow> native_window) {
  LOGI("NotifyCreate start");
  if (ohos_surface_) {
    InstallFirstFrameCallback();
    LOGI("NotifyCreate start1");
    fml::AutoResetWaitableEvent latch;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&latch, surface = ohos_surface_.get(),
         native_window = std::move(native_window)]() {
          LOGI("NotifyCreate start4");
          surface->SetNativeWindow(native_window);
          latch.Signal();
        });
    latch.Wait();
  }

  PlatformView::NotifyCreated();
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

// |PlatformView|
void PlatformViewOHOS::NotifyDestroyed() {
  LOGI("PlatformViewOHOS NotifyDestroyed enter");
  PlatformView::NotifyDestroyed();
  if (ohos_surface_) {
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

// todo

void PlatformViewOHOS::DispatchPlatformMessage(std::string name,
                                               void* message,
                                               int messageLenth,
                                               int reponseId) {
  FML_DLOG(INFO) << "DispatchSemanticsAction（" << name << ",," << messageLenth
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
  FML_DLOG(INFO) << "DispatchEmptyPlatformMessage（" << name << "" << ","
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
  FML_DLOG(INFO) << "DispatchSemanticsAction（" << id << "," << action << ","
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
  LOGI("ReleaseResourceContext");
  if (ohos_surface_) {
    ohos_surface_->ResourceContextClearCurrent();
  }
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

void PlatformViewOHOS::InstallFirstFrameCallback() {
  FML_DLOG(INFO) << "InstallFirstFrameCallback";
  SetNextFrameCallback(
      [platform_view = GetWeakPtr(),
       platform_task_runner = task_runners_.GetPlatformTaskRunner()]() {
        platform_task_runner->PostTask([platform_view]() {
          // Back on Platform Task Runner.
          FML_DLOG(INFO) << "install InstallFirstFrameCallback ";
          if (platform_view) {
            reinterpret_cast<PlatformViewOHOS*>(platform_view.get())
                ->FireFirstFrameCallback();
          }
        });
      });
}

void PlatformViewOHOS::FireFirstFrameCallback() {
  FML_DLOG(INFO) << "FlutterViewOnFirstFrame";
  napi_facade_->FlutterViewOnFirstFrame();
}

uint64_t PlatformViewOHOS::RegisterExternalTexture(int64_t texture_id) {
  uint64_t surface_id = 0;
  int ret = -1;
  if (ohos_context_->RenderingApi() == OHOSRenderingAPI::kOpenGLES) {
    std::shared_ptr<OHOSExternalTextureGL> ohos_external_gl =
        std::make_shared<OHOSExternalTextureGL>(texture_id, ohos_surface_);
    ohos_external_gl->nativeImage_ =
        OH_NativeImage_Create(texture_id, GL_TEXTURE_EXTERNAL_OES);
    if (ohos_external_gl->nativeImage_ == nullptr) {
      FML_DLOG(ERROR) << "Error with OH_NativeImage_Create";
      return surface_id;
    }
    void* contextData = new OhosImageFrameData(this, texture_id);
    OH_OnFrameAvailableListener listener;
    listener.context = contextData;
    listener.onFrameAvailable = &PlatformViewOHOS::OnNativeImageFrameAvailable;
    ret = OH_NativeImage_SetOnFrameAvailableListener(
        ohos_external_gl->nativeImage_, listener);
    if (ret != 0) {
      FML_DLOG(ERROR)
          << "Error with OH_NativeImage_SetOnFrameAvailableListener";
      return surface_id;
    }
    ret = OH_NativeImage_GetSurfaceId(ohos_external_gl->nativeImage_,
                                      &surface_id);
    ohos_external_gl->first_update_ = false;
    if (ret != 0) {
      FML_DLOG(ERROR) << "Error with OH_NativeImage_GetSurfaceId";
      return surface_id;
    }
    RegisterTexture(ohos_external_gl);
  }
  return surface_id;
}

void PlatformViewOHOS::OnNativeImageFrameAvailable(void* data) {
  auto frameData = reinterpret_cast<OhosImageFrameData*>(data);
  if (frameData == nullptr || frameData->context_ == nullptr) {
    FML_DLOG(ERROR)
        << "OnNativeImageFrameAvailable, frameData or context_ is null.";
    return;
  }
  std::shared_ptr<OHOSSurface> ohos_surface =
      frameData->context_->ohos_surface_;
  const TaskRunners task_runners = frameData->context_->task_runners_;
  if (ohos_surface) {
    fml::TaskRunner::RunNowOrPostTask(
        task_runners.GetPlatformTaskRunner(), [frameData]() {
          frameData->context_->MarkTextureFrameAvailable(
              frameData->texture_id_);
        });
  }
}

void PlatformViewOHOS::UnRegisterExternalTexture(int64_t texture_id) {
  external_texture_gl_.erase(texture_id);
  UnregisterTexture(texture_id);
}

void PlatformViewOHOS::RegisterExternalTextureByPixelMap(
    int64_t texture_id,
    NativePixelMap* pixelMap) {
  if (ohos_context_->RenderingApi() == OHOSRenderingAPI::kOpenGLES) {
    auto iter = external_texture_gl_.find(texture_id);
    if (iter != external_texture_gl_.end()) {
      iter->second->DispatchPixelMap(pixelMap);
    } else {
      std::shared_ptr<OHOSExternalTextureGL> ohos_external_gl =
          std::make_shared<OHOSExternalTextureGL>(texture_id, ohos_surface_);
      external_texture_gl_[texture_id] = ohos_external_gl;
      RegisterTexture(ohos_external_gl);
      ohos_external_gl->DispatchPixelMap(pixelMap);
    }
    MarkTextureFrameAvailable(texture_id);
  }
}

OhosImageFrameData::OhosImageFrameData(PlatformViewOHOS* context,
                                       int64_t texture_id)
    : context_(context), texture_id_(texture_id) {}

OhosImageFrameData::~OhosImageFrameData() = default;

}  // namespace flutter
