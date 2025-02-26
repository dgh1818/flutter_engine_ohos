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
#ifndef FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_TREE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_TREE_H_

#include <arkui/native_interface_accessibility.h>
#include <unordered_map>
#include <vector>
#include "ohos_semantics_node.h"

namespace flutter {

class SemanticsTree {
 public:
  SemanticsTree() = default;
  ~SemanticsTree();

  void ClearSemanticsTree();

  SemanticsNodeExtend* FindNodeById(int32_t id);
  SemanticsNodeExtend* GetOrAddNode(int32_t id);
  void RemoveNode(int32_t id);
  bool SetAccessibilityFocusNode(int32_t id);
  void ClearAccessibilityFocusNode();

  std::vector<SemanticsNodeExtend*> UpdateWithNodes(
      std::unordered_map<int32_t, SemanticsNode>& nodes);

  SemanticsNodeExtend* FindFocusNode(int32_t id,
                                     ArkUI_AccessibilityFocusType focusType);
  SemanticsNodeExtend* FindNextFocusNode(
      int32_t id,
      ArkUI_AccessibilityFocusMoveDirection direction);

  bool FillNodesWithSearchText(int32_t id,
                               const char* text,
                               ArkUI_AccessibilityElementInfoList* list);
  bool FillNodesWithSearch(int32_t id,
                           ArkUI_AccessibilitySearchMode mode,
                           ArkUI_AccessibilityElementInfoList* list);
  SemanticsNodeExtend* GetRootNode() { return root_node_; }
  bool UpdateNextFocusWhenDisappear(
      std::unordered_set<int32_t>& need_remove_ids);

  // This flag is set after the event is sent, indicating the event has been
  // dispatched, but the focus node has not updated yet.
  bool focus_request_has_send_ = false;
  // This flag indicates that a request for need_request_focused_node_ is needed
  // later. The request may occur multiple times (as the previously requested
  // node may disappear) until a node is successfully focused.
  bool in_request_progress_ = false;
  SemanticsNodeExtend* need_request_focused_node_ = nullptr;
  SemanticsNodeExtend* focused_node_ = nullptr;

 private:
  std::unordered_map<int32_t, SemanticsNodeExtend*> all_semantics_nodes_;
  SemanticsNodeExtend* root_node_ = nullptr;
  SemanticsNodeExtend* input_focused_node_ = nullptr;
  SemanticsNodeExtend* last_input_focused_node_ = nullptr;

  bool FillNodesRecursive(int32_t id,
                          const char* text,
                          ArkUI_AccessibilityElementInfoList* list);
  bool FillNodeInfo(SemanticsNodeExtend* node,
                    ArkUI_AccessibilityElementInfoList* list);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_TREE_H_