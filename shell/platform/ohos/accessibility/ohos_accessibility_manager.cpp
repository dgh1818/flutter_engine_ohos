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
#include "ohos_accessibility_manager.h"

namespace flutter {

OhosAccessibilityManager::OhosAccessibilityManager() {}

OhosAccessibilityManager::~OhosAccessibilityManager() {}

/**
 * 监听ohos平台是否开启无障碍屏幕朗读功能
 */
void OhosAccessibilityManager::OnAccessibilityStateChanged(
    bool ohosAccessibilityEnabled) {}

void OhosAccessibilityManager::SetOhosAccessibilityEnabled(bool isEnabled) {
  this->isOhosAccessibilityEnabled_ = isEnabled;
}

bool OhosAccessibilityManager::GetOhosAccessibilityEnabled() {
  return this->isOhosAccessibilityEnabled_;
}

}  // namespace flutter
