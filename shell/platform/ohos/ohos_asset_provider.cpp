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

#include "ohos_asset_provider.h"
#include <rawfile/raw_file.h>
#include <rawfile/raw_file_manager.h>
#include "napi_common.h"

namespace flutter {

class FileDescriptionMapping : public fml::Mapping {
 public:
  explicit FileDescriptionMapping(RawFile* fileHandle)
      : file_handle_(fileHandle) {
    LOGD("FileDescriptionMapping :%{public}p", file_handle_);
    int ret = 0;
    if (0 > (ret = ReadFile())) {
      LOGD("FileDescriptionMapping :%{public}d", ret);
    }
  }
  void* GetBuffer() {
    // TODO 考虑线程安全

    if (file_buf_ != nullptr)
      return file_buf_;

    size_t bufLenth = GetSize();

    if (file_handle_ != nullptr && bufLenth > 0) {
      LOGD("FileDescriptionMapping buflenth = %{public}ld", bufLenth);
      file_buf_ = malloc(bufLenth + 1);
      memset(file_buf_, 0, bufLenth + 1);
    }
    return file_buf_;
  }

  ~FileDescriptionMapping() override {
    if (file_handle_ != nullptr) {
      OH_ResourceManager_CloseRawFile(file_handle_);
    }
    if (file_buf_ != nullptr) {
      free(file_buf_);
      file_buf_ = nullptr;
    }
  }

  size_t GetSize() const override {
    if (file_handle_ == nullptr)
      return 0;

    size_t ret = OH_ResourceManager_GetRawFileSize(file_handle_);
    LOGD("GetSize():%{public}zu", ret);
    return ret;
  }

  int ReadFile() {
    if (file_handle_ == nullptr) {
      LOGE("GetMapping  failed. fileHandle_:%{public}p,filebuf:%{public}p",
           file_handle_, file_buf_);
      return -1;
    }

    size_t len = GetSize();
    void* buf = GetBuffer();
    int ret = 0;

    if (buf != nullptr) {
      ret = OH_ResourceManager_ReadRawFile(file_handle_, buf, len);
      read_from_file_ = true;
    }
    LOGD("GetMapping ... total:%{public}zu, ret:%{public}d,buf:%{public}p", len,
         ret, buf);
    return ret;
  }
  const uint8_t* GetMapping() const override {
    return reinterpret_cast<const uint8_t*>(file_buf_);
  }

  bool IsDontNeedSafe() const override {
    // thread unsafe
    return false;
  }

 private:
  RawFile* file_handle_;
  void* file_buf_ = nullptr;
  bool read_from_file_ = false;
  FML_DISALLOW_COPY_AND_ASSIGN(FileDescriptionMapping);
};

OHOSAssetProvider::OHOSAssetProvider(void* assetHandle, const std::string& dir)
    : asset_handle_(assetHandle), dir_(dir) {
  LOGD("assets dir:%{public}s", dir.c_str());
}

OHOSAssetProvider::OHOSAssetProvider(
    std::shared_ptr<OHOSAssetProviderInternal> handle)
    : asset_handle_(handle.get()) {
}

bool OHOSAssetProvider::IsValid() const {
  return (asset_handle_ != nullptr);
}

bool OHOSAssetProvider::IsValidAfterAssetManagerChange() const {
  return true;
}

AssetResolver::AssetResolverType OHOSAssetProvider::GetType() const {
  return AssetResolver::AssetResolverType::kApkAssetProvider;
}

std::unique_ptr<fml::Mapping> OHOSAssetProvider::GetAsMapping(
    const std::string& asset_name) const {
  NativeResourceManager* nativeResMgr =
      reinterpret_cast<NativeResourceManager*>(asset_handle_);
  if (asset_handle_ == nullptr || nativeResMgr == nullptr) {
    LOGE("nativeResMgr is null:%{public}p or  nativeResMgr is null:%{public}p ",
         asset_handle_, nativeResMgr);
  }
  std::string relativePath = dir_ + "/" + asset_name;

  LOGD("GetAsMapping=%{public}s->%{public}s", asset_name.c_str(),
       relativePath.c_str());

  RawFile* fileHandle =
      OH_ResourceManager_OpenRawFile(nativeResMgr, relativePath.c_str());
  LOGD("GetAsMapping=%{public}s->%{public}p", relativePath.c_str(), fileHandle);

  if (fileHandle == nullptr) {
    fileHandle =
        OH_ResourceManager_OpenRawFile(nativeResMgr, asset_name.c_str());
    LOGD("GetAsMapping2 ..fallback:%{public}s->%{public}p", asset_name.c_str(),
         fileHandle);
  }
  LOGD("GetAsMappingend:%{public}p", fileHandle);
  return fileHandle == nullptr
             ? nullptr
             : (std::make_unique<FileDescriptionMapping>(fileHandle));
}

std::unique_ptr<OHOSAssetProvider> OHOSAssetProvider::Clone() const {
  return std::make_unique<OHOSAssetProvider>(asset_handle_);
}

bool OHOSAssetProvider::operator==(const AssetResolver& other) const {
  auto other_provider = other.as_ohos_asset_provider();
  if (!other_provider) {
    return false;
  }
  return asset_handle_ == other_provider->asset_handle_;
}

}  // namespace flutter
