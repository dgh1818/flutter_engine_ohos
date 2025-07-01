/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_CONTEXT_OHOS_CONTEXT_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_CONTEXT_OHOS_CONTEXT_H_

#include "common/settings.h"
#include "flutter/fml/macros.h"
#include "flutter/impeller/renderer/context.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

namespace flutter {

class OHOSContext {
 public:
  explicit OHOSContext(OHOSRenderingAPI rendering_api);

  virtual ~OHOSContext();

  OHOSRenderingAPI RenderingApi() const;

  virtual bool IsValid() const;

  void SetMainSkiaContext(const sk_sp<GrDirectContext>& main_context);

  sk_sp<GrDirectContext> GetMainSkiaContext() const;

  //----------------------------------------------------------------------------
  /// @brief      Accessor for the Impeller context associated with
  ///             HarmonyOSSurfaces and the raster thread.
  ///
  std::shared_ptr<impeller::Context> GetImpellerContext() const;

 protected:
  /// Intended to be called from a subclass constructor after setup work for the
  /// context has completed.

  // note: if this vulkan_dylib_ is in OHOSContextVulkanImpeller,
  // it will get vulkan func addr not mapped crash when ~ContextVK()
  // because ~ContextVK() is invoked later then ~NativeLibrary().
  fml::RefPtr<fml::NativeLibrary> vulkan_dylib_;

  void SetImpellerContext(const std::shared_ptr<impeller::Context>& context);

 private:
  const OHOSRenderingAPI rendering_api_;

  // This is the Skia context used for on-screen rendering.
  sk_sp<GrDirectContext> main_context_;

  std::shared_ptr<impeller::Context> impeller_context_;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSContext);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_CONTEXT_OHOS_CONTEXT_H_
