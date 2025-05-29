/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_LIB_UI_WINDOW_DYNAMIC_LIBRARY_LOADER_H_
#define FLUTTER_LIB_UI_WINDOW_DYNAMIC_LIBRARY_LOADER_H_

#include <dlfcn.h>
#include <cstdint>
#include <vector>

namespace flutter {

struct SymbolInfo {
  const char* name;  // 符号名称
  void** target;     // 接收函数指针的变量地址
  int minApi;        // 最低支持的 API 版本
};

class DynamicLibraryLoader {
 public:
  explicit DynamicLibraryLoader(const char* libName);
  ~DynamicLibraryLoader();

  bool LoadSymbols(const std::vector<SymbolInfo>& symbolInfos);

  bool IsLoaded() const { return handle_ != nullptr; }

  static int GetApiVersion();

 private:
  void* handle_;
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_WINDOW_DYNAMIC_LIBRARY_LOADER_H_
