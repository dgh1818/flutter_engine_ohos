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
  SkM44 transform;
  if (root_node_) {
    root_node_->UpdateSelfRecursively(visitorId, transform, false);
    root_node_->UpdateSelfElementInfo();
    assert(!root_node_->IsFocusable());
  }

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

  bool focused_node_is_disappear = false;
  if (focused_node_ && (need_remove_ids.count(focused_node_->id) != 0 ||
                        !focused_node_->IsVisible())) {
    focused_node_is_disappear = true;
  }
  bool request_focused_node_need_update =
      !need_request_focused_node_ || !need_request_focused_node_->IsVisible();

  if ((focused_node_is_disappear || need_find_focus_node_again_) &&
      request_focused_node_need_update) {
    // if focused_node is not null, focused_node cannot be root and must have
    // parent.
    if (focused_node_) {
      assert(focused_node_->parentNode != nullptr &&
             focused_node_ != root_node_);
    }

    SemanticsNodeExtend* find_node = nullptr;

    if (focused_node_ &&
        need_remove_ids.count(focused_node_->parentNode->id) == 0) {
      // father is exsit
      // first find brother
      if (focused_node_->nextNode) {
        find_node = focused_node_->nextNode;
        while (find_node != nullptr) {
          if (find_node->isExist && find_node->IsFocusable() &&
              find_node->IsVisible() &&
              need_remove_ids.count(find_node->id) == 0) {
            break;
          }
          find_node = find_node->nextNode;
        }
      }

      if (!find_node && focused_node_->previousNode) {
        find_node = focused_node_->previousNode;
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
             focused_node_->parentNode->childrenInTraversalOrderList) {
          if (child->isExist && child->IsFocusable() && child->IsVisible() &&
              need_remove_ids.count(child->id) == 0) {
            find_node = child;
            break;
          }
        }
      }
    }

    // last go ancestor
    if (!find_node && focused_node_) {
      find_node = focused_node_->parentNode;
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
        need_find_focus_node_again_ = false;
        return true;
      } else {
        // find root again--means it don't have a focusable node, request again.
        need_find_focus_node_again_ = true;
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
        if (currentNode->previousNode != nullptr) {
          returnNode = currentNode->previousNode;
        } else if (currentNode->parentNode != nullptr) {
          returnNode = currentNode->parentNode;
        }
        break;
      }
      case ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD: {
        SemanticsNodeExtend* candidateNode = nullptr;
        for (auto childNode : currentNode->childrenInTraversalOrderList) {
          if (childNode && childNode->isExist &&
              childNode->focusableInSubtree) {
            candidateNode = childNode;
            break;
          }
        }
        if (candidateNode) {
          returnNode = candidateNode;
        } else if (currentNode->nextNode != nullptr) {
          returnNode = currentNode->nextNode;
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
    if (currentNode) {
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
  need_find_focus_node_again_ = false;
  input_focused_node_ = nullptr;
  last_input_focused_node_ = nullptr;
}
}  // namespace flutter