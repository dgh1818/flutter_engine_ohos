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
#ifndef FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_NATIVE_ACCESSIBILITY_CHANNEL_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_NATIVE_ACCESSIBILITY_CHANNEL_H_
#include <memory>
#include "flutter/fml/mapping.h"
#include "flutter/lib/ui/semantics/custom_accessibility_action.h"
#include "flutter/lib/ui/semantics/semantics_node.h"
namespace flutter {

class NativeAccessibilityChannel {
 public:
  NativeAccessibilityChannel();
  ~NativeAccessibilityChannel();

  void OnOhosAccessibilityEnabled(int64_t shellHolderId);

  void OnOhosAccessibilityDisabled(int64_t shellHolderId);

  void SetSemanticsEnabled(int64_t shellHolderId, bool enabled);

  void SetAccessibilityFeatures(int64_t shellHolderId, int32_t flags);

  void DispatchSemanticsAction(int64_t shellHolderId,
                               int32_t id,
                               flutter::SemanticsAction action,
                               fml::MallocMapping args);

  void UpdateSemantics(flutter::SemanticsNodeUpdates update,
                       flutter::CustomAccessibilityActionUpdates actions);

  class AccessibilityMessageHandler {
   public:
    void Announce(std::unique_ptr<char[]>& message);

    void OnTap(int32_t nodeId);

    void OnLongPress(int32_t nodeId);

    void OnTooltip(std::unique_ptr<char[]>& message);
  };

  void SetAccessibilityMessageHandler(
      std::shared_ptr<AccessibilityMessageHandler> handler);

 private:
  std::shared_ptr<AccessibilityMessageHandler> handler;
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_NATIVE_ACCESSIBILITY_CHANNEL_H_
