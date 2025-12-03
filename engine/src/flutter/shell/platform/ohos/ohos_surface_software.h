/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_SOFTWARE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_SOFTWARE_H_

#include "flutter/fml/macros.h"
#include "flutter/shell/gpu/gpu_surface_software.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"

#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"

namespace flutter {

class OHOSSurfaceSoftware final : public OHOSSurface,
                                  public GPUSurfaceSoftwareDelegate {
 public:
  OHOSSurfaceSoftware(const std::shared_ptr<OHOSContext>& ohos_context);
  ~OHOSSurfaceSoftware() override;

  bool IsValid() const override;

  // |OHOSSurface|
  bool ResourceContextMakeCurrent() override;

  // |OHOSSurface|
  bool ResourceContextClearCurrent() override;

  // |OHOSSurface|
  std::unique_ptr<Surface> CreateGPUSurface(
      GrDirectContext* gr_context) override;

  // |OHOSSurface|
  void TeardownOnScreenContext() override;

  // |OHOSSurface|
  bool OnScreenSurfaceResize(const SkISize& size) override;

  // |OHOSSurface|
  bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) override;

  // |GPUSurfaceSoftwareDelegate|
  sk_sp<SkSurface> AcquireBackingStore(const SkISize& size) override;

  // |GPUSurfaceSoftwareDelegate|
  bool PresentBackingStore(sk_sp<SkSurface> backing_store) override;

 private:
  sk_sp<SkSurface> sk_surface_;
  fml::RefPtr<OHOSNativeWindow> native_window_;
  SkColorType target_color_type_;
  SkAlphaType target_alpha_type_;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSSurfaceSoftware);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_SURFACE_SOFTWARE_H_