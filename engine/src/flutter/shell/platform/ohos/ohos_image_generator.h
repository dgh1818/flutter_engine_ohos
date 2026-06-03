/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_IMAGE_GENERATOR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_IMAGE_GENERATOR_H_

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <sstream>
#include <vector>
#include "flutter/lib/ui/painting/image_generator.h"
#include "include/core/SkRefCnt.h"

#define RBGA8888_BYTES 4

// 定义最大缓存大小为1GB
const int IMAGE_MAX_CACHE_SIZE = 1024 * 1024 * 1024;  // 1GB

namespace flutter {

class OHOSImageGenerator : public ImageGenerator {
 public:
  /// Lifetime-managed wrapper around an `OH_PixelmapNative*` produced by the
  /// platform image source. Exposed publicly so the external-texture helpers
  /// in `ohos_image_generator.cpp` can hold strong references to a decoded
  /// DMA pixelmap.
  struct PixelMapOHOS {
    OH_PixelmapNative* pixelmap_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t row_stride_ = 0;
    int32_t pixel_format_ = 0;
    uint32_t color_space_ = 0;
    IMAGE_ALLOCATOR_TYPE allocator_type_ = IMAGE_ALLOCATOR_TYPE_AUTO;

    PixelMapOHOS(OH_PixelmapNative* pixelmap,
                 IMAGE_ALLOCATOR_TYPE allocator_type);

    ~PixelMapOHOS() {
      if (pixelmap_) {
        OH_PixelmapNative_Release(pixelmap_);
      }
    };

    void setColorSpace(uint32_t colorSpace) { color_space_ = colorSpace; }

    bool IsValid() const {
      return pixelmap_ != nullptr && width_ != 0 && height_ != 0;
    };

    Image_ErrorCode ReadPixels(uint8_t* dst_buffer,
                               size_t buffer_size,
                               size_t row_stride);

    PixelMapOHOS(const PixelMapOHOS&) = delete;
    PixelMapOHOS& operator=(const PixelMapOHOS&) = delete;
  };

 private:
  explicit OHOSImageGenerator(OH_ImageSourceNative* image_source,
                              const sk_sp<SkData>& data,
                              bool unsupportedDmaEncodedData);

 public:
  ~OHOSImageGenerator();

  const SkImageInfo& GetInfo() override;

  unsigned int GetFrameCount() const override;

  unsigned int GetPlayCount() const override;

  const ImageGenerator::FrameInfo GetFrameInfo(
      unsigned int frame_index) override;

  SkISize GetScaledDimensions(float desired_scale) override;

  bool GetPixels(const SkImageInfo& info,
                 void* pixels,
                 size_t row_bytes,
                 unsigned int frame_index,
                 std::optional<unsigned int> prior_frame) override;
  
  uint32_t GetColorSpace(unsigned int frame_index) override;

#if defined(FML_OS_OHOS) && IMPELLER_SUPPORTS_RENDERING
  std::unique_ptr<ExternalTextureSource> CreateExternalTextureSource(
      const SkISize& decode_dimensions,
      unsigned int frame_index,
      std::optional<unsigned int> prior_frame) override;
#endif  // FML_OS_OHOS && IMPELLER_SUPPORTS_RENDERING

  static std::shared_ptr<ImageGenerator> MakeFromData(sk_sp<SkData> data);

  std::string to_string() const {
    std::ostringstream oss;
    oss << "ImageGenerate-" << reinterpret_cast<const void*>(this) << ":(size-"
        << origin_image_info_.width() << "*" << origin_image_info_.height()
        << ",frame_count-" << frame_count_ << "-duration-"
        << (frame_time_duration_.size() == 0 ? 0 : frame_time_duration_[0])
        << ",isHdr-" << is_hdr_ << ")";
    return oss.str();
  }

 private:
  OH_ImageSourceNative* image_source_;
  const sk_sp<SkData> data_;

  SkImageInfo origin_image_info_;
  float rotate_degree_ = 0.f;
  bool need_flip_ = false;
  uint32_t frame_count_ = 0;
  bool is_hdr_ = false;
  bool unsupported_dma_encoded_data_ = false;
  std::vector<int32_t> frame_time_duration_;

  // 静态缓存计数和最大缓存限制声明
  static std::atomic<size_t> total_cached_bytes_;
  // 最大全局缓存大小
  static constexpr size_t kMaxGlobalCacheSize = IMAGE_MAX_CACHE_SIZE;

  std::map<uint32_t, std::shared_ptr<PixelMapOHOS>> cached_pixelmaps_;
  std::map<uint32_t, uint32_t> cached_colorspaces_;

#if defined(FML_OS_OHOS) && IMPELLER_SUPPORTS_RENDERING
  bool CanCreateDmaPixelMap(
      const SkISize& decodeDimensions,
      std::optional<unsigned int> priorFrame) const;

  bool IsValidDmaPixelMap(const std::shared_ptr<PixelMapOHOS>& pixelmap,
                          const SkISize& decodeDimensions) const;

  void LogAcceptedDmaPixelMap(
      const std::shared_ptr<PixelMapOHOS>& pixelmap) const;
#endif  // FML_OS_OHOS && IMPELLER_SUPPORTS_RENDERING

  std::shared_ptr<PixelMapOHOS> CreatePixelMap(int width,
                                               int height,
                                               int frame_index,
                                               bool preferDma = false);

  bool CreateDmaPixelMap(OH_DecodingOptions* opts,
                         OH_PixelmapNative** pixelmap,
                         IMAGE_ALLOCATOR_TYPE* actualAllocator,
                         Image_ErrorCode* errCode);

  std::shared_ptr<PixelMapOHOS> AdoptPixelMap(
      OH_PixelmapNative* pixelmap,
      IMAGE_ALLOCATOR_TYPE actualAllocator,
      int frameIndex,
      bool preferDma);

  bool IsValidImageData();

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(OHOSImageGenerator);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_IMAGE_GENERATOR_H_
