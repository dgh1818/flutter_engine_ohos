/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ohos_image_generator.h"

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_buffer/native_buffer.h>
#include <native_color_space_manager/native_color_space_manager.h>
#include <native_window/external_window.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

#include <multimedia/image_framework/image_pixel_map_napi.h>
#include "flutter/fml/platform/ohos/dynamic_library_loader.h"
#include "flutter/lib/ui/painting/ohos_color_space.h"
#include "fml/logging.h"
#include "fml/trace_event.h"
#include "include/core/SkAlphaType.h"
#include "include/core/SkColorType.h"
#include "include/core/SkImageInfo.h"
#include "third_party/skia/include/codec/SkCodecAnimation.h"

#include "flutter/impeller/renderer/context.h"
#include "flutter/shell/platform/ohos/mpf_decoder.h"
#include "third_party/skia/include/core/SkColorSpace.h"

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

namespace {

struct OhosDecodingOptionsDeleter {
  void operator()(OH_DecodingOptions* opts) const {
    if (opts != nullptr) {
      OH_DecodingOptions_Release(opts);
    }
  }
};

using OhosDecodingOptionsPtr =
    std::unique_ptr<OH_DecodingOptions, OhosDecodingOptionsDeleter>;

static OhosDecodingOptionsPtr CreateOhosDecodingOptions(int width,
                                                        int height,
                                                        int frameIndex,
                                                        float rotateDegree,
                                                        bool isHdr) {
  OH_DecodingOptions* rawOpts = nullptr;
  Image_ErrorCode errCode = OH_DecodingOptions_Create(&rawOpts);
  if (errCode != IMAGE_SUCCESS || rawOpts == nullptr) {
    FML_LOG(ERROR) << "Create DecodingOptions failed:" << errCode;
    return nullptr;
  }
  OhosDecodingOptionsPtr opts(rawOpts);
  Image_Size size = {(uint32_t)width, (uint32_t)height};
  OH_DecodingOptions_SetDesiredSize(opts.get(), &size);
  if (!impeller::Context::enable_hdr_ || !isHdr) {
    OH_DecodingOptions_SetPixelFormat(opts.get(), PIXEL_FORMAT_RGBA_8888);
    OH_DecodingOptions_SetDesiredDynamicRange(opts.get(),
                                              IMAGE_DYNAMIC_RANGE_SDR);
  } else {
    OH_DecodingOptions_SetDesiredDynamicRange(opts.get(),
                                              IMAGE_DYNAMIC_RANGE_HDR);
    OH_DecodingOptions_SetPixelFormat(opts.get(), PIXEL_FORMAT_RGBA_1010102);
  }
  OH_DecodingOptions_SetRotate(opts.get(), rotateDegree);
  OH_DecodingOptions_SetIndex(opts.get(), frameIndex);
  return opts;
}

}  // namespace

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

constexpr size_t kChunkTagSize = 4;
constexpr size_t kGifSignatureSize = 6;
constexpr size_t kPngSignatureSize = 8;
constexpr size_t kMaxEncodedHeaderSearchBytes = 32 * 1024;
constexpr char kGif87aSignature[] = "GIF87a";
constexpr char kGif89aSignature[] = "GIF89a";
constexpr char kPngSignature[] = "\x89PNG\r\n\x1A\n";
constexpr char kRiffSignature[] = "RIFF";
constexpr char kWebpSignature[] = "WEBP";
constexpr char kWebpAnimationChunk[] = "ANIM";
constexpr char kPngAnimationChunk[] = "acTL";

static bool DataStartsWith(const sk_sp<SkData>& data,
                           const char* pattern,
                           size_t patternSize) {
  if (!data || !data->data() || data->size() < patternSize) {
    return false;
  }
  return memcmp(data->data(), pattern, patternSize) == 0;
}

static bool DataContains(const sk_sp<SkData>& data,
                         const char* pattern,
                         size_t patternSize,
                         size_t maxSearch = kMaxEncodedHeaderSearchBytes) {
  if (!data || !data->data() || data->size() < patternSize) {
    return false;
  }
  const auto* bytes = static_cast<const uint8_t*>(data->data());
  const size_t limit = std::min(data->size(), maxSearch);
  if (limit < patternSize) {
    return false;
  }
  for (size_t i = 0; i + patternSize <= limit; i++) {
    if (memcmp(bytes + i, pattern, patternSize) == 0) {
      return true;
    }
  }
  return false;
}

static bool IsGifEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, kGif87aSignature, kGifSignatureSize) ||
         DataStartsWith(data, kGif89aSignature, kGifSignatureSize);
}

static bool IsAnimatedPngEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, kPngSignature, kPngSignatureSize) &&
         DataContains(data, kPngAnimationChunk, kChunkTagSize);
}

static bool IsAnimatedWebPEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, kRiffSignature, kChunkTagSize) &&
         DataContains(data, kWebpSignature, kChunkTagSize) &&
         DataContains(data, kWebpAnimationChunk, kChunkTagSize);
}

static bool IsUnsupportedDmaEncodedData(const sk_sp<SkData>& data) {
  return IsGifEncodedData(data) || IsAnimatedPngEncodedData(data) ||
         IsAnimatedWebPEncodedData(data);
}

class OhosExternalTextureSourceImpl final : public ExternalTextureSource {
 public:
  explicit OhosExternalTextureSourceImpl(
      std::shared_ptr<OHOSImageGenerator::PixelMapOHOS> pixelmap)
      : pixelmap_(std::move(pixelmap)) {}

  std::shared_ptr<impeller::Texture> CreateImpellerTexture(
      const std::shared_ptr<impeller::Context>& context) override {
    TRACE_EVENT0("flutter", "OhosExternalTextureSource::CreateImpellerTexture");

    auto context_vk = GetContextVK(context);
    if (!context_vk) {
      return nullptr;
    }
    auto holder = CreateDmaTextureContext();
    if (!holder) {
      return nullptr;
    }
    auto texture_source = CreateTextureSource(context_vk, holder);
    if (!texture_source) {
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
  std::shared_ptr<impeller::ContextVK> GetContextVK(
      const std::shared_ptr<impeller::Context>& context) const {
    if (!context || !context->IsValid() ||
        context->GetBackendType() != impeller::Context::BackendType::kVulkan) {
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
    const bool supportsNativeBuffer = caps.HasExtension(
        impeller::RequiredOHOSDeviceExtensionVK::kOHOSNativeBuffer);
    if (!supportsNativeBuffer) {
      return nullptr;
    }
    return context_vk;
  }

  std::shared_ptr<DmaTextureContext> CreateDmaTextureContext() const {
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

    return holder;
  }

  std::shared_ptr<impeller::OHBTextureSourceVK> CreateTextureSource(
      const std::shared_ptr<impeller::ContextVK>& context_vk,
      const std::shared_ptr<DmaTextureContext>& holder) const {
    const auto textureColorSpace =
        OhosColorSpaceToTextureColorSpace(pixelmap_->color_space_);
    auto texture_source = std::make_shared<impeller::OHBTextureSourceVK>(
        context_vk, holder->window_buffer, textureColorSpace);
    if (!texture_source->IsValid()) {
      FML_LOG(INFO)
          << "OHOS DMA: OHBTextureSourceVK rejected the native buffer ("
          << pixelmap_->width_ << "x" << pixelmap_->height_ << " fmt "
          << pixelmap_->pixel_format_ << ")";
      return nullptr;
    }
    return texture_source;
  }

  std::shared_ptr<OHOSImageGenerator::PixelMapOHOS> pixelmap_;
};

}  // namespace
#endif  // IMPELLER_SUPPORTS_RENDERING

class OhosImageSourceLoader {
  using CreateFromDataWithUserBufferFunc =
      Image_ErrorCode (*)(uint8_t* data,
                          size_t datalength,
                          OH_ImageSourceNative** imageSource);
  using CreatePixelmapUsingAllocatorFunc =
      Image_ErrorCode (*)(OH_ImageSourceNative* source,
                          OH_DecodingOptions* opts,
                          IMAGE_ALLOCATOR_TYPE allocator,
                          OH_PixelmapNative** pixelmap);

 public:
  OhosImageSourceLoader(void);
  ~OhosImageSourceLoader() = default;
  static std::shared_ptr<OhosImageSourceLoader> GetInstance(void);
  Image_ErrorCode CreateFromDataWithUserBuffer(
      uint8_t* data,
      size_t datalength,
      OH_ImageSourceNative** imageSource);

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

static std::shared_ptr<OhosImageSourceLoader> OhosImageSourceLoderInstance =
    nullptr;
static std::once_flag OhosImageSourceLoderInitFlag;

std::shared_ptr<OhosImageSourceLoader> OhosImageSourceLoader::GetInstance() {
  std::call_once(OhosImageSourceLoderInitFlag, [&] {
    OhosImageSourceLoderInstance = std::make_shared<OhosImageSourceLoader>();
  });
  return OhosImageSourceLoderInstance;
}

OhosImageSourceLoader::OhosImageSourceLoader(void)
    : loader_(std::make_unique<flutter::DynamicLibraryLoader>(
          IMAGE_SOURCE_LIB_NAME)) {
  std::vector<flutter::SymbolInfo> symbols = {
      {"OH_ImageSourceNative_CreateFromDataWithUserBuffer",
       reinterpret_cast<void**>(&createFromDataWithUserBufferFunc_), 20},
      {"OH_ImageSourceNative_CreatePixelmapUsingAllocator",
       reinterpret_cast<void**>(&createPixelmapUsingAllocatorFunc_), 15},
  };

  loader_->LoadSymbols(symbols);
}

Image_ErrorCode OhosImageSourceLoader::CreateFromDataWithUserBuffer(
    uint8_t* data,
    size_t datalength,
    OH_ImageSourceNative** imageSource) {
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
                                       bool unsupportedDmaEncodedData)
    : image_source_(image_source),
      data_(data),
      unsupported_dma_encoded_data_(unsupportedDmaEncodedData) {
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
  if (!is_hdr_) {
    InitMpfInfo();
  }
  OH_ImageSourceInfo_Release(info);
  FML_LOG(INFO) << "Image info: width=" << width << ", height=" << height
                << ", is_hdr=" << is_hdr_;

  const bool use_hdr = is_hdr_ && impeller::Context::enable_hdr_;
  const SkColorType color_type =
      use_hdr ? kRGBA_1010102_SkColorType : kRGBA_8888_SkColorType;
  const SkAlphaType alpha_type =
      use_hdr ? kOpaque_SkAlphaType : kPremul_SkAlphaType;
  if (rotate_degree_ == 90.f || rotate_degree_ == 270.f) {
    origin_image_info_ =
        SkImageInfo::Make(height, width, color_type, alpha_type);
  } else {
    origin_image_info_ =
        SkImageInfo::Make(width, height, color_type, alpha_type);
  }
  if (use_hdr) {
    origin_image_info_ = origin_image_info_.makeColorSpace(
        SkColorSpace::MakeRGB(SkNamedTransferFn::kHLG, SkNamedGamut::kRec2020));
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
  if (mpf_gainmap_image_source_) {
    OH_ImageSourceNative_Release(mpf_gainmap_image_source_);
    mpf_gainmap_image_source_ = nullptr;
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

  if (info.colorType() != kRGBA_8888_SkColorType &&
      info.colorType() != kRGBA_1010102_SkColorType) {
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
      const uint32_t cached_width = image_pixelmap->width_;
      const uint32_t cached_height = image_pixelmap->height_;
      FML_LOG(WARNING) << "get changed size:" << std::to_string(cached_width)
                       << "*" << std::to_string(cached_height) << "->"
                       << std::to_string(info.width()) << "*"
                       << std::to_string(info.height());
      image_pixelmap = nullptr;
    }
  }
  if (image_pixelmap == nullptr) {
    if (has_mpf_gainmap_ &&
        TryComposeMpfGainmap(info, pixels, row_bytes, frame_index)) {
      return true;
    }
    image_pixelmap = CreatePixelMap(info.width(), info.height(), frame_index);
  }

  if (image_pixelmap) {
    // OH_PixelmapNative_ReadPixels() needs a buffer large enough for the
    // PixelMap's physical row stride, not just width * bytes-per-pixel.
    // Keep this aligned with the working 3.41 OHOS path.
    uint32_t buffer_size =
        image_pixelmap->row_stride_ * image_pixelmap->height_;
    std::string read_pixels_trace_str =
        "size:" + std::to_string(buffer_size) +
        "-dst_stride:" + std::to_string(row_bytes) +
        "-pixelmap_stride:" + std::to_string(image_pixelmap->row_stride_);
    TRACE_EVENT1("flutter", "Image", "ReadPixels",
                 read_pixels_trace_str.c_str());
    if (frame_index == 0) {
      FML_DLOG(INFO) << read_pixels_trace_str;
    }
    Image_ErrorCode err_code =
        image_pixelmap->ReadPixels((uint8_t*)pixels, buffer_size, row_bytes);
    if (err_code != IMAGE_SUCCESS) {
      FML_LOG(ERROR) << "Pixelmap ReadPixels failed:" << err_code << " "
                     << to_string();
      return false;
    }
    if (image_pixelmap && buffer_size <= kMaxGlobalCacheSize &&
        total_cached_bytes_.load(std::memory_order_relaxed) <=
            kMaxGlobalCacheSize - buffer_size) {
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
bool OHOSImageGenerator::CanCreateDmaPixelMap(
    const SkISize& decodeDimensions,
    std::optional<unsigned int> priorFrame) const {
  if (unsupported_dma_encoded_data_) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, unsupported DMA "
                     "encoded image "
                  << to_string();
    return false;
  }
  if (frame_count_ != 1 || priorFrame.has_value()) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, animated or dependent "
                     "frame "
                  << to_string();
    return false;
  }
  if (decodeDimensions.isEmpty() || image_source_ == nullptr) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, invalid decode request "
                  << to_string();
    return false;
  }
  auto loader = OhosImageSourceLoader::GetInstance();
  if (!loader || !loader->HasPixelmapAllocator()) {
    FML_LOG(INFO)
        << "OHOS DMA: regular path selected, DMA allocator unavailable "
        << to_string();
    return false;
  }
  if (is_hdr_) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, HDR source "
                  << to_string();
    return false;
  }
  return true;
}

bool OHOSImageGenerator::IsValidDmaPixelMap(
    const std::shared_ptr<PixelMapOHOS>& pixelmap,
    const SkISize& decodeDimensions) const {
  if (!pixelmap || !pixelmap->IsValid() ||
      pixelmap->allocator_type_ != IMAGE_ALLOCATOR_TYPE_DMA) {
    FML_LOG(INFO)
        << "OHOS DMA: regular path selected, DMA pixelmap unavailable "
        << to_string();
    return false;
  }
  if (static_cast<int>(pixelmap->width_) != decodeDimensions.width() ||
      static_cast<int>(pixelmap->height_) != decodeDimensions.height()) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, pixelmap actual size "
                  << pixelmap->width_ << "x" << pixelmap->height_
                  << " != requested " << decodeDimensions.width() << "x"
                  << decodeDimensions.height() << " " << to_string();
    return false;
  }
  if (pixelmap->pixel_format_ != PIXEL_FORMAT_RGBA_8888) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, unsupported pixel "
                     "format "
                  << pixelmap->pixel_format_ << " " << to_string();
    return false;
  }
  const size_t minRowStride =
      static_cast<size_t>(pixelmap->width_) * RBGA8888_BYTES;
  if (static_cast<size_t>(pixelmap->row_stride_) < minRowStride) {
    FML_LOG(INFO) << "OHOS DMA: regular path selected, invalid row stride "
                  << pixelmap->row_stride_ << " < " << minRowStride << " "
                  << to_string();
    return false;
  }
  return true;
}

void OHOSImageGenerator::LogAcceptedDmaPixelMap(
    const std::shared_ptr<PixelMapOHOS>& pixelmap) const {
  const auto textureColorSpace =
      OhosColorSpaceToTextureColorSpace(pixelmap->color_space_);
  FML_LOG(INFO) << "OHOS DMA: external texture source accepted "
                << pixelmap->width_ << "x" << pixelmap->height_
                << " format=" << pixelmap->pixel_format_
                << " row_stride=" << pixelmap->row_stride_
                << " allocator=" << pixelmap->allocator_type_
                << " color_space=" << pixelmap->color_space_
                << " texture_color_space="
                << static_cast<int>(textureColorSpace) << " " << to_string();
}

std::unique_ptr<ExternalTextureSource>
OHOSImageGenerator::CreateExternalTextureSource(
    const SkISize& decode_dimensions,
    unsigned int frame_index,
    std::optional<unsigned int> prior_frame) {
  if (!CanCreateDmaPixelMap(decode_dimensions, prior_frame)) {
    return nullptr;
  }
  TRACE_EVENT1("flutter", "Image", "CreateExternalTextureSourceOHOS",
               to_string().c_str());

  constexpr bool kPreferDma = true;
  auto pixelmap =
      CreatePixelMap(decode_dimensions.width(), decode_dimensions.height(),
                     static_cast<int>(frame_index), kPreferDma);
  if (!IsValidDmaPixelMap(pixelmap, decode_dimensions)) {
    return nullptr;
  }
  LogAcceptedDmaPixelMap(pixelmap);
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
  const bool unsupportedDmaEncodedData = IsUnsupportedDmaEncodedData(data);
#else
  const bool unsupportedDmaEncodedData = false;
#endif  // IMPELLER_SUPPORTS_RENDERING

  OH_ImageSourceNative* image_source = nullptr;

  bool isHeldSkData = true;
  Image_ErrorCode err_code = IMAGE_BAD_PARAMETER;
  std::shared_ptr<OhosImageSourceLoader> ohosImageSourceLoader =
      OhosImageSourceLoader::GetInstance();
  if (ohosImageSourceLoader != nullptr) {
    err_code = ohosImageSourceLoader->CreateFromDataWithUserBuffer(
        (uint8_t*)data->bytes(), data->size(), &image_source);
  }

  if (err_code != IMAGE_SUCCESS || image_source == nullptr) {
    // The data will be coyied to ImageSourceNative.
    // No modifications will be made to origin data.
    err_code = OH_ImageSourceNative_CreateFromData((uint8_t*)data->bytes(),
                                                   data->size(), &image_source);
    if (err_code != IMAGE_SUCCESS || image_source == nullptr) {
      FML_LOG(ERROR) << "Create ImageSource failed: " << err_code;
      return nullptr;
    }
    isHeldSkData = false;
  }

  // Preventing data from being released by the system
  std::shared_ptr<OHOSImageGenerator> generator(new OHOSImageGenerator(
      image_source, isHeldSkData ? data : sk_sp<SkData>(),
      unsupportedDmaEncodedData));

  if (!generator->has_mpf_gainmap_) {
    generator->InitMpfGainmap(data);
  }

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
                                   bool preferDma) {
  auto opts = CreateOhosDecodingOptions(width, height, frame_index,
                                        rotate_degree_, is_hdr_);
  if (!opts) {
    return nullptr;
  }

  OH_PixelmapNative* pixelmap = nullptr;
  IMAGE_ALLOCATOR_TYPE actualAllocator = IMAGE_ALLOCATOR_TYPE_AUTO;
  Image_ErrorCode errCode = IMAGE_BAD_PARAMETER;
  if (preferDma) {
    if (!CreateDmaPixelMap(opts.get(), &pixelmap, &actualAllocator, &errCode)) {
      return nullptr;
    }
  }

  if (pixelmap == nullptr) {
    // This could be time-consuming.
    errCode = OH_ImageSourceNative_CreatePixelmap(image_source_, opts.get(),
                                                  &pixelmap);
  }

  if (!pixelmap || errCode != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "Create Pixelmap from Image source failed:" << errCode
                   << " request size:" << std::to_string(width) << "*"
                   << std::to_string(height) << " " << to_string();
    if (pixelmap != nullptr) {
      OH_PixelmapNative_Release(pixelmap);
    }
    return nullptr;
  }
  return AdoptPixelMap(pixelmap, actualAllocator, frame_index, preferDma);
}

bool OHOSImageGenerator::CreateDmaPixelMap(
    OH_DecodingOptions* opts,
    OH_PixelmapNative** pixelmap,
    IMAGE_ALLOCATOR_TYPE* actualAllocator,
    Image_ErrorCode* errCode) {
  auto loader = OhosImageSourceLoader::GetInstance();
  if (is_hdr_ || !loader || !loader->HasPixelmapAllocator()) {
    return false;
  }
  *errCode = loader->CreatePixelmapUsingAllocator(
      image_source_, opts, IMAGE_ALLOCATOR_TYPE_DMA, pixelmap);
  if (*errCode == IMAGE_SUCCESS && *pixelmap != nullptr) {
    *actualAllocator = IMAGE_ALLOCATOR_TYPE_DMA;
    return true;
  }
  FML_LOG(INFO) << "OHOS DMA: CreatePixelmapUsingAllocator failed (" << *errCode
                << "), regular path will decode " << to_string();
  if (*pixelmap != nullptr) {
    OH_PixelmapNative_Release(*pixelmap);
    *pixelmap = nullptr;
  }
  return false;
}

std::shared_ptr<OHOSImageGenerator::PixelMapOHOS>
OHOSImageGenerator::AdoptPixelMap(OH_PixelmapNative* pixelmap,
                                  IMAGE_ALLOCATOR_TYPE actualAllocator,
                                  int frameIndex,
                                  bool preferDma) {
  OH_NativeColorSpaceManager* mgr = nullptr;
  auto colorSpaceRes = OH_PixelmapNative_GetColorSpaceNative(pixelmap, &mgr);
  uint32_t colorSpaceName = 0;
  if (colorSpaceRes == IMAGE_SUCCESS && mgr != nullptr) {
    std::unique_ptr<OH_NativeColorSpaceManager,
                    decltype(&OH_NativeColorSpaceManager_Destroy)>
        mgrHolder(mgr, OH_NativeColorSpaceManager_Destroy);
    colorSpaceName =
        OH_NativeColorSpaceManager_GetColorSpaceName(mgrHolder.get());
  }
  if (!preferDma) {
    cached_colorspaces_[frameIndex] = colorSpaceName;
  }
  if (need_flip_) {
    OH_PixelmapNative_Flip(pixelmap, need_flip_, false);
  }
  auto imagePixelmap =
      std::make_shared<PixelMapOHOS>(pixelmap, actualAllocator);
  imagePixelmap->setColorSpace(colorSpaceName);
  return imagePixelmap;
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
  Image_ErrorCode ret_code = IMAGE_SUCCESS;
  uint8_t* temp_dst_buffer = dst_buffer;

  std::unique_ptr<uint8_t[]> tempBuffer;
  if (row_stride > width_ * RBGA8888_BYTES) {
    tempBuffer = std::make_unique<uint8_t[]>(buffer_size);
    temp_dst_buffer = tempBuffer.get();
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
  }
  return ret_code;
}

void OHOSImageGenerator::InitMpfInfo() {
  if (data_ && data_->data() && data_->size() > 0) {
    MpfGainmapInfo mpf_info;
    if (ParseMpfGainmapInfo(static_cast<const uint8_t*>(data_->data()),
                            data_->size(), &mpf_info)) {
      is_hdr_ = true;
      has_mpf_info_ = true;
      mpf_info_ = mpf_info;
    }
  }
}

bool OHOSImageGenerator::TryComposeMpfGainmapInternal(const SkImageInfo& info,
                                                      void* pixels,
                                                      size_t row_bytes) {
  MpfGainmapComposeInput compose_input = {
      image_source_,     mpf_gainmap_image_source_,
      info.width(),      info.height(),
      rotate_degree_,    need_flip_,
      gainmap_headroom_, pixels,
      row_bytes,
  };
  MpfGainmapComposeResult compose_result = {};
  if (ComposeMpfGainmap(compose_input, &compose_result) &&
      compose_result.success) {
    if (!compose_result.used_vpe) {
      return true;
    }

    std::shared_ptr<PixelMapOHOS> hdr_wrapper = std::make_shared<PixelMapOHOS>(
        compose_result.hdr_pixelmap, IMAGE_ALLOCATOR_TYPE_AUTO);
    if (!hdr_wrapper->IsValid()) {
      FML_LOG(ERROR) << "[MPF] HDR Pixelmap invalid";
    } else {
      uint32_t buffer_size = hdr_wrapper->row_stride_ * hdr_wrapper->height_;
      Image_ErrorCode read_err = hdr_wrapper->ReadPixels(
          static_cast<uint8_t*>(pixels), buffer_size, row_bytes);
      if (read_err == IMAGE_SUCCESS) {
        if (frame_count_ > 1 &&
            total_cached_bytes_ <= kMaxGlobalCacheSize - buffer_size) {
          cached_pixelmaps_[0] = hdr_wrapper;
          total_cached_bytes_.fetch_add(buffer_size, std::memory_order_relaxed);
        }
        return true;
      }
      FML_LOG(ERROR) << "[MPF] HDR Pixelmap ReadPixels failed:" << read_err;
    }
  }

  FML_LOG(ERROR) << "[MPF] compose failed; fallback to base decode.";
  has_mpf_gainmap_ = false;
  is_hdr_ = false;
  origin_image_info_ =
      SkImageInfo::Make(origin_image_info_.width(), origin_image_info_.height(),
                        kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  return false;
}

bool OHOSImageGenerator::TryComposeMpfGainmap(const SkImageInfo& info,
                                              void* pixels,
                                              size_t row_bytes,
                                              unsigned int frame_index) {
  if (!has_mpf_gainmap_) {
    return false;
  }
  if (frame_index != 0) {
    FML_LOG(ERROR) << "[MPF] gainmap only supports frame 0";
    return false;
  }
  return TryComposeMpfGainmapInternal(info, pixels, row_bytes);
}

void OHOSImageGenerator::InitMpfGainmap(const sk_sp<SkData>& data) {
  MpfGainmapInfo mpf_info;
  bool has_mpf_gainmap = false;
  if (has_mpf_info_) {
    mpf_info = mpf_info_;
    has_mpf_gainmap = true;
  } else if (!is_hdr_ && data && data->data() && data->size() > 0) {
    has_mpf_gainmap = ParseMpfGainmapInfo(
        static_cast<const uint8_t*>(data->data()), data->size(), &mpf_info);
  }

  if (!has_mpf_gainmap) {
    return;
  }

  MpfGainmapInitResult init_result;
  if (!InitMpfGainmapFromData(static_cast<const uint8_t*>(data->data()),
                              data->size(), mpf_info.offset, mpf_info.size,
                              &init_result)) {
    FML_LOG(ERROR) << "[MPF] gainmap init failed; decoding base only.";
    is_hdr_ = false;
    origin_image_info_ = SkImageInfo::Make(
        origin_image_info_.width(), origin_image_info_.height(),
        kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    return;
  }

  if (mpf_gainmap_image_source_) {
    OH_ImageSourceNative_Release(mpf_gainmap_image_source_);
    mpf_gainmap_image_source_ = nullptr;
  }
  mpf_gainmap_image_source_ = init_result.gainmap_image_source;
  gainmap_data_ = std::move(init_result.gainmap_data);
  gainmap_headroom_ = init_result.headroom;
  has_mpf_gainmap_ = true;
  is_hdr_ = true;
  origin_image_info_ =
      SkImageInfo::Make(origin_image_info_.width(), origin_image_info_.height(),
                        kRGBA_1010102_SkColorType, kOpaque_SkAlphaType);
  origin_image_info_ = origin_image_info_.makeColorSpace(
      SkColorSpace::MakeRGB(SkNamedTransferFn::kHLG, SkNamedGamut::kRec2020));
  if (!impeller::Context::enable_hdr_) {
    FML_LOG(ERROR) << "[MPF] HDR composition requested but HDR is disabled.";
  }
}

}  // namespace flutter
