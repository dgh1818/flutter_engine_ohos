/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// OHOS-only assertions for the FML_OS_OHOS branches in
// yuv_conversion_vk.cc/.h (externalFormat participates in the descriptor
// hash and equality). Registered only in flutter_ohos_unittests; the guard
// keeps this file an empty translation unit on non-OHOS builds.
#include "flutter/fml/build_config.h"

#include "impeller/renderer/backend/vulkan/yuv_conversion_vk.h"

#include "gtest/gtest.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(YUVConversionVKOhosTest, DescriptorHashIncludesExternalFormatOHOS) {
  YUVConversionDescriptorVK desc_a;
  YUVConversionDescriptorVK desc_b;
  desc_b.get<vk::ExternalFormatOHOS>().externalFormat = 42;

  YUVConversionDescriptorVKHash hash;
  EXPECT_EQ(hash(desc_a), hash(desc_a));
  // Same base fields, different OHOS externalFormat -> different hash.
  EXPECT_NE(hash(desc_a), hash(desc_b));
}

TEST(YUVConversionVKOhosTest, DescriptorEqualComparesExternalFormatOHOS) {
  YUVConversionDescriptorVK desc_a;
  YUVConversionDescriptorVK desc_b;

  YUVConversionDescriptorVKEqual equal;
  EXPECT_TRUE(equal(desc_a, desc_a));
  // Both default-constructed (externalFormat == 0) -> equal.
  EXPECT_TRUE(equal(desc_a, desc_b));

  desc_b.get<vk::ExternalFormatOHOS>().externalFormat = 42;
  EXPECT_FALSE(equal(desc_a, desc_b));
}

TEST(YUVConversionVKOhosTest, DescriptorChainCarriesExternalFormatOHOS) {
  YUVConversionDescriptorVK desc;
  desc.get<vk::ExternalFormatOHOS>().externalFormat = 7;
  EXPECT_EQ(desc.get<vk::ExternalFormatOHOS>().externalFormat, 7u);
}

}  // namespace testing
}  // namespace impeller

#endif  // FML_OS_OHOS
