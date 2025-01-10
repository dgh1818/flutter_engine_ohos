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

#include "flutter/shell/platform/ohos/surface/ohos_surface.h"
#include <native_window/external_window.h>
#include <sys/stat.h>
#include <cstdint>
#include "fml/trace_event.h"
namespace flutter {

std::map<uint64_t, bool> g_surface_is_alive;
std::mutex g_surface_alive_mutex;

OHOSSurface::OHOSSurface(const std::shared_ptr<OHOSContext>& ohos_context)
    : ohos_context_(ohos_context) {
  FML_DCHECK(ohos_context->IsValid());
  ohos_context_ = ohos_context;
}

std::unique_ptr<Surface> OHOSSurface::CreateSnapshotSurface() {
  return nullptr;
}

OHOSSurface::~OHOSSurface() {
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  g_surface_is_alive.erase((uint64_t)this);
  ReleaseOffscreenWindow();
}

std::shared_ptr<impeller::Context> OHOSSurface::GetImpellerContext() {
  return nullptr;
}

bool OHOSSurface::PrepareOffscreenWindow(int32_t width, int32_t height) {
  TRACE_EVENT0("flutter", "OHOSSurface-PrepareContext");

  if (offscreen_native_image_ != nullptr && offscreen_height_ == height &&
      offscreen_width_ == width) {
    return true;
  }

  offscreen_native_image_ = OH_NativeImage_Create(0, 0);

  offscreen_height_ = height;
  offscreen_width_ = width;

  offscreen_nativewindow_ =
      OH_NativeImage_AcquireNativeWindow(offscreen_native_image_);
  if (offscreen_nativewindow_ == nullptr) {
    FML_LOG(ERROR) << "offscreen OH_NativeImage_AcquireNativeWindow get null";
    return false;
  }

  int ret = OH_NativeWindow_NativeWindowHandleOpt(
      offscreen_nativewindow_, SET_BUFFER_GEOMETRY, width, height);
  if (ret != 0) {
    FML_LOG(ERROR) << "offscreen OH_NativeWindow_NativeWindowHandleOpt "
                      "set_buffer_size err:"
                   << ret;
    return false;
  }
  FML_LOG(INFO) << "set offscreen window" << offscreen_nativewindow_;

  SetNativeWindow(fml::MakeRefCounted<OHOSNativeWindow>(
      static_cast<OHNativeWindow*>(offscreen_nativewindow_)));

  OH_OnFrameAvailableListener listener;
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  listener.context = (void*)this;
  listener.onFrameAvailable = &OHOSSurface::OnFrameAvailable;
  g_surface_is_alive[(uint64_t)this] = true;
  ret = OH_NativeImage_SetOnFrameAvailableListener(offscreen_native_image_,
                                                   listener);
  if (ret != 0) {
    FML_LOG(ERROR) << "offscreen SetOnFrameAvailableListener err:" << ret;
  }

  return true;
}

void OHOSSurface::ReleaseOffscreenWindow() {
  if (last_nativewindow_buffer_) {
    FML_LOG(INFO) << "release last_nativewindow_buffer_ "
                  << last_nativewindow_buffer_;

    // OH_NativeImage_ReleaseNativeWindowBuffer will close the fence_fd even if
    // it fails.
    int ret = OH_NativeImage_ReleaseNativeWindowBuffer(
        offscreen_native_image_, last_nativewindow_buffer_, last_fence_fd_);
    if (ret != 0) {
      // Swapchain destroying may clean the buffercache of
      // offscreen_native_image_. In this situation, we need destroy the buffer.
      FML_LOG(ERROR) << "ReleaseOffscreenWindow failed err:" << ret;
      OH_NativeWindow_DestroyNativeWindowBuffer(last_nativewindow_buffer_);
    }
    last_nativewindow_buffer_ = nullptr;
    last_fence_fd_ = -1;
  }
  FML_LOG(INFO) << "ReleaseOffscreenWindow " << offscreen_nativewindow_;
  if (offscreen_native_image_) {
    // offscreen_nativewindow_ will be destroy in OH_NativeImage_Destroy.
    OH_NativeImage_Destroy(&offscreen_native_image_);
    offscreen_native_image_ = nullptr;
    offscreen_nativewindow_ = nullptr;
  }
}

bool OHOSSurface::SetDisplayWindow(fml::RefPtr<OHOSNativeWindow> window) {
  if (!window || !window->IsValid()) {
    return false;
  }
  TRACE_EVENT0("flutter", "surface:SetDisplayWindow");

  SkISize size = window->GetSize();
  SkISize old_size = window_size_;
  window_size_ = size;
  need_schedule_frame_ = false;
  FML_LOG(INFO) << "SetDisplayWindow " << window->Gethandle();

  if (native_window_ && native_window_->IsValid() &&
      window->Gethandle() == native_window_->Gethandle()) {
    // window is same, we just set surface resize.
    FML_LOG(INFO) << "window size change: (" << old_size.width() << ","
                  << old_size.height() << ")=>(" << size.width() << ","
                  << size.height() << ")";
    // Note: In vulkan mode, creating a swapchain with the same window can cause
    // the process to hang (stuck on requestBuffer). Therefore, SurfaceResize is
    // called here instead of directly calling SetNativeWindow. We should invoke
    // this always because it may create surface again (even sizes don't
    // change). In GL mode, EGLSurface will be be destroy after
    // TeardownOnScreenContext. In vulkan mode, nothing will happen after
    // TeardownOnScreenContext so swapchain can be reused.
    OnScreenSurfaceResize(size);
    return true;
  }

  if (offscreen_nativewindow_ == nullptr || size.width() != offscreen_width_ ||
      size.height() != offscreen_height_) {
    // The old swapchain must be destroyed before releasing the window to
    // prevent application crashes in Vulkan.
    bool ret = SetNativeWindow(window);
    ReleaseOffscreenWindow();
    return ret;
  }

  TRACE_EVENT0("flutter", "surface:SetNativeWindow");
  SetNativeWindow(window);
  if (PaintOffscreenData(last_nativewindow_buffer_, last_fence_fd_)) {
    last_nativewindow_buffer_ = nullptr;
    last_fence_fd_ = -1;
  } else {
    need_schedule_frame_ = true;
  }

  ReleaseOffscreenWindow();

  return true;
}

void OHOSSurface::OnFrameAvailable(void* data) {
  TRACE_EVENT0("flutter", "OHOSSurface-OnFrameAvailable");
  // this callback will be invoked in raster thread because we are the producer.

  FML_LOG(INFO) << "OHOSSurface get frame data";
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  if (!g_surface_is_alive[(uint64_t)data]) {
    return;
  }
  OHOSSurface* surface = (OHOSSurface*)data;

  if (surface->offscreen_native_image_ != nullptr &&
      surface->last_nativewindow_buffer_ != nullptr) {
    // there is no consumer, so we just released it.
    // OH_NativeImage_ReleaseNativeWindowBuffer will close the fence_fd even if
    // it fails.
    int ret = OH_NativeImage_ReleaseNativeWindowBuffer(
        surface->offscreen_native_image_, surface->last_nativewindow_buffer_,
        surface->last_fence_fd_);
    if (ret != 0) {
      // this cannot hanppen
      FML_LOG(ERROR) << "release offscreen windowbuffer failed:" << ret;
      OH_NativeWindow_DestroyNativeWindowBuffer(
          surface->last_nativewindow_buffer_);
    }
    surface->last_nativewindow_buffer_ = nullptr;
    surface->last_fence_fd_ = -1;
  }

  int ret = OH_NativeImage_AcquireNativeWindowBuffer(
      surface->offscreen_native_image_, &surface->last_nativewindow_buffer_,
      &surface->last_fence_fd_);
  if (surface->last_nativewindow_buffer_ == nullptr || ret != 0) {
    FML_LOG(ERROR) << "acquire offscreen windowbuffer failed: " << ret;
    return;
  }
  return;
}

}  // namespace flutter