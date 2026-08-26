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

namespace {

#if defined(OHOS_X64_UNITTEST)
constexpr const char* kLibName = "libc.so";
constexpr const char* kSymbolA = "malloc";
constexpr const char* kSymbolB = "free";
constexpr const char* kSymbolC = "strlen";
constexpr int kMinApi = 0;
#else
constexpr const char* kLibName = "libace_ndk.z.so";
constexpr const char* kSymbolA = "OH_ArkUI_UIInputEvent_GetDeviceId";
constexpr const char* kSymbolB = "OH_ArkUI_AxisEvent_GetAxisAction";
constexpr const char* kSymbolC = "OH_ArkUI_UIInputEvent_GetModifierKeyStates";
constexpr int kMinApi = 14;
#endif

}  // namespace

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
  DynamicLibraryLoader loader(kLibName);
  ASSERT_TRUE(loader.IsLoaded()) << kLibName << " not found on device";

  void* symbol_a_func = nullptr;
  std::vector<SymbolInfo> symbols = {
      {kSymbolA, &symbol_a_func, kMinApi},
  };

  EXPECT_TRUE(loader.LoadSymbols(symbols));
  EXPECT_NE(symbol_a_func, nullptr);
}

// LoadSymbols should succeed loading multiple real symbols, covering loop iteration branch
// Using 3 symbols actually loaded by ohos_touch_processor.cpp
TEST(DynamicLibraryLoaderTest, LoadSymbolsSucceedsForMultipleRealSymbols) {
  DynamicLibraryLoader loader(kLibName);
  ASSERT_TRUE(loader.IsLoaded()) << kLibName << " not found on device";

  void* symbol_a_func = nullptr;
  void* symbol_b_func = nullptr;
  void* symbol_c_func = nullptr;
  std::vector<SymbolInfo> symbols = {
      {kSymbolA, &symbol_a_func, kMinApi},
      {kSymbolB, &symbol_b_func, kMinApi},
      {kSymbolC, &symbol_c_func, kMinApi},
  };

  EXPECT_TRUE(loader.LoadSymbols(symbols));
  EXPECT_NE(symbol_a_func, nullptr);
  EXPECT_NE(symbol_b_func, nullptr);
  EXPECT_NE(symbol_c_func, nullptr);
}

// LoadSymbols with an empty vector should return true, covering loop skip branch
TEST(DynamicLibraryLoaderTest, LoadSymbolsReturnsTrueForEmptyVector) {
  DynamicLibraryLoader loader(kLibName);
  ASSERT_TRUE(loader.IsLoaded()) << kLibName << " not found on device";

  std::vector<SymbolInfo> symbols = {};
  EXPECT_TRUE(loader.LoadSymbols(symbols));
}

// LoadSymbols with mixed real and non-existent symbols should return false
// Covers the branch where iteration continues after a successful load
TEST(DynamicLibraryLoaderTest, LoadSymbolsMixedRealAndMissingSymbols) {
  DynamicLibraryLoader loader(kLibName);
  ASSERT_TRUE(loader.IsLoaded()) << kLibName << " not found on device";

  void* real_func = nullptr;
  void* fake_func = nullptr;
  std::vector<SymbolInfo> symbols = {
      {kSymbolA, &real_func, kMinApi},
      {"nonexistent_symbol_xyz123", &fake_func, kMinApi},
  };

  EXPECT_FALSE(loader.LoadSymbols(symbols));
  EXPECT_NE(real_func, nullptr);
  EXPECT_EQ(fake_func, nullptr);
}

TEST(DynamicLibraryLoaderTest, LoadSymbolsMinApiEqualToCurrentLoads) {
  DynamicLibraryLoader loader(kLibName);
  ASSERT_TRUE(loader.IsLoaded()) << kLibName << " not found on device";

  void* func = nullptr;
  std::vector<SymbolInfo> symbols = {
      {kSymbolA, &func, DynamicLibraryLoader::GetApiVersion()},
  };
  EXPECT_TRUE(loader.LoadSymbols(symbols));
  EXPECT_NE(func, nullptr);
}

TEST(DynamicLibraryLoaderTest, LoadSymbolsResetsNonNullTargetOnSkip) {
  DynamicLibraryLoader loader(kLibName);
  ASSERT_TRUE(loader.IsLoaded()) << kLibName << " not found on device";

  void* target = reinterpret_cast<void*>(0x1234);
  std::vector<SymbolInfo> symbols = {
      {"dummy_symbol", &target, 99999},
  };
  EXPECT_FALSE(loader.LoadSymbols(symbols));
  EXPECT_EQ(target, nullptr);
}

TEST(DynamicLibraryLoaderTest, GetApiVersionIsStableAcrossCalls) {
  int first = DynamicLibraryLoader::GetApiVersion();
  int second = DynamicLibraryLoader::GetApiVersion();
  EXPECT_GT(first, 0);
  EXPECT_EQ(first, second);
}

TEST(DynamicLibraryLoaderTest, DestructorClosesLoadedHandle) {
  bool was_loaded = false;
  EXPECT_NO_FATAL_FAILURE({
    DynamicLibraryLoader loader("libc.so");
    was_loaded = loader.IsLoaded();
  });
  EXPECT_TRUE(was_loaded);
}

}  // namespace testing
}  // namespace flutter
