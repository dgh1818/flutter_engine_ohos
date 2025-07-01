/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
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
  void UpdateFocusableNodesInfo(std::vector<int32_t>& visitorOrder);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_TREE_H_