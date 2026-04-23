/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_GL_SKIA_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_GL_SKIA_H_

#include <memory>

#include "flutter/fml/macros.h"
#include "flutter/shell/gpu/gpu_surface_gl_skia.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_context_gl_skia.h"
#include "flutter/shell/platform/ohos/ohos_environment_gl.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"

namespace flutter {

class OhosSurfaceGLSkia final : public GPUSurfaceGLDelegate,
                                public OHOSSurface {
 public:
  OhosSurfaceGLSkia(const std::shared_ptr<OHOSContext>& ohos_context);

  ~OhosSurfaceGLSkia() override;

  // |OhosSurface|
  bool IsValid() const override;

  // |OhosSurface|
  std::unique_ptr<Surface> CreateGPUSurface(
      GrDirectContext* gr_context) override;

  // |OhosSurface|
  void TeardownOnScreenContext() override;

  // |OhosSurface|
  bool OnScreenSurfaceResize(const SkISize& size) override;

  // |OhosSurface|
  bool ResourceContextMakeCurrent() override;

  // |OhosSurface|
  bool ResourceContextClearCurrent() override;

  // |OhosSurface|
  bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) override;

  // |OhosSurface|
  virtual std::unique_ptr<Surface> CreateSnapshotSurface() override;

  // |GPUSurfaceGLDelegate|
  std::unique_ptr<GLContextResult> GLContextMakeCurrent() override;

  // |GPUSurfaceGLDelegate|
  bool GLContextClearCurrent() override;

  // |GPUSurfaceGLDelegate|
  SurfaceFrame::FramebufferInfo GLContextFramebufferInfo() const override;

  // |GPUSurfaceGLDelegate|
  void GLContextSetDamageRegion(const std::optional<SkIRect>& region) override;

  // |GPUSurfaceGLDelegate|
  bool GLContextPresent(const GLPresentInfo& present_info) override;

  // |GPUSurfaceGLDelegate|
  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) const override;

  // |GPUSurfaceGLDelegate|
  sk_sp<const GrGLInterface> GetGLInterface() const override;

  // Obtain a raw pointer to the on-screen OhosEGLSurface.
  //
  // This method is intended for use in tests. Callers must not
  // delete the returned pointer.
  OhosEGLSurface* GetOnscreenSurface() const { return onscreen_surface_.get(); }

  bool PaintOffscreenData(OHNativeWindowBuffer* buffer, int fence_fd) override;

 private:
  std::unique_ptr<OhosEGLSurface> onscreen_surface_;
  std::unique_ptr<OhosEGLSurface> offscreen_surface_;

  //----------------------------------------------------------------------------
  /// @brief      Takes the super class OhosSurface's OhosContext and
  ///             return a raw pointer to an OhosContextGL.
  ///
  OhosContextGLSkia* GLContextPtr() const;

  FML_DISALLOW_COPY_AND_ASSIGN(OhosSurfaceGLSkia);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_GL_SKIA_H_
