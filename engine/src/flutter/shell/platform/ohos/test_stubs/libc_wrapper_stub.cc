/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Link-time wrappers (-Wl,--wrap=..., set in BUILD.gn) for libc syscalls.
// Tests inject behavior via the Update* / SetDlopenRedirect functions;
// without an injection every call passes through to the real function, so
// non-injecting tests keep real behavior. Same pattern as
// base/startup/init test/mock/libs func_wrapper.cpp.

#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
struct DlopenRedirectEntry {
  const char* substring;
  DlopenRedirectMode mode;
};
DlopenRedirectEntry g_dlopen_redirects[4] = {};
}  // namespace

int g_dlopen_redirect_count = 0;

const char* GetUtTmpDir() {
  // App 进程内:NDK ApplicationContext 接口取应用临时目录。
  static const std::string app_temp_dir = []() -> std::string {
    constexpr auto kSym = "OH_AbilityRuntime_ApplicationContextGetTempDir";
    void* lib = dlopen("libability_runtime.so", RTLD_LAZY | RTLD_LOCAL);
    if (lib == nullptr) {
      return {};
    }
    using GetTempDirFn = int (*)(char*, const int32_t, int32_t*);
    auto get_temp_dir = reinterpret_cast<GetTempDirFn>(dlsym(lib, kSym));
    if (get_temp_dir == nullptr) {
      return {};
    }
    char buffer[PATH_MAX] = {0};
    int32_t written = 0;
    // ABILITY_RUNTIME_ERROR_CODE_NO_ERROR == 0。
    if (get_temp_dir(buffer, sizeof(buffer), &written) != 0 || written <= 0 ||
        written >= static_cast<int32_t>(sizeof(buffer))) {
      return {};
    }
    return std::string(buffer, written);
  }();
  if (!app_temp_dir.empty()) {
    return app_temp_dir.c_str();
  }
  return "/data/local/tmp";
}

extern "C" {

int __real_open(const char* path, int flags, ...);
int __real_fstat(int fd, struct stat* st);
void* __real_dlopen(const char* filename, int flags);

// ---- open ----
static OpenFunc g_open = nullptr;

void UpdateOpenFunc(OpenFunc func) {
  g_open = func;
}

int __wrap_open(const char* path, int flags, ...) {
  if (g_open) {
    return g_open(path, flags);
  }
  mode_t mode = 0;
  // mode is only passed (and only read) when O_CREAT is set; reading it
  // unconditionally is UB (C11 7.16.1.1).
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = static_cast<mode_t>(va_arg(args, int));
    va_end(args);
  }
  return __real_open(path, flags, mode);
}

// ---- fstat ----
static FstatFunc g_fstat = nullptr;

void UpdateFstatFunc(FstatFunc func) {
  g_fstat = func;
}

int __wrap_fstat(int fd, struct stat* st) {
  if (g_fstat) {
    return g_fstat(fd, st);
  }
  return __real_fstat(fd, st);
}

static bool g_dlopen_force_fail = false;

void UpdateDlopenForceFail(int force_fail) {
  g_dlopen_force_fail = force_fail;
}

void* __wrap_dlopen(const char* filename, int flags) {
  if (g_dlopen_force_fail) {
    __real_dlopen("libflutter_ut_no_such_lib.so", flags);
    return nullptr;
  }
  if (filename != nullptr) {
    for (const auto& e : g_dlopen_redirects) {
      if (e.substring == nullptr ||
          e.mode == DlopenRedirectMode::kPassthrough) {
        continue;
      }
      if (strstr(filename, e.substring) != nullptr) {
        g_dlopen_redirect_count++;
        if (e.mode == DlopenRedirectMode::kFailOpen) {
          // 先制造一次真实的动态库错误,让调用侧随后的 dlerror() 拿到非空
          // 描述:部分生产代码(如 DynamicLibraryLoader)把 dlerror() 的
          // 返回值直接送 ostream,若此刻无错误记录,dlerror() 返回 nullptr
          // 会在 strlen(nullptr) 处崩溃。
          __real_dlopen("libflutter_ut_redirect_failopen.so", flags);
          return nullptr;
        }
        return __real_dlopen("libc.so", flags);
      }
    }
  }
  return __real_dlopen(filename, flags);
}

}  // extern "C"

void SetDlopenRedirect(const char* lib_substring, DlopenRedirectMode mode) {
  // 已有条目:更新或清除
  for (auto& e : g_dlopen_redirects) {
    if (e.substring != nullptr && strcmp(e.substring, lib_substring) == 0) {
      if (mode == DlopenRedirectMode::kPassthrough) {
        e.substring = nullptr;
      }
      e.mode = mode;
      return;
    }
  }
  if (mode == DlopenRedirectMode::kPassthrough) {
    return;
  }
  // 空槽写入
  for (auto& e : g_dlopen_redirects) {
    if (e.substring == nullptr) {
      e.substring = lib_substring;
      e.mode = mode;
      return;
    }
  }
}

int GetAndResetDlopenRedirectCount() {
  int count = g_dlopen_redirect_count;
  g_dlopen_redirect_count = 0;
  return count;
}
