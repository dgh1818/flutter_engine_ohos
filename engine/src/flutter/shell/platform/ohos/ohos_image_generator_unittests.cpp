/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_buffer/native_buffer.h>
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
#include "flutter/fml/log_settings.h"
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"
#include "flutter/lib/ui/painting/image_generator.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkRefCnt.h"

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

// Named constants for the values used to build container test data and the
// expected results, so the tests document the wire format semantics.
constexpr unsigned int kBitsPerByte = 8;
constexpr size_t kLeUint32Bytes = 4;
constexpr size_t kGifSignatureSize = 6;  // "GIF89a"
// Wire loop counts. A GIF wire value N means N repeats after the first play
// (N + 1 total plays); a WebP ANIM value is already the total play count.
constexpr uint16_t kInfiniteLoop = 0;
constexpr uint16_t kSingleRepeatWireValue = 1;
constexpr uint16_t kTwoRepeatsWireValue = 2;
constexpr uint16_t kFiveRepeatsWireValue = 5;
constexpr uint16_t kSevenPlaysTotal = 7;
// Expected total play counts.
constexpr int kPlaysOnce = 1;
constexpr int kSingleRepeatTotalPlays = 2;  // wire 1 -> 1 + 1 plays
constexpr int kTwoRepeatsTotalPlays = 3;    // wire 2 -> 2 + 1 plays
constexpr int kFiveRepeatsTotalPlays = 6;   // wire 5 -> 5 + 1 plays
constexpr int kInfinitePlays = 0;
constexpr int kUnknownPlays = -1;
constexpr size_t kTruncatedAnimTailBytes = 3;

sk_sp<SkData> Bytes(const std::vector<uint8_t>& bytes) {
  return SkData::MakeWithCopy(bytes.data(), bytes.size());
}

// Builds a minimal GIF byte stream: the GIF89a header, a placeholder
// logical screen descriptor, and an optional NETSCAPE2.0 / ANIMEXTS1.0
// application extension (a null app_id means no looping extension).
std::vector<uint8_t> MakeGifBytes(const char* app_id, uint16_t loop) {
  std::vector<uint8_t> bytes = {'G', 'I', 'F', '8', '9', 'a'};
  const uint8_t kDescriptor[] = {0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
  bytes.insert(bytes.end(), kDescriptor, kDescriptor + sizeof(kDescriptor));
  if (app_id != nullptr) {
    const uint8_t kExtHeader[] = {0x21, 0xFF, 0x0B};
    bytes.insert(bytes.end(), kExtHeader, kExtHeader + sizeof(kExtHeader));
    while (*app_id != '\0') {
      bytes.push_back(static_cast<uint8_t>(*app_id++));
    }
    bytes.push_back(0x03);
    bytes.push_back(0x01);
    bytes.push_back(static_cast<uint8_t>(loop & 0xFF));
    bytes.push_back(static_cast<uint8_t>(loop >> kBitsPerByte));
    bytes.push_back(0x00);
  }
  bytes.push_back(0x3B);  // trailer
  return bytes;
}

// Inserts a comment extension containing the "NETSCAPE2.0" text as a decoy
// before the real extension, to verify that only a structurally valid
// application extension is recognized.
std::vector<uint8_t> MakeGifWithDecoy() {
  std::vector<uint8_t> bytes = {'G', 'I', 'F', '8', '9', 'a'};
  const char kDecoyText[] = "see NETSCAPE2.0 here";
  bytes.push_back(0x21);
  bytes.push_back(0xFE);  // comment extension
  bytes.push_back(static_cast<uint8_t>(sizeof(kDecoyText) - 1));
  bytes.insert(bytes.end(), kDecoyText, kDecoyText + sizeof(kDecoyText) - 1);
  bytes.push_back(0x00);
  const auto real = MakeGifBytes("NETSCAPE2.0", kTwoRepeatsWireValue);
  bytes.insert(bytes.end(), real.begin() + kGifSignatureSize, real.end());
  return bytes;
}

// Builds a minimal WebP: a RIFF/WEBP container with a VP8X chunk and an
// optional ANIM chunk.
std::vector<uint8_t> MakeWebpBytes(uint16_t loop, bool with_anim = true) {
  auto push_chunk = [](std::vector<uint8_t>& out, const char* tag,
                       const std::vector<uint8_t>& payload) {
    out.insert(out.end(), tag, tag + 4);
    const uint32_t size = static_cast<uint32_t>(payload.size());
    for (size_t i = 0; i < kLeUint32Bytes; ++i) {
      out.push_back(static_cast<uint8_t>((size >> (kBitsPerByte * i)) & 0xFF));
    }
    out.insert(out.end(), payload.begin(), payload.end());
    if (size & 1) {
      out.push_back(0x00);  // RIFF chunks are padded to even sizes
    }
  };

  std::vector<uint8_t> body = {'W', 'E', 'B', 'P'};
  push_chunk(body, "VP8X", {0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0});
  if (with_anim) {
    const std::vector<uint8_t> anim = {
        0,
        0,
        0,
        0,  // background color
        static_cast<uint8_t>(loop & 0xFF),
        static_cast<uint8_t>(loop >> kBitsPerByte)};
    push_chunk(body, "ANIM", anim);
  }
  std::vector<uint8_t> bytes = {'R', 'I', 'F', 'F'};
  const uint32_t size = static_cast<uint32_t>(body.size());
  for (size_t i = 0; i < kLeUint32Bytes; ++i) {
    bytes.push_back(static_cast<uint8_t>((size >> (kBitsPerByte * i)) & 0xFF));
  }
  bytes.insert(bytes.end(), body.begin(), body.end());
  return bytes;
}

sk_sp<SkData> RealPngData() {
  static const uint8_t kPng[] = {
      137, 80, 78,  71, 13, 10, 26,  10, 0,  0,   0,   13, 73,  72,  68,  82,
      0,   0,  0,   2,  0,  0,  0,   2,  8,  6,   0,   0,  0,   114, 182, 13,
      36,  0,  0,   0,  20, 73, 68,  65, 84, 120, 156, 99, 248, 207, 192, 240,
      31,  12, 129, 52, 16, 48, 252, 7,  0,  71,  202, 8,  248, 139, 78,  67,
      133, 0,  0,   0,  0,  73, 69,  78, 68, 174, 66,  96, 130};
  return SkData::MakeWithCopy(kPng, sizeof(kPng));
}

std::shared_ptr<OHOSImageGenerator> MakeRealGenerator() {
  return std::static_pointer_cast<OHOSImageGenerator>(
      OHOSImageGenerator::MakeFromData(RealPngData()));
}

struct FakeDmaPixelMap {
  std::shared_ptr<OHOSImageGenerator::PixelMapOHOS> pm;

  FakeDmaPixelMap(uint32_t width,
                  uint32_t height,
                  int32_t format,
                  uint32_t row_stride,
                  IMAGE_ALLOCATOR_TYPE allocator) {
    auto* raw = new OHOSImageGenerator::PixelMapOHOS(nullptr, allocator);
    static char fake_pixelmap_storage;
    raw->pixelmap_ =
        reinterpret_cast<OH_PixelmapNative*>(&fake_pixelmap_storage);
    raw->width_ = width;
    raw->height_ = height;
    raw->pixel_format_ = format;
    raw->row_stride_ = row_stride;
    pm.reset(raw);
  }

  ~FakeDmaPixelMap() {
    if (pm) {
      pm->pixelmap_ = nullptr;
    }
  }
};

}  // namespace

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

TEST_F(OHOSImageGeneratorTest, InvalidEncodedDataDoesNotCrash) {
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(SkData::MakeWithCopy("xxxx", 4)),
            nullptr);
  auto generator = MakeRealGenerator();
  if (generator) {
    uint8_t buffer[16];
    SkImageInfo bgra =
        SkImageInfo::Make(2, 2, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    EXPECT_FALSE(generator->GetPixels(bgra, buffer, 8, 0, std::nullopt));
    OH_ImageSourceNative* real_source = generator->image_source_;
    generator->image_source_ = nullptr;
    SkImageInfo info =
        SkImageInfo::Make(2, 2, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    EXPECT_FALSE(generator->GetPixels(info, buffer, 8, 0, std::nullopt));
    generator->image_source_ = real_source;
  }
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
  const uint8_t kStaticPng[] = {0x89, 'P',  'N', 'G', '\r', '\n',
                                0x1A, '\n', 'z', 'z', 'z',  'z'};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kStaticPng, sizeof(kStaticPng))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, AnimatedPngSignatureReturnsNull) {
  const uint8_t kAnimPng[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n',
                              'a',  'c', 'T', 'L', 0x00, 0x00, 0x00, 0x00};
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
  const uint8_t kAnimWebp[] = {'R', 'I', 'F', 'F', '0',  '0',  '0',  '0',
                               'W', 'E', 'B', 'P', '0',  '0',  '0',  '0',
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
  OHOSImageGenerator::PixelMapOHOS pixelmap(nullptr, IMAGE_ALLOCATOR_TYPE_AUTO);
  EXPECT_EQ(pixelmap.pixelmap_, nullptr);
  EXPECT_FALSE(pixelmap.IsValid());
}

TEST_F(OHOSImageGeneratorTest, PixelMapIsValidRequiresNonZeroSize) {
  OHOSImageGenerator::PixelMapOHOS pixelmap(nullptr, IMAGE_ALLOCATOR_TYPE_AUTO);
  pixelmap.pixelmap_ = reinterpret_cast<OH_PixelmapNative*>(0x1);
  pixelmap.width_ = 0;
  pixelmap.height_ = 1;
  EXPECT_FALSE(pixelmap.IsValid());
  pixelmap.width_ = 1;
  pixelmap.height_ = 0;
  EXPECT_FALSE(pixelmap.IsValid());
  pixelmap.width_ = 1;
  pixelmap.height_ = 1;
  EXPECT_TRUE(pixelmap.IsValid());
  pixelmap.pixelmap_ = nullptr;
}

TEST_F(OHOSImageGeneratorTest, MakeFromDataNullBytesReturnsNull) {
  auto null_bytes = SkData::MakeWithoutCopy(nullptr, 8);
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(null_bytes), nullptr);
  const uint8_t kTiny[] = {0x01};
  EXPECT_EQ(OHOSImageGenerator::MakeFromData(
                SkData::MakeWithCopy(kTiny, sizeof(kTiny))),
            nullptr);
}

TEST_F(OHOSImageGeneratorTest, PixelMapReadPixelsRejectsNullHandle) {
  OHOSImageGenerator::PixelMapOHOS pixelmap(nullptr, IMAGE_ALLOCATOR_TYPE_AUTO);
  uint8_t buffer[16];
  EXPECT_EQ(pixelmap.ReadPixels(buffer, sizeof(buffer), 16),
            IMAGE_BAD_PARAMETER);
}

TEST_F(OHOSImageGeneratorTest, RealPngMetadata) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  EXPECT_EQ(generator->GetInfo().width(), 2);
  EXPECT_EQ(generator->GetInfo().height(), 2);
  EXPECT_EQ(generator->GetFrameCount(), 1u);
  EXPECT_EQ(generator->GetPlayCount(), 1u);
  EXPECT_TRUE(generator->IsValidImageData());
  const SkISize half = generator->GetScaledDimensions(0.5f);
  EXPECT_EQ(half.width(), 1);
  EXPECT_EQ(half.height(), 1);
}

TEST_F(OHOSImageGeneratorTest, GetPixelsRejectsNonRgbaColorType) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  uint8_t buffer[16];
  SkImageInfo bgra =
      SkImageInfo::Make(2, 2, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
  EXPECT_FALSE(generator->GetPixels(bgra, buffer, 8, 0, std::nullopt));
}

TEST_F(OHOSImageGeneratorTest, GetPixelsNullSourceReturnsFalse) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  OH_ImageSourceNative* real_source = generator->image_source_;
  generator->image_source_ = nullptr;
  uint8_t buffer[16];
  SkImageInfo info =
      SkImageInfo::Make(2, 2, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  EXPECT_FALSE(generator->GetPixels(info, buffer, 8, 0, std::nullopt));
  generator->image_source_ = real_source;
}

TEST_F(OHOSImageGeneratorTest, GetPixelsDecodesCachesAndRepeats) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  uint8_t buffer[16] = {0};
  SkImageInfo info =
      SkImageInfo::Make(2, 2, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    ASSERT_TRUE(generator->GetPixels(info, buffer, 8, 0, std::nullopt));
  }
  EXPECT_EQ(buffer[0], 255);
  EXPECT_EQ(buffer[1], 0);
  EXPECT_EQ(buffer[2], 0);
  EXPECT_EQ(buffer[3], 255);
  EXPECT_EQ(buffer[12], 255);
  EXPECT_EQ(buffer[13], 255);
  EXPECT_EQ(buffer[14], 0);
  EXPECT_EQ(buffer[15], 255);

  memset(buffer, 0, sizeof(buffer));
  EXPECT_TRUE(generator->GetPixels(info, buffer, 8, 0, std::nullopt));
  EXPECT_EQ(buffer[0], 255);
  ASSERT_EQ(generator->cached_pixelmaps_.size(), 1u);
  EXPECT_NE(generator->cached_pixelmaps_[0], nullptr);
  EXPECT_EQ(OHOSImageGenerator::total_cached_bytes_.load(), 2u * 16u);
}

TEST_F(OHOSImageGeneratorTest, GetColorSpacePrecedence) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  EXPECT_EQ(generator->GetColorSpace(0), 0u);
  generator->cached_colorspaces_[0] = 77;
  EXPECT_EQ(generator->GetColorSpace(0), 77u);
  generator->cached_colorspaces_.erase(0);
  uint8_t buffer[16];
  SkImageInfo info =
      SkImageInfo::Make(2, 2, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  ASSERT_TRUE(generator->GetPixels(info, buffer, 8, 0, std::nullopt));
  auto it = generator->cached_pixelmaps_.find(0);
  ASSERT_NE(it, generator->cached_pixelmaps_.end());
  EXPECT_EQ(generator->GetColorSpace(0), it->second->color_space_);
}

TEST_F(OHOSImageGeneratorTest, CreatePixelMapPrefersDmaWithFallback) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  auto pixelmap = generator->CreatePixelMap(2, 2, 0, true);
  ASSERT_NE(pixelmap, nullptr);
  EXPECT_EQ(pixelmap->width_, 2u);
  EXPECT_EQ(pixelmap->height_, 2u);
}

TEST_F(OHOSImageGeneratorTest, CanCreateDmaPixelMapGates) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  const SkISize dims = SkISize::Make(2, 2);
  EXPECT_FALSE(
      generator->CanCreateDmaPixelMap(SkISize::Make(0, 0), std::nullopt));
  EXPECT_FALSE(generator->CanCreateDmaPixelMap(dims, 1u));
  generator->unsupported_dma_encoded_data_ = true;
  EXPECT_FALSE(generator->CanCreateDmaPixelMap(dims, std::nullopt));
  generator->unsupported_dma_encoded_data_ = false;
  generator->frame_count_ = 2;
  EXPECT_FALSE(generator->CanCreateDmaPixelMap(dims, std::nullopt));
  generator->frame_count_ = 1;
  generator->is_hdr_ = true;
  EXPECT_FALSE(generator->CanCreateDmaPixelMap(dims, std::nullopt));
  generator->is_hdr_ = false;
  EXPECT_NO_FATAL_FAILURE(generator->CanCreateDmaPixelMap(dims, std::nullopt));
}

TEST_F(OHOSImageGeneratorTest, IsValidDmaPixelMapMatrix) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  const SkISize dims = {2, 2};
  EXPECT_FALSE(generator->IsValidDmaPixelMap(nullptr, dims));
  FakeDmaPixelMap zero(0, 2, PIXEL_FORMAT_RGBA_8888, 8,
                       IMAGE_ALLOCATOR_TYPE_DMA);
  EXPECT_FALSE(generator->IsValidDmaPixelMap(zero.pm, dims));
  FakeDmaPixelMap shared(2, 2, PIXEL_FORMAT_RGBA_8888, 8,
                         IMAGE_ALLOCATOR_TYPE_SHARE_MEMORY);
  EXPECT_FALSE(generator->IsValidDmaPixelMap(shared.pm, dims));
  FakeDmaPixelMap wrong_size(4, 4, PIXEL_FORMAT_RGBA_8888, 16,
                             IMAGE_ALLOCATOR_TYPE_DMA);
  EXPECT_FALSE(generator->IsValidDmaPixelMap(wrong_size.pm, dims));
  FakeDmaPixelMap rgb565(2, 2, PIXEL_FORMAT_RGB_565, 8,
                         IMAGE_ALLOCATOR_TYPE_DMA);
  EXPECT_FALSE(generator->IsValidDmaPixelMap(rgb565.pm, dims));
  FakeDmaPixelMap tight(2, 2, PIXEL_FORMAT_RGBA_8888, 4,
                        IMAGE_ALLOCATOR_TYPE_DMA);
  EXPECT_FALSE(generator->IsValidDmaPixelMap(tight.pm, dims));
  FakeDmaPixelMap valid(2, 2, PIXEL_FORMAT_RGBA_8888, 8,
                        IMAGE_ALLOCATOR_TYPE_DMA);
  EXPECT_TRUE(generator->IsValidDmaPixelMap(valid.pm, dims));
}

TEST_F(OHOSImageGeneratorTest, ReadPixelsRejectsShortRowStride) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  FakeDmaPixelMap short_stride(2, 2, PIXEL_FORMAT_RGBA_8888, 4,
                               IMAGE_ALLOCATOR_TYPE_DMA);
  uint8_t buffer[16];
  EXPECT_EQ(short_stride.pm->ReadPixels(buffer, sizeof(buffer), 4),
            IMAGE_BAD_PARAMETER);
}

TEST_F(OHOSImageGeneratorTest, GetPlayCountAndFrameInfo) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  EXPECT_EQ(generator->GetPlayCount(), 1u);
  generator->frame_count_ = 3;
  EXPECT_EQ(generator->GetPlayCount(), ImageGenerator::kInfinitePlayCount);
  generator->play_count_ = kTwoRepeatsTotalPlays;
  EXPECT_EQ(generator->GetPlayCount(), 3u);
  generator->play_count_ = kSingleRepeatTotalPlays;
  EXPECT_EQ(generator->GetPlayCount(), 2u);
  generator->play_count_ = kInfinitePlays;
  EXPECT_EQ(generator->GetPlayCount(), ImageGenerator::kInfinitePlayCount);
  generator->play_count_ = kUnknownPlays;
  EXPECT_EQ(generator->GetPlayCount(), ImageGenerator::kInfinitePlayCount);
  generator->play_count_ = kTwoRepeatsTotalPlays;
  generator->frame_count_ = 1;
  EXPECT_EQ(generator->GetPlayCount(), 1u);

  generator->frame_time_duration_ = {16, -5};
  EXPECT_EQ(generator->GetFrameInfo(0).duration, 16u);
  EXPECT_EQ(generator->GetFrameInfo(1).duration, 0u);
  EXPECT_EQ(generator->GetFrameInfo(9).duration, 0u);
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    auto again = MakeRealGenerator();
    ASSERT_NE(again, nullptr);
    EXPECT_EQ(again->GetPlayCount(), 1u);
  }
}

TEST_F(OHOSImageGeneratorTest, GetScaledDimensionsAndToString) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  EXPECT_EQ(generator->GetScaledDimensions(0.0f), SkISize::Make(0, 0));
  EXPECT_EQ(generator->GetScaledDimensions(2.0f), SkISize::Make(4, 4));
  EXPECT_FALSE(generator->to_string().empty());
  generator->frame_time_duration_ = {16};
  EXPECT_NE(generator->to_string().find("16"), std::string::npos);
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    auto again = MakeRealGenerator();
    ASSERT_NE(again, nullptr);
    EXPECT_FALSE(again->to_string().empty());
  }
}

TEST_F(OHOSImageGeneratorTest, CreatePixelMapWithRotateDegrees) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  for (float deg : {0.f, 90.f, 180.f, 270.f}) {
    generator->rotate_degree_ = deg;
    auto pixelmap = generator->CreatePixelMap(2, 2, 0, false);
    ASSERT_NE(pixelmap, nullptr);
    EXPECT_EQ(pixelmap->width_, 2u);
    EXPECT_EQ(pixelmap->height_, 2u);
  }
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    generator->rotate_degree_ = 90.f;
    auto pixelmap = generator->CreatePixelMap(2, 2, 0, false);
    ASSERT_NE(pixelmap, nullptr);
  }
  generator->rotate_degree_ = 0.f;
  generator->need_flip_ = true;
  auto flipped = generator->CreatePixelMap(2, 2, 0, false);
  ASSERT_NE(flipped, nullptr);
  generator->need_flip_ = false;
}

TEST_F(OHOSImageGeneratorTest, CreateDmaPixelMapHdrAndNullLoader) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  generator->is_hdr_ = true;
  OH_PixelmapNative* pixelmap = nullptr;
  IMAGE_ALLOCATOR_TYPE allocator = IMAGE_ALLOCATOR_TYPE_AUTO;
  Image_ErrorCode err = IMAGE_SUCCESS;
  EXPECT_FALSE(
      generator->CreateDmaPixelMap(nullptr, &pixelmap, &allocator, &err));
  EXPECT_EQ(generator->CreatePixelMap(2, 2, 0, true), nullptr);
  generator->is_hdr_ = false;
}

TEST_F(OHOSImageGeneratorTest, GetPixelsUnknownFrameReturnsFalse) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  uint8_t buffer[16];
  SkImageInfo info =
      SkImageInfo::Make(2, 2, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  EXPECT_FALSE(generator->GetPixels(info, buffer, 8, 99, std::nullopt));
}

TEST_F(OHOSImageGeneratorTest, DmaHelpersRejectInvalidInput) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    generator->unsupported_dma_encoded_data_ = true;
    EXPECT_FALSE(
        generator->CanCreateDmaPixelMap(SkISize::Make(2, 2), std::nullopt));
    generator->unsupported_dma_encoded_data_ = false;
    generator->frame_count_ = 3;
    EXPECT_FALSE(
        generator->CanCreateDmaPixelMap(SkISize::Make(2, 2), std::nullopt));
    generator->frame_count_ = 1;
    EXPECT_FALSE(generator->CanCreateDmaPixelMap(SkISize::Make(2, 2), 0u));
    EXPECT_FALSE(
        generator->CanCreateDmaPixelMap(SkISize::Make(0, 0), std::nullopt));
    OH_ImageSourceNative* saved = generator->image_source_;
    generator->image_source_ = nullptr;
    EXPECT_FALSE(
        generator->CanCreateDmaPixelMap(SkISize::Make(2, 2), std::nullopt));
    generator->image_source_ = saved;
    generator->is_hdr_ = true;
    EXPECT_FALSE(
        generator->CanCreateDmaPixelMap(SkISize::Make(2, 2), std::nullopt));
    generator->is_hdr_ = false;
  }

  auto pixelmap = std::make_shared<OHOSImageGenerator::PixelMapOHOS>(
      nullptr, IMAGE_ALLOCATOR_TYPE_AUTO);
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    EXPECT_FALSE(generator->IsValidDmaPixelMap(nullptr, SkISize::Make(2, 2)));
    EXPECT_FALSE(generator->IsValidDmaPixelMap(pixelmap, SkISize::Make(2, 2)));
  }
  EXPECT_FALSE(generator->IsValidDmaPixelMap(nullptr, SkISize::Make(2, 2)));
  EXPECT_FALSE(generator->IsValidDmaPixelMap(pixelmap, SkISize::Make(2, 2)));
  pixelmap->pixelmap_ = reinterpret_cast<OH_PixelmapNative*>(0x1);
  pixelmap->width_ = 2;
  pixelmap->height_ = 2;
  pixelmap->allocator_type_ = IMAGE_ALLOCATOR_TYPE_AUTO;
  EXPECT_FALSE(generator->IsValidDmaPixelMap(pixelmap, SkISize::Make(2, 2)));
  pixelmap->allocator_type_ = IMAGE_ALLOCATOR_TYPE_DMA;
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    EXPECT_FALSE(generator->IsValidDmaPixelMap(pixelmap, SkISize::Make(4, 4)));
    pixelmap->pixel_format_ = 0;
    pixelmap->row_stride_ = 8;
    EXPECT_FALSE(generator->IsValidDmaPixelMap(pixelmap, SkISize::Make(2, 2)));
    pixelmap->pixel_format_ = PIXEL_FORMAT_RGBA_8888;
    pixelmap->row_stride_ = 1;
    EXPECT_FALSE(generator->IsValidDmaPixelMap(pixelmap, SkISize::Make(2, 2)));
  }
  pixelmap->row_stride_ = 8;
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    EXPECT_TRUE(generator->IsValidDmaPixelMap(pixelmap, SkISize::Make(2, 2)));
    generator->LogAcceptedDmaPixelMap(pixelmap);
  }
  pixelmap->pixelmap_ = nullptr;
}

TEST_F(OHOSImageGeneratorTest, CreateExternalTextureSourceConsistency) {
  auto generator = MakeRealGenerator();
  if (!generator) {
    GTEST_SKIP() << "decoding needs the real image framework (device)";
  }
  EXPECT_EQ(generator->CreateExternalTextureSource(SkISize::Make(0, 0), 0,
                                                   std::nullopt),
            nullptr);
  const bool can =
      generator->CanCreateDmaPixelMap(SkISize::Make(2, 2), std::nullopt);
  if (!can) {
    EXPECT_EQ(generator->CreateExternalTextureSource(SkISize::Make(2, 2), 0,
                                                     std::nullopt),
              nullptr);
  } else {
    EXPECT_NO_FATAL_FAILURE(generator->CreateExternalTextureSource(
        SkISize::Make(2, 2), 0, std::nullopt));
  }
}

// ===== Animation loop count parsing (pure functions, no device required) =====

TEST_F(OHOSImageGeneratorTest, ParseEncodedPlayCountGifScenarios) {
  // A NETSCAPE2.0 wire loop count N means "repeat N times after the first
  // play", so the total play count is N + 1.
  const auto netscape_two_repeats =
      Bytes(MakeGifBytes("NETSCAPE2.0", kTwoRepeatsWireValue));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(netscape_two_repeats),
            kTwoRepeatsTotalPlays);
  const auto netscape_single_repeat =
      Bytes(MakeGifBytes("NETSCAPE2.0", kSingleRepeatWireValue));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(netscape_single_repeat),
            kSingleRepeatTotalPlays);
  // A loop count of 0 means infinite.
  const auto netscape_infinite =
      Bytes(MakeGifBytes("NETSCAPE2.0", kInfiniteLoop));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(netscape_infinite),
            kInfinitePlays);
  // Without a looping extension every frame plays exactly once (matching the
  // Skia path).
  const auto no_extension = Bytes(MakeGifBytes(nullptr, 0));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(no_extension),
            kPlaysOnce);
  // ANIMEXTS1.0 is an equivalent alternative application identifier.
  const auto animexts_two_repeats =
      Bytes(MakeGifBytes("ANIMEXTS1.0", kTwoRepeatsWireValue));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(animexts_two_repeats),
            kTwoRepeatsTotalPlays);
  // Truncated data with only the file signature is treated as having no
  // extension.
  const auto signature_only =
      Bytes(std::vector<uint8_t>{'G', 'I', 'F', '8', '9', 'a'});
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(signature_only),
            kPlaysOnce);
  // The "NETSCAPE2.0" text inside a comment extension must not be mistaken
  // for a looping extension.
  const auto decoy = Bytes(MakeGifWithDecoy());
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(decoy),
            kTwoRepeatsTotalPlays);
}

TEST_F(OHOSImageGeneratorTest, ParseEncodedPlayCountWebpScenarios) {
  // The WebP ANIM loop count is the total play count (0 = infinite).
  const auto webp_two_repeats = Bytes(MakeWebpBytes(kTwoRepeatsWireValue));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(webp_two_repeats),
            kTwoRepeatsWireValue);
  const auto webp_single_repeat = Bytes(MakeWebpBytes(kSingleRepeatWireValue));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(webp_single_repeat),
            kSingleRepeatWireValue);
  const auto webp_infinite = Bytes(MakeWebpBytes(kInfiniteLoop));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(webp_infinite),
            kInfinitePlays);
  // A static WebP without an ANIM chunk is not animated.
  const auto static_webp = Bytes(MakeWebpBytes(kTwoRepeatsWireValue, false));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(static_webp),
            kUnknownPlays);
  // A truncated ANIM chunk is treated as unknown.
  auto truncated = MakeWebpBytes(kTwoRepeatsWireValue);
  truncated.resize(truncated.size() - kTruncatedAnimTailBytes);
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(Bytes(truncated)),
            kUnknownPlays);
  // Non-animated format (PNG signature).
  const uint8_t kPng[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  const auto png_bytes = Bytes(std::vector<uint8_t>(kPng, kPng + sizeof(kPng)));
  EXPECT_EQ(OHOSImageGenerator::ParseEncodedPlayCount(png_bytes),
            kUnknownPlays);
}

// Directly exercise the two format-specific parsers (exposed as members for
// testing).
TEST_F(OHOSImageGeneratorTest, ParseGifAndWebpLoopCountDirect) {
  const auto gif_five_repeats =
      Bytes(MakeGifBytes("NETSCAPE2.0", kFiveRepeatsWireValue));
  EXPECT_EQ(OHOSImageGenerator::ParseGifLoopCount(gif_five_repeats),
            kFiveRepeatsTotalPlays);
  const auto webp_seven_plays = Bytes(MakeWebpBytes(kSevenPlaysTotal));
  EXPECT_EQ(OHOSImageGenerator::ParseWebpAnimLoopCount(webp_seven_plays),
            kSevenPlaysTotal);
}

}  // namespace testing
}  // namespace flutter
