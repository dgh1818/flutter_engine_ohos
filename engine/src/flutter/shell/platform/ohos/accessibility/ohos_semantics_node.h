/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_NODE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_NODE_H_

#include <arkui/native_interface_accessibility.h>
#include <stdint.h>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include "flutter/lib/ui/semantics/semantics_node.h"

namespace flutter {
typedef flutter::SemanticsFlags FLAGS_;
typedef flutter::SemanticsAction ACTIONS_;

class OHWidgetName {
 public:
  static constexpr const char* kOtherWidgetName = "View";
  static constexpr const char* kTextWidgetName = "Text";
  static constexpr const char* kEditTextWidgetName = "TextInput";
  static constexpr const char* kEditMultilineTextWidgetName = "TextArea";
  static constexpr const char* kImageWidgetName = "Image";
  static constexpr const char* kScrollWidgetName = "Scroll";
  static constexpr const char* kButtonWidgetName = "Button";
  static constexpr const char* kLinkWidgetName = "Link";
  static constexpr const char* kSliderWidgetName = "Slider";
  static constexpr const char* kHeaderWidgetName = "Header";
  static constexpr const char* kRadioButtonWidgetName = "Radio";
  static constexpr const char* kCheckBoxWidgetName = "Checkbox";
  static constexpr const char* kSwitchWidgetName = "Toggle";
  static constexpr const char* kSeekbarWidgetName = "SeekBar";
  static constexpr const char* kRootWidgetName = "root";
};

struct SemanticsNodeExtend : flutter::SemanticsNode {
  static constexpr int32_t kScrollableAction =
      static_cast<int32_t>(ACTIONS_::kScrollLeft) |
      static_cast<int32_t>(ACTIONS_::kScrollRight) |
      static_cast<int32_t>(ACTIONS_::kScrollUp) |
      static_cast<int32_t>(ACTIONS_::kScrollDown);

  // @TODO why? documents don't say this.
  static constexpr int32_t kArkuiAccessibilityRootParentId = -2100000;

  bool hasUpdate = false;
  bool hasInit = false;
  bool childrenChanged = false;
  bool scrollChanged = false;
  bool selectChanged = false;
  bool flagChanged = false;
  bool actionChanged = false;
  bool propertyChanged = false;
  bool contentChanged = false;
  bool rectChanged = false;
  bool parentChanged = false;
  bool idChanged = false;
  bool isExist = false;

  bool performSelectAction = false;
  bool isAccessibilityFocued = false;
  SemanticsFlags previousFlags;
  int32_t previousActions = 0;
  double previousScrollPosition = std::nan("");
  std::string previousLabel;
  SemanticsNodeExtend* parentNode = nullptr;
  SemanticsNodeExtend* previousNode = nullptr;
  SemanticsNodeExtend* nextNode = nullptr;
  SemanticsNodeExtend* previousFocusableNode = nullptr;
  SemanticsNodeExtend* nextFocusableNode = nullptr;
  std::vector<SemanticsNodeExtend*> childrenInTraversalOrderList;
  std::vector<int64_t> existChildrenInTraversalOrder;

  SkRect absoluteRect;
  SkM44 absoluteTransform;
  int32_t scrollEndIndex = 0;
  int32_t scrollCurrentIndex = -1;
  int32_t scrollVisibleNum = 0;
  int32_t scrollVisibleEndIndex = 0;
  uint32_t parentId = 0;

  std::string contentString = "";
  ArkUI_AccessibilityElementInfo* elementInfoOHOS = nullptr;
  const char* componentType = OHWidgetName::kOtherWidgetName;
  std::vector<ArkUI_AccessibleAction> ohActions;

  SemanticsNodeExtend() {
    elementInfoOHOS = OH_ArkUI_CreateAccessibilityElementInfo();
  }

  ~SemanticsNodeExtend() {
    OH_ArkUI_DestoryAccessibilityElementInfo(elementInfoOHOS);
  }

  void FillElementInfo(ArkUI_AccessibilityElementInfo* info);
  void UpdateSelfElementInfo();
  void FillElementInfoWithId(ArkUI_AccessibilityElementInfo* info);
  void FillElementInfoWithProperty(ArkUI_AccessibilityElementInfo* info);
  void FillElementInfoWithContent(ArkUI_AccessibilityElementInfo* info);
  void FillElementInfoWithChildren(ArkUI_AccessibilityElementInfo* info);
  void FillElementInfoWithScroll(ArkUI_AccessibilityElementInfo* info);
  void FillElementInfoWithRect(ArkUI_AccessibilityElementInfo* info);
  void FillElementInfoWithSelect(ArkUI_AccessibilityElementInfo* info);
  void FillElementInfoWithParent(ArkUI_AccessibilityElementInfo* info);

  void OHOSActionsUpdate();
  void OHOSComponentTypeUpdate();

  void UpdateWithNode(flutter::SemanticsNode& node);
  void UpdateSelfRecursively(std::unordered_set<int32_t>& visitorId,
                             std::vector<int32_t>& visitorOrder,
                             SkM44& fatherTransform,
                             bool needUpdat);

  bool HasPrevAction(SemanticsAction action) const {
    return (previousActions & this->actions) != 0;
  }

  void setAbsoluteRect(float left, float top, float right, float bottom) {
    absoluteRect.fLeft = left;
    absoluteRect.fTop = top;
    absoluteRect.fRight = right;
    absoluteRect.fBottom = bottom;
  }

  bool IsTextField() { return flags.isTextField; }
  bool IsEditable() { return IsTextField() && !flags.isReadOnly; }
  bool IsSlider() { return flags.isSlider; }
  bool IsVisible() { return !flags.isHidden; }
  bool IsCheckable() {
    return flags.hasCheckedState || flags.hasToggledState;
  }
  bool IsChecked() {
    return flags.isChecked || flags.isToggled;
  }
  bool IsSelected() { return flags.isSelected; }
  bool IsPassword() {
    return flags.isTextField && flags.isObscured;
  }
  bool IsEnabled() {
    return !flags.hasEnabledState || flags.isEnabled;
  }
  bool IsClickable() { return HasAction(ACTIONS_::kTap); }
  bool IsHasLongPress() { return HasAction(ACTIONS_::kLongPress); }
  bool HasScrolled() {
    return scrollPosition != std::nan("") &&
           previousScrollPosition != std::nan("") &&
           previousScrollPosition != scrollPosition;
  }
  bool HasChangedLabel() {
    if (label.empty() && previousLabel.empty()) {
      return false;
    }
    return label.empty() || previousLabel.empty() || label != previousLabel;
  }
  bool IsFocusable() {
    if (flags.scopesRoute) {
      return false;
    }
    if (flags.isFocusable) {
      return true;
    }
    if (IsPlatformViewNode()) {
      return true;
    }
    // Check if any focusable flags are set
    if (flags.hasCheckedState || flags.isChecked || flags.isSelected ||
        flags.isTextField || flags.isFocused || flags.hasEnabledState ||
        flags.isEnabled || flags.isInMutuallyExclusiveGroup ||
        flags.hasToggledState || flags.isToggled || flags.isSlider) {
      return true;
    }
    if ((actions & ~kScrollableAction) != 0) {
      return true;
    }
    return !label.empty() || !value.empty() || !hint.empty();
  }
  bool IsFocused() { return flags.isFocused; }
  bool IsScrollable() {
    return HasAction(ACTIONS_::kScrollLeft) ||
           HasAction(ACTIONS_::kScrollRight) ||
           HasAction(ACTIONS_::kScrollUp) || HasAction(ACTIONS_::kScrollDown);
  }

  std::string GetHintText() {
    std::string result = label;
    if (result.length() != 0) {
      if (hint.length() != 0) {
        result += " ," + hint;
      }
    } else {
      result = hint;
    }
    return result;
  }

  std::string GetAccessibilityText() {
    std::string result = GetHintText();
    if (value.length() != 0) {
      if (result.length() != 0) {
        result = value + " ," + result;
      } else {
        result = value;
      }
    }
    return result;
  }
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_NODE_H_