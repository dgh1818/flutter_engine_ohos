/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
  auto mockHandle1 = std::make_shared<MockOHOSAssetProviderImpl>();
  auto mockHandle2 = std::make_shared<MockOHOSAssetProviderImpl>();
  auto first_provider = std::make_unique<OHOSAssetProvider>(mockHandle1);
  auto second_provider = std::make_unique<OHOSAssetProvider>(mockHandle2);
  auto third_provider = first_provider->Clone();

  ASSERT_NE(first_provider->GetHandle(), second_provider->GetHandle());
  ASSERT_EQ(first_provider->GetHandle(), third_provider->GetHandle());
  ASSERT_FALSE(*first_provider == *second_provider);
  ASSERT_TRUE(*first_provider == *third_provider);
}
}  // namespace testing
}  // namespace flutter
