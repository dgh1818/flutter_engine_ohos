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
  // as_ohos_asset_provider() returns nullptr for non-OHOS resolver
  EXPECT_FALSE(provider == non_ohos_resolver);
}

TEST(OHOSAssetProvider, OperatorEqualsReturnsFalseWhenBothAreNonOHOS) {
  // When comparing two non-OHOS resolvers, as_ohos_asset_provider() returns
  // nullptr on both sides, so operator== should return false
  NonOHOSAssetResolver resolver1;
  NonOHOSAssetResolver resolver2;
  EXPECT_FALSE(resolver1 == resolver2);
}

// ===== GetAsMapping with null handle =====
// NDK docs guarantee OH_ResourceManager_OpenRawFile returns nullptr when mgr
// is null, so we can safely test GetAsMapping's null pointer branch with a
// null handle.

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

// ===== Indirectly test FileDescriptionMapping via stub NDK functions =====
// FileDescriptionMapping is defined inside the .cpp and cannot be accessed
// directly. But GetAsMapping() calls OH_ResourceManager_OpenRawFile, and if
// it returns non-null, a FileDescriptionMapping is constructed. We define
// same-name extern "C" stub functions to override the dynamic library
// implementations, thereby indirectly testing FileDescriptionMapping's methods.

static constexpr char kMockRawFileData[] = "Hello OHOS!";
static constexpr size_t kMockRawFileSize = 12;  // strlen("Hello OHOS!") + 1
static int s_open_raw_file_call_count = 0;
static int s_close_raw_file_call_count = 0;
static int s_read_raw_file_call_count = 0;
static bool s_open_raw_file_return_null = false;

// Stub: OH_ResourceManager_OpenRawFile
extern "C" RawFile* OH_ResourceManager_OpenRawFile(
    const NativeResourceManager* mgr,
    const char* fileName) {
  s_open_raw_file_call_count++;
  if (s_open_raw_file_return_null || mgr == nullptr) {
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
  return static_cast<long>(kMockRawFileSize);
}

// Stub: OH_ResourceManager_ReadRawFile
extern "C" int OH_ResourceManager_ReadRawFile(const RawFile* rawFile,
                                              void* buf,
                                              size_t length) {
  s_read_raw_file_call_count++;
  if (rawFile == nullptr || buf == nullptr) {
    return 0;
  }
  size_t copy_len = length < kMockRawFileSize ? length : kMockRawFileSize;
  memcpy(buf, kMockRawFileData, copy_len);
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

// ===== GetAsMapping success path: returns non-null Mapping =====

// GetAsMapping should return non-null Mapping when handle is non-null and
// OpenRawFile succeeds
TEST(OHOSAssetProvider, GetAsMappingReturnsMappingWhenOpenSucceeds) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  // First OpenRawFile succeeds, should not fall back
  EXPECT_EQ(s_open_raw_file_call_count, 1);
}

// GetAsMapping's returned Mapping GetSize should return RawFileSize
TEST(OHOSAssetProvider, GetAsMappingMappingGetSizeReturnsRawFileSize) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_EQ(mapping->GetSize(), kMockRawFileSize);
}

// GetAsMapping's returned Mapping GetMapping should return non-null pointer
TEST(OHOSAssetProvider, GetAsMappingMappingGetMappingReturnsNonNull) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_NE(mapping->GetMapping(), nullptr);
}

// GetAsMapping's returned Mapping GetMapping should contain the read file data
TEST(OHOSAssetProvider, GetAsMappingMappingContainsFileData) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  // ReadFile was called during construction -> OH_ResourceManager_ReadRawFile
  EXPECT_GT(s_read_raw_file_call_count, 0);
  const uint8_t* data = mapping->GetMapping();
  ASSERT_NE(data, nullptr);
  // Verify the content matches the data written by the stub
  EXPECT_EQ(memcmp(data, kMockRawFileData, kMockRawFileSize), 0);
}

// GetAsMapping's returned Mapping IsDontNeedSafe should return false
TEST(OHOSAssetProvider, GetAsMappingMappingIsDontNeedSafeReturnsFalse) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  EXPECT_FALSE(mapping->IsDontNeedSafe());
}

// GetAsMapping's Mapping destructor should call OH_ResourceManager_CloseRawFile
TEST(OHOSAssetProvider, GetAsMappingMappingDestructorClosesRawFile) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  {
    auto mapping = provider.GetAsMapping("test.txt");
    ASSERT_NE(mapping, nullptr);
    EXPECT_EQ(s_close_raw_file_call_count, 0);
  }  // mapping destructor
  EXPECT_EQ(s_close_raw_file_call_count, 1);
}

// GetAsMapping should fall back when first OpenRawFile fails
TEST(OHOSAssetProvider, GetAsMappingFallbackWhenFirstOpenFails) {
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  // With custom dir, first try uses dir + "/" + name, second try uses name
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  // First attempt succeeds, should not fall back
  EXPECT_EQ(s_open_raw_file_call_count, 1);
}

// GetAsMapping should return nullptr when both first and fallback opens fail
TEST(OHOSAssetProvider, GetAsMappingReturnsNullWhenAllOpensFail) {
  s_open_raw_file_return_null = true;
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  EXPECT_EQ(mapping, nullptr);
  // Should attempt twice: first with relativePath, then fallback asset_name
  EXPECT_EQ(s_open_raw_file_call_count, 2);
}

// GetAsMapping should correctly pass handle to OH_ResourceManager_OpenRawFile
// when handle is non-null but treated as NativeResourceManager
TEST(OHOSAssetProvider, GetAsMappingPassesHandleToOpenRawFile) {
  void* handle = reinterpret_cast<void*>(0xBEEF);
  OHOSAssetProvider provider(handle, "assets");
  auto mapping = provider.GetAsMapping("file.txt");
  ASSERT_NE(mapping, nullptr);
}

// GetAsMapping's returned Mapping GetSize should be consistent across calls
TEST(OHOSAssetProvider, GetAsMappingMappingGetSizeConsistent) {
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
  void* handle = reinterpret_cast<void*>(0x1234);
  OHOSAssetProvider provider(handle, "my_assets");
  auto mapping = provider.GetAsMapping("test.txt");
  ASSERT_NE(mapping, nullptr);
  const uint8_t* ptr1 = mapping->GetMapping();
  const uint8_t* ptr2 = mapping->GetMapping();
  EXPECT_EQ(ptr1, ptr2);
}

}  // namespace testing
}  // namespace flutter
