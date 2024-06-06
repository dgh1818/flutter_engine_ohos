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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_CONTEXT_OHOS_CONTEXT_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_CONTEXT_OHOS_CONTEXT_H_

#include "common/settings.h"
#include "flutter/fml/macros.h"
#include "flutter/fml/task_runner.h"
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
  ///             AndroidSurfaces and the raster thread.
  ///
  std::shared_ptr<impeller::Context> GetImpellerContext() const;

 protected:
  /// Intended to be called from a subclass constructor after setup work for the
  /// context has completed.
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
