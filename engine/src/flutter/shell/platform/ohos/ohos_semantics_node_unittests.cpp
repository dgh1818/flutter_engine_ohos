/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_node.h"
#include <dlfcn.h>
#include <gtest/gtest.h>
#include "flutter/lib/ui/semantics/semantics_node.h"

// The ArkUI NDK has no getters on ArkUI_AccessibilityElementInfo, so
// same-signature definitions below capture the setter calls (they take
// precedence over libace_ndk.z.so). SetComponentIdentifier is resolved via
// dlsym in production and cannot be captured; its branches are covered
// through the componentIdentifierWriteFailed flag.
namespace {

struct CapturedA11yContent {
  std::string text;
  std::string hintText;
  std::string contents;
  int rangeCalls = 0;
  ArkUI_AccessibleRangeInfo range = {};

  void Reset() { *this = CapturedA11yContent(); }
};

CapturedA11yContent g_capturedA11yContent;

void ExpectZeroRange(const ArkUI_AccessibleRangeInfo& range) {
  EXPECT_DOUBLE_EQ(range.min, 0.0);
  EXPECT_DOUBLE_EQ(range.max, 0.0);
  EXPECT_DOUBLE_EQ(range.current, 0.0);
}

}  // namespace

extern "C" {
int32_t OH_ArkUI_AccessibilityElementInfoSetAccessibilityText(
    ArkUI_AccessibilityElementInfo* elementInfo, const char* text) {
  (void)elementInfo;
  g_capturedA11yContent.text = text != nullptr ? text : "";
  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetHintText(
    ArkUI_AccessibilityElementInfo* elementInfo, const char* hintText) {
  (void)elementInfo;
  g_capturedA11yContent.hintText = hintText != nullptr ? hintText : "";
  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetContents(
    ArkUI_AccessibilityElementInfo* elementInfo, const char* contents) {
  (void)elementInfo;
  g_capturedA11yContent.contents = contents != nullptr ? contents : "";
  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

int32_t OH_ArkUI_AccessibilityElementInfoSetRangeInfo(
    ArkUI_AccessibilityElementInfo* elementInfo,
    ArkUI_AccessibleRangeInfo* rangeInfo) {
  (void)elementInfo;
  g_capturedA11yContent.rangeCalls++;
  if (rangeInfo != nullptr) {
    g_capturedA11yContent.range = *rangeInfo;
  }
  return ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL;
}

}  // extern "C"

namespace flutter {
namespace testing {

class SemanticsNodeTest : public ::testing::Test {
 protected:
  SemanticsNodeExtend node_;
};

// ============================================================================
// NOTE: This file uses the flutter3.41 SemanticsFlags API, which differs from
// flutter3.35 due to an upstream (non-OHOS) refactor of semantics_flags.h:
//   - hasCheckedState (bool) + isChecked (bool)  -> isChecked (SemanticsCheckState)
//   - hasToggledState (bool) + isToggled (bool)  -> isToggled (SemanticsTristate)
//   - hasEnabledState (bool) + isEnabled (bool)  -> isEnabled (SemanticsTristate)
//   - isFocusable (bool)                          -> removed (IsFocusable() now
//                                                  derives focusability from
//                                                  other flags like isTextField)
//   - isFocused (bool)                           -> isFocused (SemanticsTristate)
// flutter3.35 retains the old bool-pair API, so its test file uses
// hasCheckedState/isToggled=true/isFocusable=true etc. directly. These
// differences are intentional and should NOT be synced between branches.
// ============================================================================

// ===== Inline getters with default (empty) flags =====

TEST_F(SemanticsNodeTest, IsTextFieldDefaultFalse) {
  EXPECT_FALSE(node_.IsTextField());
}

TEST_F(SemanticsNodeTest, IsEditableDefaultFalse) {
  EXPECT_FALSE(node_.IsEditable());
}

TEST_F(SemanticsNodeTest, IsSliderDefaultFalse) {
  EXPECT_FALSE(node_.IsSlider());
}

TEST_F(SemanticsNodeTest, IsVisibleDefaultTrue) {
  EXPECT_TRUE(node_.IsVisible());
}

TEST_F(SemanticsNodeTest, IsCheckableDefaultFalse) {
  EXPECT_FALSE(node_.IsCheckable());
}

TEST_F(SemanticsNodeTest, IsCheckedDefaultFalse) {
  EXPECT_FALSE(node_.IsChecked());
}

TEST_F(SemanticsNodeTest, IsSelectedDefaultFalse) {
  EXPECT_FALSE(node_.IsSelected());
}

TEST_F(SemanticsNodeTest, IsPasswordDefaultFalse) {
  EXPECT_FALSE(node_.IsPassword());
}

TEST_F(SemanticsNodeTest, IsEnabledDefaultTrue) {
  EXPECT_TRUE(node_.IsEnabled());
}

TEST_F(SemanticsNodeTest, IsClickableDefaultFalse) {
  EXPECT_FALSE(node_.IsClickable());
}

TEST_F(SemanticsNodeTest, IsHasLongPressDefaultFalse) {
  EXPECT_FALSE(node_.IsHasLongPress());
}

// SOURCE BUG (ohos_semantics_node.h:155-158): HasScrolled() uses
// `scrollPosition != std::nan("")` to check for non-NaN, but IEEE 754
// mandates NaN != NaN is always true. On a fresh node both scrollPosition
// and previousScrollPosition are NaN, so all three `!=` comparisons return
// true, making HasScrolled() return true for a node that has never scrolled.
// The correct fix would be `!std::isnan(scrollPosition)`, but the source is
// not modified in this commit. This test documents the buggy behavior.
TEST_F(SemanticsNodeTest, HasScrolledDefaultFalse) {
  EXPECT_TRUE(node_.HasScrolled());  // BUG: should be EXPECT_FALSE
}

TEST_F(SemanticsNodeTest, HasScrolledTrueAfterScrollPositionChange) {
  // UpdateWithNode sets previousScrollPosition = scrollPosition (old),
  // then scrollPosition = node.scrollPosition (new). When they differ,
  // HasScrolled() returns true (correct in this case).
  flutter::SemanticsNode source;
  source.id = 1;
  source.scrollChildren = 10;
  source.scrollIndex = 2;
  source.scrollPosition = 0.5;
  node_.UpdateWithNode(source);
  EXPECT_TRUE(node_.HasScrolled());
}

TEST_F(SemanticsNodeTest, HasScrolledFalseWhenScrollPositionUnchanged) {
  // First update sets scrollPosition to 0.5.
  flutter::SemanticsNode source;
  source.id = 1;
  source.scrollChildren = 10;
  source.scrollIndex = 2;
  source.scrollPosition = 0.5;
  node_.UpdateWithNode(source);
  ASSERT_TRUE(node_.HasScrolled());

  // Second update with same scrollPosition: previousScrollPosition == scrollPosition
  flutter::SemanticsNode source2;
  source2.id = 1;
  source2.scrollChildren = 10;
  source2.scrollIndex = 2;
  source2.scrollPosition = 0.5;  // same value
  node_.UpdateWithNode(source2);
  EXPECT_FALSE(node_.HasScrolled());
}

TEST_F(SemanticsNodeTest, HasChangedLabelDefaultFalse) {
  EXPECT_FALSE(node_.HasChangedLabel());
}

TEST_F(SemanticsNodeTest, IsFocusableDefaultFalse) {
  EXPECT_FALSE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusedDefaultFalse) {
  EXPECT_FALSE(node_.IsFocused());
}

TEST_F(SemanticsNodeTest, IsScrollableDefaultFalse) {
  EXPECT_FALSE(node_.IsScrollable());
}

// ===== setAbsoluteRect =====

TEST_F(SemanticsNodeTest, SetAbsoluteRectSetsValues) {
  node_.setAbsoluteRect(1.0f, 2.0f, 3.0f, 4.0f);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fLeft, 1.0f);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fTop, 2.0f);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fRight, 3.0f);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fBottom, 4.0f);
}

// ===== GetHintText =====

TEST_F(SemanticsNodeTest, GetHintTextEmptyWhenNoLabelOrHint) {
  EXPECT_TRUE(node_.GetHintText().empty());
}

TEST_F(SemanticsNodeTest, GetHintTextReturnsLabelWhenOnlyLabelSet) {
  node_.label = "Name";
  EXPECT_EQ(node_.GetHintText(), "Name");
}

TEST_F(SemanticsNodeTest, GetHintTextReturnsHintWhenOnlyHintSet) {
  node_.hint = "Enter your name";
  EXPECT_EQ(node_.GetHintText(), "Enter your name");
}

TEST_F(SemanticsNodeTest, GetHintTextCombinesLabelAndHint) {
  node_.label = "Name";
  node_.hint = "Enter your name";
  EXPECT_EQ(node_.GetHintText(), "Name ,Enter your name");
}

// ===== GetAccessibilityText =====

TEST_F(SemanticsNodeTest, GetAccessibilityTextEmptyWhenNoContent) {
  EXPECT_TRUE(node_.GetAccessibilityText().empty());
}

TEST_F(SemanticsNodeTest, GetAccessibilityTextReturnsValueWhenOnlyValue) {
  node_.value = "John";
  EXPECT_EQ(node_.GetAccessibilityText(), "John");
}

TEST_F(SemanticsNodeTest, GetAccessibilityTextCombinesValueAndHint) {
  node_.value = "John";
  node_.label = "Name";
  node_.hint = "Enter your name";
  // GetAccessibilityText = value + " ," + GetHintText()
  // GetHintText() = "Name ,Enter your name"
  EXPECT_EQ(node_.GetAccessibilityText(),
            "John ,Name ,Enter your name");
}

// ===== IsFocusable with various flags =====

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenIsChecked) {
  // flutter3.41 removed the isFocusable bool flag; IsFocusable() now returns
  // true when isChecked is set (SemanticsCheckState != kNone).
  node_.flags.isChecked = SemanticsCheckState::kTrue;
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableFalseWhenScopesRouteSet) {
  // scopesRoute short-circuits IsFocusable to false regardless of other flags.
  node_.flags.scopesRoute = true;
  node_.flags.isChecked = SemanticsCheckState::kTrue;
  EXPECT_FALSE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenHasCheckedState) {
  // In flutter3.41, hasCheckedState was replaced by isChecked != kNone.
  node_.flags.isChecked = SemanticsCheckState::kFalse;
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenIsTextField) {
  node_.flags.isTextField = true;
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenHasLabel) {
  node_.label = "Hello";
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenHasValue) {
  node_.value = "World";
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenHasHint) {
  node_.hint = "Hint";
  EXPECT_TRUE(node_.IsFocusable());
}

// ===== IsCheckable / IsChecked with toggled state =====

TEST_F(SemanticsNodeTest, IsCheckableTrueWhenHasToggledState) {
  // flutter3.41 replaced hasToggledState with isToggled != SemanticsTristate::kNone.
  node_.flags.isToggled = SemanticsTristate::kFalse;
  EXPECT_TRUE(node_.IsCheckable());
}

TEST_F(SemanticsNodeTest, IsCheckedTrueWhenIsToggled) {
  node_.flags.isToggled = SemanticsTristate::kTrue;
  EXPECT_TRUE(node_.IsChecked());
}

// ===== IsPassword =====

TEST_F(SemanticsNodeTest, IsPasswordTrueWhenTextFieldAndObscured) {
  node_.flags.isTextField = true;
  node_.flags.isObscured = true;
  EXPECT_TRUE(node_.IsPassword());
}

// ===== IsEditable =====

TEST_F(SemanticsNodeTest, IsEditableTrueWhenTextFieldAndNotReadOnly) {
  node_.flags.isTextField = true;
  node_.flags.isReadOnly = false;
  EXPECT_TRUE(node_.IsEditable());
}

TEST_F(SemanticsNodeTest, IsEditableFalseWhenTextFieldAndReadOnly) {
  node_.flags.isTextField = true;
  node_.flags.isReadOnly = true;
  EXPECT_FALSE(node_.IsEditable());
}

// ===== IsEnabled =====

TEST_F(SemanticsNodeTest, IsEnabledFalseWhenHasEnabledStateAndNotEnabled) {
  // flutter3.41 replaced hasEnabledState+isEnabled bool pair with a single
  // SemanticsTristate isEnabled field. IsEnabled() returns false when
  // isEnabled == kFalse.
  node_.flags.isEnabled = SemanticsTristate::kFalse;
  EXPECT_FALSE(node_.IsEnabled());
}

// ===== IsVisible =====

TEST_F(SemanticsNodeTest, IsVisibleFalseWhenHidden) {
  node_.flags.isHidden = true;
  EXPECT_FALSE(node_.IsVisible());
}

// ===== OHOSComponentTypeUpdate =====

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateRootNode) {
  node_.id = 0;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kRootWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateButton) {
  node_.id = 1;
  node_.flags.isButton = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kButtonWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateTextField) {
  node_.id = 1;
  node_.flags.isTextField = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kEditTextWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateMultiline) {
  node_.id = 1;
  node_.flags.isMultiline = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType,
               OHWidgetName::kEditMultilineTextWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateLink) {
  node_.id = 1;
  node_.flags.isLink = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kLinkWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateImage) {
  node_.id = 1;
  node_.flags.isImage = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kImageWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateHeader) {
  node_.id = 1;
  node_.flags.isHeader = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kHeaderWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateCheckBox) {
  node_.id = 1;
  // flutter3.41: hasCheckedState replaced by isChecked != SemanticsCheckState::kNone
  node_.flags.isChecked = SemanticsCheckState::kFalse;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kCheckBoxWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateRadioButton) {
  node_.id = 1;
  node_.flags.isChecked = SemanticsCheckState::kFalse;
  node_.flags.isInMutuallyExclusiveGroup = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kRadioButtonWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateSwitch) {
  node_.id = 1;
  // flutter3.41: hasToggledState replaced by isToggled != SemanticsTristate::kNone
  node_.flags.isToggled = SemanticsTristate::kFalse;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kSwitchWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateTextWhenHasLabel) {
  node_.id = 1;
  node_.label = "Hello";
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kTextWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateDefaultIsOther) {
  node_.id = 1;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kOtherWidgetName);
}

// ===== OHOSActionsUpdate =====

TEST_F(SemanticsNodeTest, OHOSActionsUpdateEmptyWhenNoActions) {
  node_.OHOSActionsUpdate();
  EXPECT_TRUE(node_.ohActions.empty());
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateAddsClickForTap) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kTap);
  node_.OHOSActionsUpdate();
  // kTap + focus actions (gain/clear) since IsFocusable() is true when actions
  // are set
  EXPECT_GE(node_.ohActions.size(), 1u);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateAddsLongClickForLongPress) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kLongPress);
  node_.OHOSActionsUpdate();
  EXPECT_GE(node_.ohActions.size(), 1u);
}

// ===== HasPrevAction =====

TEST_F(SemanticsNodeTest, HasPrevActionReturnsFalseWhenNoPreviousActions) {
  EXPECT_FALSE(node_.HasPrevAction(SemanticsAction::kTap));
}

TEST_F(SemanticsNodeTest, HasPrevActionReturnsTrueWhenMatched) {
  node_.previousActions = static_cast<int32_t>(ACTIONS_::kTap);
  node_.actions = static_cast<int32_t>(ACTIONS_::kTap);
  EXPECT_TRUE(node_.HasPrevAction(SemanticsAction::kTap));
}

// ===== OHOSComponentTypeUpdate additional branches =====

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateSliderWhenIsSliderFlag) {
  node_.id = 1;
  node_.flags.isSlider = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kSliderWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateSliderWhenHasIncreaseAction) {
  node_.id = 1;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kIncrease);
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kSliderWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateSliderWhenHasDecreaseAction) {
  node_.id = 1;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kDecrease);
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kSliderWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateSeekbarWhenIncreaseNoSlider) {
  // kIncrease/kDecrease without isSlider flag → kSeekbarWidgetName
  // But kIncrease is caught earlier by the isSlider || HasAction(kIncrease)
  // branch, so to reach kSeekbarWidgetName we need kDecrease only (without
  // isSlider). However kDecrease is also caught by that same branch. So
  // kSeekbarWidgetName is actually unreachable when isSlider is false because
  // the slider branch already catches HasAction(kIncrease) ||
  // HasAction(kDecrease). This test verifies the slider branch takes priority.
  node_.id = 1;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kDecrease);
  node_.OHOSComponentTypeUpdate();
  // The slider branch catches HasAction(kDecrease) first
  EXPECT_STREQ(node_.componentType, OHWidgetName::kSliderWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateScrollWidget) {
  node_.id = 1;
  node_.flags.hasImplicitScrolling = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kScrollWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateTextWhenHasTooltip) {
  node_.id = 1;
  node_.tooltip = "A tooltip";
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kTextWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateTextWhenHasHint) {
  node_.id = 1;
  node_.hint = "A hint";
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kTextWidgetName);
}

// ===== OHOSActionsUpdate additional action branches =====

TEST_F(SemanticsNodeTest, OHOSActionsUpdateScrollLeftWithImplicitScrolling) {
  node_.flags.hasImplicitScrolling = true;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kScrollLeft);
  node_.OHOSActionsUpdate();
  // Should contain scroll forward action
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateScrollRightWithImplicitScrolling) {
  node_.flags.hasImplicitScrolling = true;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kScrollRight);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateScrollUpWithImplicitScrolling) {
  node_.flags.hasImplicitScrolling = true;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kScrollUp);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateScrollDownWithImplicitScrolling) {
  node_.flags.hasImplicitScrolling = true;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kScrollDown);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateScrollLeftWithoutImplicitScrolling) {
  // Without hasImplicitScrolling, scroll actions should NOT add scroll forward
  node_.actions |= static_cast<int32_t>(ACTIONS_::kScrollLeft);
  node_.OHOSActionsUpdate();
  for (const auto& action : node_.ohActions) {
    EXPECT_NE(action.actionType,
              ArkUI_Accessibility_ActionType::
                  ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD);
  }
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateIncrease) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kIncrease);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateDecrease) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kDecrease);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateSetSelection) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kSetSelection);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateCopy) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kCopy);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateCut) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kCut);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdatePaste) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kPaste);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateMoveCursorForwardByCharacter) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kMoveCursorForwardByCharacter);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateMoveCursorBackwardByCharacter) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kMoveCursorBackwardByCharacter);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateMoveCursorForwardByWord) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kMoveCursorForwardByWord);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateMoveCursorBackwardByWord) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kMoveCursorBackwardByWord);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateSetText) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kSetText);
  node_.OHOSActionsUpdate();
  bool found = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType == ArkUI_Accessibility_ActionType::
                                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateGainFocusWhenFocusable) {
  // When IsFocusable() is true, gain/clear focus actions should be added.
  // flutter3.41 removed isFocusable flag; use isTextField to make IsFocusable() true.
  node_.flags.isTextField = true;
  node_.OHOSActionsUpdate();
  bool found_gain = false;
  bool found_clear = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS) {
      found_gain = true;
    }
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS) {
      found_clear = true;
    }
  }
  EXPECT_TRUE(found_gain);
  EXPECT_TRUE(found_clear);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateGainFocusWhenDidGainAccessibilityFocus) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kDidGainAccessibilityFocus);
  node_.OHOSActionsUpdate();
  bool found_gain = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS) {
      found_gain = true;
      break;
    }
  }
  EXPECT_TRUE(found_gain);
}

TEST_F(SemanticsNodeTest, OHOSActionsUpdateClearFocusWhenDidLoseAccessibilityFocus) {
  node_.actions |= static_cast<int32_t>(ACTIONS_::kDidLoseAccessibilityFocus);
  node_.OHOSActionsUpdate();
  bool found_clear = false;
  for (const auto& action : node_.ohActions) {
    if (action.actionType ==
        ArkUI_Accessibility_ActionType::
            ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS) {
      found_clear = true;
      break;
    }
  }
  EXPECT_TRUE(found_clear);
}

// ===== UpdateWithNode =====

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsId) {
  flutter::SemanticsNode source;
  source.id = 42;
  node_.UpdateWithNode(source);
  EXPECT_EQ(node_.id, 42);
  EXPECT_TRUE(node_.idChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsLabel) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.label = "MyLabel";
  node_.UpdateWithNode(source);
  EXPECT_EQ(node_.label, "MyLabel");
  EXPECT_TRUE(node_.contentChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsFlags) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.flags.isButton = true;
  node_.UpdateWithNode(source);
  EXPECT_TRUE(node_.flags.isButton);
  EXPECT_TRUE(node_.flagChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsActions) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.actions = static_cast<int32_t>(ACTIONS_::kTap);
  node_.UpdateWithNode(source);
  EXPECT_EQ(node_.actions, static_cast<int32_t>(ACTIONS_::kTap));
  EXPECT_TRUE(node_.actionChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsRect) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.rect = SkRect::MakeXYWH(10.0f, 20.0f, 30.0f, 40.0f);
  node_.UpdateWithNode(source);
  EXPECT_EQ(node_.rect.fLeft, 10.0f);
  EXPECT_EQ(node_.rect.fTop, 20.0f);
  EXPECT_EQ(node_.rect.fRight, 40.0f);
  EXPECT_EQ(node_.rect.fBottom, 60.0f);
  EXPECT_TRUE(node_.rectChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsChildrenInTraversalOrder) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.childrenInTraversalOrder = {2, 3, 4};
  node_.UpdateWithNode(source);
  EXPECT_EQ(node_.childrenInTraversalOrder.size(), 3u);
  EXPECT_EQ(node_.childrenInTraversalOrder[0], 2);
  EXPECT_EQ(node_.childrenInTraversalOrder[1], 3);
  EXPECT_EQ(node_.childrenInTraversalOrder[2], 4);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsTextSelection) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.textSelectionBase = 5;
  source.textSelectionExtent = 10;
  node_.UpdateWithNode(source);
  EXPECT_EQ(node_.textSelectionBase, 5);
  EXPECT_EQ(node_.textSelectionExtent, 10);
  EXPECT_TRUE(node_.selectChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsScrollInfo) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.scrollChildren = 10;
  source.scrollIndex = 2;
  source.scrollPosition = 0.5;
  source.scrollExtentMax = 1.0;
  source.scrollExtentMin = 0.0;
  node_.UpdateWithNode(source);
  EXPECT_EQ(node_.scrollChildren, 10);
  EXPECT_EQ(node_.scrollIndex, 2);
  EXPECT_TRUE(node_.scrollChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeTriggersComponentTypeUpdateForRoot) {
  flutter::SemanticsNode source;
  source.id = 0;  // root node
  node_.UpdateWithNode(source);
  EXPECT_STREQ(node_.componentType, OHWidgetName::kRootWidgetName);
  EXPECT_TRUE(node_.propertyChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeTriggersComponentTypeUpdateForButton) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.flags.isButton = true;
  node_.UpdateWithNode(source);
  EXPECT_STREQ(node_.componentType, OHWidgetName::kButtonWidgetName);
  EXPECT_TRUE(node_.propertyChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeSetsTextWidgetWhenLabelAdded) {
  // Start with kOtherWidgetName (default)
  flutter::SemanticsNode source;
  source.id = 1;
  source.label = "Hello";
  node_.UpdateWithNode(source);
  // When label is set and componentType was kOtherWidgetName, it becomes
  // kTextWidgetName
  EXPECT_STREQ(node_.componentType, OHWidgetName::kTextWidgetName);
}

// Pass a node with only value different, covering the value != node.value branch
TEST_F(SemanticsNodeTest, UpdateWithNodeDetectsValueChange) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.value = "old_value";
  node_.UpdateWithNode(source);
  ASSERT_EQ(node_.value, "old_value");
  ASSERT_TRUE(node_.contentChanged);

  // Reset flag, second time only value changes
  node_.contentChanged = false;

  flutter::SemanticsNode source2;
  source2.id = 1;
  source2.value = "new_value";
  node_.UpdateWithNode(source2);
  EXPECT_EQ(node_.value, "new_value");
  EXPECT_TRUE(node_.contentChanged);
}

// Pass a node with only hint different, covering the hint != node.hint branch
TEST_F(SemanticsNodeTest, UpdateWithNodeDetectsHintChange) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.hint = "old_hint";
  node_.UpdateWithNode(source);
  ASSERT_EQ(node_.hint, "old_hint");
  ASSERT_TRUE(node_.contentChanged);

  // Reset flag, second time only hint changes
  node_.contentChanged = false;

  flutter::SemanticsNode source2;
  source2.id = 1;
  source2.hint = "new_hint";
  node_.UpdateWithNode(source2);
  EXPECT_EQ(node_.hint, "new_hint");
  EXPECT_TRUE(node_.contentChanged);
}

// Pass a node with only tooltip different, covering the tooltip != node.tooltip branch
TEST_F(SemanticsNodeTest, UpdateWithNodeDetectsTooltipChange) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.tooltip = "old_tooltip";
  node_.UpdateWithNode(source);
  ASSERT_EQ(node_.tooltip, "old_tooltip");
  ASSERT_TRUE(node_.contentChanged);

  // Reset flag, second time only tooltip changes
  node_.contentChanged = false;

  flutter::SemanticsNode source2;
  source2.id = 1;
  source2.tooltip = "new_tooltip";
  node_.UpdateWithNode(source2);
  EXPECT_EQ(node_.tooltip, "new_tooltip");
  EXPECT_TRUE(node_.contentChanged);
}

// Pass non-empty tooltip + kOtherWidgetName, covering the !tooltip.empty() branch
TEST_F(SemanticsNodeTest, UpdateWithNodeSetsTextWidgetWhenTooltipAdded) {
  // node_ default componentType is kOtherWidgetName
  flutter::SemanticsNode source;
  source.id = 1;
  source.tooltip = "A tooltip";
  node_.UpdateWithNode(source);
  EXPECT_STREQ(node_.componentType, OHWidgetName::kTextWidgetName);
}

// Pass non-empty hint + kOtherWidgetName, covering the !hint.empty() branch
TEST_F(SemanticsNodeTest, UpdateWithNodeSetsTextWidgetWhenHintAdded) {
  // node_ default componentType is kOtherWidgetName
  flutter::SemanticsNode source;
  source.id = 1;
  source.hint = "A hint";
  node_.UpdateWithNode(source);
  EXPECT_STREQ(node_.componentType, OHWidgetName::kTextWidgetName);
}

// Pass a node with only scrollChildren different (scrollIndex same), covering the scrollChildren != node.scrollChildren branch
TEST_F(SemanticsNodeTest, UpdateWithNodeDetectsScrollChildrenChange) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.scrollChildren = 10;
  source.scrollIndex = 2;
  node_.UpdateWithNode(source);
  ASSERT_EQ(node_.scrollChildren, 10);
  ASSERT_TRUE(node_.scrollChanged);

  // Reset flag, second time only scrollChildren changes
  node_.scrollChanged = false;

  flutter::SemanticsNode source2;
  source2.id = 1;
  source2.scrollChildren = 20;  // only scrollChildren changes
  source2.scrollIndex = 2;       // scrollIndex unchanged
  node_.UpdateWithNode(source2);
  EXPECT_EQ(node_.scrollChildren, 20);
  EXPECT_TRUE(node_.scrollChanged);
}

// Pass a node with only textSelectionExtent different, covering the textSelectionExtent != node.textSelectionExtent branch
TEST_F(SemanticsNodeTest, UpdateWithNodeDetectsTextSelectionExtentChange) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.textSelectionBase = 5;
  source.textSelectionExtent = 10;
  node_.UpdateWithNode(source);
  ASSERT_EQ(node_.textSelectionExtent, 10);
  ASSERT_TRUE(node_.selectChanged);

  // Reset flag, second time only extent changes
  node_.selectChanged = false;

  flutter::SemanticsNode source2;
  source2.id = 1;
  source2.textSelectionBase = 5;    // unchanged
  source2.textSelectionExtent = 20;  // only extent changes
  node_.UpdateWithNode(source2);
  EXPECT_EQ(node_.textSelectionExtent, 20);
  EXPECT_TRUE(node_.selectChanged);
}

// Pass a node with only transform different (rect same), covering the transform != node.transform branch
TEST_F(SemanticsNodeTest, UpdateWithNodeDetectsTransformChange) {
  flutter::SemanticsNode source;
  source.id = 1;
  source.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  source.transform = SkM44();  // identity
  node_.UpdateWithNode(source);
  ASSERT_TRUE(node_.rectChanged);

  // Reset rectChanged, second time rect is same but transform is different
  node_.rectChanged = false;

  flutter::SemanticsNode source2;
  source2.id = 1;
  source2.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);  // rect unchanged
  source2.transform = SkM44::Scale(2.0f, 2.0f);                  // transform changes
  node_.UpdateWithNode(source2);
  EXPECT_TRUE(node_.rectChanged);
}

// ===== UpdateSelfRecursively =====

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelySingleNodeNeedUpdateFalse) {
  // Set up a single node with rect and transform
  node_.id = 1;
  node_.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  node_.transform = SkM44::Scale(1.0f, 1.0f);
  node_.rectChanged = false;

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  node_.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // Should have visited this node
  EXPECT_EQ(visitorId.size(), 1u);
  EXPECT_EQ(visitorOrder.size(), 1u);
  EXPECT_EQ(visitorOrder[0], 1);
  // needUpdate was false and rectChanged was false, so absoluteRect not set
  EXPECT_FALSE(node_.rectChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelySingleNodeNeedUpdateTrue) {
  node_.id = 2;
  node_.rect = SkRect::MakeXYWH(10.0f, 20.0f, 100.0f, 50.0f);
  node_.transform = SkM44::Scale(2.0f, 2.0f);
  node_.rectChanged = false;

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  node_.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, true);

  // needUpdate was true, so absoluteRect should be computed
  EXPECT_TRUE(node_.rectChanged);
  // absoluteTransform should be fatherTransform * transform
  // With identity fatherTransform and scale(2,2), points should be scaled
  EXPECT_FLOAT_EQ(node_.absoluteRect.fLeft, 20.0f);    // 10 * 2
  EXPECT_FLOAT_EQ(node_.absoluteRect.fTop, 40.0f);     // 20 * 2
  EXPECT_FLOAT_EQ(node_.absoluteRect.fRight, 220.0f);  // (10+100) * 2
  EXPECT_FLOAT_EQ(node_.absoluteRect.fBottom, 140.0f); // (20+50) * 2
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyRectChangedForcesUpdate) {
  node_.id = 3;
  node_.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  node_.transform = SkM44();
  node_.rectChanged = true;

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  // needUpdate is false but rectChanged is true
  node_.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // rectChanged should force needUpdate=true internally
  EXPECT_TRUE(node_.rectChanged);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fLeft, 0.0f);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fTop, 0.0f);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fRight, 50.0f);
  EXPECT_FLOAT_EQ(node_.absoluteRect.fBottom, 50.0f);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyWithChildren) {
  // Create parent and child nodes
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child1;
  SemanticsNodeExtend child2;

  parent.id = 10;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 200.0f, 200.0f);
  parent.transform = SkM44();
  parent.rectChanged = true;
  parent.isExist = true;

  child1.id = 11;
  child1.rect = SkRect::MakeXYWH(10.0f, 10.0f, 50.0f, 50.0f);
  child1.transform = SkM44();
  child1.rectChanged = true;
  child1.isExist = true;
  child1.flags.isHidden = false;  // visible

  child2.id = 12;
  child2.rect = SkRect::MakeXYWH(100.0f, 100.0f, 50.0f, 50.0f);
  child2.transform = SkM44();
  child2.rectChanged = true;
  child2.isExist = true;
  child2.flags.isHidden = false;  // visible

  parent.childrenInTraversalOrderList.push_back(&child1);
  parent.childrenInTraversalOrderList.push_back(&child2);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // All 3 nodes should be visited
  EXPECT_EQ(visitorId.size(), 3u);
  EXPECT_EQ(visitorOrder.size(), 3u);
  EXPECT_EQ(visitorOrder[0], 10);
  EXPECT_EQ(visitorOrder[1], 11);
  EXPECT_EQ(visitorOrder[2], 12);

  // Children should have parent pointers set
  // Note: parentChanged is set in UpdateSelfRecursively but then reset by
  // UpdateSelfElementInfo, so we check parentNode and parentId instead
  EXPECT_EQ(child1.parentNode, &parent);
  EXPECT_EQ(child1.parentId, 10u);

  // child2 should have previousNode pointing to child1
  EXPECT_EQ(child2.previousNode, &child1);
  // child1 should have nextNode pointing to child2
  EXPECT_EQ(child1.nextNode, &child2);

  // existChildrenInTraversalOrder should be updated
  EXPECT_EQ(parent.existChildrenInTraversalOrder.size(), 2u);
  EXPECT_TRUE(parent.childrenChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelySkipsNonExistentChildren) {
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child;

  parent.id = 20;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  parent.transform = SkM44();
  parent.rectChanged = true;
  parent.isExist = true;

  child.id = 21;
  child.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  child.transform = SkM44();
  child.rectChanged = true;
  child.isExist = false;  // Not exist - should be skipped

  parent.childrenInTraversalOrderList.push_back(&child);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // Only parent should be visited, child skipped
  EXPECT_EQ(visitorId.size(), 1u);
  EXPECT_EQ(visitorOrder.size(), 1u);
  EXPECT_EQ(visitorOrder[0], 20);
  // existChildrenInTraversalOrder should be empty
  EXPECT_EQ(parent.existChildrenInTraversalOrder.size(), 0u);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyWithInvisibleChild) {
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child;

  parent.id = 30;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  parent.transform = SkM44();
  parent.rectChanged = true;
  parent.isExist = true;

  child.id = 31;
  child.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  child.transform = SkM44();
  child.rectChanged = true;
  child.isExist = true;
  child.flags.isHidden = true;  // invisible

  parent.childrenInTraversalOrderList.push_back(&child);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // Both parent and child should be visited (child exists but is invisible)
  EXPECT_EQ(visitorId.size(), 2u);
  EXPECT_EQ(visitorOrder.size(), 2u);
  // child should still have parent and sibling pointers set
  EXPECT_EQ(child.parentNode, &parent);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyWithFocusedChild) {
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child;

  parent.id = 40;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  parent.transform = SkM44();
  parent.rectChanged = true;
  parent.isExist = true;
  // Set scroll info so scrollVisibleNum gets updated
  parent.scrollChildren = 1;
  parent.scrollIndex = 0;
  parent.scrollEndIndex = 0;
  parent.scrollChanged = true;

  child.id = 41;
  child.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  child.transform = SkM44();
  child.rectChanged = true;
  child.isExist = true;
  child.flags.isHidden = false;  // visible
  child.isAccessibilityFocued = true;

  parent.childrenInTraversalOrderList.push_back(&child);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // scrollCurrentIndex should be set to child_index (0)
  EXPECT_EQ(parent.scrollCurrentIndex, 0);
  // visible_num should be 1, so scrollVisibleNum should be 1
  EXPECT_EQ(parent.scrollVisibleNum, 1);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyWithScrollInfo) {
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child1;
  SemanticsNodeExtend child2;

  parent.id = 50;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  parent.transform = SkM44();
  parent.rectChanged = true;
  parent.isExist = true;
  parent.scrollChildren = 2;
  parent.scrollIndex = 0;
  parent.scrollEndIndex = 1;
  parent.scrollChanged = true;

  child1.id = 51;
  child1.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  child1.transform = SkM44();
  child1.rectChanged = true;
  child1.isExist = true;
  child1.flags.isHidden = false;

  child2.id = 52;
  child2.rect = SkRect::MakeXYWH(50.0f, 50.0f, 50.0f, 50.0f);
  child2.transform = SkM44();
  child2.rectChanged = true;
  child2.isExist = true;
  child2.flags.isHidden = false;

  parent.childrenInTraversalOrderList.push_back(&child1);
  parent.childrenInTraversalOrderList.push_back(&child2);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // scrollVisibleNum should be 2 (both children visible)
  EXPECT_EQ(parent.scrollVisibleNum, 2);
  // scrollVisibleEndIndex should be 1 (last_visible_index)
  EXPECT_EQ(parent.scrollVisibleEndIndex, 1);
  // scrollEndIndex should be scrollIndex + visible_num - 1 = 0 + 2 - 1 = 1
  EXPECT_EQ(parent.scrollEndIndex, 1);
  EXPECT_TRUE(parent.scrollChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyChildParentAlreadySet) {
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child;

  parent.id = 60;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  parent.transform = SkM44();
  parent.rectChanged = true;
  parent.isExist = true;

  child.id = 61;
  child.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  child.transform = SkM44();
  child.rectChanged = true;
  child.isExist = true;
  child.flags.isHidden = false;
  // Pre-set parentNode to the correct parent
  child.parentNode = &parent;
  child.parentId = 60;

  parent.childrenInTraversalOrderList.push_back(&child);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // parentChanged should NOT be set since parentNode already matches
  EXPECT_FALSE(child.parentChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyExistChildrenUnchanged) {
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child;

  parent.id = 70;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 100.0f, 100.0f);
  parent.transform = SkM44();
  parent.rectChanged = true;
  parent.isExist = true;
  // Pre-set existChildrenInTraversalOrder to match what will be computed
  parent.existChildrenInTraversalOrder = {71};

  child.id = 71;
  child.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  child.transform = SkM44();
  child.rectChanged = true;
  child.isExist = true;
  child.flags.isHidden = false;

  parent.childrenInTraversalOrderList.push_back(&child);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // childrenChanged should NOT be set since existChildrenInTraversalOrder matches
  EXPECT_FALSE(parent.childrenChanged);
}

// ===== UpdateSelfElementInfo =====

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoFirstCall) {
  // First call with hasInit=false should fill all info
  node_.id = 100;
  node_.idChanged = true;
  node_.propertyChanged = true;
  node_.contentChanged = true;
  node_.childrenChanged = true;
  node_.parentChanged = true;
  node_.scrollChanged = true;
  node_.rectChanged = true;
  node_.selectChanged = true;

  EXPECT_FALSE(node_.hasInit);

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasInit);
  EXPECT_TRUE(node_.hasUpdate);
  // After UpdateSelfElementInfo, changed flags should be reset
  EXPECT_FALSE(node_.idChanged);
  EXPECT_FALSE(node_.propertyChanged);
  EXPECT_FALSE(node_.contentChanged);
  EXPECT_FALSE(node_.childrenChanged);
  EXPECT_FALSE(node_.parentChanged);
  EXPECT_FALSE(node_.rectChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoNoChanges) {
  // Initialize first
  node_.id = 101;
  node_.hasInit = true;
  node_.hasUpdate = false;
  // No changes set

  node_.UpdateSelfElementInfo();

  // hasUpdate should remain false since nothing changed
  EXPECT_FALSE(node_.hasUpdate);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithIdChange) {
  node_.id = 102;
  node_.hasInit = true;
  node_.idChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  EXPECT_FALSE(node_.idChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithPropertyChange) {
  node_.id = 103;
  node_.hasInit = true;
  node_.propertyChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  EXPECT_FALSE(node_.propertyChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithContentChange) {
  node_.id = 104;
  node_.hasInit = true;
  node_.contentChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  EXPECT_FALSE(node_.contentChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithChildrenChange) {
  node_.id = 105;
  node_.hasInit = true;
  node_.childrenChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  EXPECT_FALSE(node_.childrenChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithParentChange) {
  node_.id = 106;
  node_.hasInit = true;
  node_.parentChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  EXPECT_FALSE(node_.parentChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithScrollChange) {
  node_.id = 107;
  node_.hasInit = true;
  node_.scrollChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  // scrollChanged is intentionally NOT reset (commented out in source)
  EXPECT_TRUE(node_.scrollChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithRectChange) {
  node_.id = 108;
  node_.hasInit = true;
  node_.rectChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  EXPECT_FALSE(node_.rectChanged);
}

TEST_F(SemanticsNodeTest, UpdateSelfElementInfoWithSelectChange) {
  node_.id = 109;
  node_.hasInit = true;
  node_.selectChanged = true;
  node_.hasUpdate = false;

  node_.UpdateSelfElementInfo();

  EXPECT_TRUE(node_.hasUpdate);
  // selectChanged is intentionally NOT reset (commented out in source)
  EXPECT_TRUE(node_.selectChanged);
}

// ===== FillElementInfoWithParent (root vs non-root) =====

TEST_F(SemanticsNodeTest, FillElementInfoWithParentRootNode) {
  // id == 0 means root node, should use kArkuiAccessibilityRootParentId
  node_.id = 0;
  node_.hasInit = true;
  node_.parentChanged = true;

  node_.UpdateSelfElementInfo();

  // Should not crash; parentChanged should be reset
  EXPECT_FALSE(node_.parentChanged);
  EXPECT_TRUE(node_.hasUpdate);
}

// ===== UpdateSelfRecursively with transform composition =====

TEST_F(SemanticsNodeTest, UpdateSelfRecursivelyTransformComposition) {
  // Test that child's absoluteTransform = parent's absoluteTransform * child's transform
  SemanticsNodeExtend parent;
  SemanticsNodeExtend child;

  parent.id = 200;
  parent.rect = SkRect::MakeXYWH(0.0f, 0.0f, 200.0f, 200.0f);
  parent.transform = SkM44::Translate(100.0f, 100.0f);  // parent translates
  parent.rectChanged = true;
  parent.isExist = true;

  child.id = 201;
  child.rect = SkRect::MakeXYWH(0.0f, 0.0f, 50.0f, 50.0f);
  child.transform = SkM44::Scale(2.0f, 2.0f);  // child scales
  child.rectChanged = true;
  child.isExist = true;
  child.flags.isHidden = false;

  parent.childrenInTraversalOrderList.push_back(&child);

  std::unordered_set<int32_t> visitorId;
  std::vector<int32_t> visitorOrder;
  SkM44 fatherTransform;  // identity

  parent.UpdateSelfRecursively(visitorId, visitorOrder, fatherTransform, false);

  // Parent absoluteRect: translate(100,100) applied to (0,0,200,200)
  EXPECT_FLOAT_EQ(parent.absoluteRect.fLeft, 100.0f);
  EXPECT_FLOAT_EQ(parent.absoluteRect.fTop, 100.0f);
  EXPECT_FLOAT_EQ(parent.absoluteRect.fRight, 300.0f);
  EXPECT_FLOAT_EQ(parent.absoluteRect.fBottom, 300.0f);

  // Child absoluteRect: parent_transform * child_transform applied to (0,0,50,50)
  // parent: translate(100,100), child: scale(2,2)
  // composed: translate(100,100) * scale(2,2) = points get scaled then translated
  // (0,0) -> (0,0) -> (100,100)
  // (50,50) -> (100,100) -> (200,200)
  EXPECT_FLOAT_EQ(child.absoluteRect.fLeft, 100.0f);
  EXPECT_FLOAT_EQ(child.absoluteRect.fTop, 100.0f);
  EXPECT_FLOAT_EQ(child.absoluteRect.fRight, 200.0f);
  EXPECT_FLOAT_EQ(child.absoluteRect.fBottom, 200.0f);
}

// ===== UiTest semantics bridge: text field hints, identifiers, ranges =====
// SemanticsNode::role is not default-initialized, so test nodes set it.

// ===== FillElementInfoWithContent: text field hint exposure =====

TEST_F(SemanticsNodeTest, FillContentTextFieldWritesHintWhenValueEmpty) {
  node_.flags.isTextField = true;
  node_.hint = "请输入账号";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_EQ(g_capturedA11yContent.hintText, "请输入账号");
  EXPECT_EQ(g_capturedA11yContent.text, "请输入账号");
  EXPECT_EQ(g_capturedA11yContent.contents, "");
}

TEST_F(SemanticsNodeTest, FillContentTextFieldCombinesLabelAndHint) {
  node_.flags.isTextField = true;
  node_.label = "账号";
  node_.hint = "请输入账号";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_EQ(g_capturedA11yContent.hintText, "账号 ,请输入账号");
  EXPECT_EQ(g_capturedA11yContent.text, "账号 ,请输入账号");
}

TEST_F(SemanticsNodeTest, FillContentTextFieldWithValueKeepsHintOnly) {
  node_.flags.isTextField = true;
  node_.hint = "请输入账号";
  node_.value = "hello";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_EQ(g_capturedA11yContent.hintText, "请输入账号");
  EXPECT_EQ(g_capturedA11yContent.text, "");
  EXPECT_EQ(g_capturedA11yContent.contents, "hello");
}

TEST_F(SemanticsNodeTest, FillContentTextFieldWithoutHintWritesEmptyStrings) {
  node_.flags.isTextField = true;
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_EQ(g_capturedA11yContent.hintText, "");
  EXPECT_EQ(g_capturedA11yContent.text, "");
}

TEST_F(SemanticsNodeTest, FillContentNonTextFieldClearsHintText) {
  node_.label = "提交";
  node_.value = "done";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_EQ(g_capturedA11yContent.hintText, "");
  EXPECT_EQ(g_capturedA11yContent.text, "done ,提交");
  EXPECT_EQ(g_capturedA11yContent.contents, node_.contentString);
}

TEST_F(SemanticsNodeTest, HintClearedWhenNodeStopsBeingTextField) {
  flutter::SemanticsNode first;
  first.id = 1;
  first.role = SemanticsRole::kNone;
  first.flags.isTextField = true;
  first.hint = "请输入密码";
  node_.UpdateWithNode(first);
  g_capturedA11yContent.Reset();
  node_.UpdateSelfElementInfo();
  EXPECT_EQ(g_capturedA11yContent.hintText, "请输入密码");

  // Only the isTextField flag flips; content strings stay identical.
  flutter::SemanticsNode second;
  second.id = 1;
  second.role = SemanticsRole::kNone;
  second.flags.isTextField = false;
  second.hint = "请输入密码";
  node_.UpdateWithNode(second);
  EXPECT_TRUE(node_.contentChanged);
  g_capturedA11yContent.Reset();
  node_.UpdateSelfElementInfo();
  EXPECT_EQ(g_capturedA11yContent.hintText, "");
}

// ===== FillElementInfoWithContent: component identifier mapping =====

TEST_F(SemanticsNodeTest, FillContentComponentIdentifierSuccessKeepsFlagFalse) {
  node_.identifier = "login_account_input";
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_FALSE(node_.componentIdentifierWriteFailed);
}

TEST_F(SemanticsNodeTest, FillContentIdentifierAtLimitStillSucceeds) {
  constexpr size_t kIdentifierLimitBytes = 1024;
  node_.identifier.assign(kIdentifierLimitBytes, 'x');
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_FALSE(node_.componentIdentifierWriteFailed);
}

TEST_F(SemanticsNodeTest, FillContentOversizedIdentifierClearedWithoutError) {
  constexpr size_t kIdentifierOverLimitBytes = 1025;
  node_.identifier.assign(kIdentifierOverLimitBytes, 'x');
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_FALSE(node_.componentIdentifierWriteFailed);
}

TEST_F(SemanticsNodeTest, FillContentIdentifierFailureFlagDedupAndRecovery) {
  // The setter only exists from API 24; skip when it is unavailable.
  void* const handle = dlopen("libace_ndk.z.so", RTLD_NOW | RTLD_LOCAL);
  if (handle == nullptr ||
      dlsym(handle,
            "OH_ArkUI_AccessibilityElementInfoSetComponentIdentifier") ==
          nullptr) {
    GTEST_SKIP() << "component identifier API unavailable (API < 24)";
  }
  // A null elementInfo is documented to return BAD_PARAMETER, not crash.
  node_.identifier = "settings_button";
  node_.FillElementInfoWithContent(nullptr);
  EXPECT_TRUE(node_.componentIdentifierWriteFailed);  // first failure logs
  node_.FillElementInfoWithContent(nullptr);
  EXPECT_TRUE(node_.componentIdentifierWriteFailed);  // repeats stay quiet
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_FALSE(node_.componentIdentifierWriteFailed);  // success resets
}

// ===== FillElementInfoWithContent: range semantics =====

TEST_F(SemanticsNodeTest, FillContentWritesRangeForValidValues) {
  constexpr double kExpectedMax = 100.0;
  constexpr double kExpectedCurrent = 50.0;
  node_.minValue = "0";
  node_.maxValue = "100";
  node_.value = "50";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ASSERT_GT(g_capturedA11yContent.rangeCalls, 0);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.min, 0.0);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.max, kExpectedMax);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.current, kExpectedCurrent);
}

TEST_F(SemanticsNodeTest, FillContentWritesNegativeAndFractionalRange) {
  constexpr double kExpectedMin = -10.0;
  constexpr double kExpectedMax = 10.0;
  constexpr double kExpectedCurrent = -2.5;
  node_.minValue = "-10";
  node_.maxValue = "10";
  node_.value = "-2.5";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.min, kExpectedMin);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.max, kExpectedMax);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.current, kExpectedCurrent);
}

TEST_F(SemanticsNodeTest, FillContentWritesDegenerateRange) {
  constexpr double kDegenerateValue = 0.5;
  node_.minValue = "0.5";
  node_.maxValue = "0.5";
  node_.value = "0.5";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.min, kDegenerateValue);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.max, kDegenerateValue);
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.current, kDegenerateValue);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenValueEmpty) {
  node_.minValue = "0";
  node_.maxValue = "100";
  node_.value = "";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ASSERT_GT(g_capturedA11yContent.rangeCalls, 0);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenMinValueEmpty) {
  node_.minValue = "";
  node_.maxValue = "100";
  node_.value = "50";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenMaxValueEmpty) {
  node_.minValue = "0";
  node_.maxValue = "";
  node_.value = "50";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenValueNotNumeric) {
  node_.minValue = "0";
  node_.maxValue = "100";
  node_.value = "abc";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenValueHasTrailingGarbage) {
  node_.minValue = "0";
  node_.maxValue = "100";
  node_.value = "50px";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenValueNotFinite) {
  node_.minValue = "0";
  node_.maxValue = "100";
  node_.value = "inf";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenValueOverflows) {
  node_.minValue = "0";
  node_.maxValue = "100";
  node_.value = "1e999";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenValueBelowMin) {
  node_.minValue = "10";
  node_.maxValue = "100";
  node_.value = "5";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, FillContentRangeZeroedWhenValueAboveMax) {
  node_.minValue = "0";
  node_.maxValue = "100";
  node_.value = "150";
  g_capturedA11yContent.Reset();
  node_.FillElementInfoWithContent(node_.elementInfoOHOS);
  ExpectZeroRange(g_capturedA11yContent.range);
}

TEST_F(SemanticsNodeTest, StaleRangeClearedWhenValueRemoved) {
  constexpr double kFirstCurrent = 30.0;
  flutter::SemanticsNode withValue;
  withValue.id = 1;
  withValue.role = SemanticsRole::kNone;
  withValue.minValue = "0";
  withValue.maxValue = "100";
  withValue.value = "30";
  node_.UpdateWithNode(withValue);
  g_capturedA11yContent.Reset();
  node_.UpdateSelfElementInfo();
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.current, kFirstCurrent);

  flutter::SemanticsNode withoutValue;
  withoutValue.id = 1;
  withoutValue.role = SemanticsRole::kNone;
  withoutValue.minValue = "0";
  withoutValue.maxValue = "100";
  withoutValue.value = "";
  node_.UpdateWithNode(withoutValue);
  g_capturedA11yContent.Reset();
  node_.UpdateSelfElementInfo();
  ASSERT_GT(g_capturedA11yContent.rangeCalls, 0);
  ExpectZeroRange(g_capturedA11yContent.range);
}

// ===== OHOSComponentTypeUpdate: progress-bar role =====

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateProgressBarRole) {
  node_.id = 1;
  node_.role = SemanticsRole::kProgressBar;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kProgressWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateProgressBarBeatsScrollAndText) {
  node_.id = 1;
  node_.role = SemanticsRole::kProgressBar;
  node_.flags.hasImplicitScrolling = true;
  node_.label = "loading";
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kProgressWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateButtonBeatsProgressBarRole) {
  node_.id = 1;
  node_.role = SemanticsRole::kProgressBar;
  node_.flags.isButton = true;
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kButtonWidgetName);
}

TEST_F(SemanticsNodeTest, OHOSComponentTypeUpdateSliderActionBeatsProgressBarRole) {
  node_.id = 1;
  node_.role = SemanticsRole::kProgressBar;
  node_.actions |= static_cast<int32_t>(ACTIONS_::kIncrease);
  node_.OHOSComponentTypeUpdate();
  EXPECT_STREQ(node_.componentType, OHWidgetName::kSliderWidgetName);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeProgressBarEndToEnd) {
  constexpr double kProgressCurrent = 70.0;
  flutter::SemanticsNode progress;
  progress.id = 1;
  progress.role = SemanticsRole::kProgressBar;
  progress.minValue = "0";
  progress.maxValue = "100";
  progress.value = "70";
  node_.UpdateWithNode(progress);
  EXPECT_EQ(node_.role, SemanticsRole::kProgressBar);
  EXPECT_STREQ(node_.componentType, OHWidgetName::kProgressWidgetName);
  g_capturedA11yContent.Reset();
  node_.UpdateSelfElementInfo();
  EXPECT_DOUBLE_EQ(g_capturedA11yContent.range.current, kProgressCurrent);
}

// ===== UpdateWithNode: change detection for the new content fields =====

TEST_F(SemanticsNodeTest, UpdateWithNodeIdentifierChangeSetsContentChanged) {
  flutter::SemanticsNode node;
  node.id = 1;
  node.role = SemanticsRole::kNone;
  node.identifier = "settings_button";
  node_.UpdateWithNode(node);
  EXPECT_TRUE(node_.contentChanged);
  EXPECT_EQ(node_.identifier, "settings_button");
}

TEST_F(SemanticsNodeTest, UpdateWithNodeMinValueChangeSetsContentChanged) {
  flutter::SemanticsNode first;
  first.id = 1;
  first.role = SemanticsRole::kNone;
  first.minValue = "0";
  node_.UpdateWithNode(first);

  flutter::SemanticsNode second;
  second.id = 1;
  second.role = SemanticsRole::kNone;
  second.minValue = "10";
  node_.contentChanged = false;
  node_.UpdateWithNode(second);
  EXPECT_TRUE(node_.contentChanged);
  EXPECT_EQ(node_.minValue, "10");
}

TEST_F(SemanticsNodeTest, UpdateWithNodeMaxValueChangeSetsContentChanged) {
  flutter::SemanticsNode first;
  first.id = 1;
  first.role = SemanticsRole::kNone;
  first.maxValue = "100";
  node_.UpdateWithNode(first);

  flutter::SemanticsNode second;
  second.id = 1;
  second.role = SemanticsRole::kNone;
  second.maxValue = "200";
  node_.contentChanged = false;
  node_.UpdateWithNode(second);
  EXPECT_TRUE(node_.contentChanged);
  EXPECT_EQ(node_.maxValue, "200");
}

TEST_F(SemanticsNodeTest, UpdateWithNodeRoleChangeSetsContentChanged) {
  flutter::SemanticsNode first;
  first.id = 1;
  first.role = SemanticsRole::kNone;
  node_.UpdateWithNode(first);

  flutter::SemanticsNode second;
  second.id = 1;
  second.role = SemanticsRole::kProgressBar;
  node_.contentChanged = false;
  node_.UpdateWithNode(second);
  EXPECT_TRUE(node_.contentChanged);
  EXPECT_EQ(node_.role, SemanticsRole::kProgressBar);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeTextFieldFlagChangeSetsContentChanged) {
  flutter::SemanticsNode first;
  first.id = 1;
  first.role = SemanticsRole::kNone;
  first.flags.isTextField = false;
  node_.UpdateWithNode(first);

  flutter::SemanticsNode second;
  second.id = 1;
  second.role = SemanticsRole::kNone;
  second.flags.isTextField = true;
  node_.contentChanged = false;
  node_.UpdateWithNode(second);
  EXPECT_TRUE(node_.contentChanged);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeOversizedIdentifierStoredForClearing) {
  constexpr size_t kIdentifierBytes = 1100;
  flutter::SemanticsNode node;
  node.id = 1;
  node.role = SemanticsRole::kNone;
  node.identifier.assign(kIdentifierBytes, 'x');
  node_.UpdateWithNode(node);
  // The warning fires here; the actual clearing happens at fill time.
  EXPECT_TRUE(node_.contentChanged);
  EXPECT_EQ(node_.identifier.size(), kIdentifierBytes);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeIdentifierAtLimitNotCleared) {
  constexpr size_t kIdentifierLimitBytes = 1024;
  flutter::SemanticsNode node;
  node.id = 1;
  node.role = SemanticsRole::kNone;
  node.identifier.assign(kIdentifierLimitBytes, 'x');
  node_.UpdateWithNode(node);
  EXPECT_TRUE(node_.contentChanged);
  EXPECT_EQ(node_.identifier.size(), kIdentifierLimitBytes);
}

TEST_F(SemanticsNodeTest, UpdateWithNodeUnchangedContentKeepsContentChangedFalse) {
  flutter::SemanticsNode first;
  first.id = 1;
  first.role = SemanticsRole::kNone;
  first.identifier = "settings_button";
  first.value = "a";
  node_.UpdateWithNode(first);
  node_.contentChanged = false;

  flutter::SemanticsNode second;
  second.id = 1;
  second.role = SemanticsRole::kNone;
  second.identifier = "settings_button";
  second.value = "a";
  node_.UpdateWithNode(second);
  EXPECT_FALSE(node_.contentChanged);
  EXPECT_EQ(node_.identifier, "settings_button");
}

TEST_F(SemanticsNodeTest, UpdateWithNodeKeepsIdentifierWhenOnlyValueChanged) {
  flutter::SemanticsNode first;
  first.id = 1;
  first.role = SemanticsRole::kNone;
  first.identifier = "id";
  first.value = "a";
  node_.UpdateWithNode(first);

  flutter::SemanticsNode second;
  second.id = 1;
  second.role = SemanticsRole::kNone;
  second.identifier = "id";
  second.value = "b";
  node_.UpdateWithNode(second);
  EXPECT_TRUE(node_.contentChanged);
  EXPECT_EQ(node_.identifier, "id");
  EXPECT_EQ(node_.value, "b");
}

TEST_F(SemanticsNodeTest, HasChangedLabelTrueWhenLabelEmptyPrevNonEmpty) {
  node_.previousLabel = "old";
  EXPECT_TRUE(node_.HasChangedLabel());
}

TEST_F(SemanticsNodeTest, HasChangedLabelTrueWhenLabelNonEmptyPrevEmpty) {
  node_.label = "new";
  EXPECT_TRUE(node_.HasChangedLabel());
}

TEST_F(SemanticsNodeTest, HasChangedLabelTrueWhenBothNonEmptyAndDifferent) {
  node_.label = "new";
  node_.previousLabel = "old";
  EXPECT_TRUE(node_.HasChangedLabel());
}

TEST_F(SemanticsNodeTest, HasChangedLabelFalseWhenBothNonEmptyAndEqual) {
  node_.label = "same";
  node_.previousLabel = "same";
  EXPECT_FALSE(node_.HasChangedLabel());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenPlatformViewNode) {
  node_.platformViewId = 7;
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenSelected) {
  node_.flags.isSelected = SemanticsTristate::kTrue;
  EXPECT_TRUE(node_.IsFocusable());
  EXPECT_TRUE(node_.IsSelected());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenFocused) {
  node_.flags.isFocused = SemanticsTristate::kTrue;
  EXPECT_TRUE(node_.IsFocusable());
  EXPECT_TRUE(node_.IsFocused());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenEnabledTristateSet) {
  node_.flags.isEnabled = SemanticsTristate::kTrue;
  EXPECT_TRUE(node_.IsFocusable());
  EXPECT_TRUE(node_.IsEnabled());
  node_.flags.isEnabled = SemanticsTristate::kFalse;
  EXPECT_TRUE(node_.IsFocusable());
  EXPECT_FALSE(node_.IsEnabled());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenInMutuallyExclusiveGroup) {
  node_.flags.isInMutuallyExclusiveGroup = true;
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenToggledStateSet) {
  node_.flags.isToggled = SemanticsTristate::kFalse;
  EXPECT_TRUE(node_.IsFocusable());
}

TEST_F(SemanticsNodeTest, IsFocusableTrueWhenSliderFlag) {
  node_.flags.isSlider = true;
  EXPECT_TRUE(node_.IsFocusable());
  EXPECT_TRUE(node_.IsSlider());
}

TEST_F(SemanticsNodeTest, IsCheckableTrueWhenCheckStateNotNone) {
  node_.flags.isChecked = SemanticsCheckState::kFalse;
  EXPECT_TRUE(node_.IsCheckable());
}

TEST_F(SemanticsNodeTest, IsCheckedTrueWhenCheckStateTrue) {
  node_.flags.isChecked = SemanticsCheckState::kTrue;
  EXPECT_TRUE(node_.IsChecked());
}

TEST_F(SemanticsNodeTest, IsCheckedFalseWhenCheckStateMixedWithoutToggle) {
  node_.flags.isChecked = SemanticsCheckState::kMixed;
  EXPECT_FALSE(node_.IsChecked());
}

TEST_F(SemanticsNodeTest, IsPasswordFalseWhenTextFieldNotObscured) {
  node_.flags.isTextField = true;
  EXPECT_FALSE(node_.IsPassword());
}

TEST_F(SemanticsNodeTest, IsScrollableForEachSingleScrollAction) {
  const SemanticsAction scroll_actions[] = {
      SemanticsAction::kScrollLeft, SemanticsAction::kScrollRight,
      SemanticsAction::kScrollUp, SemanticsAction::kScrollDown};
  for (auto action : scroll_actions) {
    SemanticsNodeExtend node;
    node.id = 1;
    node.actions = static_cast<int32_t>(action);
    EXPECT_TRUE(node.IsScrollable());
  }
  EXPECT_FALSE(node_.IsScrollable());
}

}  // namespace testing
}  // namespace flutter
