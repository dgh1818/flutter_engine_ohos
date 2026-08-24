/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_TESTING_LIBC_WRAPPER_STUB_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_TESTING_LIBC_WRAPPER_STUB_H_

#include <sys/stat.h>
#include <sys/types.h>

extern "C" {
using OpenFunc = int (*)(const char* path, int flags);
using FstatFunc = int (*)(int fd, struct stat* st);

void UpdateOpenFunc(OpenFunc func);
void UpdateFstatFunc(FstatFunc func);
}

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_TESTING_LIBC_WRAPPER_STUB_H_
