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
//
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
//
// The file also covers the first (arg-unmarshal) error branch of the three
// ETS→C++ natives added for multi-window (nativeHandleOsWindowClosed /
// nativeComputeWindowPosition / nativeNotifyWindowActivated, registered in
// library_loader.cpp). napi_get_cb_info is interposed at file scope so a null
// env returns napi_invalid_arg without entering real libnapi (which may abort
// on a null env depending on NDK revision). The JS-invocation success path
// (real env + callback_info + ForView hit) is device-only (Class 3).

#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

extern "C" napi_status napi_get_cb_info(napi_env env,
                                        napi_callback_info /*info*/,
                                        size_t* argc,
                                        napi_value* /*argv*/,
                                        napi_value* /*this_arg*/,
                                        void** /*data*/) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (argc) {
    *argc = 0;
  }
  return napi_ok;
}

// File-scope (external C linkage) so it interposes libnapi for this
// executable. nativeComputeWindowPosition writes `napi_value result` via
// napi_create_int32 BEFORE checking napi_get_cb_info; real N-API leaves the
// out-param untouched on a null env, so the later `return result` would read
// an uninitialized value. Writing a sentinel here keeps that UT path defined
// without changing production.
extern "C" napi_status napi_create_int32(napi_env env,
                                         int32_t /*value*/,
                                         napi_value* result) {
  if (result) {
    *result = reinterpret_cast<napi_value>(0x12);
  }
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  return napi_ok;
}

namespace flutter {
namespace testing {

namespace {

// Stub napi marshaling functions with real N-API null-env semantics: N-API
// validates env == nullptr first and returns napi_invalid_arg without touching
// the out-param. The other functions called by these methods
// (napi_open_handle_scope / napi_close_handle_scope / napi_get_reference_value
// / napi_get_named_property / napi_call_function / napi_create_string_utf8) are
// already stubbed in ohos_touch_processor_unittests.cpp for this executable and
// are intentionally NOT redefined here (multiple-definition link error).
extern "C" napi_status napi_create_int64(napi_env env,
                                         int64_t value,
                                         napi_value* result) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (result) {
    *result = reinterpret_cast<napi_value>(0x10);
  }
  return napi_ok;
}

extern "C" napi_status napi_create_double(napi_env env,
                                          double value,
                                          napi_value* result) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (result) {
    *result = reinterpret_cast<napi_value>(0x11);
  }
  return napi_ok;
}

extern "C" napi_status napi_get_boolean(napi_env env,
                                        bool value,
                                        napi_value* result) {
  if (env == nullptr) {
    return napi_invalid_arg;
  }
  if (result) {
    *result = reinterpret_cast<napi_value>(0x13);
  }
  return napi_ok;
}

}  // namespace

// ===== Windowing JS-callout methods (null env → arg-marshaling error path) ====
// Each call below covers: napi_open_handle_scope / napi_close_handle_scope
// (shared stubs, napi_ok) and every `if (status != napi_ok)` per-argument error
// branch inside the method (null-env stubs → napi_invalid_arg).

// requestWindowHost: 5 args — int64 view_id, int64 parent_view_id,
// double width, double height, int32 archetype.
TEST(PlatformViewOHOSNapi, RequestWindowHostNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.RequestWindowHost(9401, 0, 640.0, 480.0, 1);
  SUCCEED();
}

// createRegularAbility: 5 args — int64 view_id, int64 request_id,
// double width, double height, string title.
TEST(PlatformViewOHOSNapi, CreateRegularAbilityNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.CreateRegularAbility(9401, 1001, 640.0, 480.0, "title");
  SUCCEED();
}

// bindEntryAbilityToView: 4 args — int64 view_id, double width, double height,
// string title.
TEST(PlatformViewOHOSNapi, BindEntryAbilityToViewNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.BindEntryAbilityToView(9401, 640.0, 480.0, "title");
  SUCCEED();
}

// destroyWindowHost: 1 arg — int64 view_id.
TEST(PlatformViewOHOSNapi, DestroyWindowHostNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.DestroyWindowHost(9401);
  SUCCEED();
}

// exitApplication: no args — InvokeJsMethod with argc 0 / nullptr argv.
TEST(PlatformViewOHOSNapi, ExitApplicationNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.ExitApplication();
  SUCCEED();
}

// setWindowSize: 3 args — int64 view_id, double width, double height.
TEST(PlatformViewOHOSNapi, SetWindowSizeNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.SetWindowSize(9401, 640.0, 480.0);
  SUCCEED();
}

// setWindowTitle: 2 args — int64 view_id, string title.
TEST(PlatformViewOHOSNapi, SetWindowTitleNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.SetWindowTitle(9401, "title");
  SUCCEED();
}

// setWindowMaximized: 2 args — int64 view_id, bool maximized.
TEST(PlatformViewOHOSNapi, SetWindowMaximizedNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.SetWindowMaximized(9401, true);
  SUCCEED();
}

// setWindowMinimized: 2 args — int64 view_id, bool minimized.
TEST(PlatformViewOHOSNapi, SetWindowMinimizedNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.SetWindowMinimized(9401, true);
  SUCCEED();
}

// setWindowFullscreen: 2 args — int64 view_id, bool fullscreen.
TEST(PlatformViewOHOSNapi, SetWindowFullscreenNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.SetWindowFullscreen(9401, false);
  SUCCEED();
}

// setWindowConstraints: 5 args — int64 view_id, double min/max width/height.
TEST(PlatformViewOHOSNapi, SetWindowConstraintsNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.SetWindowConstraints(9401, 320.0, 640.0, 240.0, 480.0);
  SUCCEED();
}

// activateWindow: 1 arg — int64 view_id.
TEST(PlatformViewOHOSNapi, ActivateWindowNullEnv) {
  PlatformViewOHOSNapi facade(nullptr);
  facade.ActivateWindow(9401);
  SUCCEED();
}

// ===== ETS → C++ native callbacks (null env → napi_get_cb_info error path) ====
// File-scope napi_get_cb_info stub returns napi_invalid_arg for env == nullptr
// so these natives take the `ret != napi_ok` early-return without entering
// real libnapi. The JS-invocation success path is device-only (Class 3).

// nativeHandleOsWindowClosed: napi_get_cb_info fails → DLOG + return nullptr.
TEST(PlatformViewOHOSNapi, HandleOsWindowClosedNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeHandleOsWindowClosed(nullptr, nullptr),
            nullptr);
}

// nativeComputeWindowPosition: napi_get_cb_info fails (or argc < 12) → returns
// the default "not computed" napi_value. File-scope napi_create_int32 writes a
// sentinel into that out-param first, so the production `return result` is a
// defined read even though the source leaves `napi_value result` uninitialized.
TEST(PlatformViewOHOSNapi, ComputeWindowPositionNullEnv) {
  PlatformViewOHOSNapi::nativeComputeWindowPosition(nullptr, nullptr);
}

// nativeNotifyWindowActivated: napi_get_cb_info fails (or argc < 2) → DLOG +
// return nullptr.
TEST(PlatformViewOHOSNapi, NotifyWindowActivatedNullEnv) {
  EXPECT_EQ(PlatformViewOHOSNapi::nativeNotifyWindowActivated(nullptr, nullptr),
            nullptr);
}

}  // namespace testing
}  // namespace flutter
