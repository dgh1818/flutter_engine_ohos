/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#define FML_USED_ON_EMBEDDER

#include <gtest/gtest.h>

#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "flutter/common/constants.h"
#include "flutter/fml/log_settings.h"
#include "flutter/fml/message_loop.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"

#include "flutter/lib/ui/semantics/semantics_node.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

#define private public
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#undef private

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
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    auto again =
        std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
    EXPECT_TRUE(again->IsValid());
  }
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    auto loud_holder =
        std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
    EXPECT_TRUE(loud_holder->IsValid());
#if FLUTTER_JIT_RUNTIME
    loud_holder->BuildRunConfiguration("main", "bundle.js", {"--a"});
    loud_holder->BuildRunConfiguration("main", "", {});
    loud_holder->BuildRunConfiguration("", "", {"--x"});
#endif
  }
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

TEST(OHOSShellHolder, NotifyLowMemoryWarning) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  holder->NotifyLowMemoryWarning();
  holder->shell_.reset();
  EXPECT_FALSE(holder->IsValid());
  holder->NotifyLowMemoryWarning();
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    holder->NotifyLowMemoryWarning();
    holder->Launch(nullptr, "main", "", {});
    EXPECT_FALSE(holder->AddViewSync(kFlutterImplicitViewId));
    EXPECT_EQ(holder->Spawn(napi_facade, "main", "", "", {}), nullptr);
  }
}

TEST(OHOSShellHolder, GetDartHeapMemoryUsage) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  auto usage = holder->GetDartHeapMemoryUsage();
  EXPECT_EQ(usage.old_used, 0u);
  EXPECT_EQ(usage.new_used, 0u);
  holder->shell_.reset();
  auto invalid = holder->GetDartHeapMemoryUsage();
  EXPECT_EQ(invalid.old_used, 0u);
  EXPECT_EQ(invalid.new_used, 0u);
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

TEST(OHOSShellHolder, ReloadSystemFonts) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  const std::string stale_local = holder->local_font_path_ + ".stale";
  holder->ReloadSystemFonts();
  holder->local_font_path_ = stale_local;
  holder->ReloadSystemFonts();
  EXPECT_NE(holder->local_font_path_, stale_local);
}

// Verify that the static InitializeSystemFont does not crash when no font
// source is found.
TEST(OHOSShellHolder, InitializeSystemFont) {
  OHOSShellHolder::InitializeSystemFont();
  SUCCEED();
}

TEST(OHOSShellHolder, GetWindowControllerIsCreatedForMainHolder) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_NE(holder->GetWindowController(), nullptr);
  EXPECT_EQ(holder->GetWindowController()->GetNapiFacade(), napi_facade);
}

#if FLUTTER_JIT_RUNTIME
static Settings MakeNoKernelSettings() {
  auto settings = MakeTestSettings();
  settings.application_kernel_asset = "/nonexistent_ut_kernel_blob";
  return settings;
}
#endif  // FLUTTER_JIT_RUNTIME

TEST(OHOSShellHolder, Launch) {
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
#if FLUTTER_JIT_RUNTIME
  auto settings = MakeNoKernelSettings();
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  holder->Launch(nullptr, "main", "", {});
#else
  auto settings = MakeTestSettings();
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
#endif
  holder->shell_.reset();
  EXPECT_FALSE(holder->IsValid());
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    holder->Launch(nullptr, "main", "", {});
  }
  holder->Launch(nullptr, "main", "", {});
}

TEST(OHOSShellHolder, Spawn) {
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
#if FLUTTER_JIT_RUNTIME
  auto settings = MakeNoKernelSettings();
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  EXPECT_EQ(holder->Spawn(napi_facade, "main", "", "", {}), nullptr);
#else
  auto settings = MakeTestSettings();
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
#endif
  holder->shell_.reset();
  EXPECT_FALSE(holder->IsValid());
  EXPECT_EQ(holder->Spawn(napi_facade, "main", "", "", {}), nullptr);
}

TEST(OHOSShellHolder, AddViewSync) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  holder->shell_.reset();
  EXPECT_FALSE(holder->AddViewSync(kFlutterImplicitViewId));
}

TEST(OHOSShellHolder, SpawnAsyncWithoutKernelBlobReturnsNull) {
#if !FLUTTER_JIT_RUNTIME
  GTEST_SKIP() << "kernel-blob early return only exists in JIT runtimes";
#else
  auto settings = MakeNoKernelSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  EXPECT_EQ(holder->SpawnAsync(napi_facade, "main", "", "", {}), nullptr);
#endif
}

TEST(OHOSShellHolder, DartMemoryMonitorCycle) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());

  holder->StopDartMemoryMonitor();
  EXPECT_FALSE(holder->memory_monitor_running_);
  EXPECT_NO_FATAL_FAILURE(
      holder->ScheduleDartMemoryMonitor());  // stopped: early return

  holder->memory_monitor_running_ = true;
  EXPECT_NO_FATAL_FAILURE(holder->CheckDartHeapMemory());  // 0 < threshold
  EXPECT_TRUE(holder->memory_monitor_running_);            // re-armed
  holder->StopDartMemoryMonitor();
  EXPECT_FALSE(holder->memory_monitor_running_);
}

TEST(OHOSShellHolder, ExecuteActionSyncGuards) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  EXPECT_EQ(holder->ExecuteAction(
                1, ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK, nullptr),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);

  static char provider_storage;
  holder->SetAccessibilityProvider(
      reinterpret_cast<ArkUI_AccessibilityProvider*>(&provider_storage));
  SemanticsNode root;
  root.id = 0;
  SemanticsNode child;
  child.id = 1;
  root.childrenInTraversalOrder = {1};
  std::unordered_map<int32_t, SemanticsNode> nodes;
  nodes[0] = root;
  nodes[1] = child;
  {
    std::lock_guard<std::mutex> lock(*holder->bridge_mutex_);
    holder->bridge_->tree_.UpdateWithNodes(nodes);
  }
  EXPECT_EQ(holder->ExecuteAction(
                99, ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK, nullptr),
            ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED);

  holder->SetAccessibilityProvider(nullptr);
}

TEST(OHOSShellHolder, ExecuteActionMapsArkuiActionsOffPlatformThread) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  static char provider_storage;
  holder->SetAccessibilityProvider(
      reinterpret_cast<ArkUI_AccessibilityProvider*>(&provider_storage));

  auto install_node = [&](int32_t actions) {
    SemanticsNode root;
    root.id = 0;
    SemanticsNode child;
    child.id = 1;
    child.actions = actions;
    root.childrenInTraversalOrder = {1};
    std::unordered_map<int32_t, SemanticsNode> nodes;
    nodes[0] = root;
    nodes[1] = child;
    std::lock_guard<std::mutex> lock(*holder->bridge_mutex_);
    holder->bridge_->tree_.UpdateWithNodes(nodes);
  };

  auto run_action = [&](ArkUI_Accessibility_ActionType action) {
    int32_t result = ARKUI_ACCESSIBILITY_NATIVE_RESULT_FAILED;
    std::thread worker([&] {
      result = holder->ExecuteAction(
          1, action,
          reinterpret_cast<ArkUI_AccessibilityActionArguments*>(0x1));
    });
    worker.join();
    EXPECT_EQ(result, ARKUI_ACCESSIBILITY_NATIVE_RESULT_SUCCESSFUL);
  };

  install_node(static_cast<int32_t>(SemanticsAction::kTap));
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLICK);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_LONG_CLICK);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_COPY);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CUT);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_PASTE);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_GAIN_ACCESSIBILITY_FOCUS);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_CLEAR_ACCESSIBILITY_FOCUS);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_TEXT);
  run_action(static_cast<ArkUI_Accessibility_ActionType>(0x7fff));

  install_node(static_cast<int32_t>(SemanticsAction::kScrollUp));
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD);
  install_node(static_cast<int32_t>(SemanticsAction::kScrollLeft));
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD);
  install_node(static_cast<int32_t>(SemanticsAction::kIncrease));
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD);
  install_node(0);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_FORWARD);

  install_node(static_cast<int32_t>(SemanticsAction::kScrollDown));
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD);
  install_node(static_cast<int32_t>(SemanticsAction::kScrollRight));
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD);
  install_node(static_cast<int32_t>(SemanticsAction::kDecrease));
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD);
  install_node(0);
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SCROLL_BACKWARD);

  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT);
  StubArkuiSetActionArgument("selectTextBegin", "1");
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT);
  StubArkuiSetActionArgument("selectTextEnd", "3");
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SELECT_TEXT);
  StubArkuiResetActionArguments();

  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION);
  StubArkuiSetActionArgument("offset", "2");
  run_action(ARKUI_ACCESSIBILITY_NATIVE_ACTION_TYPE_SET_CURSOR_POSITION);
  StubArkuiResetActionArguments();

  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  holder->SetAccessibilityProvider(nullptr);
}

namespace {

class ScopedKernelBlob {
 public:
  ScopedKernelBlob() {
    snprintf(path_, sizeof(path_), "%s/ut_shell_holder_kernel_blob",
             GetUtTmpDir());
    std::ofstream out(path_, std::ios::binary);
    out << "placeholder kernel blob";
  }
  ~ScopedKernelBlob() { std::remove(path_); }
  const char* path() const { return path_; }

 private:
  char path_[4096];
};

std::unique_ptr<OHOSShellHolder> MakeHolderWithAssets(Settings& settings) {
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  holder->asset_provider_ = std::make_unique<OHOSAssetProvider>(nullptr, "");
  return holder;
}

}  // namespace

TEST(OHOSShellHolder, BuildRunConfigurationWithoutKernelBlob) {
  auto settings = MakeTestSettings();
  auto holder = MakeHolderWithAssets(settings);
  ASSERT_TRUE(holder->IsValid());
#if FLUTTER_JIT_RUNTIME
  auto config = holder->BuildRunConfiguration("main", "lib", {});
  EXPECT_FALSE(config.has_value());
#else
  auto config = holder->BuildRunConfiguration("main", "lib", {});
  EXPECT_TRUE(config.has_value());
#endif
}

TEST(OHOSShellHolder, BuildRunConfigurationFullChain) {
#if !FLUTTER_JIT_RUNTIME
  GTEST_SKIP() << "kernel-blob path only exists in JIT runtimes";
#else
  ScopedKernelBlob blob;
  auto settings = MakeTestSettings();
  settings.application_kernel_asset = blob.path();
  auto holder = MakeHolderWithAssets(settings);
  ASSERT_TRUE(holder->IsValid());

  auto both = holder->BuildRunConfiguration("main", "bundle.js", {"--a"});
  ASSERT_TRUE(both.has_value());

  auto entry_only = holder->BuildRunConfiguration("main", "", {});
  ASSERT_TRUE(entry_only.has_value());

  auto library_only = holder->BuildRunConfiguration("", "bundle.js", {});
  ASSERT_TRUE(library_only.has_value());

  auto defaults = holder->BuildRunConfiguration("", "", {});
  ASSERT_TRUE(defaults.has_value());

  auto args_only = holder->BuildRunConfiguration("", "", {"--x", "--y"});
  EXPECT_TRUE(args_only.has_value());
#endif
}

}  // namespace testing
}  // namespace flutter
