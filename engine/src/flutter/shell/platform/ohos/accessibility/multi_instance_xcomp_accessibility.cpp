/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#include "multi_instance_xcomp_accessibility.h"
#include "flutter/shell/platform/ohos/ohos_logging.h"
#include "flutter/shell/platform/ohos/ohos_xcomponent_adapter.h"

namespace flutter {

/** Called when need to get element infos based on a specified node. */
static int32_t FindA11yNodeInfosByIdCallbackWithInstance(
    const char* instanceId,
    int64_t elementId,
    ArkUI_AccessibilitySearchMode mode,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  LOGD(
      "a11yProviderCallbackWithInstance_.FindAccessibilityNodeInfosById, "
      "instanceId:%{public}s mode:%{public}d id:%{public}ld",
      instanceId, mode, elementId);
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetXcomponentBase(
      std::string(instanceId));
  if (xcomp != nullptr) {
    return xcomp->FindAccessibilityNodeInfosById(elementId, mode, requestId,
                                                 elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Called when need to get element infos based on a specified node and text
 * content. */
int32_t FindA11yNodeInfosByTextCallbackWithInstance(
    const char* instanceId,
    int64_t elementId,
    const char* text,
    int32_t requestId,
    ArkUI_AccessibilityElementInfoList* elementList) {
  LOGD("a11yProviderCallbackWithInstance_.FindAccessibilityNodeInfosByText");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetXcomponentBase(
      std::string(instanceId));
  if (xcomp != nullptr) {
    return xcomp->FindAccessibilityNodeInfosByText(elementId, text, requestId,
                                                   elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Called when need to get the focused element info based on a specified node.
 */
int32_t FindFocusedA11yNodeCallbackWithInstance(
    const char* instanceId,
    int64_t elementId,
    ArkUI_AccessibilityFocusType focusType,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementinfo) {
  LOGD("a11yProviderCallbackWithInstance_.FindFocusedAccessibilityNode");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetXcomponentBase(
      std::string(instanceId));
  if (xcomp != nullptr) {
    return xcomp->FindFocusedAccessibilityNode(elementId, focusType, requestId,
                                               elementinfo);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Query the node that can be focused based on the reference node. Query the
 * next node that can be focused based on the mode and direction. */
int32_t FindNextFocusA11yNodeCallbackWithInstance(
    const char* instanceId,
    int64_t elementId,
    ArkUI_AccessibilityFocusMoveDirection direction,
    int32_t requestId,
    ArkUI_AccessibilityElementInfo* elementList) {
  LOGD("a11yProviderCallbackWithInstance_.FindNextFocusAccessibilityNode");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetXcomponentBase(
      std::string(instanceId));
  if (xcomp != nullptr) {
    return xcomp->FindNextFocusAccessibilityNode(elementId, direction,
                                                 requestId, elementList);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Performing the Action operation on a specified node. */
int32_t ExecuteA11yActionCallbackWithInstance(
    const char* instanceId,
    int64_t elementId,
    ArkUI_Accessibility_ActionType action,
    ArkUI_AccessibilityActionArguments* actionArguments,
    int32_t requestId) {
  LOGD(
      "a11yProviderCallbackWithInstance_.ExecuteAccessibilityAction, "
      "instanceId:%{public}s action:%{public}d id:%{public}ld",
      instanceId, action, elementId);
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetXcomponentBase(
      std::string(instanceId));
  if (xcomp != nullptr) {
    return xcomp->ExecuteAccessibilityAction(elementId, action, actionArguments,
                                             requestId);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Clears the focus status of the currently focused node */
int32_t ClearFocusedFocusA11yNodeCallbackWithInstance(const char* instanceId) {
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetXcomponentBase(
      std::string(instanceId));
  LOGD("a11yProviderCallbackWithInstance_.ClearFocusedFocusAccessibilityNode");
  if (xcomp != nullptr) {
    return xcomp->ClearFocusedFocusAccessibilityNode(0);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

/** Queries the current cursor position of a specified node. */
int32_t GetA11yNodeCursorPositionCallbackWithInstance(const char* instanceId,
                                                      int64_t elementId,
                                                      int32_t requestId,
                                                      int32_t* index) {
  LOGD("a11yProviderCallbackWithInstance_.GetAccessibilityNodeCursorPosition");
  std::lock_guard<std::recursive_mutex> lock(
      XComponentAdapter::GetInstance()->xcomponentMap_mutex_);
  auto xcomp = XComponentAdapter::GetInstance()->GetXcomponentBase(
      std::string(instanceId));
  if (xcomp != nullptr) {
    return xcomp->GetAccessibilityNodeCursorPosition(elementId, requestId,
                                                     index);
  } else {
    return ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
  }
}

void MultiInstanceXCompAccessibility::BindAccessibilityCallbackWithInstance() {
  a11yProviderCallbackWithInstance_.findAccessibilityNodeInfosById =
      FindA11yNodeInfosByIdCallbackWithInstance;
  a11yProviderCallbackWithInstance_.findAccessibilityNodeInfosByText =
      FindA11yNodeInfosByTextCallbackWithInstance;
  a11yProviderCallbackWithInstance_.findFocusedAccessibilityNode =
      FindFocusedA11yNodeCallbackWithInstance;
  a11yProviderCallbackWithInstance_.findNextFocusAccessibilityNode =
      FindNextFocusA11yNodeCallbackWithInstance;
  a11yProviderCallbackWithInstance_.executeAccessibilityAction =
      ExecuteA11yActionCallbackWithInstance;
  a11yProviderCallbackWithInstance_.clearFocusedFocusAccessibilityNode =
      ClearFocusedFocusA11yNodeCallbackWithInstance;
  a11yProviderCallbackWithInstance_.getAccessibilityNodeCursorPosition =
      GetA11yNodeCursorPositionCallbackWithInstance;
}
}  // namespace flutter