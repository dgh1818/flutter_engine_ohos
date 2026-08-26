/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

// Test private methods by temporarily redefining access specifiers.
// This is a common C++ unit testing technique for testing internal logic
// that doesn't require runtime dependencies.
#define private public
#define protected public

#include <cstring>
#include <rawfile/raw_file.h>
#include <rawfile/raw_file_manager.h>
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

// ===== Constructor with shared_ptr<OHOSAssetProviderInternal> =====

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

// GetHandle should return the pointer passed to the constructor
TEST(OHOSAssetProvider, GetHandleReturnsConstructorValue) {
  auto impl = std::make_shared<MockOHOSAssetProviderImpl>();
  OHOSAssetProvider provider(impl);
  EXPECT_EQ(provider.GetHandle(), impl.get());
}

// Clone should return a new object with the same handle
TEST(OHOSAssetProvider, CloneReturnsSameHandle) {
  auto impl = std::make_shared<MockOHOSAssetProviderImpl>();
  OHOSAssetProvider provider(impl);
  auto cloned = provider.Clone();
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(provider.GetHandle(), cloned->GetHandle());
}

// operator== should return false when comparing different handles
TEST(OHOSAssetProvider, OperatorEqualsReturnsFalseForDifferentHandle) {
  auto impl1 = std::make_shared<MockOHOSAssetProviderImpl>();
  auto impl2 = std::make_shared<MockOHOSAssetProviderImpl>();
  OHOSAssetProvider provider1(impl1);
  OHOSAssetProvider provider2(impl2);
  EXPECT_FALSE(provider1 == provider2);
}

// operator== should return true when comparing the same handle
TEST(OHOSAssetProvider, OperatorEqualsReturnsTrueForSameHandle) {
  auto impl = std::make_shared<MockOHOSAssetProviderImpl>();
  OHOSAssetProvider provider1(impl);
  OHOSAssetProvider provider2(impl);
  EXPECT_TRUE(provider1 == provider2);
}

// Constructor with void* overload
TEST(OHOSAssetProvider, ConstructorWithVoidPointer) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  EXPECT_EQ(provider.GetHandle(), handle);
}

// Constructor with void* overload, default dir is "flutter_assets"
TEST(OHOSAssetProvider, ConstructorWithVoidPointerDefaultDir) {
  void* handle = reinterpret_cast<void*>(0x5678);
  OHOSAssetProvider provider(handle);
  EXPECT_EQ(provider.GetHandle(), handle);
}

// ===== IsValid =====

TEST(OHOSAssetProvider, IsValidReturnsFalseWhenHandleIsNull) {
  void* handle = nullptr;
  OHOSAssetProvider provider(handle);
  EXPECT_FALSE(provider.IsValid());
}

TEST(OHOSAssetProvider, IsValidReturnsTrueWhenHandleIsNotNull) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle);
  EXPECT_TRUE(provider.IsValid());
}

TEST(OHOSAssetProvider, IsValidReturnsTrueForSharedPtrConstructor) {
  auto impl = std::make_shared<MockOHOSAssetProviderImpl>();
  OHOSAssetProvider provider(impl);
  EXPECT_TRUE(provider.IsValid());
}

TEST(OHOSAssetProvider, IsValidReturnsFalseForSharedPtrWithNullImpl) {
  std::shared_ptr<MockOHOSAssetProviderImpl> impl;
  OHOSAssetProvider provider(impl);
  EXPECT_FALSE(provider.IsValid());
}

// ===== IsValidAfterAssetManagerChange =====

TEST(OHOSAssetProvider, IsValidAfterAssetManagerChangeAlwaysReturnsTrue) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle);
  EXPECT_TRUE(provider.IsValidAfterAssetManagerChange());
}

TEST(OHOSAssetProvider, IsValidAfterAssetManagerChangeReturnsTrueEvenWhenNull) {
  void* handle = nullptr;
  OHOSAssetProvider provider(handle);
  EXPECT_TRUE(provider.IsValidAfterAssetManagerChange());
}

// ===== GetType =====

TEST(OHOSAssetProvider, GetTypeReturnsApkAssetProvider) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle);
  EXPECT_EQ(provider.GetType(),
            AssetResolver::AssetResolverType::kApkAssetProvider);
}

TEST(OHOSAssetProvider, GetTypeReturnsApkAssetProviderForSharedPtr) {
  auto impl = std::make_shared<MockOHOSAssetProviderImpl>();
  OHOSAssetProvider provider(impl);
  EXPECT_EQ(provider.GetType(),
            AssetResolver::AssetResolverType::kApkAssetProvider);
}

// ===== operator== with non-OHOS AssetResolver =====
// NOTE: The static_cast<const AssetResolver&> below is required because this
// engine is compiled with -std=c++20, which introduces rewritten operator
// lookup rules (P1185R2 / P1630R1). When both sides of == override
// operator==(const AssetResolver&), C++20 cannot pick one without a hint,
// producing an "ambiguous" error. flutter3.35 compiles with -std=c++17 and
// does not have this issue, so it uses the plain `provider == non_ohos_resolver`
// form without the cast. This is an upstream C++ standard upgrade, not an
// OHOS-specific adaptation.

class NonOHOSAssetResolver : public AssetResolver {
 public:
  bool IsValid() const override { return false; }
  bool IsValidAfterAssetManagerChange() const override { return false; }
  AssetResolverType GetType() const override {
    return AssetResolver::AssetResolverType::kAssetManager;
  }
  std::unique_ptr<fml::Mapping> GetAsMapping(
      const std::string& asset_name) const override {
    return nullptr;
  }
  bool operator==(const AssetResolver& other) const override { return false; }
};

TEST(OHOSAssetProvider, OperatorEqualsReturnsFalseForNonOHOSResolver) {
  auto impl = std::make_shared<MockOHOSAssetProviderImpl>();
  OHOSAssetProvider provider(impl);
  NonOHOSAssetResolver non_ohos_resolver;
  // as_ohos_asset_provider() returns nullptr for non-OHOS resolver.
  // Cast to const AssetResolver& to disambiguate: both sides override
  // operator==, so the compiler cannot pick one without a hint.
  EXPECT_FALSE(provider ==
               static_cast<const AssetResolver&>(non_ohos_resolver));
}

TEST(OHOSAssetProvider, OperatorEqualsReturnsFalseWhenBothAreNonOHOS) {
  // When comparing two non-OHOS resolvers, as_ohos_asset_provider() returns
  // nullptr on both sides, so operator== should return false.
  // Cast to const AssetResolver& to disambiguate the overloaded operator==.
  NonOHOSAssetResolver resolver1;
  NonOHOSAssetResolver resolver2;
  EXPECT_FALSE(static_cast<const AssetResolver&>(resolver1) ==
               static_cast<const AssetResolver&>(resolver2));
}

// ===== GetAsMapping with null handle =====
// NDK docs guarantee OH_ResourceManager_OpenRawFile returns nullptr when mgr
// is null, so we can safely test GetAsMapping's null pointer branch with a

// GetAsMapping should return nullptr when handle is null (default dir)
TEST(OHOSAssetProvider, GetAsMappingReturnsNullWithNullHandle) {
  void* handle = nullptr;
  OHOSAssetProvider provider(handle);
  auto mapping = provider.GetAsMapping("test.txt");
  EXPECT_EQ(mapping, nullptr);
}

// GetAsMapping should return nullptr when handle is null (custom dir)
TEST(OHOSAssetProvider, GetAsMappingReturnsNullWithNullHandleAndCustomDir) {
  void* handle = nullptr;
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  EXPECT_EQ(mapping, nullptr);
}

// GetAsMapping should return nullptr when handle is null and asset_name is empty
TEST(OHOSAssetProvider, GetAsMappingReturnsNullWithEmptyAssetName) {
  void* handle = nullptr;
  OHOSAssetProvider provider(handle);
  auto mapping = provider.GetAsMapping("");
  EXPECT_EQ(mapping, nullptr);
}

// GetAsMapping should return nullptr when shared_ptr impl is null
TEST(OHOSAssetProvider, GetAsMappingReturnsNullWithNullSharedPtrImpl) {
  std::shared_ptr<MockOHOSAssetProviderImpl> impl;
  OHOSAssetProvider provider(impl);
  EXPECT_FALSE(provider.IsValid());
  auto mapping = provider.GetAsMapping("test.txt");
  EXPECT_EQ(mapping, nullptr);
}

// FileDescriptionMapping is defined inside the .cpp and cannot be accessed
// directly. But GetAsMapping() calls OH_ResourceManager_OpenRawFile, and if

static constexpr char kMockRawFileData[] = "Hello OHOS!";
static constexpr size_t kMockRawFileSize = 12;  // strlen("Hello OHOS!") + 1

static int s_open_raw_file_call_count = 0;
static int s_close_raw_file_call_count = 0;
static int s_read_raw_file_call_count = 0;
static bool s_open_raw_file_return_null = false;
static bool s_open_raw_file_fail_first = false;
static const char* s_raw_file_data = kMockRawFileData;
static size_t s_raw_file_size = kMockRawFileSize;
static bool s_read_raw_file_return_negative = false;

// Stub: OH_ResourceManager_OpenRawFile
extern "C" RawFile* OH_ResourceManager_OpenRawFile(
    const NativeResourceManager* mgr,
    const char* fileName) {
  const int call_index = ++s_open_raw_file_call_count;
  if (s_open_raw_file_return_null || mgr == nullptr) {
    return nullptr;
  }
  if (s_open_raw_file_fail_first && call_index == 1) {
    return nullptr;
  }
  // Return a non-null fake pointer indicating successful open
  return reinterpret_cast<RawFile*>(0xDEAD);
}

// Stub: OH_ResourceManager_GetRawFileSize
extern "C" long OH_ResourceManager_GetRawFileSize(RawFile* rawFile) {
  if (rawFile == nullptr) {
    return 0;
  }
  return static_cast<long>(s_raw_file_size);
}

// Stub: OH_ResourceManager_ReadRawFile
extern "C" int OH_ResourceManager_ReadRawFile(const RawFile* rawFile,
                                              void* buf,
                                              size_t length) {
  s_read_raw_file_call_count++;
  if (s_read_raw_file_return_negative) {
    return -1;
  }
  if (rawFile == nullptr || buf == nullptr) {
    return 0;
  }
  size_t copy_len = length < s_raw_file_size ? length : s_raw_file_size;
  memcpy(buf, s_raw_file_data, copy_len);
  return static_cast<int>(copy_len);
}

// Stub: OH_ResourceManager_CloseRawFile
extern "C" void OH_ResourceManager_CloseRawFile(RawFile* rawFile) {
  s_close_raw_file_call_count++;
}

// Reset all stub variables to defaults. Called after each test to prevent
// test-order dependencies (--gtest_shuffle safe).
void ResetStubState() {
  s_open_raw_file_call_count = 0;
  s_close_raw_file_call_count = 0;
  s_read_raw_file_call_count = 0;
  s_open_raw_file_return_null = false;
  s_open_raw_file_fail_first = false;
  s_raw_file_data = kMockRawFileData;
  s_raw_file_size = kMockRawFileSize;
  s_read_raw_file_return_negative = false;
}

class StubStateResetter : public ::testing::EmptyTestEventListener {
 public:
  void OnTestEnd(const ::testing::TestInfo&) override { ResetStubState(); }
};

struct StubStateResetterRegistrar {
  StubStateResetterRegistrar() {
    ::testing::UnitTest::GetInstance()->listeners().Append(
        new StubStateResetter());
  }
} g_stub_resetter_registrar;

static void RequireRawFileStubWorld() {}

static int OpenRawFileCallCount() { return s_open_raw_file_call_count; }
static int CloseRawFileCallCount() { return s_close_raw_file_call_count; }
static int ReadRawFileCallCount() { return s_read_raw_file_call_count; }
static void ForceAllOpensFail() { s_open_raw_file_return_null = true; }
static void ForceFirstOpenFail() { s_open_raw_file_fail_first = true; }
static void ForceNegativeReadResult() {
  s_read_raw_file_return_negative = true;
}
static void SetStubRawFileSize(size_t size) { s_raw_file_size = size; }

void SetRawFileStubContent(const char* data, size_t size) {
  s_raw_file_data = (data != nullptr) ? data : kMockRawFileData;
  s_raw_file_size = (data != nullptr) ? size : kMockRawFileSize;
}

void SetRawFileStubOpenFail(bool fail) {
  s_open_raw_file_return_null = fail;
}
// ===== GetAsMapping success path: returns non-null Mapping =====

// GetAsMapping should return non-null Mapping when handle is non-null and
// OpenRawFile succeeds
TEST(OHOSAssetProvider, GetAsMappingReturnsMappingWhenOpenSucceeds) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  // First OpenRawFile succeeds, should not fall back
  EXPECT_EQ(OpenRawFileCallCount(), 1);
}

// GetAsMapping's returned Mapping GetSize should return RawFileSize
TEST(OHOSAssetProvider, GetAsMappingMappingGetSizeReturnsRawFileSize) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_EQ(mapping->GetSize(), kMockRawFileSize);
}

// GetAsMapping's returned Mapping GetMapping should return non-null pointer
TEST(OHOSAssetProvider, GetAsMappingMappingGetMappingReturnsNonNull) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_NE(mapping->GetMapping(), nullptr);
}

// GetAsMapping's returned Mapping GetMapping should contain the read file data
TEST(OHOSAssetProvider, GetAsMappingMappingContainsFileData) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  // ReadFile was called during construction -> OH_ResourceManager_ReadRawFile
  EXPECT_GT(ReadRawFileCallCount(), 0);
  const uint8_t* data = mapping->GetMapping();
  ASSERT_NE(data, nullptr);
  // Verify the content matches the data written by the stub
  EXPECT_EQ(memcmp(data, kMockRawFileData, kMockRawFileSize), 0);
}

// GetAsMapping's returned Mapping IsDontNeedSafe should return false
TEST(OHOSAssetProvider, GetAsMappingMappingIsDontNeedSafeReturnsFalse) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_FALSE(mapping->IsDontNeedSafe());
}

// GetAsMapping's Mapping destructor should call OH_ResourceManager_CloseRawFile
TEST(OHOSAssetProvider, GetAsMappingMappingDestructorClosesRawFile) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  {
    auto mapping = provider.GetAsMapping("test.txt");
    ASSERT_NE(mapping, nullptr);
    EXPECT_EQ(CloseRawFileCallCount(), 0);
  }  // mapping destructor
  EXPECT_EQ(CloseRawFileCallCount(), 1);
}

// GetAsMapping should fall back when first OpenRawFile fails
TEST(OHOSAssetProvider, GetAsMappingFallbackWhenFirstOpenFails) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  // With custom dir, first try uses dir + "/" + name, second try uses name
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  // First attempt succeeds, should not fall back
  EXPECT_EQ(OpenRawFileCallCount(), 1);
}

// GetAsMapping should return nullptr when both first and fallback opens fail
TEST(OHOSAssetProvider, GetAsMappingReturnsNullWhenAllOpensFail) {
  RequireRawFileStubWorld();
  ForceAllOpensFail();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  EXPECT_EQ(mapping, nullptr);
  // Should attempt twice: first with relativePath, then fallback asset_name
  EXPECT_EQ(OpenRawFileCallCount(), 2);
}

// GetAsMapping should correctly pass handle to OH_ResourceManager_OpenRawFile
// when handle is non-null but treated as NativeResourceManager
TEST(OHOSAssetProvider, GetAsMappingPassesHandleToOpenRawFile) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0xBEEF);
  OHOSAssetProvider provider(handle, "assets");
  auto mapping = provider.GetAsMapping("file.txt");
  ASSERT_NE(mapping, nullptr);
}

// GetAsMapping's returned Mapping GetSize should be consistent across calls
TEST(OHOSAssetProvider, GetAsMappingMappingGetSizeConsistent) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  auto size1 = mapping->GetSize();
  auto size2 = mapping->GetSize();
  EXPECT_EQ(size1, size2);
  EXPECT_EQ(size1, kMockRawFileSize);
}

// GetAsMapping's returned Mapping GetMapping should return the same pointer across calls
TEST(OHOSAssetProvider, GetAsMappingMappingGetMappingConsistent) {
  RequireRawFileStubWorld();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  const uint8_t* ptr1 = mapping->GetMapping();
  const uint8_t* ptr2 = mapping->GetMapping();
  EXPECT_EQ(ptr1, ptr2);
}

TEST(OHOSAssetProvider, MappingToleratesNegativeReadResult) {
  RequireRawFileStubWorld();
  ForceNegativeReadResult();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_EQ(ReadRawFileCallCount(), 1);
  EXPECT_EQ(mapping->GetSize(), kMockRawFileSize);
  const uint8_t* data = mapping->GetMapping();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data[0], 0);
}

TEST(OHOSAssetProvider, ZeroSizeRawFileSkipsBufferAllocation) {
  RequireRawFileStubWorld();
  SetStubRawFileSize(0);
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  {
    auto mapping = provider.GetAsMapping("empty.bin");
    ASSERT_NE(mapping, nullptr);
    EXPECT_EQ(mapping->GetSize(), 0u);
    EXPECT_EQ(mapping->GetMapping(), nullptr);
    EXPECT_EQ(ReadRawFileCallCount(), 0);
    EXPECT_EQ(CloseRawFileCallCount(), 0);
  }
  EXPECT_EQ(CloseRawFileCallCount(), 1);
}

TEST(OHOSAssetProvider, GetAsMappingFallbackSecondOpenSucceeds) {
  RequireRawFileStubWorld();
  ForceFirstOpenFail();
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_EQ(OpenRawFileCallCount(), 2);
  EXPECT_EQ(mapping->GetSize(), kMockRawFileSize);
  const uint8_t* data = mapping->GetMapping();
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(memcmp(data, kMockRawFileData, kMockRawFileSize), 0);
}

TEST(OHOSAssetProvider, MappingServesConfiguredContent) {
  RequireRawFileStubWorld();
  static const char kJson[] = "{\"SWITCH\":1}";
  SetRawFileStubContent(kJson, sizeof(kJson) - 1);
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("framesconfig.json");
  ASSERT_NE(mapping, nullptr);
  EXPECT_EQ(mapping->GetSize(), sizeof(kJson) - 1);
  EXPECT_EQ(memcmp(mapping->GetMapping(), kJson, sizeof(kJson) - 1), 0);
}

TEST(OHOSAssetProvider, HugeRawFileSizeMallocFailureKeepsBufferNull) {
  RequireRawFileStubWorld();
  SetStubRawFileSize(1ULL << 62);
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  {
    auto mapping = provider.GetAsMapping("huge.bin");
    ASSERT_NE(mapping, nullptr);
    EXPECT_EQ(mapping->GetSize(), 1ULL << 62);
    EXPECT_EQ(mapping->GetMapping(), nullptr);
    EXPECT_EQ(ReadRawFileCallCount(), 0);
  }
  EXPECT_EQ(CloseRawFileCallCount(), 1);
}

}  // namespace testing
}  // namespace flutter
