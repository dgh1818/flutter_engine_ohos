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

#include "ohos_semantics_node.h"
#include <arkui/native_interface_accessibility.h>
#include <cassert>
#include <cstddef>
#include <vector>
#include "flutter/shell/platform/ohos/ohos_logging.h"
#include "ohos_semantics_tree.h"

namespace flutter {
void SemanticsNodeExtend::FillElementInfo(
    ArkUI_AccessibilityElementInfo* info) {
  if (!info) {
    return;
  }

  FillElementInfoWithId(info);
  FillElementInfoWithProperty(info);
  FillElementInfoWithContent(info);
  FillElementInfoWithChildren(info);
  FillElementInfoWithParent(info);
  FillElementInfoWithScroll(info);
  FillElementInfoWithRect(info);
  FillElementInfoWithSelect(info);
}

void SemanticsNodeExtend::UpdateSelfElementInfo() {
  if (!hasInit || idChanged) {
    FillElementInfoWithId(elementInfoOHOS);
    idChanged = false;
    hasUpdate = true;
  }
  if (!hasInit || propertyChanged) {
    FillElementInfoWithProperty(elementInfoOHOS);
    propertyChanged = false;
    hasUpdate = true;
  }
  if (!hasInit || contentChanged) {
    FillElementInfoWithContent(elementInfoOHOS);
    contentChanged = false;
    hasUpdate = true;
  }
  if (!hasInit || childrenChanged) {
    FillElementInfoWithChildren(elementInfoOHOS);
    childrenChanged = false;
    hasUpdate = true;
  }
  if (!hasInit || parentChanged) {
    FillElementInfoWithParent(elementInfoOHOS);
    parentChanged = false;
    hasUpdate = true;
  }
  if (!hasInit || scrollChanged) {
    FillElementInfoWithScroll(elementInfoOHOS);
    hasUpdate = true;
    // scroll event need sent, so we don't make it false.
    // scrollChanged = false;
  }
  if (!hasInit || rectChanged) {
    FillElementInfoWithRect(elementInfoOHOS);
    rectChanged = false;
    hasUpdate = true;
  }
  if (!hasInit || selectChanged) {
    FillElementInfoWithSelect(elementInfoOHOS);
    hasUpdate = true;
    // select event need sent, so we don't make it false.
    // selectChanged = false;
  }
  hasInit = true;
}

void SemanticsNodeExtend::FillElementInfoWithId(
    ArkUI_AccessibilityElementInfo* info) {
  OH_ArkUI_AccessibilityElementInfoSetElementId(info, id);
  OH_ArkUI_AccessibilityElementInfoSetAccessibilityGroup(info, false);
}

void SemanticsNodeExtend::FillElementInfoWithProperty(
    ArkUI_AccessibilityElementInfo* info) {
  OH_ArkUI_AccessibilityElementInfoSetScrollable(info, IsScrollable());
  OH_ArkUI_AccessibilityElementInfoSetLongClickable(info, IsHasLongPress());
  OH_ArkUI_AccessibilityElementInfoSetClickable(info, IsClickable());

  OH_ArkUI_AccessibilityElementInfoSetEnabled(info, IsEnabled());
  OH_ArkUI_AccessibilityElementInfoSetFocused(info, IsFocused());
  OH_ArkUI_AccessibilityElementInfoSetIsPassword(info, IsPassword());
  OH_ArkUI_AccessibilityElementInfoSetCheckable(info, IsCheckable());
  OH_ArkUI_AccessibilityElementInfoSetChecked(info, IsChecked());
  OH_ArkUI_AccessibilityElementInfoSetVisible(info, IsVisible());
  OH_ArkUI_AccessibilityElementInfoSetSelected(info, IsSelected());
  OH_ArkUI_AccessibilityElementInfoSetEditable(info, IsEditable());
  OH_ArkUI_AccessibilityElementInfoSetFocusable(info, IsFocusable());

  OH_ArkUI_AccessibilityElementInfoSetComponentType(info, componentType);
  OH_ArkUI_AccessibilityElementInfoSetOperationActions(info, ohActions.size(),
                                                       ohActions.data());
}

void SemanticsNodeExtend::FillElementInfoWithContent(
    ArkUI_AccessibilityElementInfo* info) {
  OH_ArkUI_AccessibilityElementInfoSetAccessibilityText(info, value.c_str());
  if (IsTextField()) {
    OH_ArkUI_AccessibilityElementInfoSetHintText(info, GetHintText().c_str());
  } else {
    contentString = GetContents();
    OH_ArkUI_AccessibilityElementInfoSetContents(info, contentString.c_str());
  }
}

void SemanticsNodeExtend::FillElementInfoWithChildren(
    ArkUI_AccessibilityElementInfo* info) {
  // childrenInTraversalOrderList may less then childrenInTraversalOrder
  OH_ArkUI_AccessibilityElementInfoSetChildNodeIds(
      info, existChildrenInTraversalOrder.size(),
      existChildrenInTraversalOrder.data());
}

void SemanticsNodeExtend::FillElementInfoWithParent(
    ArkUI_AccessibilityElementInfo* info) {
  if (id == 0) {
    OH_ArkUI_AccessibilityElementInfoSetParentId(
        info, kArkuiAccessibilityRootParentId);
  } else {
    OH_ArkUI_AccessibilityElementInfoSetParentId(info, parentId);
  }
}

void SemanticsNodeExtend::FillElementInfoWithScroll(
    ArkUI_AccessibilityElementInfo* info) {
  if (scrollChildren > 0) {
    OH_ArkUI_AccessibilityElementInfoSetItemCount(info, scrollChildren);
    OH_ArkUI_AccessibilityElementInfoSetStartItemIndex(info, scrollIndex);
    OH_ArkUI_AccessibilityElementInfoSetEndItemIndex(info, scrollEndIndex);
    OH_ArkUI_AccessibilityElementInfoSetAccessibilityOffset(info,
                                                            scrollPosition);
    // todo check current index
    // if (scrollCurrentIndex != -1) {
    // OH_ArkUI_AccessibilityElementInfoSetCurrentItemIndex(info,
    //                                                      scrollIndex);
    // }
  }
}

void SemanticsNodeExtend::FillElementInfoWithRect(
    ArkUI_AccessibilityElementInfo* info) {
  ArkUI_AccessibleRect rect = {
      static_cast<int32_t>(absoluteRect.fLeft),
      static_cast<int32_t>(absoluteRect.fTop),
      static_cast<int32_t>(absoluteRect.fRight),
      static_cast<int32_t>(absoluteRect.fBottom),
  };
  OH_ArkUI_AccessibilityElementInfoSetScreenRect(info, &rect);
}

void SemanticsNodeExtend::FillElementInfoWithSelect(
    ArkUI_AccessibilityElementInfo* info) {
  if (textSelectionBase != -1 && textSelectionExtent != -1) {
    OH_ArkUI_AccessibilityElementInfoSetSelectedTextStart(info,
                                                          textSelectionBase);
    OH_ArkUI_AccessibilityElementInfoSetSelectedTextEnd(info,
                                                        textSelectionExtent);
  }
}

void SemanticsNodeExtend::OHOSComponentTypeUpdate() {
  if (id == 0) {
    componentType = OHWidgetName::kRootWidgetName;
  } else if (HasFlag(FLAGS_::kIsButton)) {
    componentType = OHWidgetName::kButtonWidgetName;
  } else if (HasFlag(FLAGS_::kIsTextField)) {
    componentType = OHWidgetName::kEditTextWidgetName;
  } else if (HasFlag(FLAGS_::kIsMultiline)) {
    componentType = OHWidgetName::kEditMultilineTextWidgetName;
  } else if (HasFlag(FLAGS_::kIsLink)) {
    componentType = OHWidgetName::kLinkWidgetName;
  } else if (HasFlag(FLAGS_::kIsSlider) || HasAction(ACTIONS_::kIncrease) ||
             HasAction(ACTIONS_::kDecrease)) {
    componentType = OHWidgetName::kSliderWidgetName;
  } else if (HasFlag(FLAGS_::kIsHeader)) {
    componentType = OHWidgetName::kHeaderWidgetName;
  } else if (HasFlag(FLAGS_::kIsImage)) {
    componentType = OHWidgetName::kImageWidgetName;
  } else if (HasFlag(FLAGS_::kHasCheckedState)) {
    if (HasFlag(FLAGS_::kIsInMutuallyExclusiveGroup)) {
      // arkui没有RadioButton，这里透传为RadioButton
      componentType = OHWidgetName::kRadioButtonWidgetName;
    } else {
      componentType = OHWidgetName::kCheckBoxWidgetName;
    }
  } else if (HasFlag(FLAGS_::kHasToggledState)) {
    componentType = OHWidgetName::kSwitchWidgetName;
  } else if (HasAction(ACTIONS_::kIncrease) || HasAction(ACTIONS_::kDecrease)) {
    componentType = OHWidgetName::kSeekbarWidgetName;
  } else if (HasFlag(FLAGS_::kHasImplicitScrolling)) {
    componentType = OHWidgetName::kScrollWidgetName;
  } else if ((!label.empty() || !tooltip.empty() || !hint.empty())) {
    componentType = OHWidgetName::kTextWidgetName;
  } else {
    componentType = OHWidgetName::kOtherWidgetName;
  }
}

void SemanticsNodeExtend::OHOSActionsUpdate() {
  ohActions.clear();
  int32_t actionTypeNum = 0;
  if (HasAction(ACTIONS_::kTap)) {
    ohActions.push_back({ArkUI_Accessibility_ActionType::
                             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK,
                         "点击操作"});
  }
  if (HasAction(ACTIONS_::kLongPress)) {
    ohActions.push_back({ArkUI_Accessibility_ActionType::
                             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK,
                         "长按操作"});
  }
  if (HasFlag(SemanticsFlags::kHasImplicitScrolling) &&
      HasAction(ACTIONS_::kScrollLeft)) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD,
         "向左滑动"});
  }
  if (HasFlag(SemanticsFlags::kHasImplicitScrolling) &&
      HasAction(ACTIONS_::kScrollRight)) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD,
         "向右滑动"});
  }
  if (HasFlag(SemanticsFlags::kHasImplicitScrolling) &&
      HasAction(ACTIONS_::kScrollUp)) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD,
         "向上滑动"});
  }
  if (HasFlag(SemanticsFlags::kHasImplicitScrolling) &&
      HasAction(ACTIONS_::kScrollDown)) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD,
         "向下滑动"});
  }
  if (HasAction(ACTIONS_::kIncrease)) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD,
         "增加"});
  }
  if (HasAction(ACTIONS_::kDecrease)) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD,
         "减少"});
  }
  if (HasAction(ACTIONS_::kShowOnScreen)) {
  }
  if (HasAction(ACTIONS_::kSetSelection)) {
    ohActions.push_back({ArkUI_Accessibility_ActionType::
                             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT,
                         "文本选择"});
  }
  if (HasAction(ACTIONS_::kCopy)) {
    ohActions.push_back({ArkUI_Accessibility_ActionType::
                             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY,
                         "文本复制"});
  }
  if (HasAction(ACTIONS_::kCut)) {
    ohActions.push_back({ArkUI_Accessibility_ActionType::
                             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT,
                         "文本剪切"});
  }
  if (HasAction(ACTIONS_::kPaste)) {
    ohActions.push_back({ArkUI_Accessibility_ActionType::
                             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE,
                         "文本粘贴"});
  }
  // We need the isVisible check to allow the accessibility framework to perform
  // a scroll action when the Scroll component focuses on the next invisible
  // node.
  if (HasAction(ACTIONS_::kDidGainAccessibilityFocus) ||
      (IsFocusable() && IsVisible())) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS,
         "获取焦点"});
  }
  if (HasAction(ACTIONS_::kDidLoseAccessibilityFocus) ||
      (IsFocusable() && IsVisible())) {
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS,
         "清除焦点"});
  }
  if (HasAction(ACTIONS_::kCustomAction)) {
  }
  if (HasAction(ACTIONS_::kDismiss)) {
  }
  if (HasAction(ACTIONS_::kMoveCursorForwardByWord) ||
      HasAction(ACTIONS_::kMoveCursorBackwardByWord) ||
      HasAction(ACTIONS_::kMoveCursorForwardByCharacter) ||
      HasAction(ACTIONS_::kMoveCursorBackwardByCharacter)) {
    // @todo we don't support this.
    ohActions.push_back(
        {ArkUI_Accessibility_ActionType::
             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION,
         "光标位置设置"});
  }
  if (HasAction(ACTIONS_::kSetText)) {
    ohActions.push_back({ArkUI_Accessibility_ActionType::
                             ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT,
                         "文本内容设置"});
  }
}

void SemanticsNodeExtend::UpdateSelfRecursively(
    std::unordered_set<int32_t>& visitorId,
    std::vector<int32_t>& visitorOrder,
    SkM44& fatherTransform,
    bool needUpdate) {
  assert(visitorId.find(id) == visitorId.end());
  visitorId.insert(id);
  visitorOrder.push_back(id);
  if (rectChanged) {
    needUpdate = true;
  }
  if (needUpdate) {
    auto [left, top, right, bottom] = rect;
    absoluteTransform = SkM44(fatherTransform, transform);
    SkV4 points[4] = {
        {left, top, 0, 1},
        {right, top, 0, 1},
        {right, bottom, 0, 1},
        {left, bottom, 0, 1},
    };
    points[0] = absoluteTransform * points[0];
    points[1] = absoluteTransform * points[1];
    points[2] = absoluteTransform * points[2];
    points[3] = absoluteTransform * points[3];
    setAbsoluteRect(
        std::min({points[0].x, points[1].x, points[2].x, points[3].x}),
        std::min({points[0].y, points[1].y, points[2].y, points[3].y}),
        std::max({points[0].x, points[1].x, points[2].x, points[3].x}),
        std::max({points[0].y, points[1].y, points[2].y, points[3].y}));
    // Update rect info: it need the father node's rect.
    rectChanged = true;
  }

  SemanticsNodeExtend* prevNode = nullptr;
  int visible_num = 0;
  int last_visible_index = 0;
  int child_index = 0;

  std::vector<int64_t> exist_children;
  for (auto& childNode : childrenInTraversalOrderList) {
    if (!childNode->isExist) {
      // some child is not updated by UpdateSemantics.
      continue;
    }
    if (childNode->IsVisible()) {
      visible_num++;
      last_visible_index = child_index;
    }
    if (childNode->isAccessibilityFocued) {
      scrollCurrentIndex = child_index;
    }
    child_index++;

    // update parent and brother ptr
    if (!childNode->parentNode || childNode->parentNode->id != id) {
      childNode->parentNode = this;
      childNode->parentId = this->id;
      childNode->parentChanged = true;
    }
    if (prevNode) {
      prevNode->nextNode = childNode;
    }
    childNode->previousNode = prevNode;
    childNode->nextNode = nullptr;
    prevNode = childNode;

    exist_children.push_back(childNode->id);
    childNode->UpdateSelfRecursively(visitorId, visitorOrder, absoluteTransform,
                                     needUpdate);
    childNode->UpdateSelfElementInfo();
  }

  if (exist_children != existChildrenInTraversalOrder) {
    existChildrenInTraversalOrder = std::move(exist_children);
    childrenChanged = true;
    // FML_DLOG(DEBUG) << id << " node children num "
    //                 << childrenInTraversalOrder.size() << " exist "
    //                 << existChildrenInTraversalOrder.size();
  }

  // Update scroll info: it need the visible child node num.
  if (scrollChildren != 0 &&
      (visible_num != scrollEndIndex - scrollIndex + 1 || scrollChanged)) {
    scrollVisibleNum = visible_num;
    scrollVisibleEndIndex = last_visible_index;

    scrollEndIndex = scrollIndex + visible_num - 1;
    scrollChanged = true;
    if (scrollIndex + visible_num > scrollChildren) {
      FML_DLOG(WARNING)
          << "UpdateSelfRecursively -> Scroll index is out of bounds "
          << scrollIndex << " visibleNum" << visible_num << " children "
          << scrollChildren;
    }
    if (!childrenInHitTestOrder.size()) {
      FML_DLOG(WARNING) << "UpdateSelfRecursively -> Had scrollChildren but no "
                           "childrenInHitTestOrder";
    }
  }
}

void SemanticsNodeExtend::UpdateWithNode(flutter::SemanticsNode& node) {
  isExist = true;

  if (id != node.id) {
    id = node.id;
    idChanged = true;
  }

  // tooltip may use for componentType
  previousLabel = label;
  if (value != node.value || label != node.label || hint != node.hint ||
      tooltip != node.tooltip) {
    value = std::move(node.value);
    label = std::move(node.label);
    hint = std::move(node.hint);
    tooltip = std::move(node.tooltip);
    if ((!label.empty() || !tooltip.empty() || !hint.empty()) &&
        componentType == OHWidgetName::kOtherWidgetName) {
      componentType = OHWidgetName::kTextWidgetName;
    }
    contentChanged = true;
  }

  previousFlags = flags;
  if (flags != node.flags) {
    flags = node.flags;
    flagChanged = true;
  }

  // IsFocusable need check flag and content
  // We will add focus action in ActionsUpdate using IsFocusable.
  previousActions = actions;
  if (actions != node.actions) {
    actions = node.actions;
    actionChanged = true;
  }

  if (textSelectionBase != node.textSelectionBase ||
      textSelectionExtent != node.textSelectionExtent) {
    textSelectionBase = node.textSelectionBase;
    textSelectionExtent = node.textSelectionExtent;
    selectChanged = true;
  }

  previousScrollPosition = scrollPosition;
  if (scrollIndex != node.scrollIndex ||
      scrollChildren != node.scrollChildren) {
    scrollPosition = node.scrollPosition;
    scrollExtentMax = node.scrollExtentMax;
    scrollExtentMin = node.scrollExtentMin;
    scrollIndex = node.scrollIndex;
    scrollChildren = node.scrollChildren;
    scrollChanged = true;
    // we need visible children num to update info.
  }

  // childrenChanged is check in UpdateSelfRecursively
  // we need know which node is not exist
  childrenInTraversalOrder = std::move(node.childrenInTraversalOrder);

  rectChanged = ((rect != node.rect) || (transform != node.transform));
  if (rectChanged) {
    rect = node.rect;
    transform = node.transform;
  }

  if (actionChanged || flagChanged || contentChanged || id == 0) {
    OHOSComponentTypeUpdate();
    OHOSActionsUpdate();
    propertyChanged = true;
  }

  maxValueLength = node.maxValueLength;
  currentValueLength = node.currentValueLength;
  platformViewId = node.platformViewId;
  elevation = node.elevation;
  thickness = node.thickness;

  valueAttributes = std::move(node.valueAttributes);
  labelAttributes = std::move(node.labelAttributes);

  hintAttributes = std::move(node.hintAttributes);
  increasedValue = std::move(node.increasedValue);
  increasedValueAttributes = std::move(node.increasedValueAttributes);
  decreasedValue = std::move(node.decreasedValue);
  decreasedValueAttributes = std::move(node.decreasedValueAttributes);
  textDirection = node.textDirection;
  childrenInHitTestOrder = std::move(node.childrenInHitTestOrder);
  customAccessibilityActions = std::move(node.customAccessibilityActions);
  identifier = std::move(node.identifier);
}

}  // namespace flutter