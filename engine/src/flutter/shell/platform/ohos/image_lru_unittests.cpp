/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/image_lru.h"

#include "display_list/image/dl_image.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace flutter {
namespace testing {

namespace {

// Build a default OH_NativeBuffer_Config used by the tests. The struct is a
// plain POD type from <native_buffer/native_buffer.h>; constructing it by
// value does not invoke any NDK runtime function, so the LRU logic can be
// exercised on the device without allocating real native buffers.
OH_NativeBuffer_Config MakeConfig(int32_t width,
                                  int32_t height,
                                  int32_t format = 0) {
  OH_NativeBuffer_Config config;
  config.width = width;
  config.height = height;
  config.format = format;
  config.usage = 0;
  config.stride = 0;
  return config;
}

// Create a DlImage backed by a null SkImage. This mirrors the pattern used by
// the Android image_lru_unittests.cc: the LRU only stores the sk_sp<DlImage>
// pointer and never dereferences the underlying pixels, so a null-backed image
// is sufficient to validate cache behavior.
sk_sp<flutter::DlImage> MakeTestImage() {
  return DlImage::Make(static_cast<SkImage*>(nullptr));
}

}  // namespace

class ImageLruTest : public ::testing::Test {
 protected:
  ImageLRU lru_;
  OH_NativeBuffer_Config default_config_ = MakeConfig(100, 100, 0);
};

// ---------------------------------------------------------------------------
// FindImage
// ---------------------------------------------------------------------------

// Finding with key == 0 must return nullptr (key 0 is treated as invalid).
TEST_F(ImageLruTest, FindImageWithZeroKeyReturnsNull) {
  NativeBufferKey delete_key = 42;
  EXPECT_EQ(lru_.FindImage(0, default_config_, &delete_key), nullptr);
  // delete_key should not be touched when nothing is found.
  EXPECT_EQ(delete_key, 42u);
}

// Finding a missing key returns nullptr.
TEST_F(ImageLruTest, FindImageMissingKeyReturnsNull) {
  NativeBufferKey delete_key = 0;
  EXPECT_EQ(lru_.FindImage(123, default_config_, &delete_key), nullptr);
  EXPECT_EQ(delete_key, 0u);
}

// Finding a missing key with a null delete_key out-param does not crash.
TEST_F(ImageLruTest, FindImageMissingKeyNullDeleteKey) {
  EXPECT_EQ(lru_.FindImage(123, default_config_, nullptr), nullptr);
}

// After AddImage, FindImage returns the same image and the entry is moved to
// the front of the LRU list.
TEST_F(ImageLruTest, FindImageReturnsCachedImage) {
  auto image = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image, default_config_, 1), 0u);

  NativeBufferKey delete_key = 0;
  auto found = lru_.FindImage(1, default_config_, &delete_key);
  EXPECT_EQ(found, image);
  // No entry should be evicted on a cache that is far below kMaxQueueSize.
  EXPECT_EQ(delete_key, 0u);
}

// FindImage returns nullptr when the cached config (width/height/format)
// differs from the query config. This validates the format/size guard.
TEST_F(ImageLruTest, FindImageReturnsNullWhenWidthDiffers) {
  auto image = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image, default_config_, 1), 0u);

  auto other_config = MakeConfig(200, 100, 0);
  EXPECT_EQ(lru_.FindImage(1, other_config, nullptr), nullptr);
}

TEST_F(ImageLruTest, FindImageReturnsNullWhenHeightDiffers) {
  auto image = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image, default_config_, 1), 0u);

  auto other_config = MakeConfig(100, 200, 0);
  EXPECT_EQ(lru_.FindImage(1, other_config, nullptr), nullptr);
}

TEST_F(ImageLruTest, FindImageReturnsNullWhenFormatDiffers) {
  auto image = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image, default_config_, 1), 0u);

  auto other_config = MakeConfig(100, 100, 1);
  EXPECT_EQ(lru_.FindImage(1, other_config, nullptr), nullptr);
}

// FindImage with a matching config updates the timestamp and promotes the
// entry to the front of the LRU list (verified indirectly via eviction
// order in the EvictsLRU test below).
TEST_F(ImageLruTest, FindImagePromotesEntryToFront) {
  auto image = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image, default_config_, 1), 0u);
  ASSERT_EQ(lru_.AddImage(image, default_config_, 2), 0u);
  ASSERT_EQ(lru_.AddImage(image, default_config_, 3), 0u);

  // Access key 1 to promote it to the front; key 2 is now the LRU.
  EXPECT_EQ(lru_.FindImage(1, default_config_, nullptr), image);

  // Fill the cache to kMaxQueueSize; the next AddImage should evict key 2
  // (the least recently used), not key 1.
  for (auto i = 4u; i <= kMaxQueueSize; i++) {
    ASSERT_EQ(lru_.AddImage(image, default_config_, i), 0u)
        << "iteration " << i;
  }
  // Adding one more triggers eviction of the LRU entry (key 2).
  EXPECT_EQ(lru_.AddImage(image, default_config_, kMaxQueueSize + 1), 2u);
}

// ---------------------------------------------------------------------------
// AddImage
// ---------------------------------------------------------------------------

// Adding a new image returns 0 (nothing evicted) when the cache is not full.
TEST_F(ImageLruTest, AddImageReturnsZeroWhenNotFull) {
  auto image = MakeTestImage();
  EXPECT_EQ(lru_.AddImage(image, default_config_, 1), 0u);
}

// Adding an existing key updates the value and config in place and returns 0
// (no eviction). The entry is promoted to the front of the LRU list.
TEST_F(ImageLruTest, AddImageExistingKeyUpdatesInPlace) {
  auto image1 = MakeTestImage();
  auto image2 = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image1, default_config_, 1), 0u);

  auto new_config = MakeConfig(200, 200, 2);
  EXPECT_EQ(lru_.AddImage(image2, new_config, 1), 0u);

  // The cached config should now be the new one; FindImage with the old config
  // returns nullptr, with the new config returns image2.
  EXPECT_EQ(lru_.FindImage(1, default_config_, nullptr), nullptr);
  EXPECT_EQ(lru_.FindImage(1, new_config, nullptr), image2);
}

// Adding more than kMaxQueueSize entries evicts the least recently used one
// and returns its key.
TEST_F(ImageLruTest, AddImageEvictsLRUWhenFull) {
  auto image = MakeTestImage();
  // Fill the cache exactly to kMaxQueueSize; nothing is evicted.
  for (auto i = 1u; i <= kMaxQueueSize; i++) {
    EXPECT_EQ(lru_.AddImage(image, default_config_, i), 0u) << "iteration " << i;
  }
  // Adding one more evicts key 1 (the LRU, since it was inserted first and
  // never accessed).
  EXPECT_EQ(lru_.AddImage(image, default_config_, kMaxQueueSize + 1), 1u);
}

// After eviction, the evicted key is no longer retrievable.
TEST_F(ImageLruTest, EvictedKeyIsNoLongerRetrievable) {
  auto image = MakeTestImage();
  for (auto i = 1u; i <= kMaxQueueSize; i++) {
    ASSERT_EQ(lru_.AddImage(image, default_config_, i), 0u);
  }
  ASSERT_EQ(lru_.AddImage(image, default_config_, kMaxQueueSize + 1), 1u);
  EXPECT_EQ(lru_.FindImage(1, default_config_, nullptr), nullptr);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

// Clear empties the cache: subsequent FindImage calls return nullptr and
// AddImage does not report any evictions from the cleared entries.
TEST_F(ImageLruTest, ClearEmptiesCache) {
  auto image = MakeTestImage();
  for (auto i = 1u; i <= kMaxQueueSize; i++) {
    ASSERT_EQ(lru_.AddImage(image, default_config_, i), 0u);
  }
  lru_.Clear();
  for (auto i = 1u; i <= kMaxQueueSize; i++) {
    EXPECT_EQ(lru_.FindImage(i, default_config_, nullptr), nullptr);
  }
  // After Clear, the cache should be empty so the next AddImage returns 0.
  EXPECT_EQ(lru_.AddImage(image, default_config_, 1), 0u);
}

// Clear on an empty cache is a no-op and does not crash.
TEST_F(ImageLruTest, ClearOnEmptyCacheIsNoop) {
  lru_.Clear();
  EXPECT_EQ(lru_.FindImage(1, default_config_, nullptr), nullptr);
}

// Clear can be called multiple times safely.
TEST_F(ImageLruTest, ClearTwiceIsSafe) {
  auto image = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image, default_config_, 1), 0u);
  lru_.Clear();
  lru_.Clear();
  EXPECT_EQ(lru_.FindImage(1, default_config_, nullptr), nullptr);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

// Adding and immediately finding the same key round-trips the image.
TEST_F(ImageLruTest, AddThenFindRoundTrips) {
  auto image = MakeTestImage();
  ASSERT_EQ(lru_.AddImage(image, default_config_, 42), 0u);
  EXPECT_EQ(lru_.FindImage(42, default_config_, nullptr), image);
}

// Re-adding the same key multiple times keeps the cache size bounded (no
// duplicates).
TEST_F(ImageLruTest, ReaddingSameKeyDoesNotGrowCache) {
  auto image = MakeTestImage();
  for (auto i = 0u; i < kMaxQueueSize + 10; i++) {
    ASSERT_EQ(lru_.AddImage(image, default_config_, 1), 0u) << "iteration " << i;
  }
  // Only one entry should exist; adding a different key should not evict
  // anything because the cache has only 1 entry.
  EXPECT_EQ(lru_.AddImage(image, default_config_, 2), 0u);
  // The original key 1 should still be present.
  EXPECT_EQ(lru_.FindImage(1, default_config_, nullptr), image);
}

}  // namespace testing
}  // namespace flutter
