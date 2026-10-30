/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <dlfcn.h>
#include <fcntl.h>
#include <hilog/log.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "napi/native_api.h"

static const char* UNITTEST_TAG = "[unittest_ohos]";

#define LOG_INFO(fmt, ...) \
  OH_LOG_INFO(LOG_APP, 0x0000, UNITTEST_TAG, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
  OH_LOG_ERROR(LOG_APP, 0x0000, UNITTEST_TAG, fmt, ##__VA_ARGS__)

namespace {

constexpr size_t kMaxFilterLength = 4096;

// 测试入口与覆盖率写入,均由 libflutter_ohos_app_test.so 导出。
using RunAllTestsFunc = int (*)(int argc, char** argv);
using WriteCoverageProfileFunc = int (*)(const char* filename);

std::string DirName(const std::string& path) {
  const auto pos = path.rfind('/');
  return pos == std::string::npos ? std::string() : path.substr(0, pos);
}

// 把沙箱路径(/data/storage/el2/base/...)换算成宿主机真实路径
// (/data/app/el2/100/base/<bundle>/...),供显示与 hdc pull 使用。
// 注意:仅供显示参考——真实前缀随设备用户(el2/100 与多用户 el2/10x)与
// 安装形态变化,沙箱内无法推知;覆盖率实际写入路径以 filesDir 参数为准,
// pull 失败时以显示的沙箱路径自行换算。
std::string SandboxToRealPath(const std::string& sandboxPath) {
  constexpr const char* kSandboxPrefix = "/data/storage/el2/base/";
  constexpr const char* kRealPrefix =
      "/data/app/el2/100/base/com.example.unittest_ohos/";
  if (sandboxPath.compare(0, strlen(kSandboxPrefix), kSandboxPrefix) != 0) {
    return sandboxPath;
  }
  return std::string(kRealPrefix) + sandboxPath.substr(strlen(kSandboxPrefix));
}

std::mutex g_live_mu;
std::string g_live_output;
std::atomic<void*> g_test_so_handle{nullptr};

void ResetLiveOutput() {
  std::lock_guard<std::mutex> lock(g_live_mu);
  g_live_output.clear();
}

void AppendLiveOutput(std::string_view chunk) {
  if (chunk.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(g_live_mu);
  g_live_output.append(chunk.data(), chunk.size());
}

std::string CopyLiveOutput() {
  std::lock_guard<std::mutex> lock(g_live_mu);
  return g_live_output;
}

std::string CopyLiveOutputForUi() {
  std::lock_guard<std::mutex> lock(g_live_mu);
  constexpr size_t kMaxUiBytes = 96 * 1024;
  if (g_live_output.size() <= kMaxUiBytes) {
    return g_live_output;
  }
  return "[live tail " + std::to_string(kMaxUiBytes) + " / " +
         std::to_string(g_live_output.size()) + " bytes]\n" +
         g_live_output.substr(g_live_output.size() - kMaxUiBytes);
}

// ── stdout/stderr 捕获(RAII) ─────────────────────────────
class OutputCapture {
 public:
  OutputCapture() {
    memFd_ = static_cast<int>(syscall(SYS_memfd_create, "gtest_output", 0));
    if (memFd_ < 0) {
      LOG_ERROR("memfd_create failed: %{public}s", strerror(errno));
      return;
    }
    origStdout_ = dup(STDOUT_FILENO);
    origStderr_ = dup(STDERR_FILENO);
    if (origStdout_ < 0 || origStderr_ < 0) {
      LOG_ERROR("dup stdout/stderr failed: %{public}s", strerror(errno));
      return;
    }
    dup2(memFd_, STDOUT_FILENO);
    dup2(memFd_, STDERR_FILENO);
  }

  ~OutputCapture() {
    if (origStdout_ >= 0) {
      fflush(stdout);
      dup2(origStdout_, STDOUT_FILENO);
      close(origStdout_);
    }
    if (origStderr_ >= 0) {
      fflush(stderr);
      dup2(origStderr_, STDERR_FILENO);
      close(origStderr_);
    }
    if (memFd_ >= 0) {
      close(memFd_);
    }
  }

  bool IsValid() const {
    return memFd_ >= 0 && origStdout_ >= 0 && origStderr_ >= 0;
  }

  std::string ReadBack() {
    fflush(stdout);
    fflush(stderr);
    std::string content;
    lseek(memFd_, 0, SEEK_SET);
    char buf[4096];
    ssize_t n;
    while ((n = read(memFd_, buf, sizeof(buf))) > 0) {
      content.append(buf, static_cast<size_t>(n));
    }
    return content;
  }

 private:
  int memFd_ = -1;
  int origStdout_ = -1;
  int origStderr_ = -1;
};

// ── 测试 so 加载 ─────────────────────────────────────────
// 注意:析构故意不 dlclose。测试执行中引擎代码会在 worker 线程注册
// TLS 析构函数(如 fml::MessageLoop 的 thread_local),这些析构代码位于
// so 内;worker 线程被线程池回收退出时会执行它们,若 so 已被卸载,
// 析构函数即成野指针,导致"测试跑完几秒后 App 自行退出"。
// so 驻留进程,由系统在 App 退出时统一回收。
class TestLibrary {
 public:
  explicit TestLibrary(const std::string& path) {
    // RTLD_GLOBAL:profile/release(AOT 运行时)下引擎靠
    // dlsym(RTLD_DEFAULT) 兜底解析 Dart snapshot 符号,LOCAL 加载时
    // 找不到会导致 DartVM 创建失败(空指针崩溃)。配套约束:测试 so
    // 以 version script 只导出入口函数与 snapshot 符号(BUILD.gn 的
    // app_test_export.map),测试桩不会进入全局命名空间污染引擎的
    // dlsym 解析(debug 下曾致 hiappevent 用例失败)。
    handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (handle_ != nullptr) {
      g_test_so_handle.store(handle_, std::memory_order_release);
    }
    if (handle_ == nullptr) {
      const char* err = dlerror();
      error_ = (err != nullptr) ? err : "dlopen failed";
      return;
    }
    dlerror();
    run_ = reinterpret_cast<RunAllTestsFunc>(
        dlsym(handle_, "FlutterOhosRunAllTests"));
    const char* dlsymErr = dlerror();
    if (dlsymErr != nullptr || run_ == nullptr) {
      error_ = std::string("dlsym FlutterOhosRunAllTests failed: ") +
               (dlsymErr ? dlsymErr : "null");
      return;
    }
    writeCoverage_ = reinterpret_cast<WriteCoverageProfileFunc>(
        dlsym(handle_, "FlutterOhosWriteCoverageProfile"));
  }

  ~TestLibrary() {
    // 故意不 dlclose,见类注释。
  }

  bool IsValid() const { return run_ != nullptr; }
  const std::string& Error() const { return error_; }
  RunAllTestsFunc RunFunc() const { return run_; }
  WriteCoverageProfileFunc CoverageFunc() const { return writeCoverage_; }

 private:
  void* handle_ = nullptr;
  RunAllTestsFunc run_ = nullptr;
  WriteCoverageProfileFunc writeCoverage_ = nullptr;
  std::string error_;
};

// ── 测试执行结果(纯数据,worker 线程产出,主线程转成 JS 对象) ──
struct RunResult {
  int exitCode = -1;
  std::string output;
  std::string profrawPath;
  size_t profrawSize = 0;
};

std::string BuildProfrawPath(const std::string& filesDir) {
  return filesDir + "/coverage.profraw";
}

// 在 worker 线程完成全部工作:dlopen → 执行测试 → 写覆盖率 → 收集输出。
RunResult RunTestsWorker(const std::string& soPath,
                         const std::string& filesDir,
                         const std::string& gtestFilter) {
  LOG_INFO(
      "RunTestsWorker: soPath=%{public}s filesDir=%{public}s filter=%{public}s",
      soPath.c_str(), filesDir.c_str(), gtestFilter.c_str());

  ResetLiveOutput();
  AppendLiveOutput("Loading .so: " + soPath + "\nfilesDir: " + filesDir + "\n");
  if (!gtestFilter.empty()) {
    AppendLiveOutput("gtest_filter: " + gtestFilter + "\n");
  }

  TestLibrary library(soPath);
  if (!library.IsValid()) {
    LOG_ERROR("load test library failed: %{public}s", library.Error().c_str());
    // -2 表示原生侧基建失败(dlopen/dlsym/捕获器),区别于 -1 的"未测试"。
    AppendLiveOutput(library.Error() + "\n");
    return RunResult{-2, CopyLiveOutput(), "", 0};
  }
  AppendLiveOutput("dlopen success.\n");

  // 覆盖率数据准备:先删旧文件;真实路径仅供显示和 hdc pull。
  std::string coverageLog;
  std::string profrawPath;
  const std::string sandboxProfraw = BuildProfrawPath(filesDir);
  if (library.CoverageFunc() != nullptr && !filesDir.empty()) {
    unlink(sandboxProfraw.c_str());
    profrawPath = SandboxToRealPath(sandboxProfraw);
    coverageLog = "[coverage] profraw target: " + sandboxProfraw +
                  " (real: " + profrawPath + ")\n";
  } else {
    coverageLog =
        "[coverage] WARNING: coverage unavailable (no export or empty "
        "filesDir)\n";
  }
  AppendLiveOutput(coverageLog);

  int exitCode = -2;
  {
    OutputCapture capture;
    if (!capture.IsValid()) {
      AppendLiveOutput("failed to create output capture\n");
      return RunResult{-2, CopyLiveOutput(), profrawPath, 0};
    }

    char filterArg[kMaxFilterLength + 32] = {0};
    if (!gtestFilter.empty()) {
      snprintf(filterArg, sizeof(filterArg), "--gtest_filter=%s",
               gtestFilter.c_str());
    }
    char* argv[] = {const_cast<char*>("flutter_ohos_unittests"), filterArg,
                    nullptr};
    const int argc = gtestFilter.empty() ? 1 : 2;

    LOG_INFO("Running tests: argc=%{public}d", argc);
    exitCode = library.RunFunc()(argc, argv);
    LOG_INFO("Tests exited with code: %{public}d", exitCode);
    AppendLiveOutput(capture.ReadBack());
  }

  size_t profrawSize = 0;
  if (library.CoverageFunc() != nullptr && !filesDir.empty()) {
    // App 不能 exit(),atexit 钩子不会触发,覆盖率必须手动写盘。
    const int writeResult = library.CoverageFunc()(sandboxProfraw.c_str());
    struct stat st = {};
    if (stat(sandboxProfraw.c_str(), &st) == 0) {
      profrawSize = static_cast<size_t>(st.st_size);
    }
    LOG_INFO("coverage write returned %{public}d profraw size %{public}zu",
             writeResult, profrawSize);
  }

  AppendLiveOutput("\n--- Tests exited with code: " + std::to_string(exitCode) +
                   " ---\n");
  std::string output = CopyLiveOutput();
  // 输出上限:MB 级输出会在 JS 侧多份拷贝并让 Text 一次性布局,卡主线程。
  // 头 4K(环境信息)+ 尾部(失败详情与总结)保留,中间截断。
  constexpr size_t kMaxOutputBytes = 512 * 1024;
  if (output.size() > kMaxOutputBytes) {
    constexpr size_t kHeadBytes = 4 * 1024;
    const std::string head = output.substr(0, kHeadBytes);
    const std::string tail =
        output.substr(output.size() - (kMaxOutputBytes - kHeadBytes));
    output = "[output truncated: total " + std::to_string(output.size()) +
             " bytes, head 4K + tail retained]\n" + head + "\n......\n" + tail;
  }
  if (!filesDir.empty()) {
    const std::string dump_path = filesDir + "/gtest_output.txt";
    FILE* dump = fopen(dump_path.c_str(), "w");
    if (dump != nullptr) {
      fwrite(output.data(), 1, output.size(), dump);
      fclose(dump);
    }
  }
  return RunResult{exitCode, output, profrawPath, profrawSize};
}

// ── napi 异步壳 ──────────────────────────────────────────
// 测试一跑就是十几秒,同步调用会把 ArkTS 主线程卡死并触发系统的
// THREAD_BLOCK_3S watchdog,因此用 napi_async_work 在 worker 线程执行,
// 主线程立即返回 Promise。
struct AsyncRunContext {
  std::string soPath;
  std::string filesDir;
  std::string filter;
  napi_deferred deferred = nullptr;
  napi_async_work work = nullptr;
  RunResult result;
};

napi_value MakeResultObject(napi_env env, const RunResult& result) {
  napi_value object = nullptr;
  napi_create_object(env, &object);

  napi_value exitCode = nullptr;
  napi_create_int32(env, result.exitCode, &exitCode);
  napi_set_named_property(env, object, "exitCode", exitCode);

  napi_value output = nullptr;
  napi_create_string_utf8(env, result.output.c_str(), result.output.size(),
                          &output);
  napi_set_named_property(env, object, "output", output);

  napi_value profrawPath = nullptr;
  napi_create_string_utf8(env, result.profrawPath.c_str(),
                          result.profrawPath.size(), &profrawPath);
  napi_set_named_property(env, object, "profrawPath", profrawPath);

  napi_value profrawSize = nullptr;
  napi_create_int64(env, static_cast<int64_t>(result.profrawSize),
                    &profrawSize);
  napi_set_named_property(env, object, "profrawSize", profrawSize);
  return object;
}

std::string GetStringArg(napi_env env, napi_value value, size_t maxLength) {
  if (value == nullptr) {
    return {};
  }
  size_t length = 0;
  if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
    return {};
  }
  if (length >= maxLength) {
    length = maxLength - 1;
  }
  std::string result(length, '\0');
  size_t copied = 0;
  if (napi_get_value_string_utf8(env, value, &result[0], length + 1, &copied) !=
      napi_ok) {
    return {};
  }
  result.resize(copied);
  return result;
}

std::atomic<bool> g_tests_running{false};

void RunTestsExecute(napi_env env, void* data) {
  auto* context = static_cast<AsyncRunContext*>(data);
  bool expected = false;
  if (!g_tests_running.compare_exchange_strong(expected, true)) {
    context->result =
        RunResult{-3, "tests already running in this process\n", "", 0};
    return;
  }
  try {
    std::thread worker([&] {
      context->result =
          RunTestsWorker(context->soPath, context->filesDir, context->filter);
    });
    worker.join();
  } catch (...) {
    context->result = RunResult{-2, "failed to start test thread\n", "", 0};
  }
  g_tests_running.store(false);
}

void RunTestsComplete(napi_env env, napi_status status, void* data) {
  auto* context = static_cast<AsyncRunContext*>(data);
  if (status == napi_closing) {
    // 环境正在销毁,env 已不可使用(再用会原生崩溃),只能回收自身
    // 内存;work 由框架 teardown 处理。
    delete context;
    return;
  }
  napi_value result = MakeResultObject(env, context->result);
  if (status == napi_ok) {
    napi_resolve_deferred(env, context->deferred, result);
  } else {
    napi_reject_deferred(env, context->deferred, result);
  }
  napi_delete_async_work(env, context->work);
  delete context;
}

napi_value RunTestsSo(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  auto* context = new AsyncRunContext{
      GetStringArg(env, args[0], 4096),
      GetStringArg(env, args[1], 4096),
      argc >= 3 && args[2] != nullptr
          ? GetStringArg(env, args[2], kMaxFilterLength)
          : std::string(),
      nullptr,
      nullptr,
      RunResult{},
  };
  napi_value promise = nullptr;
  if (napi_create_promise(env, &context->deferred, &promise) != napi_ok ||
      promise == nullptr) {
    delete context;
    return nullptr;
  }
  napi_value resourceName = nullptr;
  napi_create_string_utf8(env, "runTestsSo", NAPI_AUTO_LENGTH, &resourceName);
  // 以下任一失败都必须 reject 掉已创建的 Promise 并清理,否则
  // AsyncRunContext 泄漏、JS 侧 await 永久 pending(loading 无限旋转)。
  if (napi_create_async_work(env, nullptr, resourceName, RunTestsExecute,
                             RunTestsComplete, context,
                             &context->work) != napi_ok ||
      context->work == nullptr) {
    napi_value err = nullptr;
    napi_create_string_utf8(env, "napi_create_async_work failed",
                            NAPI_AUTO_LENGTH, &err);
    napi_reject_deferred(env, context->deferred, err);
    delete context;
    return promise;
  }
  if (napi_queue_async_work(env, context->work) != napi_ok) {
    napi_value err = nullptr;
    napi_create_string_utf8(env, "napi_queue_async_work failed",
                            NAPI_AUTO_LENGTH, &err);
    napi_reject_deferred(env, context->deferred, err);
    napi_delete_async_work(env, context->work);
    delete context;
    return promise;
  }
  return promise;
}

napi_value CheckBinary(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  const std::string path = GetStringArg(env, args[0], 4096);

  napi_value object = nullptr;
  napi_create_object(env, &object);
  napi_value exists = nullptr;
  napi_get_boolean(env, access(path.c_str(), F_OK) == 0, &exists);
  napi_set_named_property(env, object, "exists", exists);
  return object;
}

napi_value GetLiveOutput(napi_env env, napi_callback_info /*info*/) {
  std::string text = CopyLiveOutputForUi();
  void* test_so = g_test_so_handle.load(std::memory_order_acquire);
  if (test_so != nullptr) {
    using CopyLiveFn = int (*)(char*, int);
    auto copy_live =
        reinterpret_cast<CopyLiveFn>(dlsym(test_so, "FlutterOhosCopyLiveLog"));
    if (copy_live != nullptr) {
      std::vector<char> buf(96 * 1024);
      const int n = copy_live(buf.data(), static_cast<int>(buf.size()));
      if (n > 0) {
        text.append(buf.data(), static_cast<size_t>(n));
      }
    }
  }
  napi_value result = nullptr;
  napi_create_string_utf8(env, text.c_str(), text.size(), &result);
  return result;
}

napi_value GetSandboxPath(napi_env env, napi_callback_info callbackInfo) {
  // dladdr 直接取本模块(libentry.so)的加载路径,免去解析 /proc/self/maps。
  Dl_info info = {};
  dladdr(reinterpret_cast<void*>(&GetSandboxPath), &info);
  const std::string soPath = info.dli_fname != nullptr ? info.dli_fname : "";
  // .../libs/arm64/libentry.so → .../libs/arm64 → .../libs → 沙箱根目录
  const std::string sandboxRoot = DirName(DirName(DirName(soPath)));

  napi_value object = nullptr;
  napi_create_object(env, &object);
  auto setString = [env, object](const char* name, const std::string& value) {
    napi_value v = nullptr;
    napi_create_string_utf8(env, value.c_str(), value.size(), &v);
    napi_set_named_property(env, object, name, v);
  };
  setString("sandboxPath", sandboxRoot);
  setString("soPath", soPath);

  char cwd[4096] = {0};
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    setString("cwd", cwd);
  }
  return object;
}

}  // namespace

EXTERN_C_START
napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"checkBinary", nullptr, CheckBinary, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"getSandboxPath", nullptr, GetSandboxPath, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"runTestsSo", nullptr, RunTestsSo, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"getLiveOutput", nullptr, GetLiveOutput, nullptr, nullptr, nullptr,
       napi_default, nullptr},
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  return exports;
}
EXTERN_C_END

static napi_module unittestOhosModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) {
  napi_module_register(&unittestOhosModule);
}
