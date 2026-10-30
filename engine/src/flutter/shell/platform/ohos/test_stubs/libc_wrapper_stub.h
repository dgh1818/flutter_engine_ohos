/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_TEST_STUBS_LIBC_WRAPPER_STUB_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_TEST_STUBS_LIBC_WRAPPER_STUB_H_

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>

extern "C" {
using OpenFunc = int (*)(const char* path, int flags);
using FstatFunc = int (*)(int fd, struct stat* st);

void UpdateOpenFunc(OpenFunc func);
void UpdateFstatFunc(FstatFunc func);
int __real_fstat(int fd, struct stat* st);
void UpdateDlopenForceFail(int force_fail);
}

// 测试侧临时目录约定:App 进程内走 NDK ApplicationContext 接口取应用
// 临时目录,root shell 环境(NDK 上下文不可用)回退
// /data/local/tmp。测试基建需要临时文件时一律用它,不要硬编码路径。
const char* GetUtTmpDir();

// dlopen 故障注入(表驱动):按库名子串把 dlopen 重定向为受控行为,用于测
// 试引擎对"系统库加载失败/符号缺失"的降级逻辑。
//   kFailOpen  → dlopen 返回 nullptr(模拟库加载失败)
//   kWrongLib  → 返回 libc.so 的句柄(模拟库在但符号缺失)
// 注入发生时计数 +1,测试用 GetAndResetDlopenRedirectCount() 断言注入生效。
enum class DlopenRedirectMode { kPassthrough, kFailOpen, kWrongLib };
void SetDlopenRedirect(const char* lib_substring, DlopenRedirectMode mode);
int GetAndResetDlopenRedirectCount();

// RAII:构造时注入,析构时自动恢复 passthrough。
class ScopedDlopenRedirect {
 public:
  ScopedDlopenRedirect(const char* lib_substring, DlopenRedirectMode mode)
      : lib_(lib_substring) {
    SetDlopenRedirect(lib_, mode);
  }
  ~ScopedDlopenRedirect() {
    SetDlopenRedirect(lib_, DlopenRedirectMode::kPassthrough);
  }
  ScopedDlopenRedirect(const ScopedDlopenRedirect&) = delete;
  ScopedDlopenRedirect& operator=(const ScopedDlopenRedirect&) = delete;

 private:
  const char* lib_;
};

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_TEST_STUBS_LIBC_WRAPPER_STUB_H_
