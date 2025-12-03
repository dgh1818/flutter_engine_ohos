/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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

  explicit OHOSAssetProvider(
      std::shared_ptr<OHOSAssetProviderInternal> assetHandle);

  ~OHOSAssetProvider() = default;

  std::unique_ptr<OHOSAssetProvider> Clone() const;

  void* GetHandle() const { return asset_handle_; }

  bool operator==(const AssetResolver& other) const override;

  std::unique_ptr<fml::Mapping> GetAsMapping(
      const std::string& asset_name) const override;

 private:
  void* asset_handle_;
  std::string dir_;

  bool IsValid() const override;

  bool IsValidAfterAssetManagerChange() const override;

  AssetResolver::AssetResolverType GetType() const override;

  // |AssetResolver|
  const OHOSAssetProvider* as_ohos_asset_provider() const override {
    return this;
  }

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSAssetProvider);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_ASSET_PROVIDER_H_