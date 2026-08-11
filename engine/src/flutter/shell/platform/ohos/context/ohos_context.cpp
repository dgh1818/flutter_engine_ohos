/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "flutter/shell/platform/ohos/context/ohos_context.h"

namespace flutter {

OHOSContext::OHOSContext(OHOSRenderingAPI rendering_api)
    : rendering_api_(rendering_api) {}

OHOSContext::~OHOSContext() {
  if (main_context_) {
    main_context_->releaseResourcesAndAbandonContext();
  }
  if (impeller_context_) {
    impeller_context_->Shutdown();
    FML_LOG(IMPORTANT) << "impeller context shutdown (Vulkan).";
  }
}

OHOSRenderingAPI OHOSContext::RenderingApi() const {
  return rendering_api_;
}

bool OHOSContext::IsValid() const {
  return true;
}

void OHOSContext::SetMainSkiaContext(
    const sk_sp<GrDirectContext>& main_context) {
  main_context_ = main_context;
}

sk_sp<GrDirectContext> OHOSContext::GetMainSkiaContext() const {
  return main_context_;
}

std::shared_ptr<impeller::Context> OHOSContext::GetImpellerContext() const {
  return impeller_context_;
}

void OHOSContext::SetImpellerContext(
    const std::shared_ptr<impeller::Context>& context) {
  impeller_context_ = context;
}

}  // namespace flutter