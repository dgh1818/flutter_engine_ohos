/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#define FML_USED_ON_EMBEDDER

#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include "gtest/gtest.h"

#include "flutter/fml/logging.h"
#include "flutter/fml/message_loop.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_napi_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"

#define private public
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_xcomponent_adapter.h"
#undef private

namespace flutter {
class PlatformViewOHOS;
extern std::map<uint64_t, PlatformViewOHOS*> g_texture_platformview_map;
extern std::recursive_mutex g_map_mutex;
extern std::string OHOSLastFontPath;
extern bool g_isMouseLeftActive;
extern double g_scrollDistance;
namespace testing {
void ResetOhosExternalTextureTestEglState();
void ResetOhosPlatformViewTestNativeImages();
namespace fake_egl {
void ResetFakeEglForProcessRerun();
}  // namespace fake_egl
}  // namespace testing
}  // namespace flutter

namespace {

std::mutex g_gtest_live_mu;
std::string g_gtest_live;

void AppendGtestLive(const std::string& line) {
  std::lock_guard<std::mutex> lock(g_gtest_live_mu);
  g_gtest_live += line;
}

void ResetGtestLive() {
  std::lock_guard<std::mutex> lock(g_gtest_live_mu);
  g_gtest_live.clear();
}

class LiveLogListener : public testing::EmptyTestEventListener {
 public:
  void OnTestProgramStart(const testing::UnitTest& unit_test) override {
    AppendGtestLive("[==========] Running " +
                    std::to_string(unit_test.test_to_run_count()) +
                    " tests from " +
                    std::to_string(unit_test.test_suite_to_run_count()) +
                    " test suites.\n");
  }

  void OnTestStart(const testing::TestInfo& info) override {
    AppendGtestLive(std::string("[ RUN      ] ") + info.test_suite_name() +
                    "." + info.name() + "\n");
  }

  void OnTestEnd(const testing::TestInfo& info) override {
    const char* status =
        info.result()->Failed() ? "[  FAILED  ] " : "[       OK ] ";
    AppendGtestLive(std::string(status) + info.test_suite_name() + "." +
                    info.name() + "\n");
    if (info.result()->Failed()) {
      FML_LOG(ERROR) << "GTEST_FAILED " << info.test_suite_name() << "."
                     << info.name();
      for (int i = 0; i < info.result()->total_part_count(); ++i) {
        const auto& part = info.result()->GetTestPartResult(i);
        if (part.failed()) {
          FML_LOG(ERROR) << "GTEST_FAILED_AT " << part.file_name() << ":"
                         << part.line_number() << " " << part.summary();
        }
      }
    }
  }

  void OnTestProgramEnd(const testing::UnitTest& unit_test) override {
    AppendGtestLive("[  PASSED  ] " +
                    std::to_string(unit_test.successful_test_count()) +
                    " tests.\n");
    if (unit_test.failed_test_count() > 0) {
      AppendGtestLive("[  FAILED  ] " +
                      std::to_string(unit_test.failed_test_count()) +
                      " tests.\n");
    }
  }
};

void EnsureLiveLogListener() {
  static LiveLogListener* listener = nullptr;
  if (listener == nullptr) {
    listener = new LiveLogListener;
    testing::UnitTest::GetInstance()->listeners().Append(listener);
  }
}

void ResetOhosTestProcessState() {
  flutter::XComponentAdapter* adapter =
      flutter::XComponentAdapter::GetInstance();
  if (adapter != nullptr) {
    std::lock_guard<std::recursive_mutex> lock(adapter->xcomponentMap_mutex_);
    for (auto& kv : adapter->xcomponetMap_) {
      delete kv.second;
    }
    adapter->xcomponetMap_.clear();
    std::lock_guard<std::mutex> pend(adapter->hcpp_overlay_pending_mutex_);
    adapter->hcpp_overlay_pending_windows_.clear();
    std::string empty;
    adapter->SetCurrentXcomponentId(empty);
  }

  {
    std::lock_guard<std::recursive_mutex> lock(flutter::g_map_mutex);
    flutter::g_texture_platformview_map.clear();
  }

  flutter::PlatformViewOHOSNapi::env_ = nullptr;
  flutter::PlatformViewOHOSNapi::notify_page_changed_func_ = nullptr;
  flutter::OHOSLastFontPath.clear();
  flutter::g_isMouseLeftActive = false;
  flutter::g_scrollDistance = 0.0;
  StubNapiReset();
  StubArkuiResetActionArguments();
  g_graphic_stub = GraphicStubState{};
  UpdateFstatFunc(nullptr);
  UpdateOpenFunc(nullptr);
  UpdateDlopenForceFail(0);
  flutter::testing::ResetOhosExternalTextureTestEglState();
  flutter::testing::ResetOhosPlatformViewTestNativeImages();
  flutter::testing::fake_egl::ResetFakeEglForProcessRerun();

  if (fml::MessageLoop::IsInitializedForCurrentThread()) {
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  }
}

}  // namespace

extern "C" __attribute__((visibility("default"))) int FlutterOhosCopyLiveLog(
    char* buf,
    int cap) {
  if (buf == nullptr || cap <= 1) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(g_gtest_live_mu);
  const char* data = g_gtest_live.data();
  size_t size = g_gtest_live.size();
  if (size + 1 > static_cast<size_t>(cap)) {
    data += size - (static_cast<size_t>(cap) - 1);
    size = static_cast<size_t>(cap) - 1;
  }
  memcpy(buf, data, size);
  buf[size] = '\0';
  return static_cast<int>(size);
}

// 当编译为共享库 (.so) 时，没有 main() 入口点。
// 此函数作为 C 导出接口，供 NAPI / dlopen 调用。
//
// 用法:
//   extern "C" int FlutterOhosRunAllTests(int argc, char** argv);
//
// 返回值: 0 表示全部通过，非 0 表示有失败
extern "C" __attribute__((visibility("default"))) int FlutterOhosRunAllTests(
    int argc,
    char** argv) {
  ResetOhosTestProcessState();
  ResetGtestLive();
  // .so 通过 dlopen 加载后驻留内存，GTEST_FLAG(filter) 是全局静态变量，
  // 上次设置的值会残留。每次调用时先重置为默认值 "*"（全量执行），
  // 再根据 argv 中的 --gtest_filter 参数覆盖。
  testing::GTEST_FLAG(filter) = "*";
  // 手动解析 --gtest_filter，因为 InitGoogleTest 在 .so 中不解析该参数
  for (int i = 1; i < argc; i++) {
    if (argv[i] && strncmp(argv[i], "--gtest_filter=", 15) == 0) {
      testing::GTEST_FLAG(filter) = argv[i] + 15;
    }
  }
  testing::InitGoogleTest(&argc, argv);
  EnsureLiveLogListener();
  const int exit_code = RUN_ALL_TESTS();
  ResetOhosTestProcessState();
  return exit_code;
}

// ── 覆盖率数据导出封装 ──────────────────────────────────
// __llvm_profile_write_file 是 LLVM profile runtime 的 local 符号，
// stripped .so 删除了 .symtab，dlsym 无法直接查找。
// 这里通过 wrapper 将其导出为全局可见符号，供 dlopen/dlsym 调用。
//
// 用法:
//   extern "C" int FlutterOhosWriteCoverageProfile(const char* filename);
//
// 参数 filename: profraw 输出路径，传 nullptr 则使用 LLVM_PROFILE_FILE 环境变量
// 返回值: 0 成功，非 0 失败
extern "C" __attribute__((visibility("default"))) int
FlutterOhosWriteCoverageProfile(const char* filename) {
  // 声明 LLVM profile runtime 函数（由 libclang_rt.profile.a 提供，仅
  // coverage 构建存在）。弱符号让本 so 在普通（无插桩）构建下也能链接，
  // 此时调用直接返回 -1——测试可正常执行，只是没有覆盖率数据。
  extern int __llvm_profile_write_file(void) __attribute__((weak));
  extern void __llvm_profile_set_filename(const char*) __attribute__((weak));

  if (__llvm_profile_write_file == nullptr ||
      __llvm_profile_set_filename == nullptr) {
    return -1;
  }
  if (filename != nullptr) {
    __llvm_profile_set_filename(filename);
  }
  return __llvm_profile_write_file();
}
