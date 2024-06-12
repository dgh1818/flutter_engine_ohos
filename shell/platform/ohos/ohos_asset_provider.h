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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_ASSET_PROVIDER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_ASSET_PROVIDER_H_

#include "flutter/assets/asset_resolver.h"
#include "flutter/fml/memory/ref_counted.h"

namespace flutter {

class OHOSAssetProviderInternal {
  public:
    virtual std::unique_ptr<fml::Mapping> GetAsMapping(
      const std::string& asset_name) const = 0;
  protected:
    virtual ~OHOSAssetProviderInternal() = default;
};

// ohos平台的文件管理 ，必须通过NativeResourceManager* 指针对它进行初始化
class OHOSAssetProvider final : public AssetResolver {
 public:
  explicit OHOSAssetProvider(void* assetHandle,
                             const std::string& dir = "flutter_assets");

  explicit OHOSAssetProvider(std::shared_ptr<OHOSAssetProviderInternal> assetHandle_);

  ~OHOSAssetProvider() = default;

  std::unique_ptr<OHOSAssetProvider> Clone() const;

  void* GetHandle() const { return assetHandle_; }

  bool operator==(const AssetResolver& other) const override;

 private:
  void* assetHandle_;
  std::string dir_;

  bool IsValid() const override;

  bool IsValidAfterAssetManagerChange() const override;

  AssetResolver::AssetResolverType GetType() const override;

  std::unique_ptr<fml::Mapping> GetAsMapping(
      const std::string& asset_name) const override;

  // |AssetResolver|
  const OHOSAssetProvider* as_ohos_asset_provider() const override {
    return this;
  }

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSAssetProvider);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_ASSET_PROVIDER_H_