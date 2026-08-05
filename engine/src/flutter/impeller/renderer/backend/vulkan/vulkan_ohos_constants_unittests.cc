/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// OHOS-only assertions for FML_OS_OHOS constants/macros defined in the
// Vulkan backend headers:
//  - OHOS_MEMORY_LEVEL_* in fence_waiter_vk.h and resource_manager_vk.h
//  - VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS in surface_context_vk.h
// Registered only in flutter_ohos_unittests; the guard keeps this file an
// empty translation unit on non-OHOS builds.
#include "flutter/fml/build_config.h"

#include "impeller/renderer/backend/vulkan/fence_waiter_vk.h"
#include "impeller/renderer/backend/vulkan/resource_manager_vk.h"
#include "impeller/renderer/backend/vulkan/surface_context_vk.h"

#include "gtest/gtest.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(VulkanOhosConstantsTest, MemoryLevelConstants) {
  // Note: the MODRATE spelling is a legacy typo for MODERATE, kept as-is
  // (both headers define the same values, which is also verified by the
  // fact that this file includes both without a redefinition error).
  EXPECT_EQ(OHOS_MEMORY_LEVEL_MODRATE, 0);
  EXPECT_EQ(OHOS_MEMORY_LEVEL_LOW, 1);
  EXPECT_EQ(OHOS_MEMORY_LEVEL_CRITICAL, 2);
}

TEST(VulkanOhosConstantsTest, SurfaceCreateInfoStructureTypeOhos) {
  EXPECT_EQ(VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS, 1000685000);
}

}  // namespace testing
}  // namespace impeller

#endif  // FML_OS_OHOS
