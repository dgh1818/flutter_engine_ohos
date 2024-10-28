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
#include "flutter/shell/platform/ohos/accessibility/ohos_accessibility_features.h"
#include "flutter/fml/logging.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

namespace flutter {

OhosAccessibilityFeatures OhosAccessibilityFeatures::instance;

OhosAccessibilityFeatures::OhosAccessibilityFeatures(){};
OhosAccessibilityFeatures::~OhosAccessibilityFeatures(){};

OhosAccessibilityFeatures* OhosAccessibilityFeatures::GetInstance() {
  return &OhosAccessibilityFeatures::instance;
}

/**
 * bold text for AccessibilityFeature
 */
void OhosAccessibilityFeatures::SetBoldText(double fontWeightScale,
                                            int64_t shell_holder_id) {
  bool shouldBold = fontWeightScale > 1.0;

  if (shouldBold) {
    accessibilityFeatureFlags |=
        static_cast<int32_t>(flutter::AccessibilityFeatureFlag::kBoldText);
    FML_DLOG(INFO) << "SetBoldText -> accessibilityFeatureFlags: "
                   << accessibilityFeatureFlags;

  } else {
    accessibilityFeatureFlags &=
        static_cast<int32_t>(flutter::AccessibilityFeatureFlag::kBoldText);
  }

  SendAccessibilityFlags(shell_holder_id);
}

/**
 * send the accessibility flags to flutter dart sdk
 */
void OhosAccessibilityFeatures::SendAccessibilityFlags(
    int64_t shell_holder_id) {
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shell_holder_id);
  ohos_shell_holder->GetPlatformView()->SetAccessibilityFeatures(
      accessibilityFeatureFlags);
  FML_DLOG(INFO) << "SendAccessibilityFlags -> accessibilityFeatureFlags = "
                 << accessibilityFeatureFlags;
  // set accessibility feature flag to 0
  accessibilityFeatureFlags = 0;
}
}  // namespace flutter
