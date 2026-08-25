/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// Link-time wrappers (-Wl,--wrap=..., set in BUILD.gn) for libc syscalls the
// x64 emulator blocks for hdc shell. Tests inject behavior via the Update*
// functions; without an injection every call passes through to the real
// function, so non-injecting tests keep real behavior. Same pattern as
// base/startup/init test/mock/libs func_wrapper.cpp.

#include "flutter/shell/platform/ohos/test_stubs/unittest_x64/libc_wrapper_stub.h"

#include <fcntl.h>

#include <stdarg.h>

extern "C" {

// Provided by the linker via -Wl,--wrap=open / -Wl,--wrap=fstat.
int __real_open(const char* path, int flags, ...);
int __real_fstat(int fd, struct stat* st);

// ---- open ----
static OpenFunc g_open = nullptr;

void UpdateOpenFunc(OpenFunc func) { g_open = func; }

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

void UpdateFstatFunc(FstatFunc func) { g_fstat = func; }

int __wrap_fstat(int fd, struct stat* st) {
  if (g_fstat) {
    return g_fstat(fd, st);
  }
  return __real_fstat(fd, st);
}

}  // extern "C"
