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

#include "ohos_image_generator.h"

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <cstdint>
#include <memory>
#include <string>

#include <multimedia/image_framework/image_pixel_map_napi.h>
#include "fml/logging.h"
#include "fml/trace_event.h"
#include "include/core/SkAlphaType.h"
#include "include/core/SkColorType.h"
#include "include/core/SkImageInfo.h"
#include "third_party/skia/include/codec/SkCodecAnimation.h"

namespace flutter {

OHOSImageGenerator::OHOSImageGenerator(OH_ImageSourceNative* image_source)
    : image_source_(image_source) {
  OH_ImageSource_Info* info = nullptr;
  OH_ImageSourceInfo_Create(&info);
  if (info == nullptr) {
    return;
  }
  uint32_t width = 0, height = 0;
  OH_ImageSourceNative_GetImageInfo(image_source, 0, info);
  OH_ImageSourceInfo_GetWidth(info, &width);
  OH_ImageSourceInfo_GetHeight(info, &height);
  OH_ImageSourceInfo_GetDynamicRange(info, &is_hdr_);
  OH_ImageSourceInfo_Release(info);
  origin_image_info_ = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType,
                                         kOpaque_SkAlphaType);

  // this is used for gif.
  OH_ImageSourceNative_GetFrameCount(image_source, &frame_count_);
  if (frame_count_ > 1) {
    int* temp_delay_time = new int[frame_count_];
    OH_ImageSourceNative_GetDelayTimeList(image_source, temp_delay_time,
                                          frame_count_);
    frame_time_duration_.reserve(frame_count_);
    frame_time_duration_.assign(temp_delay_time,
                                temp_delay_time + frame_count_);
    FML_LOG(INFO) << "Create animated image generator frame:" << frame_count_
                  << " duration:" << temp_delay_time[0];
    delete[] temp_delay_time;
  }
}

OHOSImageGenerator::~OHOSImageGenerator() {
  if (image_source_) {
    OH_ImageSourceNative_Release(image_source_);
  }
}

const SkImageInfo& OHOSImageGenerator::GetInfo() {
  std::string trace_str = "Image-GetInfo-" + to_string();
  TRACE_EVENT0("flutter", trace_str.c_str());
  return origin_image_info_;
}

unsigned int OHOSImageGenerator::GetFrameCount() const {
  return frame_count_;
}

unsigned int OHOSImageGenerator::GetPlayCount() const {
  return frame_count_ == 1 ? 1 : kInfinitePlayCount;
}

const ImageGenerator::FrameInfo OHOSImageGenerator::GetFrameInfo(
    unsigned int frame_index) {
  int32_t frame_duration = 0;
  if (frame_index < frame_time_duration_.size()) {
    frame_duration = frame_time_duration_[frame_index];
  }
  if (frame_duration < 0) {
    frame_duration = 0;
  }
  return {.required_frame = std::nullopt,
          .duration = (uint32_t)frame_duration,
          .disposal_method = SkCodecAnimation::DisposalMethod::kKeep};
}

SkISize OHOSImageGenerator::GetScaledDimensions(float desired_scale) {
  // OHOS's PixelMap has the ability to automatically resize, and it will always
  // return the requested size.
  return {int(GetInfo().width() * desired_scale),
          int(GetInfo().height() * desired_scale)};
}

bool OHOSImageGenerator::GetPixels(const SkImageInfo& info,
                                   void* pixels,
                                   size_t row_bytes,
                                   unsigned int frame_index,
                                   std::optional<unsigned int> prior_frame) {
  std::string trace_str = "GetPixelsOHOS-" + to_string() + "->" +
                          std::to_string(info.width()) + "*" +
                          std::to_string(info.height()) +
                          "-index:" + std::to_string(frame_index);
  TRACE_EVENT0("flutter", trace_str.c_str());
  if (frame_index == 0) {
    FML_DLOG(INFO) << trace_str;
  }
  if (image_source_ == nullptr || info.colorType() != kRGBA_8888_SkColorType) {
    FML_LOG(ERROR) << "invailed color type:" << std::to_string(info.colorType())
                   << " " << to_string();
    return false;
  }

  std::shared_ptr<PixelMapOHOS> image_pixelmap = nullptr;
  if (cached_pixelmaps_.find(frame_index) != cached_pixelmaps_.end()) {
    // find pixelmap from the cache.
    image_pixelmap = cached_pixelmaps_[frame_index];
    if ((int)image_pixelmap->width_ != info.width() ||
        (int)image_pixelmap->height_ != info.height()) {
      // this cannot happen.
      image_pixelmap = nullptr;
      FML_LOG(WARNING) << "get changed size:"
                       << std::to_string(image_pixelmap->width_) << "*"
                       << std::to_string(image_pixelmap->height_) << "->"
                       << std::to_string(info.width()) << "*"
                       << std::to_string(info.height());
    }
  }

  if (image_pixelmap == nullptr) {
    image_pixelmap = CreatePixelMap(info.width(), info.height(), frame_index);
  }

  if (image_pixelmap) {
    uint32_t buffer_size =
        image_pixelmap->width_ * image_pixelmap->height_ * RBGA8888_BYTES;
    std::string trace_str = "ReadPixels-size:" + std::to_string(buffer_size) +
                            "-stride:" + std::to_string(row_bytes);
    TRACE_EVENT0("flutter", trace_str.c_str());
    if (frame_index == 0) {
      FML_DLOG(INFO) << trace_str;
    }
    Image_ErrorCode err_code =
        image_pixelmap->ReadPixels((uint8_t*)pixels, buffer_size, row_bytes);
    if (err_code != IMAGE_SUCCESS) {
      FML_LOG(ERROR) << "Pixelmap ReadPixels failed:" << err_code << " "
                     << to_string();
      return false;
    }
    if (image_pixelmap && frame_count_ > 1) {
      // Cache animated images to improve performance.
      cached_pixelmaps_[frame_index] = image_pixelmap;
    }
    return true;
  } else {
    return false;
  }
}

std::shared_ptr<ImageGenerator> OHOSImageGenerator::MakeFromData(
    sk_sp<SkData> data) {
  // Return directly if the image data is empty.
  if (!data->data() || !data->size()) {
    return nullptr;
  }
  std::string trace_str = "MakeFromDataOHOS-" + std::to_string(data->size());
  TRACE_EVENT0("flutter", trace_str.c_str());

  OH_ImageSourceNative* image_source = nullptr;
  // The data will be coyied to ImageSourceNative.
  // No modifications will be made to origin data.
  Image_ErrorCode err_code = OH_ImageSourceNative_CreateFromData(
      (uint8_t*)data->bytes(), data->size(), &image_source);

  if (err_code != IMAGE_SUCCESS || image_source == nullptr) {
    FML_LOG(ERROR) << "Create ImageSource failed: " << err_code;
    return nullptr;
  }

  std::shared_ptr<OHOSImageGenerator> generator(
      new OHOSImageGenerator(image_source));

  if (generator->IsValidImageData()) {
    return generator;
  } else {
    FML_LOG(ERROR) << "Invalid ImageData:" << generator->to_string();
    return nullptr;
  }
}

std::shared_ptr<OHOSImageGenerator::PixelMapOHOS>
OHOSImageGenerator::CreatePixelMap(int width, int height, int frame_index) {
  OH_DecodingOptions* opts = nullptr;
  Image_ErrorCode err_code = OH_DecodingOptions_Create(&opts);
  if (err_code != IMAGE_SUCCESS || opts == nullptr) {
    FML_LOG(ERROR) << "Create DecodingOptions failed:" << err_code;
    return nullptr;
  }

  Image_Size size = {(uint32_t)width, (uint32_t)height};
  OH_DecodingOptions_SetDesiredSize(opts, &size);
  OH_DecodingOptions_SetPixelFormat(opts, PIXEL_FORMAT_RGBA_8888);

  // HDR requires the RGBA1010102 format and will need future support.
  OH_DecodingOptions_SetDesiredDynamicRange(opts, IMAGE_DYNAMIC_RANGE_SDR);
  OH_DecodingOptions_SetIndex(opts, frame_index);

  OH_PixelmapNative* pixelmap = nullptr;
  // This could be time-consuming.
  err_code =
      OH_ImageSourceNative_CreatePixelmap(image_source_, opts, &pixelmap);
  if (pixelmap && err_code == IMAGE_SUCCESS) {
    auto image_pixelmap = std::make_shared<PixelMapOHOS>(pixelmap);
    FML_LOG(INFO) << "Create Pixelmap size:"
                  << std::to_string(image_pixelmap->width_) << "*"
                  << std::to_string(image_pixelmap->height_) << " stride "
                  << std::to_string(image_pixelmap->row_stride_) << " format "
                  << std::to_string(image_pixelmap->pixel_format_);
    return image_pixelmap;
  } else {
    FML_LOG(ERROR) << "Create Pixelmap from Image source failed:" << err_code
                   << " request size:" << std::to_string(width) << "*"
                   << std::to_string(height) << " " << to_string();
    if (pixelmap) {
      OH_PixelmapNative_Release(pixelmap);
    }
    return nullptr;
  }
}

bool OHOSImageGenerator::IsValidImageData() {
  return GetInfo().width() != 0 && GetInfo().height() != 0 && frame_count_ != 0;
}

OHOSImageGenerator::PixelMapOHOS::PixelMapOHOS(OH_PixelmapNative* pixelmap) {
  if (pixelmap == nullptr) {
    return;
  }
  pixelmap_ = pixelmap;
  OH_Pixelmap_ImageInfo* info = nullptr;
  OH_PixelmapImageInfo_Create(&info);
  if (info == nullptr) {
    return;
  }
  OH_PixelmapNative_GetImageInfo(pixelmap, info);
  OH_PixelmapImageInfo_GetWidth(info, &width_);
  OH_PixelmapImageInfo_GetHeight(info, &height_);
  OH_PixelmapImageInfo_GetRowStride(info, &row_stride_);
  OH_PixelmapImageInfo_GetPixelFormat(info, &pixel_format_);
  OH_PixelmapImageInfo_Release(info);
}

Image_ErrorCode OHOSImageGenerator::PixelMapOHOS::ReadPixels(
    uint8_t* dst_buffer,
    uint32_t buffer_size,
    uint32_t row_stride) {
  if (pixelmap_ == nullptr || row_stride < width_ * RBGA8888_BYTES) {
    return IMAGE_BAD_PARAMETER;
  }
  Image_ErrorCode ret_code = IMAGE_SUCCESS;
  uint8_t* temp_dst_buffer = dst_buffer;
  if (row_stride > width_ * RBGA8888_BYTES) {
    temp_dst_buffer = new uint8_t[buffer_size];
  }
  if (temp_dst_buffer != NULL) {
    size_t dst_size = buffer_size;
    ret_code =
        OH_PixelmapNative_ReadPixels(pixelmap_, temp_dst_buffer, &dst_size);
  }
  if (row_stride > width_ * RBGA8888_BYTES && temp_dst_buffer != nullptr) {
    if (ret_code == IMAGE_SUCCESS) {
      for (int i = 0; i < int(height_); i++) {
        memcpy(dst_buffer + row_stride * i,
               temp_dst_buffer + (width_ * RBGA8888_BYTES) * i,
               width_ * RBGA8888_BYTES);
      }
    }
    delete[] temp_dst_buffer;
  }
  return ret_code;
}

}  // namespace flutter