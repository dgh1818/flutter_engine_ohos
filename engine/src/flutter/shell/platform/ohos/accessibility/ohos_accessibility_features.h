/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_ACCESSIBILITY_FEATURES_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_ACCESSIBILITY_FEATURES_H_
#include <cstdint>
#include "flutter/lib/ui/window/platform_configuration.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {

class OhosAccessibilityFeatures {
 public:
  OhosAccessibilityFeatures();
  ~OhosAccessibilityFeatures();

  static OhosAccessibilityFeatures* GetInstance();

  void SetBoldText(double fontWeightScale, int64_t shell_holder_id);
  void SendAccessibilityFlags(int64_t shell_holder_id);

 private:
  static OhosAccessibilityFeatures instance;

  // Font weight adjustment (FontWeight.Bold - FontWeight.Normal = w700 - w400 =
  // 300)
  static const int32_t BOLD_TEXT_WEIGHT_ADJUSTMENT = 300;
  int32_t accessibilityFeatureFlags = 0;
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_ACCESSIBILITY_FEATURES_H_
