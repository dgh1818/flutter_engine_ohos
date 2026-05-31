/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ohos_image_generator.h"

#include <native_color_space_manager/native_color_space_manager.h>
#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

#include <multimedia/image_framework/image_pixel_map_napi.h>
#include "fml/logging.h"
#include "fml/trace_event.h"
#include "include/core/SkAlphaType.h"
#include "include/core/SkColorType.h"
#include "include/core/SkImageInfo.h"
#include "third_party/skia/include/codec/SkCodecAnimation.h"
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"

#if IMPELLER_SUPPORTS_RENDERING
#include "flutter/impeller/renderer/backend/vulkan/capabilities_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/context_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/ohos/ohb_texture_source_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/surface_context_vk.h"
#include "flutter/impeller/renderer/backend/vulkan/texture_vk.h"
#include "flutter/impeller/renderer/context.h"
#endif  // IMPELLER_SUPPORTS_RENDERING

std::atomic<size_t> flutter::OHOSImageGenerator::total_cached_bytes_{0};

namespace flutter {

#if IMPELLER_SUPPORTS_RENDERING
namespace {

/// Holds every native handle whose lifetime must outlive the resulting
/// `impeller::Texture`.
struct DmaTextureContext {
  std::shared_ptr<OHOSImageGenerator::PixelMapOHOS> pixelmap;
  OH_NativeBuffer* native_buffer = nullptr;
  OHNativeWindowBuffer* window_buffer = nullptr;

  ~DmaTextureContext() {
    if (window_buffer != nullptr) {
      OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
      window_buffer = nullptr;
    }
    if (native_buffer != nullptr) {
      OH_NativeBuffer_Unreference(native_buffer);
      native_buffer = nullptr;
    }
  }
};

static bool DataStartsWith(const sk_sp<SkData>& data,
                           const char* pattern,
                           size_t pattern_size) {
  if (!data || !data->data() || data->size() < pattern_size) {
    return false;
  }
  return memcmp(data->data(), pattern, pattern_size) == 0;
}

static bool DataContains(const sk_sp<SkData>& data,
                         const char* pattern,
                         size_t pattern_size) {
  if (!data || !data->data() || data->size() < pattern_size) {
    return false;
  }
  const auto* bytes = static_cast<const uint8_t*>(data->data());
  const size_t size = data->size();
  for (size_t i = 0; i <= size - pattern_size; i++) {
    if (memcmp(bytes + i, pattern, pattern_size) == 0) {
      return true;
    }
  }
  return false;
}

static bool IsGifEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, "GIF87a", 6) ||
         DataStartsWith(data, "GIF89a", 6);
}

static bool IsAnimatedPngEncodedData(const sk_sp<SkData>& data) {
  constexpr char kPngSignature[] = "\x89PNG\r\n\x1A\n";
  return DataStartsWith(data, kPngSignature, 8) &&
         DataContains(data, "acTL", 4);
}

static bool IsAnimatedWebPEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, "RIFF", 4) && DataContains(data, "WEBP", 4) &&
         DataContains(data, "ANIM", 4);
}

static bool IsUnsupportedDmaEncodedData(const sk_sp<SkData>& data) {
  return IsGifEncodedData(data) || IsAnimatedPngEncodedData(data) ||
         IsAnimatedWebPEncodedData(data);
}

static impeller::TextureColorSpace OhosColorSpaceToTextureColorSpace(
    uint32_t color_space) {
  switch (static_cast<ColorSpaceName>(color_space)) {
    case DISPLAY_P3:
    case DISPLAY_P3_LIMIT:
    case LINEAR_P3:
      return impeller::TextureColorSpace::kDisplayP3;
    case ADOBE_RGB:
    case DCI_P3:
    case BT2020_HLG:
    case BT2020_PQ:
    case P3_HLG:
    case P3_PQ:
    case ADOBE_RGB_LIMIT:
    case BT2020_HLG_LIMIT:
    case BT2020_PQ_LIMIT:
    case P3_HLG_LIMIT:
    case P3_PQ_LIMIT:
    case LINEAR_BT2020:
      return impeller::TextureColorSpace::kExtendedSRGB;
    case NONE:
    case SRGB:
    case CUSTOM:
    case SRGB_LIMIT:
    case BT709:
    default:
      return impeller::TextureColorSpace::kSRGB;
  }
}

class OhosExternalTextureSourceImpl final : public ExternalTextureSource {
 public:
  explicit OhosExternalTextureSourceImpl(
      std::shared_ptr<OHOSImageGenerator::PixelMapOHOS> pixelmap)
      : pixelmap_(std::move(pixelmap)) {}

  std::shared_ptr<impeller::Texture> CreateImpellerTexture(
      const std::shared_ptr<impeller::Context>& context) override {
    TRACE_EVENT0("flutter", "OhosExternalTextureSource::CreateImpellerTexture");

    if (!context || !context->IsValid() ||
        context->GetBackendType() !=
            impeller::Context::BackendType::kVulkan) {
      return nullptr;
    }
    if (!pixelmap_ || !pixelmap_->IsValid() ||
        pixelmap_->allocator_type_ != IMAGE_ALLOCATOR_TYPE_DMA) {
      return nullptr;
    }

    auto& surface_context_vk = impeller::SurfaceContextVK::Cast(*context);
    auto context_vk = surface_context_vk.GetParent();
    if (!context_vk || !context_vk->GetDevice() ||
        !context_vk->GetCapabilities()) {
      return nullptr;
    }

    const auto& caps =
        impeller::CapabilitiesVK::Cast(*context_vk->GetCapabilities());
    if (!caps.HasExtension(
            impeller::RequiredOHOSDeviceExtensionVK::kOHOSNativeBuffer)) {
      return nullptr;
    }

    auto holder = std::make_shared<DmaTextureContext>();
    holder->pixelmap = pixelmap_;

    Image_ErrorCode err = OH_PixelmapNative_GetNativeBuffer(
        pixelmap_->pixelmap_, &holder->native_buffer);
    if (err != IMAGE_SUCCESS || holder->native_buffer == nullptr) {
      FML_LOG(INFO) << "OHOS DMA: OH_PixelmapNative_GetNativeBuffer failed ("
                    << err << ")";
      return nullptr;
    }

    holder->window_buffer =
        OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
            holder->native_buffer);
    if (holder->window_buffer == nullptr) {
      FML_LOG(INFO)
          << "OHOS DMA: failed to wrap native buffer in window buffer";
      return nullptr;
    }

    const auto texture_color_space =
        OhosColorSpaceToTextureColorSpace(pixelmap_->color_space_);
    auto texture_source = std::make_shared<impeller::OHBTextureSourceVK>(
        context_vk, holder->window_buffer, texture_color_space);
    if (!texture_source->IsValid()) {
      FML_LOG(INFO)
          << "OHOS DMA: OHBTextureSourceVK rejected the native buffer ("
          << pixelmap_->width_ << "x" << pixelmap_->height_ << " fmt "
          << pixelmap_->pixel_format_ << ")";
      return nullptr;
    }

    auto texture = std::shared_ptr<impeller::Texture>(
        new impeller::TextureVK(context_vk, std::move(texture_source)),
        [holder](impeller::Texture* texture) { delete texture; });
    std::ostringstream label;
    label << "ui.Image(OHOS DMABuf " << pixelmap_->width_ << "x"
          << pixelmap_->height_ << ")";
    texture->SetLabel(label.str());
    return texture;
  }

 private:
  std::shared_ptr<OHOSImageGenerator::PixelMapOHOS> pixelmap_;
};

}  // namespace
#endif  // IMPELLER_SUPPORTS_RENDERING

class OhosImageSourceLoader {
  using CreateFromDataWithUserBufferFunc = Image_ErrorCode (*)(
    uint8_t *data, size_t datalength, OH_ImageSourceNative **imageSource);
  using CreatePixelmapUsingAllocatorFunc = Image_ErrorCode (*)(
      OH_ImageSourceNative* source,
      OH_DecodingOptions* opts,
      IMAGE_ALLOCATOR_TYPE allocator,
      OH_PixelmapNative** pixelmap);

 public:
  OhosImageSourceLoader(void);
  ~OhosImageSourceLoader() = default;
  static std::shared_ptr<OhosImageSourceLoader> GetInstance(void);
  Image_ErrorCode CreateFromDataWithUserBuffer(uint8_t *data, size_t datalength, OH_ImageSourceNative **imageSource);

  bool HasPixelmapAllocator() const {
    return loader_ && loader_->IsLoaded() &&
           createPixelmapUsingAllocatorFunc_ != nullptr;
  }

  Image_ErrorCode CreatePixelmapUsingAllocator(OH_ImageSourceNative* source,
                                               OH_DecodingOptions* opts,
                                               IMAGE_ALLOCATOR_TYPE allocator,
                                               OH_PixelmapNative** pixelmap);

  private:
    static constexpr char IMAGE_SOURCE_LIB_NAME[] = "libimage_source.so";
    std::unique_ptr<flutter::DynamicLibraryLoader> loader_;
    CreateFromDataWithUserBufferFunc createFromDataWithUserBufferFunc_ = nullptr;
    CreatePixelmapUsingAllocatorFunc createPixelmapUsingAllocatorFunc_ = nullptr;
};

static std::shared_ptr<OhosImageSourceLoader> OhosImageSourceLoderInstance = nullptr;
static std::once_flag OhosImageSourceLoderInitFlag;

std::shared_ptr<OhosImageSourceLoader> OhosImageSourceLoader::GetInstance() {
  std::call_once(OhosImageSourceLoderInitFlag, [&] {
    OhosImageSourceLoderInstance = std::make_shared<OhosImageSourceLoader>();
  });
  return OhosImageSourceLoderInstance;
}

OhosImageSourceLoader::OhosImageSourceLoader(void)
    : loader_(std::make_unique<flutter::DynamicLibraryLoader>(IMAGE_SOURCE_LIB_NAME)) {

  std::vector<flutter::SymbolInfo> symbols = {
      {"OH_ImageSourceNative_CreateFromDataWithUserBuffer",
       reinterpret_cast<void**>(&createFromDataWithUserBufferFunc_), 20},
      {"OH_ImageSourceNative_CreatePixelmapUsingAllocator",
       reinterpret_cast<void**>(&createPixelmapUsingAllocatorFunc_), 15},
  };

  loader_->LoadSymbols(symbols);
}

Image_ErrorCode OhosImageSourceLoader::CreateFromDataWithUserBuffer(
  uint8_t *data, size_t datalength, OH_ImageSourceNative **imageSource) {
  if (!loader_->IsLoaded() || createFromDataWithUserBufferFunc_ == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }

  return createFromDataWithUserBufferFunc_(data, datalength, imageSource);
}

Image_ErrorCode OhosImageSourceLoader::CreatePixelmapUsingAllocator(
    OH_ImageSourceNative* source,
    OH_DecodingOptions* opts,
    IMAGE_ALLOCATOR_TYPE allocator,
    OH_PixelmapNative** pixelmap) {
  if (!loader_->IsLoaded() || createPixelmapUsingAllocatorFunc_ == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  return createPixelmapUsingAllocatorFunc_(source, opts, allocator, pixelmap);
}

static void ResolveEncodedOrigin(char* data,
                                 int size,
                                 float* rotate,
                                 bool* need_flip) {
  if (size == sizeof("Top-left") - 1 && memcmp("Top-left", data, size) == 0) {
    *rotate = 0.f;
    *need_flip = false;
  } else if (size == sizeof("Top-right") - 1 &&
             memcmp("Top-right", data, size) == 0) {
    *rotate = 0.f;
    *need_flip = true;
  } else if (size == sizeof("Bottom-right") - 1 &&
             memcmp("Bottom-right", data, size) == 0) {
    *rotate = 180.f;
    *need_flip = false;
  } else if (size == sizeof("Bottom-left") - 1 &&
             memcmp("Bottom-left", data, size) == 0) {
    *rotate = 180.f;
    *need_flip = true;
  } else if (size == sizeof("Left-top") - 1 &&
             memcmp("Left-top", data, size) == 0) {
    *rotate = 90.f;
    *need_flip = true;
  } else if (size == sizeof("Right-top") - 1 &&
             memcmp("Right-top", data, size) == 0) {
    *rotate = 90.f;
    *need_flip = false;
  } else if (size == sizeof("Right-bottom") - 1 &&
             memcmp("Right-bottom", data, size) == 0) {
    *rotate = 270.f;
    *need_flip = true;
  } else if (size == sizeof("Left-bottom") - 1 &&
             memcmp("Left-bottom", data, size) == 0) {
    *rotate = 270.f;
    *need_flip = false;
  } else {
    *rotate = 0.f;
    *need_flip = false;
  }
}

OHOSImageGenerator::OHOSImageGenerator(OH_ImageSourceNative* image_source,
                                       const sk_sp<SkData>& data,
                                       bool unsupported_dma_encoded_data)
    : image_source_(image_source),
      data_(data),
      unsupported_dma_encoded_data_(unsupported_dma_encoded_data) {
  OH_ImageSource_Info* info = nullptr;
  OH_ImageSourceInfo_Create(&info);
  if (info == nullptr) {
    FML_LOG(ERROR) << "OH_ImageSourceInfo_Create() failed, returned nullptr";
    return;
  }

  Image_String key;
  key.data = (char*)OHOS_IMAGE_PROPERTY_ORIENTATION;
  key.size = strlen(OHOS_IMAGE_PROPERTY_ORIENTATION);
  Image_String value = {nullptr, 0};
  int err = OH_ImageSourceNative_GetImageProperty(image_source, &key, &value);
  if (err != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "cannot get pixelmap orientation:" << err;
  }
  if (value.data != nullptr) {
    ResolveEncodedOrigin(value.data, value.size, &rotate_degree_, &need_flip_);
    free(value.data);
  }

  uint32_t width = 0, height = 0;
  OH_ImageSourceNative_GetImageInfo(image_source, 0, info);
  OH_ImageSourceInfo_GetWidth(info, &width);
  OH_ImageSourceInfo_GetHeight(info, &height);
  OH_ImageSourceInfo_GetDynamicRange(info, &is_hdr_);
  OH_ImageSourceInfo_Release(info);
  FML_LOG(INFO) << "Image info: width=" << width << ", height=" << height << ", is_hdr=" << is_hdr_;
  if (rotate_degree_ == 90.f || rotate_degree_ == 270.f) {
    origin_image_info_ = SkImageInfo::Make(
        height, width, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  } else {
    origin_image_info_ = SkImageInfo::Make(
        width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  }

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
  for (const auto& kv : cached_pixelmaps_) {
    if (kv.second) {
      size_t old_size = kv.second->width_ * kv.second->height_ * RBGA8888_BYTES;
      total_cached_bytes_.fetch_sub(old_size, std::memory_order_relaxed);
    }
  }
  cached_pixelmaps_.clear();
}

const SkImageInfo& OHOSImageGenerator::GetInfo() {
  TRACE_EVENT1("flutter", "Image", "GetInfo", to_string().c_str());
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
  std::string trace_str = to_string() + "->" + std::to_string(info.width()) +
                          "*" + std::to_string(info.height()) +
                          "-index:" + std::to_string(frame_index);
  TRACE_EVENT1("flutter", "Image", "GetPixelsOHOS", trace_str.c_str());
  if (frame_index == 0) {
    FML_DLOG(INFO) << trace_str;
  }
  if (image_source_ == nullptr) {
    FML_LOG(ERROR) << "image_source is nullptr";
    return false;
  }

  if (info.colorType() != kRGBA_8888_SkColorType) {
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
    const uint32_t min_row_bytes =
        image_pixelmap->width_ * RBGA8888_BYTES;
    const uint32_t pixelmap_row_bytes =
        std::max(image_pixelmap->row_stride_, min_row_bytes);
    uint32_t buffer_size = pixelmap_row_bytes * image_pixelmap->height_;
    std::string trace_str = "size:" + std::to_string(buffer_size) +
                            "-dst_stride:" + std::to_string(row_bytes) +
                            "-pixelmap_stride:" +
                            std::to_string(image_pixelmap->row_stride_);
    TRACE_EVENT1("flutter", "Image", "ReadPixels", trace_str.c_str());
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
    if (image_pixelmap &&
        total_cached_bytes_ <= kMaxGlobalCacheSize - buffer_size) {
      // Cache animated images to improve performance.
      cached_pixelmaps_[frame_index] = image_pixelmap;
      total_cached_bytes_.fetch_add(buffer_size, std::memory_order_relaxed);
    }
    return true;
  } else {
    return false;
  }
}

 uint32_t OHOSImageGenerator::GetColorSpace(unsigned int frame_index) {
  if (cached_colorspaces_.find(frame_index) != cached_colorspaces_.end()) {
    return cached_colorspaces_[frame_index];
  }
  if (cached_pixelmaps_.find(frame_index) != cached_pixelmaps_.end()) {
    return cached_pixelmaps_[frame_index]->color_space_;
  }
  return 0;
}

#if IMPELLER_SUPPORTS_RENDERING
std::unique_ptr<ExternalTextureSource>
OHOSImageGenerator::CreateExternalTextureSource(
    const SkISize& decode_dimensions,
    unsigned int frame_index,
    std::optional<unsigned int> prior_frame) {
  if (unsupported_dma_encoded_data_) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, unsupported DMA "
                     "encoded image "
                  << to_string();
    return nullptr;
  }
  if (frame_count_ != 1 || prior_frame.has_value()) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, animated or dependent "
                     "frame "
                  << to_string();
    return nullptr;
  }
  if (decode_dimensions.isEmpty() || image_source_ == nullptr) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, invalid decode request "
                  << to_string();
    return nullptr;
  }
  auto loader = OhosImageSourceLoader::GetInstance();
  if (!loader || !loader->HasPixelmapAllocator()) {
    FML_LOG(INFO)
        << "OHOS DMA: regular path selected, DMA allocator unavailable "
        << to_string();
    return nullptr;
  }
  if (is_hdr_) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, HDR source "
                  << to_string();
    return nullptr;
  }
  TRACE_EVENT1("flutter", "Image", "CreateExternalTextureSourceOHOS",
               to_string().c_str());

  auto pixelmap = CreatePixelMap(decode_dimensions.width(),
                                 decode_dimensions.height(),
                                 static_cast<int>(frame_index),
                                 /*prefer_dma=*/true);
  if (!pixelmap || !pixelmap->IsValid() ||
      pixelmap->allocator_type_ != IMAGE_ALLOCATOR_TYPE_DMA) {
    FML_LOG(INFO)
        << "OHOS DMA: regular path selected, DMA pixelmap unavailable "
        << to_string();
    return nullptr;
  }
  if (static_cast<int>(pixelmap->width_) != decode_dimensions.width() ||
      static_cast<int>(pixelmap->height_) != decode_dimensions.height()) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, pixelmap actual size "
                  << pixelmap->width_ << "x" << pixelmap->height_
                  << " != requested " << decode_dimensions.width() << "x"
                  << decode_dimensions.height() << " " << to_string();
    return nullptr;
  }
  if (pixelmap->pixel_format_ != PIXEL_FORMAT_RGBA_8888) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, unsupported pixel "
                     "format "
                  << pixelmap->pixel_format_ << " " << to_string();
    return nullptr;
  }
  const uint32_t min_row_stride = pixelmap->width_ * RBGA8888_BYTES;
  if (pixelmap->row_stride_ < min_row_stride) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, invalid row stride "
                  << pixelmap->row_stride_ << " < " << min_row_stride << " "
                  << to_string();
    return nullptr;
  }
  FML_LOG(INFO) << "OHOS DMA: external texture source accepted "
                << pixelmap->width_ << "x" << pixelmap->height_
                << " format=" << pixelmap->pixel_format_
                << " row_stride=" << pixelmap->row_stride_
                << " allocator=" << pixelmap->allocator_type_
                << " color_space=" << pixelmap->color_space_
                << " texture_color_space="
                << static_cast<int>(
                       OhosColorSpaceToTextureColorSpace(pixelmap->color_space_))
                << " "
                << to_string();
  return std::make_unique<OhosExternalTextureSourceImpl>(std::move(pixelmap));
}
#endif  // IMPELLER_SUPPORTS_RENDERING

std::shared_ptr<ImageGenerator> OHOSImageGenerator::MakeFromData(
    sk_sp<SkData> data) {
  // Return directly if the image data is empty.
  if (!data->data() || !data->size()) {
    return nullptr;
  }
  TRACE_EVENT1("flutter", "Image", "MakeFromDataOHOS",
               std::to_string(data->size()).c_str());

#if IMPELLER_SUPPORTS_RENDERING
  const bool unsupported_dma_encoded_data =
      IsUnsupportedDmaEncodedData(data);
#else
  const bool unsupported_dma_encoded_data = false;
#endif  // IMPELLER_SUPPORTS_RENDERING

  OH_ImageSourceNative* image_source = nullptr;

  bool isHeldSkData = true;
  Image_ErrorCode err_code = IMAGE_BAD_PARAMETER;
  std::shared_ptr<OhosImageSourceLoader> ohosImageSourceLoader = OhosImageSourceLoader::GetInstance();
  if (ohosImageSourceLoader != nullptr) {
    err_code = ohosImageSourceLoader->CreateFromDataWithUserBuffer(
      (uint8_t*)data->bytes(), data->size(), &image_source);
  }

  if (err_code != IMAGE_SUCCESS || image_source == nullptr) {
    // The data will be coyied to ImageSourceNative.
    // No modifications will be made to origin data.
    err_code = OH_ImageSourceNative_CreateFromData(
        (uint8_t*)data->bytes(), data->size(), &image_source);
    if (err_code != IMAGE_SUCCESS || image_source == nullptr) {
      FML_LOG(ERROR) << "Create ImageSource failed: " << err_code;
      return nullptr;
    }
    isHeldSkData = false;
  }

  // Preventing data from being released by the system
  std::shared_ptr<OHOSImageGenerator> generator(new OHOSImageGenerator(
      image_source, isHeldSkData ? data : sk_sp<SkData>(),
      unsupported_dma_encoded_data));

  if (generator->IsValidImageData()) {
    return generator;
  } else {
    FML_LOG(ERROR) << "Invalid ImageData:" << generator->to_string();
    return nullptr;
  }
}

std::shared_ptr<OHOSImageGenerator::PixelMapOHOS>
OHOSImageGenerator::CreatePixelMap(int width,
                                   int height,
                                   int frame_index,
                                   bool prefer_dma) {
  OH_DecodingOptions* opts = nullptr;
  Image_ErrorCode err_code = OH_DecodingOptions_Create(&opts);
  if (err_code != IMAGE_SUCCESS || opts == nullptr) {
    FML_LOG(ERROR) << "Create DecodingOptions failed:" << err_code;
    return nullptr;
  }

  Image_Size size = {(uint32_t)width, (uint32_t)height};
  OH_DecodingOptions_SetDesiredSize(opts, &size);
  OH_DecodingOptions_SetPixelFormat(opts, PIXEL_FORMAT_RGBA_8888);
  OH_DecodingOptions_SetRotate(opts, rotate_degree_);

  // HDR requires the RGBA1010102 format and will need future support.
  OH_DecodingOptions_SetDesiredDynamicRange(opts, IMAGE_DYNAMIC_RANGE_SDR);
  OH_DecodingOptions_SetIndex(opts, frame_index);

  OH_PixelmapNative* pixelmap = nullptr;
  IMAGE_ALLOCATOR_TYPE actual_allocator = IMAGE_ALLOCATOR_TYPE_AUTO;
  auto loader = OhosImageSourceLoader::GetInstance();
  if (prefer_dma) {
    if (is_hdr_ || !loader || !loader->HasPixelmapAllocator()) {
      OH_DecodingOptions_Release(opts);
      return nullptr;
    }
    err_code = loader->CreatePixelmapUsingAllocator(
        image_source_, opts, IMAGE_ALLOCATOR_TYPE_DMA, &pixelmap);
    if (err_code == IMAGE_SUCCESS && pixelmap != nullptr) {
      actual_allocator = IMAGE_ALLOCATOR_TYPE_DMA;
    } else {
      FML_LOG(INFO) << "OHOS DMA: CreatePixelmapUsingAllocator failed ("
                    << err_code << "), regular path will decode "
                    << to_string();
      if (pixelmap) {
        OH_PixelmapNative_Release(pixelmap);
      }
      OH_DecodingOptions_Release(opts);
      return nullptr;
    }
  }

  if (pixelmap == nullptr) {
    // This could be time-consuming.
    err_code =
        OH_ImageSourceNative_CreatePixelmap(image_source_, opts, &pixelmap);
  }
  OH_DecodingOptions_Release(opts);

  if (pixelmap && err_code == IMAGE_SUCCESS) {
    OH_NativeColorSpaceManager* mgr = nullptr;
    auto colorspace_res = OH_PixelmapNative_GetColorSpaceNative(pixelmap, &mgr);
    auto colorspace_name = 0;
    if (colorspace_res == IMAGE_SUCCESS) {
      colorspace_name = OH_NativeColorSpaceManager_GetColorSpaceName(mgr);
    }
    if (!prefer_dma) {
      cached_colorspaces_[frame_index] = colorspace_name;
    }
    if (need_flip_) {
      OH_PixelmapNative_Flip(pixelmap, need_flip_, false);
    }
    auto image_pixelmap =
        std::make_shared<PixelMapOHOS>(pixelmap, actual_allocator);
    image_pixelmap->setColorSpace(colorspace_name);
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

OHOSImageGenerator::PixelMapOHOS::PixelMapOHOS(
    OH_PixelmapNative* pixelmap,
    IMAGE_ALLOCATOR_TYPE allocator_type)
    : allocator_type_(allocator_type) {
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
  const uint32_t min_row_bytes = width_ * RBGA8888_BYTES;
  const uint32_t source_row_bytes = std::max(row_stride_, min_row_bytes);
  const uint32_t source_size = source_row_bytes * height_;
  if (buffer_size < source_size) {
    return IMAGE_BAD_PARAMETER;
  }
  Image_ErrorCode ret_code = IMAGE_SUCCESS;
  uint8_t* temp_dst_buffer = dst_buffer;
  std::unique_ptr<uint8_t[]> temp_buffer;
  if (row_stride != source_row_bytes) {
    temp_buffer = std::make_unique<uint8_t[]>(source_size);
    temp_dst_buffer = temp_buffer.get();
  }
  if (temp_dst_buffer != NULL) {
    size_t dst_size = source_size;
    ret_code =
        OH_PixelmapNative_ReadPixels(pixelmap_, temp_dst_buffer, &dst_size);
  }
  if (row_stride != source_row_bytes && temp_dst_buffer != nullptr) {
    if (ret_code == IMAGE_SUCCESS) {
      for (int i = 0; i < int(height_); i++) {
        memcpy(dst_buffer + row_stride * i,
               temp_dst_buffer + source_row_bytes * i, min_row_bytes);
      }
    }
  }
  return ret_code;
}

}  // namespace flutter
