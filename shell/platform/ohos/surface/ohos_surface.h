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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_SURFACE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_SURFACE_H_

#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <memory>
#include "flutter/flow/surface.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "third_party/skia/include/core/SkSize.h"

namespace impeller {
class Context;
}  // namespace impeller

namespace flutter {

class OHOSSurface {
 public:
  virtual ~OHOSSurface();
  virtual bool IsValid() const = 0;
  virtual void TeardownOnScreenContext() = 0;

  virtual bool OnScreenSurfaceResize(const SkISize& size) = 0;
  // virtual bool OnScreenSurfaceHdrUpdate(const int hdr) { return false; }

  virtual bool ResourceContextMakeCurrent() = 0;

  virtual bool ResourceContextClearCurrent() = 0;

  virtual bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) = 0;

  virtual std::unique_ptr<Surface> CreateSnapshotSurface();

  virtual std::unique_ptr<Surface> CreateGPUSurface(
      GrDirectContext* gr_context = nullptr) = 0;

  virtual std::shared_ptr<impeller::Context> GetImpellerContext();

  // Return true means it will consume and release the buffer.
  virtual bool PaintOffscreenData(OHNativeWindowBuffer* buffer, int fence_fd) {
    return false;
  };

  // Preload GPU surface for vulkan (GPUSurface creating is time consuming in
  // vulkan because of graphic pipeline building)
  virtual void PrepareGpuSurface() {};

  virtual bool PrepareOffscreenWindow(int32_t width, int32_t height);

  void ReleaseOffscreenWindow();

  bool SetDisplayWindow(fml::RefPtr<OHOSNativeWindow> window);

  bool NeedNewFrame() { return need_schedule_frame_; }

  static void OnFrameAvailable(void* data);

 protected:
  explicit OHOSSurface(const std::shared_ptr<OHOSContext>& ohos_context);
  std::shared_ptr<OHOSContext> ohos_context_;
  fml::RefPtr<OHOSNativeWindow> native_window_;
  SkISize window_size_ = {0, 0};
  //int window_hdr_ = -1;

 private:
  OH_NativeImage* offscreen_native_image_ = nullptr;
  //
  // std::vector<OHNativeWindowBuffer*> all_offscreen_window_buffers_;
  OHNativeWindow* offscreen_nativewindow_ = nullptr;
  OHNativeWindowBuffer* last_nativewindow_buffer_ = nullptr;

  int32_t offscreen_width_ = 0;
  int32_t offscreen_height_ = 0;
  //int offscreen_hdr_ = -1;

  // int32_t free_buffer_cnt_;
  // int32_t max_buffer_cnt_;
  int last_fence_fd_ = -1;

  bool need_schedule_frame_ = false;
};

class OhosSurfaceFactory {
 public:
  OhosSurfaceFactory() = default;

  virtual ~OhosSurfaceFactory() = default;

  virtual std::unique_ptr<OHOSSurface> CreateSurface() = 0;
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_SURFACE_H_
