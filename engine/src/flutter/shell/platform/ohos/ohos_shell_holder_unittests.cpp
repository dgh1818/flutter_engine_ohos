/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {
namespace testing {

// Helper to create Settings configured for software rendering (no GPU needed).
static Settings MakeTestSettings() {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  return settings;
}

// Verify that OHOSShellHolder can be constructed with software rendering and a
// null napi facade, mirroring the AndroidShellHolder test pattern.
TEST(OHOSShellHolder, Create) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_NE(holder.get(), nullptr);
  EXPECT_TRUE(holder->IsValid());
  EXPECT_NE(holder->GetPlatformView().get(), nullptr);
}

// Verify that GetSettings returns the same settings passed to the constructor.
TEST(OHOSShellHolder, GetSettings) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_EQ(holder->GetSettings().ohos_rendering_api,
            OHOSRenderingAPI::kSoftware);
}

// Verify that GetNapiFacade returns the facade passed to the constructor.
TEST(OHOSShellHolder, GetNapiFacade) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_EQ(holder->GetNapiFacade(), napi_facade);
}

// Verify that GetPlatformMessageHandler returns a valid handler.
TEST(OHOSShellHolder, GetPlatformMessageHandler) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_TRUE(holder->GetPlatformMessageHandler());
}

// Verify that NotifyLowMemoryWarning on a valid holder does not crash.
TEST(OHOSShellHolder, NotifyLowMemoryWarning) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  holder->NotifyLowMemoryWarning();
  SUCCEED();
}

// Verify that GetDartHeapMemoryUsage returns zero values for a freshly
// created holder (no Dart isolate running yet).
TEST(OHOSShellHolder, GetDartHeapMemoryUsage) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto usage = holder->GetDartHeapMemoryUsage();
  EXPECT_EQ(usage.old_used, 0u);
  EXPECT_EQ(usage.new_used, 0u);
}

// Verify that merged platform/UI thread setting works correctly.
TEST(OHOSShellHolder, CreateWithMergedPlatformAndUIThread) {
  auto settings = MakeTestSettings();
  // Default is MergedPlatformUIThread::kEnabled
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_TRUE(holder->IsValid());
}

// Verify that unmerged platform/UI thread setting works correctly.
TEST(OHOSShellHolder, CreateWithUnMergedPlatformAndUIThread) {
  auto settings = MakeTestSettings();
  settings.merged_platform_ui_thread =
      Settings::MergedPlatformUIThread::kDisabled;
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_TRUE(holder->IsValid());
}

// Verify that WaitRasterTasksFinished completes without blocking on a valid
// holder. The raster task runner posts the signal back, so the wait should
// return promptly.
TEST(OHOSShellHolder, WaitRasterTasksFinished) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  EXPECT_TRUE(holder->IsValid());
  holder->WaitRasterTasksFinished();
  SUCCEED();
}

// Verify that GetVsyncWaiter returns a valid (non-null) waiter from the shell.
TEST(OHOSShellHolder, GetVsyncWaiter) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto waiter = holder->GetVsyncWaiter();
  EXPECT_FALSE(waiter.expired());
}

// Verify that SetAccessibilityProvider with nullptr does not crash and the
// provider is stored.
TEST(OHOSShellHolder, SetAccessibilityProviderWithNull) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  holder->SetAccessibilityProvider(nullptr);
  SUCCEED();
}

// Verify that FindFocusNode on an empty semantics tree returns FAILED.
TEST(OHOSShellHolder, FindFocusNodeEmptyTree) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto result = holder->FindFocusNode(
      0, ARKUI_ACCESSIBILITY_NATIVE_FOCUS_TYPE_ACCESSIBILITY, nullptr);
  EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

// Verify that FindNextFocusNode on an empty semantics tree returns FAILED.
TEST(OHOSShellHolder, FindNextFocusNodeEmptyTree) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto result = holder->FindNextFocusNode(
      0, ARKUI_ACCESSIBILITY_NATIVE_DIRECTION_FORWARD, nullptr);
  EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

// Verify that FillNodesWithSearchText on an empty tree returns FAILED without
// touching the null list pointer.
TEST(OHOSShellHolder, FillNodesWithSearchTextEmptyTree) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto result = holder->FillNodesWithSearchText(0, "test", nullptr);
  EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

// Verify that FillNodesWithSearch on an empty tree returns FAILED without
// touching the null list pointer.
TEST(OHOSShellHolder, FillNodesWithSearchEmptyTree) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto result = holder->FillNodesWithSearch(
      0, ARKUI_ACCESSIBILITY_NATIVE_SEARCH_MODE_PREFETCH_CURRENT, nullptr);
  EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

// Verify that ExecuteAction on an empty tree (no provider set) returns FAILED
// immediately without touching the null arguments pointer.
TEST(OHOSShellHolder, ExecuteActionNoProvider) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto result = holder->ExecuteAction(
      0, ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK, nullptr);
  EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

// Verify that ClearAccessibilityFocus on an empty tree returns SUCCESSFUL
// (it is a no-op when no focused node exists).
TEST(OHOSShellHolder, ClearAccessibilityFocusEmptyTree) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto result = holder->ClearAccessibilityFocus(0);
  EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
}

// Verify that GetAccessibilityNodeCursorPosition on an empty tree returns
// FAILED without touching the null index pointer.
TEST(OHOSShellHolder, GetAccessibilityNodeCursorPositionEmptyTree) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto result = holder->GetAccessibilityNodeCursorPosition(0, nullptr);
  EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);
}

// Verify that ReloadSystemFonts on a valid holder does not crash. With no
// font source available on the test device path, it should be a no-op.
TEST(OHOSShellHolder, ReloadSystemFonts) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  holder->ReloadSystemFonts();
  SUCCEED();
}

// Verify that the static InitializeSystemFont does not crash when no font
// source is found.
TEST(OHOSShellHolder, InitializeSystemFont) {
  OHOSShellHolder::InitializeSystemFont();
  SUCCEED();
}

}  // namespace testing
}  // namespace flutter
