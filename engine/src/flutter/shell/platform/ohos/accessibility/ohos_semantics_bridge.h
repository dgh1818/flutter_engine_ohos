/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_BRIDGE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_BRIDGE_H_

#include "ohos_semantics_tree.h"

namespace flutter {
class SemanticsBridge {
 public:
  SemanticsBridge() = default;

  SemanticsTree tree_;
  ArkUI_AccessibilityProvider* provider_ohos_ = nullptr;
  ArkUI_AccessibilityProvider* old_provider_ohos_ = nullptr;
  bool is_accessibility_enabled_ = false;
  bool has_navigationed_ = false;
  void UpdateNodeTree(flutter::SemanticsNodeUpdates& nodes);
  void UpdateFocusedNode();
  SemanticsNodeExtend* GetNodeById(int32_t id);

  void SendSemanticsEvent(SemanticsNodeExtend* node,
                          ArkUI_AccessibilityEventType type,
                          const char* message);

  int32_t FindFocusNode(int32_t id,
                        ArkUI_AccessibilityFocusType focusType,
                        ArkUI_AccessibilityElementInfo* info);

  int32_t FindNextFocusNode(int32_t id,
                            ArkUI_AccessibilityFocusMoveDirection direction,
                            ArkUI_AccessibilityElementInfo* info);

  int32_t FillNodesWithSearchText(int32_t id,
                                  const char* text,
                                  ArkUI_AccessibilityElementInfoList* list);

  int32_t FillNodesWithSearch(int32_t id,
                              ArkUI_AccessibilitySearchMode mode,
                              ArkUI_AccessibilityElementInfoList* list);

  int32_t ClearAccessibilityFocus(int32_t id);
  int32_t GainAccessibilityFocus(int32_t id);

  int32_t GetAccessibilityNodeCursorPosition(int64_t elementId, int32_t* index);

  void Announce(std::unique_ptr<char[]>& message);
  void OnTap(int32_t nodeId);
  void OnLongPress(int32_t nodeId);
  void OnTooltip(std::unique_ptr<char[]>& message);
  void OnAccessibilityStateChange(bool state);
  void OnAccessibilityNavigation(bool is_nav);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_BRIDGE_H_