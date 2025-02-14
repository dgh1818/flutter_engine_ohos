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

#include "ohos_semantics_bridge.h"
#include <arkui/native_interface_accessibility.h>
#include <cassert>
#include <string>

namespace flutter {

void SemanticsBridge::UpdateNodeTree(flutter::SemanticsNodeUpdates& nodes) {
  auto update_nodes = tree_.UpdateWithNodes(nodes);

  for (auto node : update_nodes) {
    // if node perform scroll
    if (node->scrollChanged && node->performScrollAction) {
      std::string child_str = "";
      for (auto id : node->childrenInTraversalOrder) {
        child_str += std::to_string(id) + ",";
      }
      FML_DLOG(INFO) << node->id << " scroll index " << node->scrollIndex
                     << " current " << node->scrollCurrentIndex << " end "
                     << node->scrollEndIndex << " child num "
                     << node->scrollChildren << " children:" << child_str
                     << " has update " << node->hasUpdate << " "
                     << node->scrollVisibleEndIndex;
      child_str = "";
      for (auto node : node->childrenInTraversalOrderList) {
        if (node->isExist && node->IsVisible()) {
          child_str += std::to_string(node->id) + ",";
        }
      }
      FML_DLOG(INFO) << node->id << " scroll index " << node->scrollIndex
                     << " end " << node->scrollEndIndex << " given child num "
                     << node->scrollChildren
                     << " visible children:" << child_str;

      if (node->scrollEndIndex != -1 && node->scrollCurrentIndex != -1) {
        SendSemanticsEvent(node, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_SCROLLED,
                           nullptr);
        FML_DLOG(INFO) << node->id << " update scroll ";
        node->scrollChanged = false;
        node->performScrollAction = false;
      }
    }

    // text is selected
    if (node->performSelectAction && node->selectChanged) {
      SendSemanticsEvent(node, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_SELECTED,
                         nullptr);
      node->selectChanged = false;
      node->performSelectAction = false;
    }
  }

  UpdateFocusedNode();
}

void SemanticsBridge::UpdateFocusedNode() {
  auto focused_node = tree_.focused_node_;

  auto root_node = tree_.GetRootNode();
  if (has_navigationed_ && root_node != nullptr) {
    has_navigationed_ = false;
    SendSemanticsEvent(
        root_node, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_PAGE_CONTENT_UPDATE,
        nullptr);
  }

  if (focused_node && focused_node->hasUpdate) {
    SendSemanticsEvent(focused_node,
                       ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_FOCUS_NODE_UPDATE,
                       nullptr);
  }

  // If focused_node disppear, we need request next focusable_node.
  if (tree_.need_request_focused_node_ && !tree_.focus_request_has_send_) {
    FML_DLOG(INFO) << "UpdateFocusedNode request_next_node "
                   << tree_.need_request_focused_node_->id << " node content "
                   << tree_.need_request_focused_node_->contentString;
    SendSemanticsEvent(
        tree_.need_request_focused_node_,
        ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_REQUEST_ACCESSIBILITY_FOCUS,
        nullptr);
    tree_.focus_request_has_send_ = true;
  }

  return;
}

SemanticsNodeExtend* SemanticsBridge::GetNodeById(int32_t id) {
  return tree_.FindNodeById(id);
}

void SemanticsBridge::SendSemanticsEvent(SemanticsNodeExtend* node,
                                         ArkUI_AccessibilityEventType type,
                                         const char* message) {
  if (!is_accessibility_enabled_) {
    return;
  }
  auto event = OH_ArkUI_CreateAccessibilityEventInfo();
  if (node) {
    OH_ArkUI_AccessibilityEventSetElementInfo(event, node->elementInfoOHOS);
    node->hasUpdate = false;
  }
  OH_ArkUI_AccessibilityEventSetEventType(event, type);
  if (type ==
          ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ANNOUNCE_FOR_ACCESSIBILITY &&
      message != nullptr) {
    OH_ArkUI_AccessibilityEventSetTextAnnouncedForAccessibility(event, message);
  }
  if (provider_ohos_) {
    auto callback = [](int32_t errorCode) {
      if (errorCode != 0) {
        FML_DLOG(INFO) << "SendSemanticsEvent callback-> errorCode ="
                       << errorCode;
      }
    };
    OH_ArkUI_SendAccessibilityAsyncEvent(provider_ohos_, event, callback);
    FML_DLOG(INFO) << "SendSemanticsEvent type " << (int64_t)type
                   << " node:" << (node ? node->id : -1) << " visible "
                   << node->IsVisible()
                   << " message:" << (message ? message : "");
  }

  OH_ArkUI_DestoryAccessibilityEventInfo(event);
}

int32_t SemanticsBridge::FindFocusNode(int32_t id,
                                       ArkUI_AccessibilityFocusType focusType,
                                       ArkUI_AccessibilityElementInfo* info) {
  auto node = tree_.FindFocusNode(id, focusType);
  if (node) {
    node->FillElementInfo(info);
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t SemanticsBridge::FindNextFocusNode(
    int32_t id,
    ArkUI_AccessibilityFocusMoveDirection direction,
    ArkUI_AccessibilityElementInfo* info) {
  auto node = tree_.FindNextFocusNode(id, direction);
  if (node) {
    node->FillElementInfo(info);
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t SemanticsBridge::FillNodesWithSearchText(
    int32_t id,
    const char* text,
    ArkUI_AccessibilityElementInfoList* list) {
  if (tree_.FillNodesWithSearchText(id, text, list)) {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t SemanticsBridge::FillNodesWithSearch(
    int32_t id,
    ArkUI_AccessibilitySearchMode mode,
    ArkUI_AccessibilityElementInfoList* list) {
  if (tree_.FillNodesWithSearch(id, mode, list)) {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t SemanticsBridge::ClearAccessibilityFocus(int32_t id) {
  auto node = tree_.FindFocusNode(
      -1, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY);
  FML_DLOG(INFO) << "ClearAccessibilityFocus " << id;
  if (node && (node->id == id || id == 0)) {
    tree_.ClearAccessibilityFocusNode();
    SendSemanticsEvent(
        node, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ACCESSIBILITY_FOCUS_CLEARED,
        nullptr);
  }
  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

int32_t SemanticsBridge::GainAccessibilityFocus(int32_t id) {
  if (tree_.SetAccessibilityFocusNode(id)) {
    auto node = tree_.FindNodeById(id);
    FML_DLOG(INFO) << "GainAccessibilityFocus " << id;
    SendSemanticsEvent(
        node, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ACCESSIBILITY_FOCUSED,
        nullptr);
    // if node has update
    if (node->hasUpdate) {
      SendSemanticsEvent(
          node, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_FOCUS_NODE_UPDATE,
          nullptr);
    }
    if (node == tree_.need_request_focused_node_) {
      tree_.need_request_focused_node_ = nullptr;
      tree_.focus_request_has_send_ = false;
    }
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

int32_t SemanticsBridge::GetAccessibilityNodeCursorPosition(int64_t elementId,
                                                            int32_t* index) {
  auto node = tree_.FindNodeById(elementId);
  if (node) {
    *index = node->textSelectionBase;
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

void SemanticsBridge::Announce(std::unique_ptr<char[]>& message) {
  SendSemanticsEvent(
      nullptr, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ANNOUNCE_FOR_ACCESSIBILITY,
      message.get());
  FML_DLOG(INFO) << "Announce -> message: " << message.get();
}

void SemanticsBridge::OnTap(int32_t nodeId) {
  SendSemanticsEvent(tree_.FindNodeById(nodeId),
                     ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_CLICKED, nullptr);
  FML_DLOG(INFO) << "OnTap -> nodeId: " << nodeId;
}

void SemanticsBridge::OnLongPress(int32_t nodeId) {
  SendSemanticsEvent(tree_.FindNodeById(nodeId),
                     ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_LONG_CLICKED,
                     nullptr);
  FML_DLOG(INFO) << "OnLongPress -> nodeId: " << nodeId;
}

void SemanticsBridge::OnTooltip(std::unique_ptr<char[]>& message) {
  SendSemanticsEvent(tree_.FindNodeById(0),
                     ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_PAGE_STATE_UPDATE,
                     nullptr);
  SendSemanticsEvent(
      nullptr, ARKUI_ACCESSIBILITY_NATIVE_EVENT_TYPE_ANNOUNCE_FOR_ACCESSIBILITY,
      message.get());
  FML_DLOG(INFO) << "OnTooltip -> message: " << message.get();
}

void SemanticsBridge::OnAccessibilityStateChange(bool state) {
  is_accessibility_enabled_ = state;
}

void SemanticsBridge::OnAccessibilityNavigation(bool is_nav) {
  has_navigationed_ = is_nav;
}

}  // namespace flutter