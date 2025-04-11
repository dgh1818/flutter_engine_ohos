/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_KHZG file.
 */

#include "flutter/shell/platform/ohos/ohos_asset_provider.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

class MockOHOSAssetProviderImpl : public OHOSAssetProviderInternal {
  public:
  MOCK_METHOD(std::unique_ptr<fml::Mapping>,
              GetAsMapping,
              (const std::string& asset_name),
              (const, override));
};

TEST(OHOSAssetProvider, CloneAndEquals) {
  auto first_impl = std::make_shared<MockOHOSAssetProviderImpl>();
  auto second_impl = std::make_shared<MockOHOSAssetProviderImpl>();
  auto first_provider = std::make_unique<OHOSAssetProvider>(first_impl);
  auto second_provider = std::make_unique<OHOSAssetProvider>(second_impl);
  auto third_provider = first_provider->Clone();

  ASSERT_NE(first_provider->GetHandle(), second_provider->GetHandle());
  ASSERT_EQ(first_provider->GetHandle(), third_provider->GetHandle());
  ASSERT_FALSE(*first_provider == *second_provider);
  ASSERT_TRUE(*first_provider == *third_provider);
}
}  // namespace testing
}  // namespace flutter
