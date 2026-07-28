/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/platform/ohos/dynamic_library_loader.h"

#include <gtest/gtest.h>

#include <cstring>

namespace flutter {
namespace testing {

// Load libc.so (system library, always exists), IsLoaded should return true
TEST(DynamicLibraryLoaderTest, LoadSystemLibrarySucceeds) {
  DynamicLibraryLoader loader("libc.so");
  EXPECT_TRUE(loader.IsLoaded());
}

// Load a non-existent library, IsLoaded should return false
TEST(DynamicLibraryLoaderTest, LoadNonexistentLibraryFails) {
  DynamicLibraryLoader loader("libnonexistent_xyz123.so");
  EXPECT_FALSE(loader.IsLoaded());
}

// GetApiVersion should return a value greater than 0
TEST(DynamicLibraryLoaderTest, GetApiVersionReturnsPositiveValue) {
  int version = DynamicLibraryLoader::GetApiVersion();
  EXPECT_GT(version, 0);
}

// LoadSymbols should return false when handle is invalid
TEST(DynamicLibraryLoaderTest, LoadSymbolsReturnsFalseWhenNotLoaded) {
  DynamicLibraryLoader loader("libnonexistent_xyz123.so");
  void* dummy_target = nullptr;
  std::vector<SymbolInfo> symbols = {
      {"dummy_symbol", &dummy_target, 0},
  };
  EXPECT_FALSE(loader.LoadSymbols(symbols));
}

// LoadSymbols should return false when handle is valid but symbol is missing
TEST(DynamicLibraryLoaderTest, LoadSymbolsReturnsFalseForMissingSymbol) {
  DynamicLibraryLoader loader("libc.so");
  ASSERT_TRUE(loader.IsLoaded());
  void* dummy_target = nullptr;
  std::vector<SymbolInfo> symbols = {
      {"nonexistent_symbol_xyz123", &dummy_target, 0},
  };
  EXPECT_FALSE(loader.LoadSymbols(symbols));
  EXPECT_EQ(dummy_target, nullptr);
}

// LoadSymbols should skip and return false when minApi is higher than current API version
TEST(DynamicLibraryLoaderTest, LoadSymbolsSkipsWhenApiTooLow) {
  DynamicLibraryLoader loader("libc.so");
  ASSERT_TRUE(loader.IsLoaded());
  void* dummy_target = nullptr;
  std::vector<SymbolInfo> symbols = {
      {"dummy_symbol", &dummy_target, 99999},
  };
  EXPECT_FALSE(loader.LoadSymbols(symbols));
  EXPECT_EQ(dummy_target, nullptr);
}

// LoadSymbols should succeed loading a real symbol from libace_ndk.z.so
// Using a symbol the engine actually loads: OH_ArkUI_UIInputEvent_GetDeviceId (minApi=14)
TEST(DynamicLibraryLoaderTest, LoadSymbolsSucceedsForRealAceNdkSymbol) {
  DynamicLibraryLoader loader("libace_ndk.z.so");
  ASSERT_TRUE(loader.IsLoaded()) << "libace_ndk.z.so not found on device";

  void* device_id_func = nullptr;
  std::vector<SymbolInfo> symbols = {
      {"OH_ArkUI_UIInputEvent_GetDeviceId", &device_id_func, 14},
  };

  EXPECT_TRUE(loader.LoadSymbols(symbols));
  EXPECT_NE(device_id_func, nullptr);
}

// LoadSymbols should succeed loading multiple real symbols, covering loop iteration branch
// Using 3 symbols actually loaded by ohos_touch_processor.cpp
TEST(DynamicLibraryLoaderTest, LoadSymbolsSucceedsForMultipleRealSymbols) {
  DynamicLibraryLoader loader("libace_ndk.z.so");
  ASSERT_TRUE(loader.IsLoaded()) << "libace_ndk.z.so not found on device";

  void* get_device_id = nullptr;
  void* get_axis_action = nullptr;
  void* get_modifier_key_states = nullptr;
  std::vector<SymbolInfo> symbols = {
      {"OH_ArkUI_UIInputEvent_GetDeviceId", &get_device_id, 14},
      {"OH_ArkUI_AxisEvent_GetAxisAction", &get_axis_action, 15},
      {"OH_ArkUI_UIInputEvent_GetModifierKeyStates",
       &get_modifier_key_states, 17},
  };

  EXPECT_TRUE(loader.LoadSymbols(symbols));
  EXPECT_NE(get_device_id, nullptr);
  EXPECT_NE(get_axis_action, nullptr);
  EXPECT_NE(get_modifier_key_states, nullptr);
}

// LoadSymbols with an empty vector should return true, covering loop skip branch
TEST(DynamicLibraryLoaderTest, LoadSymbolsReturnsTrueForEmptyVector) {
  DynamicLibraryLoader loader("libace_ndk.z.so");
  ASSERT_TRUE(loader.IsLoaded()) << "libace_ndk.z.so not found on device";

  std::vector<SymbolInfo> symbols = {};
  EXPECT_TRUE(loader.LoadSymbols(symbols));
}

// LoadSymbols with mixed real and non-existent symbols should return false
// Covers the branch where iteration continues after a successful load
TEST(DynamicLibraryLoaderTest, LoadSymbolsMixedRealAndMissingSymbols) {
  DynamicLibraryLoader loader("libace_ndk.z.so");
  ASSERT_TRUE(loader.IsLoaded()) << "libace_ndk.z.so not found on device";

  void* real_func = nullptr;
  void* fake_func = nullptr;
  std::vector<SymbolInfo> symbols = {
      {"OH_ArkUI_UIInputEvent_GetDeviceId", &real_func, 14},
      {"nonexistent_symbol_xyz123", &fake_func, 14},
  };

  EXPECT_FALSE(loader.LoadSymbols(symbols));
  EXPECT_NE(real_func, nullptr);
  EXPECT_EQ(fake_func, nullptr);
}

}  // namespace testing
}  // namespace flutter
