/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

namespace flutter {

class OHOSImageGenerator : public ImageGenerator {
 private:
  explicit OHOSImageGenerator(OH_ImageSourceNative* image_source);

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
  SkImageInfo origin_image_info_;
  uint32_t frame_count_ = 0;
  bool is_hdr_ = false;
  std::vector<int32_t> frame_time_duration_;

  struct PixelMapOHOS {
    OH_PixelmapNative* pixelmap_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t row_stride_ = 0;
    int32_t pixel_format_ = 0;

    explicit PixelMapOHOS(OH_PixelmapNative* pixelmap);

    ~PixelMapOHOS() {
      if (pixelmap_) {
        OH_PixelmapNative_Release(pixelmap_);
      }
    };

    bool IsValid() {
      return pixelmap_ != nullptr && width_ != 0 && height_ != 0;
    };

    Image_ErrorCode ReadPixels(uint8_t* dst_buffer,
                               uint32_t buffer_size,
                               uint32_t row_stride);

    PixelMapOHOS(const PixelMapOHOS&) = delete;
    PixelMapOHOS& operator=(const PixelMapOHOS&) = delete;
  };

  std::map<uint32_t, std::shared_ptr<PixelMapOHOS>> cached_pixelmaps_;

  std::shared_ptr<PixelMapOHOS> CreatePixelMap(int width,
                                               int height,
                                               int frame_index);

  bool IsValidImageData();

  FML_DISALLOW_COPY_ASSIGN_AND_MOVE(OHOSImageGenerator);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_IMAGE_GENERATOR_H_