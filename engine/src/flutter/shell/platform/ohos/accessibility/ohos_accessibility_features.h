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
#include "native_accessibility_channel.h"
namespace flutter {

class OhosAccessibilityFeatures {
 public:
  OhosAccessibilityFeatures();
  ~OhosAccessibilityFeatures();

  bool ACCESSIBLE_NAVIGATION = false;

  void SetAccessibleNavigation(bool isAccessibleNavigation,
                               int64_t shell_holder_id);
  void SetBoldText(double fontWeightScale, int64_t shell_holder_id);

  void SendAccessibilityFlags(int64_t shell_holder_id);

 private:
  std::shared_ptr<NativeAccessibilityChannel> nativeAccessibilityChannel_;
  int32_t accessibilityFeatureFlags = 0;
};

/**
 * 无障碍特征枚举类（flutter平台通用）
 * 注意：必须同src/flutter/lib/ui/window/platform_configuration.h
 * 中的`AccessibilityFeatureFlag`枚举类保持一致
 */
enum AccessibilityFeatures : int32_t {
  AccessibleNavigation = 1 << 0,
  InvertColors = 1 << 1,
  DisableAnimations = 1 << 2,
  BoldText = 1 << 3,
  ReduceMotion = 1 << 4,
  HighContrast = 1 << 5,
  OnOffSwitchLabels = 1 << 6,
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_ACCESSIBILITY_FEATURES_H_
