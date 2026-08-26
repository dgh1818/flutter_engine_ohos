/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Unit tests for the multi-window JS-callout facade methods added to
// PlatformViewOHOSNapi (RequestWindowHost / CreateRegularAbility /
// BindEntryAbilityToView / DestroyWindowHost / ExitApplication /
// SetWindowSize / SetWindowTitle / SetWindowMaximized / SetWindowMinimized /
// SetWindowFullscreen / SetWindowConstraints / ActivateWindow).
// Testability model: the facade is constructed with a null napi_env
// (PlatformViewOHOSNapi(nullptr)), mirroring the pattern established in
// ohos_touch_processor_unittests.cpp. Each method opens a handle scope,
// marshals its arguments with napi_create_int64/double/int32/get_boolean, then
// hands them to fml::napi::InvokeJsMethod. With a null env:
//   - napi_create_int64/double/int32/get_boolean (stubbed below, matching real
//     N-API null-env behavior) return napi_invalid_arg → every per-argument
//     `if (status != napi_ok)` error branch is taken.
//   - InvokeJsMethod's chain (napi_get_reference_value / napi_get_named_property
//     / napi_call_function) resolves to the shared stubs in
//     ohos_touch_processor_unittests.cpp, which return napi_ok → the
//     "InvokeJsMethod <name> fail" LOG branch is NOT reachable in this
//     executable. Driving it needs a real live NAPI env + JS object (a
//     success/failure JS round-trip), which is device-only (Class 3).
//   - The real-JS success path (napi_ok end-to-end) is likewise Class 3.
// The file also covers the first (arg-unmarshal) error branch of the three
// ETS→C++ natives added for multi-window (nativeHandleOsWindowClosed /
// nativeComputeWindowPosition / nativeNotifyWindowActivated, registered in
// library_loader.cpp). napi_get_cb_info is interposed at file scope so a null
// env returns napi_invalid_arg without entering real libnapi (which may abort
// on a null env depending on NDK revision). The JS-invocation success path
// (real env + callback_info + ForView hit) is device-only (Class 3).

#include <gtest/gtest.h>
#include "flutter/shell/platform/ohos/test_stubs/ace_napi_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#define private public
#include "flutter/lib/ui/plugins/callback_cache.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_vsync_voting_mgr.h"
#include "flutter/shell/platform/ohos/ohos_xcomponent_adapter.h"
#include "flutter/shell/platform/ohos/windowing/ohos_window_controller.h"
#undef private

#include "flutter/shell/platform/ohos/ohos_main.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

namespace flutter {
namespace testing {

// ===== Windowing JS-callout methods (null env → arg-marshaling error path) ====
// Each call below covers: napi_open_handle_scope / napi_close_handle_scope
// (shared stubs, napi_ok) and every `if (status != napi_ok)` per-argument error
// branch inside the method (null-env stubs → napi_invalid_arg).

// requestWindowHost: 5 args — int64 view_id, int64 parent_view_id,
// double width, double height, int32 archetype.
class PlatformViewOHOSNapiTest : public ::testing::Test {
 protected:
  void SetUp() override {
    StubNapiReset();
    saved_env_ = PlatformViewOHOSNapi::env_;
    saved_languages_ = PlatformViewOHOSNapi::system_languages;
    PlatformViewOHOSNapi::env_ = nullptr;
    saved_notify_func_ = PlatformViewOHOSNapi::notify_page_changed_func_;
    PlatformViewOHOSNapi::notify_page_changed_func_ = nullptr;
    saved_refresh_rate_ = PlatformViewOHOSNapi::display_refresh_rate;
    saved_display_width_ = PlatformViewOHOSNapi::display_width;
    saved_display_height_ = PlatformViewOHOSNapi::display_height;
    saved_density_pixels_ = PlatformViewOHOSNapi::display_density_pixels;
  }

  void TearDown() override {
    UpdateDlopenForceFail(false);
    PlatformViewOHOSNapi::notify_page_changed_func_ = saved_notify_func_;
    PlatformViewOHOSNapi::env_ = saved_env_;
    PlatformViewOHOSNapi::system_languages = saved_languages_;
    PlatformViewOHOSNapi::display_refresh_rate = saved_refresh_rate_;
    PlatformViewOHOSNapi::display_width = saved_display_width_;
    PlatformViewOHOSNapi::display_height = saved_display_height_;
    PlatformViewOHOSNapi::display_density_pixels = saved_density_pixels_;
    StubNapiReset();
  }

  std::vector<std::string> saved_languages_;
  napi_env saved_env_ = nullptr;
  PlatformViewOHOSNapi::NotifyPageChangedFunc saved_notify_func_ = nullptr;
  int32_t saved_refresh_rate_ = 0;
  int64_t saved_display_width_ = 0;
  int64_t saved_display_height_ = 0;
  double saved_density_pixels_ = 0.0;
};

TEST_F(PlatformViewOHOSNapiTest, RequestWindowHostNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.RequestWindowHost(9401, 0, 640.0, 480.0, 1));
}

// createRegularAbility: 5 args — int64 view_id, int64 request_id,
// double width, double height, string title.
TEST_F(PlatformViewOHOSNapiTest, CreateRegularAbilityNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(
      facade.CreateRegularAbility(9401, 1001, 640.0, 480.0, "title"));
}

// bindEntryAbilityToView: 4 args — int64 view_id, double width, double height,
// string title.
TEST_F(PlatformViewOHOSNapiTest, BindEntryAbilityToViewNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(
      facade.BindEntryAbilityToView(9401, 640.0, 480.0, "title"));
}

// destroyWindowHost: 1 arg — int64 view_id.
TEST_F(PlatformViewOHOSNapiTest, DestroyWindowHostNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.DestroyWindowHost(9401));
}

// exitApplication: no args — InvokeJsMethod with argc 0 / nullptr argv.
TEST_F(PlatformViewOHOSNapiTest, ExitApplicationNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.ExitApplication());
}

// setWindowSize: 3 args — int64 view_id, double width, double height.
TEST_F(PlatformViewOHOSNapiTest, SetWindowSizeNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.SetWindowSize(9401, 640.0, 480.0));
}

// setWindowTitle: 2 args — int64 view_id, string title.
TEST_F(PlatformViewOHOSNapiTest, SetWindowTitleNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.SetWindowTitle(9401, "title"));
}

// setWindowMaximized: 2 args — int64 view_id, bool maximized.
TEST_F(PlatformViewOHOSNapiTest, SetWindowMaximizedNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.SetWindowMaximized(9401, true));
}

// setWindowMinimized: 2 args — int64 view_id, bool minimized.
TEST_F(PlatformViewOHOSNapiTest, SetWindowMinimizedNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.SetWindowMinimized(9401, true));
}

// setWindowFullscreen: 2 args — int64 view_id, bool fullscreen.
TEST_F(PlatformViewOHOSNapiTest, SetWindowFullscreenNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.SetWindowFullscreen(9401, false));
}

// setWindowConstraints: 5 args — int64 view_id, double min/max width/height.
TEST_F(PlatformViewOHOSNapiTest, SetWindowConstraintsNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(
      facade.SetWindowConstraints(9401, 320.0, 640.0, 240.0, 480.0));
}

// activateWindow: 1 arg — int64 view_id.
TEST_F(PlatformViewOHOSNapiTest, ActivateWindowNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE(facade.ActivateWindow(9401));
}

TEST_F(PlatformViewOHOSNapiTest, SetDVsyncSwitchNullEnv) {
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeSetDVsyncSwitch(nullptr, nullptr));
}

TEST_F(PlatformViewOHOSNapiTest, LTPODispatchHighFrameRateNullEnv) {
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeLTPODispatchHighFrameRate(nullptr, nullptr),
      nullptr);
}

// ===== ETS → C++ native callbacks (null env → napi_get_cb_info error path) ====
// File-scope napi_get_cb_info stub returns napi_invalid_arg for env == nullptr
// so these natives take the `ret != napi_ok` early-return without entering
// real libnapi. The JS-invocation success path is device-only (Class 3).

// nativeHandleOsWindowClosed: napi_get_cb_info fails → DLOG + return nullptr.
TEST_F(PlatformViewOHOSNapiTest, HandleOsWindowClosedNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeHandleOsWindowClosed(nullptr, nullptr),
            nullptr);
}

// nativeComputeWindowPosition: napi_get_cb_info fails (or argc < 12) → returns
// the default "not computed" napi_value. Like real libnapi, the stub does not
// write the out-param when env is null, so the production `return result`
// reads an uninitialized local on that error path; the test only pins the
// call itself (no crash, no hang).
TEST_F(PlatformViewOHOSNapiTest, ComputeWindowPositionNullEnv) {
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeComputeWindowPosition(nullptr, nullptr));
}

// nativeNotifyWindowActivated: napi_get_cb_info fails (or argc < 2) → DLOG +
// return nullptr.
TEST_F(PlatformViewOHOSNapiTest, NotifyWindowActivatedNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyWindowActivated(nullptr, nullptr),
            nullptr);
}

namespace {

class SystemLanguagesGuard {
 public:
  SystemLanguagesGuard() : saved_(PlatformViewOHOSNapi::system_languages) {}
  ~SystemLanguagesGuard() { PlatformViewOHOSNapi::system_languages = saved_; }

 private:
  std::vector<std::string> saved_;
};

locale MakeLocale(const char* language,
                  const char* script,
                  const char* region) {
  locale l;
  l.language = language;
  l.script = script;
  l.region = region;
  return l;
}

constexpr napi_status kStubFailure = napi_generic_failure;

inline napi_env FakeNapiEnv() {
  return reinterpret_cast<napi_env>(0xF00D);
}

}

TEST_F(PlatformViewOHOSNapiTest, EmptySupportedLocalesReturnsDefault) {
  PlatformViewOHOSNapi facade(nullptr);
  flutter::locale resolved = facade.resolveNativeLocale({});
  EXPECT_EQ(resolved.language, "zh");
  EXPECT_EQ(resolved.script, "Hans");
  EXPECT_EQ(resolved.region, "CN");
}

TEST_F(PlatformViewOHOSNapiTest, EmptySystemLanguagesInjectsZhHansDefault) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  PlatformViewOHOSNapi::system_languages = {};
  flutter::locale resolved = facade.resolveNativeLocale(
      {MakeLocale("en", "Latn", "US"), MakeLocale("fr", "Frac", "FR")});
  EXPECT_EQ(resolved.language, "en");
  EXPECT_EQ(resolved.script, "Latn");
  EXPECT_EQ(resolved.region, "US");
  ASSERT_EQ(PlatformViewOHOSNapi::system_languages.size(), 1u);
  EXPECT_EQ(PlatformViewOHOSNapi::system_languages[0], "zh-Hans");
}

TEST_F(PlatformViewOHOSNapiTest, FullFormLanguageScriptRegionMatch) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  PlatformViewOHOSNapi::system_languages = {"en-Latn-US"};
  flutter::locale resolved = facade.resolveNativeLocale(
      {MakeLocale("de", "Hans", "CN"), MakeLocale("en", "Latn", "US")});
  EXPECT_EQ(resolved.language, "en");
  EXPECT_EQ(resolved.script, "Latn");
  EXPECT_EQ(resolved.region, "US");
}

TEST_F(PlatformViewOHOSNapiTest, LanguageRegionMatchIgnoresScript) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  PlatformViewOHOSNapi::system_languages = {"en-US"};
  flutter::locale resolved =
      facade.resolveNativeLocale({MakeLocale("en", "Arab", "US")});
  EXPECT_EQ(resolved.language, "en");
  EXPECT_EQ(resolved.script, "Arab");
  EXPECT_EQ(resolved.region, "US");
}

TEST_F(PlatformViewOHOSNapiTest, LanguageOnlyMatchWinsLast) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  PlatformViewOHOSNapi::system_languages = {"de-Latn-DE"};
  flutter::locale resolved =
      facade.resolveNativeLocale({MakeLocale("de", "Hant", "TW")});
  EXPECT_EQ(resolved.language, "de");
  EXPECT_EQ(resolved.script, "Hant");
  EXPECT_EQ(resolved.region, "TW");
}

TEST_F(PlatformViewOHOSNapiTest, NoMatchFallsBackToFirstSupported) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  PlatformViewOHOSNapi::system_languages = {"ja-Jpan-JP"};
  flutter::locale resolved = facade.resolveNativeLocale(
      {MakeLocale("ko", "Hang", "KR"), MakeLocale("zh", "Hans", "CN")});
  EXPECT_EQ(resolved.language, "ko");
  EXPECT_EQ(resolved.script, "Hang");
  EXPECT_EQ(resolved.region, "KR");
}

TEST_F(PlatformViewOHOSNapiTest, ComputeResolvedLocalesEmptyInput) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  auto result = facade.FlutterViewComputePlatformResolvedLocales({});
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->size(), 3u);
  EXPECT_EQ((*result)[0], "zh");
  EXPECT_EQ((*result)[1], "CN");
  EXPECT_EQ((*result)[2], "Hans");
}

TEST_F(PlatformViewOHOSNapiTest, ComputeResolvedLocalesFullMatch) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  PlatformViewOHOSNapi::system_languages = {"en-Latn-US"};
  auto result = facade.FlutterViewComputePlatformResolvedLocales(
      {"en", "US", "Latn", "zz", "ZZ", "Zzzz"});
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->size(), 3u);
  EXPECT_EQ((*result)[0], "en");
  EXPECT_EQ((*result)[1], "US");
  EXPECT_EQ((*result)[2], "Latn");
}

TEST_F(PlatformViewOHOSNapiTest, ComputeResolvedLocalesFallbackFirst) {
  SystemLanguagesGuard guard;
  PlatformViewOHOSNapi facade(nullptr);
  PlatformViewOHOSNapi::system_languages = {"ja-Jpan-JP"};
  auto result = facade.FlutterViewComputePlatformResolvedLocales(
      {"ko", "KR", "Hang"});
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->size(), 3u);
  EXPECT_EQ((*result)[0], "ko");
  EXPECT_EQ((*result)[1], "KR");
  EXPECT_EQ((*result)[2], "Hang");
}

TEST_F(PlatformViewOHOSNapiTest, WindowingCalloutsCompleteMarshaling) {
  PlatformViewOHOSNapi::env_ = FakeNapiEnv();
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE({
    facade.RequestWindowHost(9401, 0, 640.0, 480.0, 1);
    facade.CreateRegularAbility(9401, 1001, 640.0, 480.0, "title");
    facade.BindEntryAbilityToView(9401, 640.0, 480.0, "title");
    facade.DestroyWindowHost(9401);
    facade.ExitApplication();
    facade.SetWindowSize(9401, 640.0, 480.0);
    facade.SetWindowTitle(9401, "title");
    facade.SetWindowMaximized(9401, true);
    facade.SetWindowMinimized(9401, false);
    facade.SetWindowFullscreen(9401, true);
    facade.SetWindowConstraints(9401, 320.0, 640.0, 240.0, 480.0);
    facade.ActivateWindow(9401);
  });
}

TEST_F(PlatformViewOHOSNapiTest, HybridCalloutsCompleteMarshaling) {
  PlatformViewOHOSNapi::env_ = FakeNapiEnv();
  PlatformViewOHOSNapi facade(nullptr);
  EXPECT_NO_FATAL_FAILURE({
    facade.OnDisplayPlatformViewHybrid(1, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
    facade.OnDisplayOverlayHybrid(1, 1.0, 2.0, 3.0, 4.0);
    facade.OnDisplayMutatorsHybrid(1, {});
    facade.OnDisplayMutatorsHybrid(1, {1.5, -2.5, 0.0});
    facade.HidePlatformViewHybrid(1);
    facade.ShowOverlaySurfaceHybrid();
    facade.HideOverlaySurfaceHybrid();
    facade.OnBeginFrameHybrid();
    facade.OnEndFrameHybrid();
  });
}

TEST_F(PlatformViewOHOSNapiTest, PlatformMessageCalloutsComplete) {
  PlatformViewOHOSNapi::env_ = FakeNapiEnv();
  PlatformViewOHOSNapi facade(nullptr);
  const uint8_t* payload = reinterpret_cast<const uint8_t*>("payload");
  EXPECT_NO_FATAL_FAILURE({
    facade.FlutterViewHandlePlatformMessageResponse(9, nullptr);
    facade.FlutterViewHandlePlatformMessageResponse(
        9, std::make_unique<fml::MallocMapping>(
               fml::MallocMapping::Copy(payload, payload + 7)));
    facade.FlutterViewHandlePlatformMessage(
        7, std::make_unique<PlatformMessage>(
               "unittest/ch",
               fml::MallocMapping::Copy(payload, payload + 7), nullptr));
    facade.FlutterViewHandlePlatformMessage(
        8, std::make_unique<PlatformMessage>("unittest/ch", nullptr));
    facade.FlutterViewOnFirstFrame(true);
    facade.FlutterViewOnFirstFrame(false);
    facade.FlutterViewOnPreEngineRestart();
    facade.FlutterViewSetApplicationLocale("zh-Hans-CN");
  });
}

TEST_F(PlatformViewOHOSNapiTest, InputEventCalloutsCompleteAndNullPacket) {
  PlatformViewOHOSNapi::env_ = FakeNapiEnv();
  PlatformViewOHOSNapi facade(nullptr);
  facade.FlutterViewOnTouchEvent(nullptr, 2);
  facade.FlutterViewOnMouseEvent(nullptr, 2);
  facade.FlutterViewOnAxisEvent(nullptr, 2);
  auto packets = std::shared_ptr<std::string[]>(new std::string[2]);
  packets[0] = "{\"change\":0}";
  packets[1] = "{\"change\":1}";
  EXPECT_NO_FATAL_FAILURE({
    facade.FlutterViewOnTouchEvent(packets, 2);
    facade.FlutterViewOnMouseEvent(packets, 2);
    facade.FlutterViewOnAxisEvent(packets, 2);
    StubNapiFailCallFunction(kStubFailure);
    facade.FlutterViewOnTouchEvent(packets, 2);
    StubNapiFailCallFunction(kStubFailure);
    facade.FlutterViewOnMouseEvent(packets, 2);
    StubNapiFailCallFunction(kStubFailure);
    facade.FlutterViewOnAxisEvent(packets, 2);
  });
}

TEST_F(PlatformViewOHOSNapiTest, CalloutsInvokeJsMethodFailureBranches) {
  PlatformViewOHOSNapi::env_ = FakeNapiEnv();
  PlatformViewOHOSNapi facade(nullptr);
  const std::vector<std::function<void()>> callouts = {
      [&] { facade.RequestWindowHost(1, 0, 1.0, 2.0, 3); },
      [&] { facade.CreateRegularAbility(1, 2, 1.0, 2.0, "t"); },
      [&] { facade.BindEntryAbilityToView(1, 1.0, 2.0, "t"); },
      [&] { facade.DestroyWindowHost(1); },
      [&] { facade.ExitApplication(); },
      [&] { facade.SetWindowSize(1, 1.0, 2.0); },
      [&] { facade.SetWindowTitle(1, "t"); },
      [&] { facade.SetWindowMaximized(1, true); },
      [&] { facade.SetWindowMinimized(1, false); },
      [&] { facade.SetWindowFullscreen(1, true); },
      [&] { facade.SetWindowConstraints(1, 1.0, 2.0, 1.0, 2.0); },
      [&] { facade.ActivateWindow(1); },
      [&] { facade.FlutterViewOnFirstFrame(true); },
      [&] { facade.FlutterViewOnPreEngineRestart(); },
      [&] { facade.FlutterViewSetApplicationLocale("en-US"); },
  };
  for (const auto& call : callouts) {
    StubNapiFailCallFunction(kStubFailure);
    EXPECT_NO_FATAL_FAILURE(call());
  }
}

TEST_F(PlatformViewOHOSNapiTest, DestructorUnrefsLiveReference) {
  PlatformViewOHOSNapi::env_ = FakeNapiEnv();
  EXPECT_NO_FATAL_FAILURE({
    PlatformViewOHOSNapi facade(nullptr);
    facade.ref_napi_obj_ = reinterpret_cast<napi_ref>(0x2);
  });
}

TEST_F(PlatformViewOHOSNapiTest, NativeGetSystemLanguages) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetSystemLanguages(nullptr, nullptr),
            nullptr);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetSystemLanguages(FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_TRUE(PlatformViewOHOSNapi::system_languages.empty());

  StubNapiSetValuetype(napi_string);
  StubNapiSetString("zh-Hans");
  StubNapiSetArrayLength(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetSystemLanguages(FakeNapiEnv(), nullptr),
            nullptr);
  ASSERT_EQ(PlatformViewOHOSNapi::system_languages.size(), 2u);
  EXPECT_EQ(PlatformViewOHOSNapi::system_languages[0], "zh-Hans");
  EXPECT_EQ(PlatformViewOHOSNapi::system_languages[1], "zh-Hans");

  StubNapiFailArrayLength(kStubFailure);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetSystemLanguages(FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::system_languages.size(), 2u);
}

TEST_F(PlatformViewOHOSNapiTest, NativeLoadDartDeferredLibrary) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(nullptr, nullptr),
            nullptr);
  StubNapiFailArrayLength(kStubFailure);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("/nonexistent_unit_test_lib.so");
  StubNapiSetArrayLength(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDeferredComponentInstallFailure) {
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeDeferredComponentInstallFailure(nullptr, nullptr),
      nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeDeferredComponentInstallFailure(FakeNapiEnv(), nullptr),
      nullptr);
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("install failed");
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeDeferredComponentInstallFailure(FakeNapiEnv(), nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeRunBundleAndSnapshotFromLibrary) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRunBundleAndSnapshotFromLibrary(
                nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRunBundleAndSnapshotFromLibrary(
                FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("/data/app");
  StubNapiFailStringUtf8(kStubFailure, 2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRunBundleAndSnapshotFromLibrary(
                FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailStringUtf8(kStubFailure, 4);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRunBundleAndSnapshotFromLibrary(
                FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailArrayLength(kStubFailure);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRunBundleAndSnapshotFromLibrary(
                FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSpawn) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawn(nullptr, nullptr), nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawn(FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("main");
  StubNapiFailStringUtf8(kStubFailure, 2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawn(FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailStringUtf8(kStubFailure, 4);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawn(FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailArrayLength(kStubFailure);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawn(FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSpawnAsync) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawnAsync(nullptr, nullptr), nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawnAsync(FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("main");
  StubNapiFailStringUtf8(kStubFailure, 2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawnAsync(FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailStringUtf8(kStubFailure, 4);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawnAsync(FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailArrayLength(kStubFailure);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawnAsync(FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSpawnAsync(FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDestroyAsync) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDestroyAsync(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDestroyAsync(FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeCleanupMessageData) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeCleanupMessageData(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeCleanupMessageData(FakeNapiEnv(), nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetViewportMetricsNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetViewportMetrics(nullptr, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeUpdateRefreshRateFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);

  StubNapiFailInt32OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateRefreshRate(env, nullptr),
            nullptr);
  StubNapiFailInt32OnCall(0);

  StubNapiSetInt32Value(60);
  auto before = PlatformViewOHOSNapi::all_refresh_rates;
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateRefreshRate(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 60);
  EXPECT_EQ(PlatformViewOHOSNapi::all_refresh_rates->size(),
            before->size());

  StubNapiSetInt32Value(165);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateRefreshRate(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::display_refresh_rate, 165);
  EXPECT_EQ(PlatformViewOHOSNapi::all_refresh_rates->count(165), 1u);
  std::atomic_store(&PlatformViewOHOSNapi::all_refresh_rates, before);
  StubNapiSetInt32Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDisplayUpdatesNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateRefreshRate(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateSize(nullptr, nullptr), nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateDensity(nullptr, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeRegisterTextureParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRegisterTexture(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRegisterTexture(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeUnregisterTextureParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnregisterTexture(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnregisterTexture(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeGetTextureWindowIdParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetTextureWindowId(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetTextureWindowId(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeGetTextureWindowPtrParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetTextureWindowPtr(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetTextureWindowPtr(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetTextureBufferSizeParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(4);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBufferSize(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBufferSize(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailInt32OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBufferSize(env, nullptr),
            nullptr);
  StubNapiFailInt32OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBufferSize(env, nullptr),
            nullptr);
  StubNapiFailInt32OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeNotifyTextureResizingParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(4);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyTextureResizing(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyTextureResizing(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailInt32OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyTextureResizing(env, nullptr),
            nullptr);
  StubNapiFailInt32OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyTextureResizing(env, nullptr),
            nullptr);
  StubNapiFailInt32OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetTextureBackGroundColorParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(3);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetTextureBackGroundColor(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetTextureBackGroundColor(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailUint32OnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetTextureBackGroundColor(env, nullptr),
      nullptr);
  StubNapiFailUint32OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeUpdateSizeFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiSetInt64Value(1080);

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateSize(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateSize(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateSize(env, nullptr), nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::display_width, 1080);
  EXPECT_EQ(PlatformViewOHOSNapi::display_height, 1080);
}

TEST_F(PlatformViewOHOSNapiTest, NativeUpdateDensityFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);
  StubNapiSetDoubleValue(2.75);

  StubNapiFailDoubleOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateDensity(env, nullptr), nullptr);
  StubNapiFailDoubleOnCall(0);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateDensity(env, nullptr), nullptr);
  EXPECT_DOUBLE_EQ(PlatformViewOHOSNapi::display_density_pixels, 2.75);
}

TEST_F(PlatformViewOHOSNapiTest, NativeCheckAndReloadFontParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeCheckAndReloadFont(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextUtilsIsEmojiFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmoji(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);

  StubNapiSetInt64Value(0x1F600);
  StubNapiFailGetBooleanOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmoji(env, nullptr), nullptr);
  StubNapiFailGetBooleanOnCall(0);

  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmoji(env, nullptr));
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextUtilsIsEmojiModifierFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifier(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);

  StubNapiSetInt64Value(0x1F3FB);
  StubNapiFailGetBooleanOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifier(env, nullptr), nullptr);
  StubNapiFailGetBooleanOnCall(0);

  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifier(env, nullptr));
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextUtilsIsEmojiModifierBaseFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifierBase(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);

  StubNapiSetInt64Value(0x1F4AA);
  StubNapiFailGetBooleanOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifierBase(env, nullptr), nullptr);
  StubNapiFailGetBooleanOnCall(0);

  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifierBase(env, nullptr));
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextUtilsIsVariationSelectorFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsVariationSelector(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);

  StubNapiSetInt64Value(0xFE0F);
  StubNapiFailGetBooleanOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsVariationSelector(env, nullptr), nullptr);
  StubNapiFailGetBooleanOnCall(0);

  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsVariationSelector(env, nullptr));
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextUtilsIsRegionalIndicatorFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsRegionalIndicator(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);

  StubNapiSetInt64Value(0x1F1FA);
  StubNapiFailGetBooleanOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsRegionalIndicator(env, nullptr), nullptr);
  StubNapiFailGetBooleanOnCall(0);

  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsRegionalIndicator(env, nullptr));
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetAccessibilityFeaturesParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetAccessibilityFeatures(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetAccessibilityFeatures(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetFontWeightScaleParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetFontWeightScale(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailDoubleOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetFontWeightScale(env, nullptr),
            nullptr);
  StubNapiFailDoubleOnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativePrefetchDefaultFontManagerRuns) {
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativePrefetchDefaultFontManager(FakeNapiEnv(),
                                                             nullptr),
      nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativePrefetchDefaultFontManager(nullptr,
                                                                   nullptr),
      nullptr);
}

// holder arms stay device-only. TODO(device).
TEST_F(PlatformViewOHOSNapiTest, NativeAttachPreHolderLegs) {
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("--enable-checked-mode");
  StubNapiSetArrayLength(1);
  EXPECT_NO_FATAL_FAILURE(OhosMain::NativeInit(FakeNapiEnv(), nullptr));
  StubNapiReset();
  EXPECT_NO_FATAL_FAILURE(OhosMain::Get().GetSettings());
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetViewportMetricsFullChain) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(20);
  StubNapiSetInt64Value(100);
  StubNapiSetDoubleValue(2.0);
  StubNapiFailDoubleOnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetViewportMetrics(env, nullptr),
            nullptr);
  StubNapiFailDoubleOnCall(0);
  StubNapiSetInt64Value(0);
  StubNapiSetDoubleValue(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetFlutterNavigationActionFullChain) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetFlutterNavigationAction(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailBoolOnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetFlutterNavigationAction(env, nullptr),
      nullptr);
  StubNapiFailBoolOnCall(0);
}

static char g_fake_holder_storage[4096];

TEST_F(PlatformViewOHOSNapiTest, NativeEnableFrameCacheFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(g_fake_holder_storage));
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeEnableFrameCache(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeEnableFrameCache(env, nullptr));
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetPipVisibleFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(g_fake_holder_storage));
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetPipVisible(env, nullptr), nullptr);
  StubNapiFailInt64OnCall(0);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeSetPipVisible(env, nullptr));
}

#define NAPO_BRAKE_TAIL2(fn, argc_v, fail_setup, clear_setup)                 \
  do {                                                                        \
    napi_env env_ = FakeNapiEnv();                                            \
    StubNapiSetCbArgc(argc_v);                                                \
    fail_setup;                                                               \
    EXPECT_EQ(fn(env_, nullptr), nullptr);                                    \
    clear_setup;                                                              \
  } while (0)

TEST_F(PlatformViewOHOSNapiTest, NativeA11yStateChangeBrakeTail) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeAccessibilityStateChange(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailBoolOnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeAccessibilityStateChange(env, nullptr),
      nullptr);
  StubNapiFailBoolOnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeA11yOnTapBrakeTail) {
  NAPO_BRAKE_TAIL2(PlatformViewOHOSNapi::nativeAccessibilityOnTap, 2,
                   StubNapiFailInt64OnCall(1), StubNapiFailInt64OnCall(0));
  NAPO_BRAKE_TAIL2(PlatformViewOHOSNapi::nativeAccessibilityOnTap, 2,
                   StubNapiFailInt32OnCall(1), StubNapiFailInt32OnCall(0));
}

TEST_F(PlatformViewOHOSNapiTest, NativeA11yOnLongPressBrakeTail) {
  NAPO_BRAKE_TAIL2(PlatformViewOHOSNapi::nativeAccessibilityOnLongPress, 2,
                   StubNapiFailInt64OnCall(1), StubNapiFailInt64OnCall(0));
  NAPO_BRAKE_TAIL2(PlatformViewOHOSNapi::nativeAccessibilityOnLongPress, 2,
                   StubNapiFailInt32OnCall(1), StubNapiFailInt32OnCall(0));
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetExternalNativeImageBrakeTail) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(3);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetExternalNativeImage(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(3);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetExternalNativeImage(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetQosOnLowMemoryParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetQosOnLowMemory(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetQosOnLowMemory(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetExternalNativeImagePtrStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(3);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetExternalNativeImagePtr(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetExternalNativeImagePtr(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiSetBigintLossless(false);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetExternalNativeImagePtr(env, nullptr),
      nullptr);
  StubNapiSetBigintLossless(true);
}

TEST_F(PlatformViewOHOSNapiTest, NativeResetExternalTextureStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(3);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeResetExternalTexture(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeResetExternalTexture(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailBoolOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeResetExternalTexture(env, nullptr),
            nullptr);
  StubNapiFailBoolOnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeMarkTextureFrameAvailableStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeMarkTextureFrameAvailable(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeMarkTextureFrameAvailable(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDispatchTouchToEngineStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchTouchToEngine(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchTouchToEngine(env, nullptr),
            nullptr);

  StubNapiSetArrayLength(1);
  StubNapiSetInt32Value(static_cast<int32_t>(
      flutter::PointerData::DeviceKind::kMouse));
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(g_fake_holder_storage));
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeDispatchTouchToEngine(env, nullptr));

  StubNapiSetInt32Value(static_cast<int32_t>(
      flutter::PointerData::Change::kDown));
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeDispatchTouchToEngine(env, nullptr));

  StubNapiSetInt32Value(static_cast<int32_t>(
      flutter::PointerData::Change::kUp));
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeDispatchTouchToEngine(env, nullptr));

  StubNapiSetInt32Value(0);
  StubNapiSetInt64Value(0);
  StubNapiSetArrayLength(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeAnimationVotingFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiSetDoubleValue(1.5);

  StubNapiFailInt32OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAnimationVoting(env, nullptr),
            nullptr);
  StubNapiFailInt32OnCall(0);
  StubNapiFailDoubleOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAnimationVoting(env, nullptr),
            nullptr);
  StubNapiFailDoubleOnCall(0);

  StubNapiSetInt32Value(0);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAnimationVoting(env, nullptr),
            nullptr);
  StubNapiSetInt32Value(99);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAnimationVoting(env, nullptr),
            nullptr);
  StubNapiSetInt32Value(0);
  StubNapiSetDoubleValue(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeVideoVotingFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt32OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeVideoVoting(env, nullptr), nullptr);
  StubNapiFailInt32OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeVideoVoting(env, nullptr), nullptr);
  StubNapiFailInt32OnCall(0);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeVideoVoting(env, nullptr), nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeLTPODispatchHighFrameRateFullPaths) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLTPODispatchHighFrameRate(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);

  StubNapiSetInt64Value(0);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLTPODispatchHighFrameRate(env, nullptr),
            nullptr);

  StubNapiSetInt64Value(reinterpret_cast<int64_t>(g_fake_holder_storage));
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLTPODispatchHighFrameRate(env, nullptr),
            nullptr);
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetSemanticsEnabledStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetSemanticsEnabled(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailBoolOnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetSemanticsEnabled(env, nullptr),
            nullptr);
  StubNapiFailBoolOnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSetAnimationStatusStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetAnimationStatus(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiFailInt32OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetAnimationStatus(env, nullptr),
            nullptr);
  StubNapiFailInt32OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeInvokeResponseCallbackStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(4);
  char payload[4] = "abc";
  StubNapiSetArraybufferData(payload, sizeof(payload));

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeInvokePlatformMessageResponseCallback(
                env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeInvokePlatformMessageResponseCallback(
                env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);

  // with a null data pointer. TODO: fix IsArrayBuffer upstream, then add
}

TEST_F(PlatformViewOHOSNapiTest, NativeA11yAnnounceTooltipBrakeTails) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiSetString("tip");
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityAnnounce(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityOnTooltip(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeUnicodePredicatesInt64FailLegs) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsEmoji(env, nullptr), nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsEmojiModifier(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsEmojiModifierBase(env,
                                                                  nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsVariationSelector(env,
                                                                  nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeUnicodeIsRegionalIndicatorSymbol(env,
                                                                  nullptr),
      nullptr);
  StubNapiFailInt64OnCall(0);
  StubNapiSetInt64Value(0x1F600);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsEmoji(env, nullptr), nullptr);
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeNotifyLowMemoryAndCleanupLegs) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(1);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyLowMemoryWarning(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeCleanupMessageData(env, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeLookupCallbackInformationStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeLookupCallbackInformation(env, nullptr));
  StubNapiFailInt64OnCall(0);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeLookupCallbackInformation(env, nullptr));
}

TEST_F(PlatformViewOHOSNapiTest, NativeLoadDartDeferredLibraryStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(3);
  StubNapiSetInt64Value(7);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(env, nullptr),
            nullptr);
  StubNapiSetInt64Value(0);
}

TEST_F(PlatformViewOHOSNapiTest, InitNotifyPageChangedLoaderAllPaths) {
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::InitNotifyPageChangedLoader());
  ASSERT_NE(PlatformViewOHOSNapi::ability_runtime_loader_, nullptr);

  PlatformViewOHOSNapi::notify_page_changed_func_ = nullptr;
  UpdateDlopenForceFail(true);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::InitNotifyPageChangedLoader());
  EXPECT_EQ(PlatformViewOHOSNapi::notify_page_changed_func_, nullptr);
  UpdateDlopenForceFail(false);
}

TEST_F(PlatformViewOHOSNapiTest, NativeNotifyPageChangedLowApiArm) {
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeNotifyPageChanged(FakeNapiEnv(), nullptr));
}

TEST_F(PlatformViewOHOSNapiTest, NativeInvokeEmptyResponseCallbackStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(2);
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::
                nativeInvokePlatformMessageEmptyResponseCallback(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::
                nativeInvokePlatformMessageEmptyResponseCallback(env, nullptr),
            nullptr);
  StubNapiFailInt64OnCall(0);
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextureEntryPointsNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRegisterTexture(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnregisterTexture(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetTextureWindowId(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetTextureWindowPtr(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBufferSize(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyTextureResizing(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetExternalNativeImage(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetExternalNativeImagePtr(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeResetExternalTexture(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeMarkTextureFrameAvailable(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeRegisterPixelMap(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBackGroundPixelMap(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBackGroundColor(nullptr, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeHolderGatedEntryPointsNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeIsHybridCompositionEnabled(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchTouchToEngine(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeEnableFrameCache(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetPipVisible(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyLowMemoryWarning(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeCheckAndReloadFont(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDestroy(nullptr, nullptr), nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetQosOnLowMemory(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetAnimationStatus(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetSemanticsEnabled(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityStateChange(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityAnnounce(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityOnTap(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityOnLongPress(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityOnTooltip(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetFlutterNavigationAction(nullptr, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDispatchPlatformMessage) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchPlatformMessage(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeDispatchPlatformMessage(FakeNapiEnv(), nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDispatchPlatformMessageParseStages) {
  napi_env env = FakeNapiEnv();
  StubNapiSetCbArgc(5);

  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchPlatformMessage(env, nullptr),
            nullptr);

  StubNapiFailInt64OnCall(0);
  StubNapiSetValuetype(napi_number);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchPlatformMessage(env, nullptr),
            nullptr);

  StubNapiSetValuetype(napi_string);
  StubNapiSetString("flutter/ch");
  StubNapiSetArrayLike(false, false, false);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchPlatformMessage(env, nullptr),
            nullptr);

  char payload[4] = "msg";
  StubNapiSetArrayLike(true, false, false);
  StubNapiSetArraybufferData(payload, sizeof(payload));
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchPlatformMessage(env, nullptr),
            nullptr);

  StubNapiFailInt64OnCall(3);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchPlatformMessage(env, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDispatchEmptyPlatformMessageGuards) {
  napi_env env = FakeNapiEnv();
  StubNapiFailInt64OnCall(1);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeDispatchEmptyPlatformMessage(env, nullptr),
      nullptr);
  StubNapiFailInt64OnCall(2);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeDispatchEmptyPlatformMessage(env, nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeDispatchTouchToEngineEmptyPacket) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeDispatchTouchToEngine(FakeNapiEnv(),
                                                              nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, PlatformMessageMarshalingNullEnvErrors) {
  PlatformViewOHOSNapi facade(nullptr);
  const uint8_t* payload = reinterpret_cast<const uint8_t*>("payload");
  EXPECT_NO_FATAL_FAILURE({
    facade.FlutterViewHandlePlatformMessage(
        7, std::make_unique<PlatformMessage>(
               "unittest/ch",
               fml::MallocMapping::Copy(payload, payload + 7), nullptr));
    facade.FlutterViewHandlePlatformMessage(
        8, std::make_unique<PlatformMessage>("unittest/ch", nullptr));
    facade.FlutterViewHandlePlatformMessageResponse(9, nullptr);
    facade.FlutterViewHandlePlatformMessageResponse(
        9, std::make_unique<fml::MallocMapping>(
               fml::MallocMapping::Copy(payload, payload + 7)));
    facade.FlutterViewOnFirstFrame(true);
  });
}

TEST_F(PlatformViewOHOSNapiTest, NativeInvokePlatformMessageResponseCallback) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeInvokePlatformMessageResponseCallback(
                FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiSetValuetype(napi_null);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeInvokePlatformMessageResponseCallback(
                FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeNotifyPageChangedLowApiLevel) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyPageChanged(FakeNapiEnv(),
                                                          nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeWindowingCallbacksArgCount) {
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeComputeWindowPosition(FakeNapiEnv(), nullptr),
      nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeNotifyWindowActivated(FakeNapiEnv(), nullptr),
      nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeHandleOsWindowClosed(FakeNapiEnv(), nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeLookupCallbackInformation) {
  constexpr int64_t kHandle = 0;
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLookupCallbackInformation(
                FakeNapiEnv(), nullptr),
            nullptr);
  DartCallbackCache::cache_[kHandle] = {"utCb", "UtClass", "/ut/lib"};
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLookupCallbackInformation(
                FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailReference(kStubFailure);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLookupCallbackInformation(
                FakeNapiEnv(), nullptr),
            nullptr);
  StubNapiFailCallFunction(kStubFailure);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLookupCallbackInformation(
                FakeNapiEnv(), nullptr),
            nullptr);
  DartCallbackCache::cache_.erase(kHandle);
}

TEST_F(PlatformViewOHOSNapiTest, NativeLookupCallbackInformationBigInt) {
  constexpr int64_t kHandle = 0;
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLookupCallbackInformationBigInt(
                FakeNapiEnv(), nullptr),
            nullptr);
  DartCallbackCache::cache_[kHandle] = {"utCb2", "UtClass2", "/ut/lib2"};
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLookupCallbackInformationBigInt(
                FakeNapiEnv(), nullptr),
            nullptr);
  DartCallbackCache::cache_.erase(kHandle);
}

TEST_F(PlatformViewOHOSNapiTest, NativeFlutterTextUtilsNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmoji(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifier(nullptr, nullptr),
      nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifierBase(
                nullptr, nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeFlutterTextUtilsIsVariationSelector(nullptr, nullptr),
      nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeFlutterTextUtilsIsRegionalIndicator(nullptr, nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeFlutterTextUtilsLiveEnv) {
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmoji(FakeNapiEnv(), nullptr),
      nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifier(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifierBase(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsVariationSelector(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeFlutterTextUtilsIsRegionalIndicator(
                FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeUnicodePredicatesLiveEnv) {
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeUnicodeIsEmoji(FakeNapiEnv(), nullptr),
      nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsEmojiModifier(FakeNapiEnv(),
                                                              nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsEmojiModifierBase(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsVariationSelector(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnicodeIsRegionalIndicatorSymbol(
                FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeEncodeDecodeUtf8) {
  StubNapiSetString("hello");
  EXPECT_EQ(PlatformViewOHOSNapi::nativeEncodeUtf8(FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_NE(PlatformViewOHOSNapi::nativeDecodeUtf8(FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeNoOpEntryPoints) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateOhosAssetManager(FakeNapiEnv(),
                                                              nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetPixelMap(FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeVotingEntryPoints) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAnimationVoting(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeVideoVoting(nullptr, nullptr), nullptr);
  OhosVsyncVotingMgr::ResetInstance();
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAnimationVoting(FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeVideoVoting(FakeNapiEnv(), nullptr),
            nullptr);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(PlatformViewOHOSNapiTest, NativePrefetchFramesCfgDrivesSwitchState) {
  OhosVsyncVotingMgr::ResetInstance();
  ASSERT_EQ(OhosVsyncVotingMgr::GetInstance()->CheckVotingSwitchState(),
            LTPOSwitchState::LTPO_SWITCH_NOT_INIT);
  EXPECT_EQ(PlatformViewOHOSNapi::nativePrefetchFramesCfg(FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_NE(OhosVsyncVotingMgr::GetInstance()->CheckVotingSwitchState(),
            LTPOSwitchState::LTPO_SWITCH_NOT_INIT);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(PlatformViewOHOSNapiTest, NativeCheckLTPOSwitchState) {
  OhosVsyncVotingMgr::ResetInstance();
  EXPECT_EQ(PlatformViewOHOSNapi::nativeCheckLTPOSwitchState(FakeNapiEnv(),
                                                            nullptr),
            nullptr);
  EXPECT_EQ(OhosVsyncVotingMgr::GetInstance()->CheckVotingSwitchState(),
            LTPOSwitchState::LTPO_SWITCH_NOT_INIT);
  OhosVsyncVotingMgr::ResetInstance();
}

TEST_F(PlatformViewOHOSNapiTest, NativeLTPODispatchHighFrameRateZeroHolder) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeLTPODispatchHighFrameRate(
                FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, XComponentAttachUpdateCurrentAndDetach) {
  auto* adapter = XComponentAdapter::GetInstance();
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("ut_napi_xc");
  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentAttachFlutterEngine(
                FakeNapiEnv(), nullptr),
            nullptr);
  auto* base = adapter->GetXcomponentBase("ut_napi_xc");
  ASSERT_NE(base, nullptr);
  EXPECT_TRUE(base->is_engine_attached_);
  EXPECT_EQ(base->shellholderId_, "0");

  EXPECT_EQ(PlatformViewOHOSNapi::nativeUpdateCurrentXComponentId(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_EQ(adapter->current_xcomponent_id_, "ut_napi_xc");

  StubNapiSetString("ut_other_xc");
  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentDispatchMouseWheel(
                FakeNapiEnv(), nullptr),
            nullptr);

  StubNapiSetString("ut_napi_xc");
  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentDetachFlutterEngine(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_FALSE(base->is_engine_attached_);
  EXPECT_EQ(base->shellholderId_, "");

  {
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    adapter->xcomponetMap_.erase("ut_napi_xc");
    delete base;
  }
}

TEST_F(PlatformViewOHOSNapiTest, XComponentPreDrawAndMouseWheelGuards) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentPreDraw(nullptr, nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeXComponentPreDraw(FakeNapiEnv(), nullptr),
      nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentDispatchMouseWheel(
                nullptr, nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeXComponentDispatchMouseWheel(FakeNapiEnv(),
                                                              nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, XComponentMouseWheelFullParseRegistered) {
  auto* adapter = XComponentAdapter::GetInstance();
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("ut_wheel_xc");
  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentAttachFlutterEngine(
                FakeNapiEnv(), nullptr),
            nullptr);
  auto* base = adapter->GetXcomponentBase("ut_wheel_xc");
  ASSERT_NE(base, nullptr);
  EXPECT_TRUE(base->is_engine_attached_);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentDispatchMouseWheel(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_TRUE(base->is_engine_attached_);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentDetachFlutterEngine(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_FALSE(base->is_engine_attached_);
  {
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    adapter->xcomponetMap_.erase("ut_wheel_xc");
    delete base;
  }
}

TEST_F(PlatformViewOHOSNapiTest,
       PlatformMessageCalloutsInvokeJsMethodFailureBranches) {
  PlatformViewOHOSNapi::env_ = FakeNapiEnv();
  PlatformViewOHOSNapi facade(nullptr);
  const uint8_t* payload = reinterpret_cast<const uint8_t*>("payload");
  EXPECT_NO_FATAL_FAILURE({
    StubNapiFailCallFunction(kStubFailure);
    facade.FlutterViewHandlePlatformMessageResponse(9, nullptr);
    StubNapiFailCallFunction(kStubFailure);
    facade.FlutterViewHandlePlatformMessageResponse(
        9, std::make_unique<fml::MallocMapping>(
               fml::MallocMapping::Copy(payload, payload + 7)));
    StubNapiFailCallFunction(kStubFailure);
    facade.FlutterViewHandlePlatformMessage(
        7, std::make_unique<PlatformMessage>(
               "unittest/reach",
               fml::MallocMapping::Copy(payload, payload + 7), nullptr));
    StubNapiFailCallFunction(kStubFailure);
    facade.FlutterViewHandlePlatformMessage(
        8, std::make_unique<PlatformMessage>("unittest/reach", nullptr));
  });
}

TEST_F(PlatformViewOHOSNapiTest,
       MouseWheelEventTypeParseFailureAbortsBeforeDispatch) {
  auto* adapter = XComponentAdapter::GetInstance();
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("ut_reach_xc");
  ASSERT_EQ(PlatformViewOHOSNapi::nativeXComponentAttachFlutterEngine(
                FakeNapiEnv(), nullptr),
            nullptr);
  auto* base = adapter->GetXcomponentBase("ut_reach_xc");
  ASSERT_NE(base, nullptr);
  ASSERT_TRUE(base->is_engine_attached_);

  StubNapiFailStringUtf8(kStubFailure, 2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentDispatchMouseWheel(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_TRUE(base->is_engine_attached_);
  EXPECT_EQ(base->shellholderId_, "0");

  EXPECT_EQ(PlatformViewOHOSNapi::nativeXComponentDetachFlutterEngine(
                FakeNapiEnv(), nullptr),
            nullptr);
  EXPECT_FALSE(base->is_engine_attached_);
  {
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    adapter->xcomponetMap_.erase("ut_reach_xc");
    delete base;
  }
}

TEST_F(PlatformViewOHOSNapiTest, NotifyPageChangedLoaderDirectInit) {
  EXPECT_NO_FATAL_FAILURE(PlatformViewOHOSNapi::InitNotifyPageChangedLoader());
  ASSERT_NE(PlatformViewOHOSNapi::ability_runtime_loader_, nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyPageChanged(FakeNapiEnv(),
                                                          nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, SoftwareRenderingEnabledGateBothSides) {
  StubNapiSetValuetype(napi_string);
  StubNapiSetString("--enable-checked-mode");
  StubNapiSetArrayLength(1);
  EXPECT_NO_FATAL_FAILURE(OhosMain::NativeInit(FakeNapiEnv(), nullptr));
  StubNapiReset();
  EXPECT_FALSE(OhosMain::Get().GetSettings().enable_software_rendering);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetIsSoftwareRenderingEnabled(
                nullptr, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeGetIsSoftwareRenderingEnabled(
                FakeNapiEnv(), nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, HandleOsWindowClosedTearsDownSeededWindow) {
  OHOSWindowController controller(reinterpret_cast<OHOSShellHolder*>(0x9999));
  OHOSWindow::InitParams params{};
  params.view_id = 0;
  params.host_handle = reinterpret_cast<void*>(1);
  controller.windows_.emplace(
      reinterpret_cast<void*>(1),
      std::make_unique<OHOSWindow>(&controller, params,
                                   FlutterWindowCreationRequest{}));
  ASSERT_EQ(controller.windows_.count(reinterpret_cast<void*>(1)), 1u);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeHandleOsWindowClosed(FakeNapiEnv(),
                                                             nullptr),
            nullptr);
  EXPECT_EQ(controller.windows_.count(reinterpret_cast<void*>(1)), 0u);
}

#if !defined(OHOS_X64_UNITTEST)

TEST_F(PlatformViewOHOSNapiTest, NativeDispatchEmptyPlatformMessageFullChain) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());

  StubNapiSetValuetype(napi_string);
  StubNapiSetString("ut_channel");
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(holder.get()));
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeDispatchEmptyPlatformMessage(FakeNapiEnv(),
                                                               nullptr),
      nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeFontHolderTails) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  napi_env env = FakeNapiEnv();
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(holder.get()));
  StubNapiSetDoubleValue(1.5);

  StubNapiSetCbArgc(1);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeCheckAndReloadFont(env, nullptr),
            nullptr);

  StubNapiSetCbArgc(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetFontWeightScale(env, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeSemanticsAndA11yHolderTails) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  napi_env env = FakeNapiEnv();
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(holder.get()));

  StubNapiSetCbArgc(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetSemanticsEnabled(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetAccessibilityFeatures(env, nullptr),
            nullptr);
  EXPECT_EQ(
      PlatformViewOHOSNapi::nativeSetFlutterNavigationAction(env, nullptr),
      nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityStateChange(env, nullptr),
            nullptr);

  StubNapiSetValuetype(napi_string);
  StubNapiSetString("ut_a11y");
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityAnnounce(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityOnTooltip(env, nullptr),
            nullptr);

  StubNapiSetValuetype(napi_undefined);
  StubNapiSetInt32Value(7);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityOnTap(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeAccessibilityOnLongPress(env, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextureRegistryHolderTails) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  napi_env env = FakeNapiEnv();
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(holder.get()));
  StubNapiSetCbArgc(2);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeRegisterTexture(env, nullptr), nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeUnregisterTexture(env, nullptr),
            nullptr);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeGetTextureWindowId(env, nullptr));
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeGetTextureWindowPtr(env, nullptr));
}

TEST_F(PlatformViewOHOSNapiTest, NativeTextureParamsHolderTails) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  napi_env env = FakeNapiEnv();
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(holder.get()));

  StubNapiSetCbArgc(2);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeMarkTextureFrameAvailable(env, nullptr),
            nullptr);
  StubNapiSetCbArgc(3);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeResetExternalTexture(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBackGroundColor(env, nullptr),
            nullptr);
  StubNapiSetCbArgc(4);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetTextureBufferSize(env, nullptr),
            nullptr);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyTextureResizing(env, nullptr),
            nullptr);
}

TEST_F(PlatformViewOHOSNapiTest, NativeMiscHolderTails) {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  auto holder = std::make_unique<OHOSShellHolder>(
      settings, std::make_shared<PlatformViewOHOSNapi>(nullptr), nullptr);
  ASSERT_TRUE(holder->IsValid());
  napi_env env = FakeNapiEnv();
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(holder.get()));
  StubNapiSetCbArgc(1);

  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyLowMemoryWarning(env, nullptr),
            nullptr);
  EXPECT_NO_FATAL_FAILURE(
      PlatformViewOHOSNapi::nativeIsHybridCompositionEnabled(env, nullptr));

  StubNapiSetCbArgc(20);
  StubNapiSetInt64Value(reinterpret_cast<int64_t>(holder.get()));
  StubNapiSetDoubleValue(2.0);
  EXPECT_EQ(PlatformViewOHOSNapi::nativeSetViewportMetrics(env, nullptr),
            nullptr);
}

#endif  // !defined(OHOS_X64_UNITTEST)

}  // namespace testing
}  // namespace flutter
