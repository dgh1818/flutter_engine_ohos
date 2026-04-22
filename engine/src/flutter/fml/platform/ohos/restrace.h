/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#ifndef FLUTTER_FML_PLATFORM_OHOS_RESTRACE_H_
#define FLUTTER_FML_PLATFORM_OHOS_RESTRACE_H_

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define OH_RESTRACE_WEAK __attribute__((weak))
#else
#define OH_RESTRACE_WEAK
#endif

#ifdef __cplusplus
extern "C" {
#endif

void restrace(uint64_t mask,
              void* addr,
              size_t size,
              const char* tag,
              bool is_using) OH_RESTRACE_WEAK;

void resTraceFreeRegion(uint64_t mask,
                        void* addr,
                        size_t size) OH_RESTRACE_WEAK;

#ifdef __cplusplus
}
#endif

#define RES_DART_HEAP ((1ULL) << 34)
#define TAG_DART_HEAP "RES_DART_HEAP"

#define OH_RESTRACE(ADDR, SIZE)                                               \
  do {                                                                        \
    if ((ADDR) != nullptr && (SIZE) > 0 && restrace != nullptr) {             \
      restrace(RES_DART_HEAP, (ADDR), (SIZE), TAG_DART_HEAP, true);           \
    }                                                                         \
  } while (0)

#define OH_RESTRACE_FREE_REGION(ADDR, SIZE)                                   \
  do {                                                                        \
    if ((ADDR) != nullptr && (SIZE) > 0 && resTraceFreeRegion != nullptr) {   \
      resTraceFreeRegion(RES_DART_HEAP, (ADDR), (SIZE));                      \
    }                                                                         \
  } while (0)

#endif  // FLUTTER_FML_PLATFORM_OHOS_RESTRACE_H_
