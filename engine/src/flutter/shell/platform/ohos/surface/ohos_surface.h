/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_SURFACE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_SURFACE_OHOS_SURFACE_H_

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

 private:
  OH_NativeImage* offscreen_native_image_ = nullptr;
  //
  // std::vector<OHNativeWindowBuffer*> all_offscreen_window_buffers_;
  OHNativeWindow* offscreen_nativewindow_ = nullptr;
  OHNativeWindowBuffer* last_nativewindow_buffer_ = nullptr;

  int32_t offscreen_width_ = 0;
  int32_t offscreen_height_ = 0;

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
