/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <gtest/gtest.h>
#include <mutex>
#include <string>
#include "flutter/shell/platform/ohos/accessibility/multi_instance_xcomp_accessibility.h"
#include "flutter/shell/platform/ohos/ohos_xcomponent_adapter.h"

namespace flutter {
namespace testing {

namespace {

constexpr int32_t kFailed = ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
constexpr const char* kMissingId = "ut_a11y_no_such_instance";
constexpr const char* kRegisteredId = "ut_a11y_registered_instance";

class ScopedXcompRegistration {
 public:
  ScopedXcompRegistration(const std::string& id, XComponentBase* xc)
      : id_(id) {
    auto* adapter = XComponentAdapter::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    adapter->xcomponetMap_[id_] = xc;
  }

  ~ScopedXcompRegistration() {
    auto* adapter = XComponentAdapter::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    adapter->xcomponetMap_.erase(id_);
  }

 private:
  std::string id_;
};

}

TEST(MultiInstanceXCompAccessibilityTest, BindWiresAllSevenCallbacks) {
  MultiInstanceXCompAccessibility a11y;
  a11y.BindAccessibilityCallbackWithInstance();
  const auto& cb = a11y.a11yProviderCallbackWithInstance_;
  EXPECT_NE(cb.findAccessibilityNodeInfosById, nullptr);
  EXPECT_NE(cb.findAccessibilityNodeInfosByText, nullptr);
  EXPECT_NE(cb.findFocusedAccessibilityNode, nullptr);
  EXPECT_NE(cb.findNextFocusAccessibilityNode, nullptr);
  EXPECT_NE(cb.executeAccessibilityAction, nullptr);
  EXPECT_NE(cb.clearFocusedFocusAccessibilityNode, nullptr);
  EXPECT_NE(cb.getAccessibilityNodeCursorPosition, nullptr);
}

TEST(MultiInstanceXCompAccessibilityTest, UnregisteredInstanceIdFails) {
  MultiInstanceXCompAccessibility a11y;
  a11y.BindAccessibilityCallbackWithInstance();
  auto& cb = a11y.a11yProviderCallbackWithInstance_;
  EXPECT_EQ(XComponentAdapter::GetInstance()->GetXcomponentBase(kMissingId),
            nullptr);

  EXPECT_EQ(cb.findAccessibilityNodeInfosById(
                kMissingId, 11,
                ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CHILDREN, 1,
                nullptr),
            kFailed);
  EXPECT_EQ(cb.findAccessibilityNodeInfosByText(kMissingId, 12, "needle", 2,
                                                nullptr),
            kFailed);
  EXPECT_EQ(cb.findFocusedAccessibilityNode(
                kMissingId, 13, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_INPUT, 3,
                nullptr),
            kFailed);
  EXPECT_EQ(cb.findNextFocusAccessibilityNode(
                kMissingId, 14, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_DOWN, 4,
                nullptr),
            kFailed);
  EXPECT_EQ(cb.executeAccessibilityAction(
                kMissingId, 15, ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK,
                nullptr, 5),
            kFailed);
  EXPECT_EQ(cb.clearFocusedFocusAccessibilityNode(kMissingId), kFailed);
  int32_t index = -1;
  EXPECT_EQ(cb.getAccessibilityNodeCursorPosition(kMissingId, 16, 6, &index),
            kFailed);
}

TEST(MultiInstanceXCompAccessibilityTest,
     RegisteredInstanceForwardsToXComponent) {
  MultiInstanceXCompAccessibility a11y;
  a11y.BindAccessibilityCallbackWithInstance();
  auto& cb = a11y.a11yProviderCallbackWithInstance_;

  XComponentBase xc(kRegisteredId);
  ScopedXcompRegistration registration(kRegisteredId, &xc);
  EXPECT_EQ(XComponentAdapter::GetInstance()->GetXcomponentBase(kRegisteredId),
            &xc);

  EXPECT_EQ(cb.findAccessibilityNodeInfosById(
                kRegisteredId, 21,
                ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_SIBLINGS, 1,
                nullptr),
            kFailed);
  EXPECT_EQ(cb.findAccessibilityNodeInfosByText(kRegisteredId, 22, "text", 2,
                                                nullptr),
            kFailed);
  EXPECT_EQ(cb.findFocusedAccessibilityNode(
                kRegisteredId, 23,
                ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY, 3,
                nullptr),
            kFailed);
  EXPECT_EQ(cb.findNextFocusAccessibilityNode(
                kRegisteredId, 24, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_UP, 4,
                nullptr),
            kFailed);
  EXPECT_EQ(cb.executeAccessibilityAction(
                kRegisteredId, 25,
                ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK, nullptr, 5),
            kFailed);
  EXPECT_EQ(cb.clearFocusedFocusAccessibilityNode(kRegisteredId), kFailed);
  int32_t index = -1;
  EXPECT_EQ(
      cb.getAccessibilityNodeCursorPosition(kRegisteredId, 26, 6, &index),
      kFailed);
}

}
}

