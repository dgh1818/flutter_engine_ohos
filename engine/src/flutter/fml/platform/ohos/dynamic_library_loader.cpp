/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "dynamic_library_loader.h"
#include "deviceinfo.h"
#include "flutter/fml/logging.h"

namespace flutter {

DynamicLibraryLoader::DynamicLibraryLoader(const char* libName)
    : handle_(nullptr) {
  handle_ = dlopen(libName, RTLD_LAZY | RTLD_LOCAL);
  if (!handle_) {
    FML_LOG(ERROR) << "dlopen(" << libName << ") failed: " << dlerror();
  }
}

DynamicLibraryLoader::~DynamicLibraryLoader() {
  if (handle_) {
    dlclose(handle_);
  }
}

int DynamicLibraryLoader::GetApiVersion() {
  // 函数内 static，第一次调用时初始化一次，后面都直接返回缓存值
  static int api_version = OH_GetSdkApiVersion();
  return api_version;
}

bool DynamicLibraryLoader::LoadSymbols(
    const std::vector<SymbolInfo>& symbolInfos) {
  if (!handle_)
    return false;

  bool allLoaded = true;
  int currentApi = GetApiVersion();
  for (const auto& info : symbolInfos) {
    *info.target = nullptr;  // 手动赋值为 nullptr，避免使用未初始化的指针
    if (currentApi >= info.minApi) {
      *info.target = dlsym(handle_, info.name);
      if (*info.target == nullptr) {
        FML_LOG(ERROR) << "dlsym failed for " << info.name << ": " << dlerror();
        allLoaded = false;
      } else {
        FML_LOG(INFO) << "Loaded symbol " << info.name;
      }
    } else {
      FML_LOG(INFO) << "Skipping " << info.name
                    << " (requires API >= " << info.minApi
                    << ", current = " << currentApi << ")";
      allLoaded = false;
    }
  }

  return allLoaded;
}

}  // namespace flutter
