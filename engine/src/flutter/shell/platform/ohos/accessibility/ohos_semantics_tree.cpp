/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "ohos_semantics_tree.h"
#include <arkui/native_interface_accessibility.h>
#include <cassert>
#include <queue>
#include <unordered_set>
#include <vector>

namespace flutter {

SemanticsTree::~SemanticsTree() {
  ClearSemanticsTree();
}

void SemanticsTree::RemoveNode(int32_t id) {
  auto node = FindNodeById(id);
  if (!node) {
    return;
  }
  all_semantics_nodes_.erase(id);

  if (focused_node_ == node) {
    ClearAccessibilityFocusNode();
  }
  if (need_request_focused_node_ == node) {
    need_request_focused_node_ = nullptr;
  }
  delete node;
}

void SemanticsTree::ClearAccessibilityFocusNode() {
  if (focused_node_) {
    focused_node_->isAccessibilityFocued = false;
  }
  focused_node_ = nullptr;
}

bool SemanticsTree::SetAccessibilityFocusNode(int32_t id) {
  auto node = FindNodeById(id);
  if (node) {
    focused_node_ = node;
    focused_node_->isAccessibilityFocued = true;
    in_request_progress_ = false;
    return true;
  } else {
    return false;
  }
}

std::vector<SemanticsNodeExtend*> SemanticsTree::UpdateWithNodes(
    std::unordered_map<int32_t, SemanticsNode>& nodes) {
  bool has_focus_node = false;
  last_input_focused_node_ = input_focused_node_;

  for (auto& it : nodes) {
    auto node = it.second;
    SemanticsNodeExtend* nodeExt = GetOrAddNode(it.first);
    nodeExt->UpdateWithNode(node);
    if (it.first == 0) {
      root_node_ = nodeExt;
    }
    if (!nodeExt->IsVisible()) {
      continue;
    }
    if (nodeExt->IsFocused()) {
      input_focused_node_ = nodeExt;
      has_focus_node = true;
    }
    nodeExt->childrenInTraversalOrderList.clear();
    for (auto nodeId : nodeExt->childrenInTraversalOrder) {
      nodeExt->childrenInTraversalOrderList.push_back(GetOrAddNode(nodeId));
    }
  }

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 transform;
  if (root_node_) {
    root_node_->UpdateSelfRecursively(visitorId, visitorOrder, transform,
                                      false);
    root_node_->UpdateSelfElementInfo();
    assert(!root_node_->IsFocusable());
  }

  UpdateFocusableNodesInfo(visitorOrder);

  std::unordered_set<int32_t> need_remove_ids;
  for (auto& item : all_semantics_nodes_) {
    if (visitorId.find(item.first) == visitorId.end()) {
      need_remove_ids.insert(item.first);
    }
  }

  UpdateNextFocusWhenDisappear(need_remove_ids);

  for (auto id : need_remove_ids) {
    RemoveNode(id);
  }

  if (!has_focus_node) {
    input_focused_node_ = nullptr;
  }

  std::vector<SemanticsNodeExtend*> updatedNodes;
  for (auto& it : nodes) {
    SemanticsNodeExtend* nodeExt = FindNodeById(it.first);
    if (nodeExt && nodeExt->hasUpdate) {
      updatedNodes.push_back(nodeExt);
    }
  }

  return updatedNodes;
}

bool SemanticsTree::UpdateNextFocusWhenDisappear(
    std::unordered_set<int32_t>& need_remove_ids) {
  // If the focused node disappears, we need to shift focus to a new node;
  // otherwise, the accessibility focus green border will not update.

  if (focused_node_ && (need_remove_ids.count(focused_node_->id) != 0 ||
                        !focused_node_->IsVisible())) {
    // in_request_progress_ will only be set to false when a node is
    // successfully focused; otherwise, it will keep searching for the next
    // focusable node.
    in_request_progress_ = true;
  }

  // Only update the new focus node when the current node does not exist or is
  // about to disappear.
  // The visible check is not added because the process of searching for the
  // next focus node may be interrupted:
  // A node:      +++++++--------------
  // B node:             ++++++++++++---
  // request:    A      B          [1]
  // focued event:           A            B[2]
  // clear focus event:           A
  // +:visible -:invisible
  // [1]: No need to refocus when there is no current focus node.
  // [2]: Failed to find node B, so focus fails.
  bool request_focused_node_need_update =
      !need_request_focused_node_ ||
      need_remove_ids.count(need_request_focused_node_->id) != 0;

  if (in_request_progress_ && request_focused_node_need_update) {
    // if focused_node is not null, focused_node cannot be root and must have
    // parent.
    if (focused_node_) {
      assert(focused_node_->parentNode != nullptr &&
             focused_node_ != root_node_);
    }

    SemanticsNodeExtend* find_node = nullptr;

    // Prioritize searching from need_request_focused_node_ downward, as
    // focused_node_ may have already disappeared, while
    // need_request_focused_node_ is about to disappear.
    auto start_find_node = need_request_focused_node_;
    if (!start_find_node) {
      start_find_node = focused_node_;
    }

    if (start_find_node &&
        need_remove_ids.count(start_find_node->parentNode->id) == 0) {
      // father is exsit
      // first find brother
      if (start_find_node->nextNode) {
        find_node = start_find_node->nextNode;
        while (find_node != nullptr) {
          if (find_node->isExist && find_node->IsFocusable() &&
              find_node->IsVisible() &&
              need_remove_ids.count(find_node->id) == 0) {
            break;
          }
          find_node = find_node->nextNode;
        }
      }

      if (!find_node && start_find_node->previousNode) {
        find_node = start_find_node->previousNode;
        while (find_node != nullptr) {
          if (find_node->isExist && find_node->IsFocusable() &&
              find_node->IsVisible() &&
              need_remove_ids.count(find_node->id) == 0) {
            break;
          }
          find_node = find_node->previousNode;
        }
      }

      // second find parent's other children
      if (!find_node) {
        for (auto child :
             start_find_node->parentNode->childrenInTraversalOrderList) {
          if (child->isExist && child->IsFocusable() && child->IsVisible() &&
              need_remove_ids.count(child->id) == 0) {
            find_node = child;
            break;
          }
        }
      }
    }

    // last go ancestor
    if (!find_node && start_find_node) {
      find_node = start_find_node->parentNode;
      while (find_node != nullptr && find_node->id != 0) {
        if (find_node->IsFocusable() && find_node->IsVisible() &&
            need_remove_ids.count(find_node->id) == 0) {
          break;
        }
        find_node = find_node->parentNode;
      }
    }

    // get root, then find the next focuabled node.
    if (!find_node || find_node->id == 0) {
      find_node =
          FindNextFocusNode(0, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD);
    }
    if (find_node) {
      if (find_node->id != 0) {
        need_request_focused_node_ = find_node;
        focus_request_has_send_ = false;
        return true;
      } else {
        // find root again--means it don't have a focusable node, request again.
        return false;
      }
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool SemanticsTree::FillNodeInfo(SemanticsNodeExtend* node,
                                 ArkUI_AccessibilityElementInfoList* list) {
  assert(node->parentNode || node->id == 0);
  auto info = OH_ArkUI_AddAndGetAccessibilityElementInfo(list);
  if (info != nullptr) {
    node->FillElementInfo(info);
  } else {
    FML_DLOG(ERROR) << "ohos_semantics_tree -> "
                       "OH_ArkUI_AddAndGetAccessibilityElementInfo -> "
                       "ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED";
    return false;
  }
  return true;
}

SemanticsNodeExtend* SemanticsTree::FindFocusNode(
    int32_t id,
    ArkUI_AccessibilityFocusType focusType) {
  SemanticsNodeExtend* return_node = nullptr;
  switch (focusType) {
    case ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT:
      return_node = input_focused_node_;
      break;
    case ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY:
      return_node = focused_node_;
      break;
    default: {
      FML_DLOG(ERROR) << "ohos_semantics_tree -> FindFocusNode -> "
                         "ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INVALID";
      break;
    }
  }

  if (id == -1) {
    return return_node;
  } else {
    auto temp_node = return_node;
    // check if start id is the ancestor.
    while (temp_node != nullptr) {
      if (temp_node->id == id) {
        return return_node;
      }
      temp_node = temp_node->parentNode;
    }
  }

  return nullptr;
}

// Only nodes configured with the action
// ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD/BACKWARD will
// calling the current function.
// If no next focusable node is found, return the search node.
SemanticsNodeExtend* SemanticsTree::FindNextFocusNode(
    int32_t id,
    ArkUI_AccessibilityFocusMoveDirection direction) {
  auto startNode = FindNodeById(id);
  if (!startNode) {
    FML_DLOG(ERROR) << "ohos_semantics_tree -> FindNextFocusNode -> "
                       "FindNodeById failed";
    return nullptr;
  }
  auto currentNode = startNode;

  while (true) {
    SemanticsNodeExtend* returnNode = currentNode;

    // pick next node
    switch (direction) {
      case ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_UP: {
        if (currentNode->parentNode != nullptr) {
          returnNode = currentNode->parentNode;
        }
        break;
      }
      case ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_DOWN: {
        if (!currentNode->childrenInTraversalOrderList.empty()) {
          returnNode = currentNode->childrenInTraversalOrderList[0];
        }
        break;
      }
      case ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_LEFT: {
        if (currentNode->previousNode != nullptr) {
          returnNode = currentNode->previousNode;
        }
        break;
      }
      case ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_RIGHT: {
        if (currentNode->nextNode != nullptr) {
          returnNode = currentNode->nextNode;
        }
        break;
      }
      case ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_BACKWARD: {
        if (currentNode->previousFocusableNode != nullptr) {
          returnNode = currentNode->previousFocusableNode;
        } else {
          return startNode;
        }
        break;
      }
      case ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD: {
        if (currentNode->nextFocusableNode) {
          returnNode = currentNode->nextFocusableNode;
        } else {
          return startNode;
        }
        break;
      }
      default: {
        FML_DLOG(ERROR) << "Invalid focus direction";
        return currentNode;
      }
    }
    // We should not focus on root node or cannot find the next node
    if (returnNode == root_node_ || returnNode == currentNode) {
      return startNode;
    }

    if (returnNode->IsFocusable()) {
      return returnNode;
    } else {
      currentNode = returnNode;  // enter the next iteration
    }
  }

  // Unreachable
  return nullptr;
}

bool SemanticsTree::FillNodesRecursive(
    int32_t id,
    const char* text,
    ArkUI_AccessibilityElementInfoList* list) {
  bool withText = (text != nullptr);
  int fillNum = 0;
  auto startNode = FindNodeById(id);
  if (!startNode) {
    return false;
  }
  std::queue<SemanticsNodeExtend*> q;
  q.push(startNode);
  bool retValue = true;
  std::string cppStringText = withText ? std::string(text) : "";
  while (!q.empty()) {
    auto currentNode = q.front();
    q.pop();
    if (currentNode && currentNode->isExist) {
      // In the SearchText process, we should precisely match the string of
      // SetContent.
      if (!withText || cppStringText == currentNode->contentString) {
        retValue = FillNodeInfo(currentNode, list) && retValue;
        fillNum++;
      }
      for (auto& childNode : currentNode->childrenInTraversalOrderList) {
        if (childNode && childNode->isExist) {
          q.push(childNode);
        }
      }
    }
  }
  FML_DLOG(DEBUG) << "FillNodesRecursive " << id << " " << (text ? text : "")
                  << " fillNum " << fillNum;
  return retValue;
}

bool SemanticsTree::FillNodesWithSearchText(
    int32_t id,
    const char* text,
    ArkUI_AccessibilityElementInfoList* list) {
  return FillNodesRecursive(id, text, list);
}

bool SemanticsTree::FillNodesWithSearch(
    int32_t id,
    ArkUI_AccessibilitySearchMode mode,
    ArkUI_AccessibilityElementInfoList* list) {
  auto startNode = FindNodeById(id);
  if (!startNode) {
    FML_DLOG(DEBUG) << "FillNodesWithSearch failed find " << id;
    return false;
  }
  bool retValue = true;
  // Currently, except for
  // ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CURRENT and
  // ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_RECURSIVE_CHILDREN, the
  // other ArkUI_AccessibilitySearchMode options are not invoked by the
  // accessibility framework.
  switch (mode) {
    case ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CURRENT: {
      retValue = FillNodeInfo(startNode, list) && retValue;
      break;
    }
    case ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_PREDECESSORS: {
      auto parentNode = startNode->parentNode;
      assert(parentNode != nullptr);
      retValue = FillNodeInfo(parentNode, list) && retValue;
      break;
    }
    case ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_SIBLINGS: {
      auto parentNode = startNode->parentNode;
      for (auto& siblingNode : parentNode->childrenInTraversalOrderList) {
        if (siblingNode && siblingNode->isExist) {
          retValue = FillNodeInfo(siblingNode, list) && retValue;
        }
      }
      break;
    }
    case ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CHILDREN: {
      retValue = FillNodeInfo(startNode, list) && retValue;
      for (auto& childNode : startNode->childrenInTraversalOrderList) {
        if (childNode && childNode->isExist) {
          retValue = FillNodeInfo(childNode, list) && retValue;
        }
      }
      break;
    }
    // The results of the current search mode should include the info of the
    // current node. The order of nodes should follow the level-order.
    case ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_RECURSIVE_CHILDREN: {
      retValue = FillNodesRecursive(id, nullptr, list) && retValue;
      break;
    }
    default: {
      FML_DLOG(ERROR) << "ohos_semantics_tree -> FillNodesWithSearch -> "
                         "ArkUI_AccessibilitySearchMode Invalid";
      return false;
    }
  }
  // FML_DLOG(DEBUG) << "FillNodesWithSearch " << id;
  return retValue;
}

void SemanticsTree::UpdateFocusableNodesInfo(
    std::vector<int32_t>& visitorOrder) {
  SemanticsNodeExtend* lastFocusableNode = nullptr;
  SemanticsNodeExtend* firstFocusableNode = nullptr;
  int firstFocusableIndex = -1;
  int lastFocusableIndex = -1;

  // Forward pass: fill previousFocusableNode and track focusable indices
  for (size_t i = 0; i < visitorOrder.size(); ++i) {
    auto id = visitorOrder[i];
    auto node = FindNodeById(id);
    if (!node) {
      continue;
    }

    if (node->IsFocusable()) {
      if (!firstFocusableNode) {
        firstFocusableNode = node;
        firstFocusableIndex = i;  // First focusable node index
      }

      node->previousFocusableNode = lastFocusableNode;

      if (lastFocusableNode) {
        lastFocusableNode->nextFocusableNode = node;
      }

      lastFocusableNode = node;
      lastFocusableIndex = i;  // Last focusable node index
    } else {
      node->previousFocusableNode = lastFocusableNode;
      node->nextFocusableNode = nullptr;
    }
  }

  if (!firstFocusableNode) {
    return;
  }

  // Backward pass: fill nextFocusableNode
  for (auto it = visitorOrder.rbegin(); it != visitorOrder.rend(); ++it) {
    auto node = FindNodeById(*it);
    if (!node) {
      continue;
    }

    if (node->IsFocusable()) {
      if (!node->nextFocusableNode) {
        // Link to the first focusable node
        node->nextFocusableNode = firstFocusableNode;
      }
      firstFocusableNode = node;
    }
  }

  // Handle trailing nodes without nextFocusableNode
  if (lastFocusableIndex >= 0) {
    for (size_t i = lastFocusableIndex + 1; i < visitorOrder.size(); ++i) {
      auto node = FindNodeById(visitorOrder[i]);
      if (node) {
        node->nextFocusableNode = firstFocusableNode;
      }
    }
  }

  // Handle leading nodes without previousFocusableNode
  if (firstFocusableIndex >= 0) {
    for (int i = firstFocusableIndex - 1; i >= 0; --i) {
      auto node = FindNodeById(visitorOrder[i]);
      if (node) {
        node->previousFocusableNode = lastFocusableNode;
      }
    }
  }
}

SemanticsNodeExtend* SemanticsTree::FindNodeById(int32_t id) {
  // redirect to root node
  if (id == -1) {
    id = 0;
  }
  if (all_semantics_nodes_.count(id) == 1) {
    auto node = all_semantics_nodes_[id];
    if (node->id != id) {
      // this node is some node's child but not given by UpdateSematics
      return nullptr;
    } else {
      return node;
    }
  } else {
    return nullptr;
  }
}

SemanticsNodeExtend* SemanticsTree::GetOrAddNode(int32_t id) {
  SemanticsNodeExtend* semanticsNode = all_semantics_nodes_[id];
  if (semanticsNode == nullptr) {
    semanticsNode = new SemanticsNodeExtend();
    all_semantics_nodes_[id] = semanticsNode;
  }
  return semanticsNode;
}

void SemanticsTree::ClearSemanticsTree() {
  for (auto it : all_semantics_nodes_) {
    delete it.second;
  }
  all_semantics_nodes_.clear();
  root_node_ = nullptr;
  focused_node_ = nullptr;
  need_request_focused_node_ = nullptr;
  focus_request_has_send_ = false;
  in_request_progress_ = false;
  input_focused_node_ = nullptr;
  last_input_focused_node_ = nullptr;
}
}  // namespace flutter