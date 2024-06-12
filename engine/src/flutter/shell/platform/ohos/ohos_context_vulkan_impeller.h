// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_VULKAN_IMPELLER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_VULKAN_IMPELLER_H_

#include "flutter/fml/macros.h"
#include "flutter/fml/native_library.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"

namespace flutter {

class OHOSContextVulkanImpeller : public OHOSContext {
 public:
  OHOSContextVulkanImpeller(bool enable_validation,
                            bool enable_gpu_tracing,
                            bool quiet = false);

  ~OHOSContextVulkanImpeller();

  // |OHOSContext|
  bool IsValid() const override;

 private:
  bool is_valid_ = false;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSContextVulkanImpeller);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_CONTEXT_VULKAN_IMPELLER_H_
