/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"
#include "flutter/lib/ui/painting/image_generator.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>

#define private public
#include "flutter/shell/platform/ohos/ohos_image_generator.h"
#undef private

#include "gtest/gtest.h"
#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"

namespace flutter {

const unsigned int ImageGenerator::kInfinitePlayCount;

namespace testing {
namespace {

sk_sp<SkData> Bytes(const std::vector<uint8_t>& bytes) {
  return SkData::MakeWithCopy(bytes.data(), bytes.size());
}

}

class OHOSImageGeneratorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saved_cached_bytes_ = OHOSImageGenerator::total_cached_bytes_.load();
    OHOSImageGenerator::total_cached_bytes_.store(0);
  }

  void TearDown() override {
    OHOSImageGenerator::total_cached_bytes_.store(saved_cached_bytes_);
  }

 private:
  size_t saved_cached_bytes_ = 0;
};

TEST_F(OHOSImageGeneratorTest, EmptyDataReturnsNull) {
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(SkData::MakeEmpty()), nullptr);
}

TEST_F(OHOSImageGeneratorTest, GarbageDataReturnsNull) {
  const uint8_t kGarbage[] = {0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kGarbage, sizeof(kGarbage))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, TruncatedSignatureReturnsNull) {
  const uint8_t kTruncated[] = {'G', 'I', 'F'};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kTruncated, sizeof(kTruncated))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, Gif87aSignatureReturnsNull) {
  const uint8_t kGif87a[] = {'G', 'I', 'F', '8', '7', 'a', 0x00, 0x00};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kGif87a, sizeof(kGif87a))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, Gif89aSignatureReturnsNull) {
  const uint8_t kGif89a[] = {'G', 'I', 'F', '8', '9', 'a', 0x00, 0x00};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kGif89a, sizeof(kGif89a))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, StaticPngSignatureReturnsNull) {
  const uint8_t kStaticPng[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n',
                                'z',  'z', 'z', 'z'};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kStaticPng, sizeof(kStaticPng))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, AnimatedPngSignatureReturnsNull) {
  const uint8_t kAnimPng[] = {0x89, 'P',   'N',  'G',  '\r', '\n', 0x1A, '\n',
                              'a',  'c',   'T',  'L',  0x00, 0x00, 0x00, 0x00};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kAnimPng, sizeof(kAnimPng))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, RiffWithoutWebpReturnsNull) {
  const uint8_t kRiff[] = {'R', 'I', 'F', 'F', '0', '0', '0', '0'};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kRiff, sizeof(kRiff))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, WebpWithoutAnimChunkReturnsNull) {
  const uint8_t kStaticWebp[] = {'R', 'I', 'F', 'F', '0', '0', '0', '0',
                                 'W', 'E', 'B', 'P', 'V', 'P', '8', ' '};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kStaticWebp, sizeof(kStaticWebp))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, AnimatedWebpSignatureReturnsNull) {
  const uint8_t kAnimWebp[] = {'R', 'I', 'F', 'F', '0', '0', '0', '0',
                               'W', 'E', 'B', 'P', '0', '0', '0', '0',
                               'A', 'N', 'I', 'M', 0x00, 0x00, 0x00, 0x00};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kAnimWebp, sizeof(kAnimWebp))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, PngHeaderWithDistantAcTLIsRejected) {
  std::vector<uint8_t> bytes = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  bytes.resize(40 * 1024, 0x00);
  const uint8_t kChunk[] = {'a', 'c', 'T', 'L'};
  std::copy(kChunk, kChunk + 4, bytes.begin() + 34 * 1024);
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(Bytes(bytes)), nullptr);
}

TEST_F(OHOSImageGeneratorTest, PixelMapWithNullHandleStaysInvalid) {
  OHOSImageGenerator::PixelMapOHOS pixelmap(nullptr,
                                            IMAGE_ALLOCATOR_TYPE_AUTO);
  EXPECT_EQ(pixelmap.pixelmap_, nullptr);
  EXPECT_FALSE(pixelmap.IsValid());
}

TEST_F(OHOSImageGeneratorTest, PixelMapReadPixelsRejectsNullHandle) {
  OHOSImageGenerator::PixelMapOHOS pixelmap(nullptr,
                                            IMAGE_ALLOCATOR_TYPE_AUTO);
  uint8_t buffer[16];
  EXPECT_EQ(pixelmap.ReadPixels(buffer, sizeof(buffer), 16),
            IMAGE_BAD_PARAMETER);
}

}
}
