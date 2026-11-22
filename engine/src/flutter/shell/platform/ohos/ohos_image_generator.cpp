/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "ohos_image_generator.h"

#include <hilog/log.h>

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_buffer/native_buffer.h>
#include <native_color_space_manager/native_color_space_manager.h>
#include <native_window/external_window.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
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
#include "include/core/SkPixmap.h"
#include "third_party/skia/include/codec/SkCodecAnimation.h"

#include "flutter/impeller/renderer/context.h"
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
                                                        bool decodeHdr) {
  OH_DecodingOptions* rawOpts = nullptr;
  Image_ErrorCode errCode = OH_DecodingOptions_Create(&rawOpts);
  if (errCode != IMAGE_SUCCESS || rawOpts == nullptr) {
    FML_LOG(ERROR) << "Create DecodingOptions failed:" << errCode;
    return nullptr;
  }
  OhosDecodingOptionsPtr opts(rawOpts);
  Image_Size size = {(uint32_t)width, (uint32_t)height};
  OH_DecodingOptions_SetDesiredSize(opts.get(), &size);
  if (!decodeHdr) {
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

constexpr size_t kChunkTagSize = 4;
constexpr size_t kGifSignatureSize = 6;
constexpr size_t kMaxEncodedHeaderSearchBytes = 32 * 1024;
constexpr char kGif87aSignature[] = "GIF87a";
constexpr char kGif89aSignature[] = "GIF89a";
constexpr char kRiffSignature[] = "RIFF";
constexpr char kWebpSignature[] = "WEBP";
constexpr char kWebpAnimationChunk[] = "ANIM";

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

// Little-endian integer readers used by the container parsers below.
static constexpr size_t kLeUint16Bytes = 2;
static constexpr size_t kLeUint32Bytes = 4;
static constexpr unsigned int kBitsPerByte = 8;

static uint16_t ReadUint16Le(const uint8_t* bytes) {
  uint16_t value = 0;
  for (size_t i = 0; i < kLeUint16Bytes; ++i) {
    value = static_cast<uint16_t>(value | (bytes[i] << (kBitsPerByte * i)));
  }
  return value;
}

static uint32_t ReadUint32Le(const uint8_t* bytes) {
  uint32_t value = 0;
  for (size_t i = 0; i < kLeUint32Bytes; ++i) {
    value |= static_cast<uint32_t>(bytes[i]) << (kBitsPerByte * i);
  }
  return value;
}

static bool IsGifEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, kGif87aSignature, kGifSignatureSize) ||
         DataStartsWith(data, kGif89aSignature, kGifSignatureSize);
}

static bool IsAnimatedWebPEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, kRiffSignature, kChunkTagSize) &&
         DataContains(data, kWebpSignature, kChunkTagSize) &&
         DataContains(data, kWebpAnimationChunk, kChunkTagSize);
}

}  // namespace

// Parses the total play count of an animated GIF from its NETSCAPE2.0 /
// ANIMEXTS1.0 looping application extension.
//
// Format reference (GIF89a spec, "Application Extension"): the block is
//   0x21 extension introducer, 0xFF application extension label,
//   0x0B block size, an 11-byte application identifier + authentication
//   code ("NETSCAPE2.0" or "ANIMEXTS1.0"), then data sub-blocks. The loop
//   count is the first sub-block (size 3, id 0x01) as a little-endian
//   16-bit value, where 0 means infinite and a wire value of N means
//   "repeat N times after the first play", so the total play count is
//   N + 1. When the extension is absent the frames play exactly once.
//
// This mirrors the Wuffs decoder used by Skia on Android/iOS
// (SkWuffsCodec::onGetRepetitionCount -> wuffs decode_gif.wuffs):
//   https://www.w3.org/Graphics/GIF/spec-gif89a.txt
//   http://www.vurdalakov.net/misc/gif/netscape-looping-application-extension
constexpr uint8_t kGifExtensionIntroducer = 0x21;
constexpr size_t kGifExtensionLabelIndex = 1;
constexpr size_t kGifExtensionBlockSizeIndex = 2;
constexpr uint8_t kGifApplicationExtensionLabel = 0xFF;
constexpr uint8_t kGifApplicationBlockSize = 0x0B;
constexpr uint8_t kGifLoopSubBlockSize = 0x03;
constexpr uint8_t kGifLoopSubBlockId = 0x01;
constexpr size_t kGifLoopSubBlockIdIndex = 1;
constexpr size_t kGifApplicationIdSize = 11;
constexpr size_t kGifApplicationIdOffset = 3;
constexpr size_t kGifLoopSubBlockHeaderSize = 2;  // sub-block size + id
constexpr size_t kGifLoopCountSize = 2;           // 16-bit little-endian
constexpr size_t kGifLoopSubBlockOffset =
    kGifApplicationIdOffset + kGifApplicationIdSize;
constexpr size_t kGifLoopCountOffset =
    kGifLoopSubBlockOffset + kGifLoopSubBlockHeaderSize;
constexpr size_t kGifLoopExtensionSize =
    kGifLoopCountOffset + kGifLoopCountSize;

int OHOSImageGenerator::ParseGifLoopCount(const sk_sp<SkData>& data) {
  constexpr char kNetscape2Id[] = "NETSCAPE2.0";
  constexpr char kAnimexts1Id[] = "ANIMEXTS1.0";
  if (!data || !data->data()) {
    return 1;
  }
  const auto* bytes = static_cast<const uint8_t*>(data->data());
  const size_t limit = std::min(data->size(), kMaxEncodedHeaderSearchBytes);
  if (limit < kGifLoopExtensionSize) {
    return 1;
  }
  // As in the Wuffs decoder, the last looping extension seen wins.
  int play_count = 1;
  for (size_t i = 0; i + kGifLoopExtensionSize <= limit; i++) {
    if (bytes[i] != kGifExtensionIntroducer ||
        bytes[i + kGifExtensionLabelIndex] != kGifApplicationExtensionLabel ||
        bytes[i + kGifExtensionBlockSizeIndex] != kGifApplicationBlockSize) {
      continue;
    }
    const char* app_id =
        reinterpret_cast<const char*>(bytes + i + kGifApplicationIdOffset);
    if (memcmp(app_id, kNetscape2Id, kGifApplicationIdSize) != 0 &&
        memcmp(app_id, kAnimexts1Id, kGifApplicationIdSize) != 0) {
      continue;
    }
    if (bytes[i + kGifLoopSubBlockOffset] != kGifLoopSubBlockSize ||
        bytes[i + kGifLoopSubBlockOffset + kGifLoopSubBlockIdIndex] !=
            kGifLoopSubBlockId) {
      continue;
    }
    const int loop = ReadUint16Le(bytes + i + kGifLoopCountOffset);
    play_count = loop == 0 ? 0 : loop + 1;
  }
  return play_count;
}

// Parses the total play count of an animated WebP from its ANIM chunk.
//
// Format reference (WebP container spec): a RIFF container starts with
//   "RIFF" <32-bit LE size> "WEBP"
// followed by chunks of
//   <4-byte tag> <32-bit LE payload size> <payload> [1 pad byte if odd]
// The ANIM chunk payload is a 32-bit background color followed by a 16-bit
// little-endian loop count, where the value is the total number of plays
// and 0 means infinite. Chunks are walked structurally so that metadata
// payloads whose bytes happen to contain the ANIM tag cannot be mistaken
// for the real chunk.
//
// This mirrors the libwebp demuxer used by Skia on Android/iOS
// (SkWebpCodec::onGetRepetitionCount -> WebPDemuxGetI(WEBP_FF_LOOP_COUNT)):
//   (see the WebP RIFF container specification, ANIM chunk)
constexpr size_t kRiffHeaderSize = 12;  // "RIFF" + 32-bit size + "WEBP"
constexpr size_t kChunkSizeFieldSize = 4;
constexpr size_t kChunkHeaderSize = kChunkTagSize + kChunkSizeFieldSize;
constexpr size_t kAnimBackgroundColorSize = 4;
constexpr size_t kAnimLoopCountSize = 2;
constexpr size_t kAnimPayloadSize =
    kAnimBackgroundColorSize + kAnimLoopCountSize;
constexpr size_t kRiffChunkAlignment =
    2;  // RIFF chunks are padded to even sizes

int OHOSImageGenerator::ParseWebpAnimLoopCount(const sk_sp<SkData>& data) {
  if (!data || !data->data() || data->size() < kRiffHeaderSize) {
    return -1;
  }
  const auto* bytes = static_cast<const uint8_t*>(data->data());
  const size_t size = data->size();
  if (memcmp(bytes, kRiffSignature, kChunkTagSize) != 0 ||
      memcmp(bytes + kRiffHeaderSize - kChunkTagSize, kWebpSignature,
             kChunkTagSize) != 0) {
    return -1;
  }
  size_t offset = kRiffHeaderSize;
  while (offset + kChunkHeaderSize + kAnimPayloadSize <= size) {
    const uint32_t chunk_size = ReadUint32Le(bytes + offset + kChunkTagSize);
    const size_t payload = offset + kChunkHeaderSize;
    if (chunk_size > size - payload) {
      return -1;  // Corrupt or truncated chunk.
    }
    if (memcmp(bytes + offset, kWebpAnimationChunk, kChunkTagSize) == 0) {
      if (chunk_size < kAnimPayloadSize) {
        return -1;
      }
      return ReadUint16Le(bytes + payload + kAnimBackgroundColorSize);
    }
    offset = payload + chunk_size + (chunk_size % kRiffChunkAlignment);
  }
  return -1;
}

// Returns the total play count of the encoded animation parsed from the raw
// data (0 = infinite, >0 = total plays), or -1 when the data is not a
// supported animation or its loop metadata cannot be parsed.
int OHOSImageGenerator::ParseEncodedPlayCount(const sk_sp<SkData>& data) {
  if (IsGifEncodedData(data)) {
    return ParseGifLoopCount(data);
  }
  if (IsAnimatedWebPEncodedData(data)) {
    return ParseWebpAnimLoopCount(data);
  }
  return -1;
}

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

constexpr size_t kPngSignatureSize = 8;
constexpr char kPngSignature[] = "\x89PNG\r\n\x1A\n";
constexpr char kPngAnimationChunk[] = "acTL";

static bool IsAnimatedPngEncodedData(const sk_sp<SkData>& data) {
  return DataStartsWith(data, kPngSignature, kPngSignatureSize) &&
         DataContains(data, kPngAnimationChunk, kChunkTagSize);
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
    FML_LOG(ERROR) << "OHOS DMA: texture imported " << pixelmap_->width_ << "x"
                   << pixelmap_->height_ << " fmt=" << pixelmap_->pixel_format_
                   << " stride=" << pixelmap_->row_stride_
                   << " cs=" << pixelmap_->color_space_;
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
      FML_LOG(ERROR) << "OHOS DMA: OH_PixelmapNative_GetNativeBuffer failed ("
                     << err << ")";
      return nullptr;
    }

    holder->window_buffer =
        OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
            holder->native_buffer);
    if (holder->window_buffer == nullptr) {
      FML_LOG(ERROR)
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
        context_vk, holder->window_buffer, textureColorSpace,
        OhosColorSpaceIsLimitedHdr(pixelmap_->color_space_));
    if (!texture_source->IsValid()) {
      FML_LOG(ERROR)
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
                                       bool unsupportedDmaEncodedData,
                                       int playCount)
    : image_source_(image_source),
      data_(data),
      play_count_(playCount),
      unsupported_dma_encoded_data_(unsupportedDmaEncodedData) {
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

  ComputeOriginImageInfo();

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
  if (frame_count_ == 1) {
    return 1;
  }
  if (play_count_ <= 0) {
    // Loop count unknown (property read failed or no loop extension) or
    // infinite - keep the legacy infinite-loop behavior.
    return kInfinitePlayCount;
  }
  return static_cast<unsigned int>(play_count_);
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
    const bool request_hdr =
        is_hdr_ && info.colorType() == kRGBA_1010102_SkColorType;
    image_pixelmap = CreatePixelMap(info.width(), info.height(), frame_index,
                                    /*preferDma=*/false, request_hdr);
    if (request_hdr && image_pixelmap &&
        (!image_pixelmap->dynamic_range_known_ || image_pixelmap->is_hdr_) &&
        impeller::TextureTransfer(OhosColorSpaceToTextureColorSpace(
            image_pixelmap->color_space_)) == 0) {
      // HDR without a known transfer function cannot be interpreted safely.
      FML_LOG(WARNING) << "HDR PixelMap has no recognized color space; "
                          "requesting an SDR decode.";
      image_pixelmap.reset();
    }
    if (request_hdr && !image_pixelmap) {
      FML_LOG(WARNING) << "HDR decode unavailable; requesting an SDR decode.";
      image_pixelmap = CreatePixelMap(info.width(), info.height(), frame_index,
                                      /*preferDma=*/false, /*decodeHdr=*/false);
      if (image_pixelmap &&
          (!image_pixelmap->dynamic_range_known_ || image_pixelmap->is_hdr_)) {
        FML_LOG(WARNING) << "SDR fallback did not return a known SDR PixelMap";
        image_pixelmap.reset();
      }
    }
  }

  if (image_pixelmap) {
    if (image_pixelmap->width_ != static_cast<uint32_t>(info.width()) ||
        image_pixelmap->height_ != static_cast<uint32_t>(info.height())) {
      FML_LOG(ERROR) << "Decoded PixelMap size differs from requested size";
      return false;
    }
    const uint64_t physical_size =
        static_cast<uint64_t>(image_pixelmap->row_stride_) *
        image_pixelmap->height_;
    if (physical_size > std::numeric_limits<uint32_t>::max()) {
      FML_LOG(ERROR) << "PixelMap buffer is too large";
      return false;
    }
    const uint32_t buffer_size = static_cast<uint32_t>(physical_size);
    const int32_t expected_pixel_format =
        info.colorType() == kRGBA_1010102_SkColorType
            ? PIXEL_FORMAT_RGBA_1010102
            : PIXEL_FORMAT_RGBA_8888;
    if (row_bytes > std::numeric_limits<uint32_t>::max()) {
      return false;
    }
    if (image_pixelmap->pixel_format_ != expected_pixel_format ||
        image_pixelmap->alpha_type_ == PIXELMAP_ALPHA_TYPE_UNPREMULTIPLIED) {
      // Convert the system's actual format and alpha representation instead of
      // interpreting its pixels as the descriptor's requested format.
      if (image_pixelmap->pixel_format_ != PIXEL_FORMAT_RGBA_8888 &&
          image_pixelmap->pixel_format_ != PIXEL_FORMAT_RGBA_1010102) {
        FML_LOG(ERROR) << "Unsupported PixelMap format: "
                       << image_pixelmap->pixel_format_;
        return false;
      }
      const size_t tight_row_bytes = info.minRowBytes();
      if (tight_row_bytes > std::numeric_limits<uint32_t>::max() ||
          (info.height() > 0 &&
           tight_row_bytes >
               std::numeric_limits<size_t>::max() / info.height())) {
        return false;
      }
      std::vector<uint8_t> staged_pixels(tight_row_bytes * info.height());
      if (image_pixelmap->ReadPixels(staged_pixels.data(), buffer_size,
                                     static_cast<uint32_t>(tight_row_bytes)) !=
          IMAGE_SUCCESS) {
        return false;
      }
      const SkAlphaType alpha_type =
          image_pixelmap->alpha_type_ == PIXELMAP_ALPHA_TYPE_UNPREMULTIPLIED
              ? kUnpremul_SkAlphaType
              : kPremul_SkAlphaType;
      const SkColorType source_color_type =
          image_pixelmap->pixel_format_ == PIXEL_FORMAT_RGBA_1010102
              ? kRGBA_1010102_SkColorType
              : kRGBA_8888_SkColorType;
      const auto source_info = SkImageInfo::Make(
          info.width(), info.height(), source_color_type, alpha_type, nullptr);
      if (!SkPixmap(source_info, staged_pixels.data(), tight_row_bytes)
               .readPixels(info, pixels, row_bytes)) {
        return false;
      }
      cached_colorspaces_[frame_index] = image_pixelmap->color_space_;
      return true;
    }
    // OH_PixelmapNative_ReadPixels() needs a buffer large enough for the
    // PixelMap's physical row stride, not just width * bytes-per-pixel.
    // Keep this aligned with the working 3.41 OHOS path.
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
    // Record the color space of the PixelMap actually decoded.
    cached_colorspaces_[frame_index] = image_pixelmap->color_space_;
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
                     static_cast<int>(frame_index), kPreferDma,
                     /*decodeHdr=*/false);
  if (!IsValidDmaPixelMap(pixelmap, decode_dimensions)) {
    return nullptr;
  }
  LogAcceptedDmaPixelMap(pixelmap);
  return std::make_unique<OhosExternalTextureSourceImpl>(std::move(pixelmap));
}
#endif  // IMPELLER_SUPPORTS_RENDERING

void OHOSImageGenerator::ComputeOriginImageInfo() {
  OH_ImageSource_Info* info = nullptr;
  OH_ImageSourceInfo_Create(&info);
  if (info == nullptr) {
    FML_LOG(ERROR) << "OH_ImageSourceInfo_Create() failed, returned nullptr";
    return;
  }

  uint32_t width = 0, height = 0;
  if (OH_ImageSourceNative_GetImageInfo(image_source_, 0, info) !=
          IMAGE_SUCCESS ||
      OH_ImageSourceInfo_GetWidth(info, &width) != IMAGE_SUCCESS ||
      OH_ImageSourceInfo_GetHeight(info, &height) != IMAGE_SUCCESS) {
    OH_ImageSourceInfo_Release(info);
    FML_LOG(ERROR) << "Could not read image source dimensions";
    return;
  }
  if (OH_ImageSourceInfo_GetDynamicRange(info, &is_hdr_) != IMAGE_SUCCESS) {
    is_hdr_ = false;
    FML_LOG(WARNING) << "Could not read image source dynamic range";
  }
  OH_ImageSourceInfo_Release(info);
  FML_LOG(INFO) << "Image info: width=" << width << ", height=" << height
                << ", is_hdr=" << is_hdr_;

  // Decode the source's native dynamic range independently of the current
  // output surface, so a preloaded HDR image retains its own metadata.
  const bool use_hdr = is_hdr_;
  const SkColorType color_type =
      use_hdr ? kRGBA_1010102_SkColorType : kRGBA_8888_SkColorType;
  const SkAlphaType alpha_type = kPremul_SkAlphaType;
  if (rotate_degree_ == 90.f || rotate_degree_ == 270.f) {
    origin_image_info_ =
        SkImageInfo::Make(height, width, color_type, alpha_type);
  } else {
    origin_image_info_ =
        SkImageInfo::Make(width, height, color_type, alpha_type);
  }
  // HDR PixelMap primaries and transfer are identified after decode.
}

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

  // Resolve the animation loop count from the encoded data while it is
  // still available here; the constructor cannot rely on data_ because the
  // copy path releases it.
  const int playCount = ParseEncodedPlayCount(data);

  // Preventing data from being released by the system
  std::shared_ptr<OHOSImageGenerator> generator(new OHOSImageGenerator(
      image_source, isHeldSkData ? data : sk_sp<SkData>(),
      unsupportedDmaEncodedData, playCount));

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
                                   bool preferDma,
                                   bool decodeHdr) {
  auto opts = CreateOhosDecodingOptions(width, height, frame_index,
                                        rotate_degree_, decodeHdr);
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
  std::unique_ptr<OH_NativeColorSpaceManager,
                  decltype(&OH_NativeColorSpaceManager_Destroy)>
      mgrHolder(mgr, OH_NativeColorSpaceManager_Destroy);
  if (colorSpaceRes == IMAGE_SUCCESS && mgrHolder) {
    colorSpaceName =
        OH_NativeColorSpaceManager_GetColorSpaceName(mgrHolder.get());
  }
  if (need_flip_) {
    OH_PixelmapNative_Flip(pixelmap, need_flip_, false);
  }
  auto imagePixelmap =
      std::make_shared<PixelMapOHOS>(pixelmap, actualAllocator);
  const auto textureSpace = OhosColorSpaceToTextureColorSpace(colorSpaceName);
  if (imagePixelmap->dynamic_range_known_ && !imagePixelmap->is_hdr_ &&
      (textureSpace == impeller::TextureColorSpace::kBT2020HLG ||
       textureSpace == impeller::TextureColorSpace::kBT2020PQ ||
       textureSpace == impeller::TextureColorSpace::kDisplayP3HLG ||
       textureSpace == impeller::TextureColorSpace::kDisplayP3PQ ||
       textureSpace == impeller::TextureColorSpace::kHLG ||
       textureSpace == impeller::TextureColorSpace::kPQ)) {
    FML_LOG(WARNING) << "PixelMap reports SDR with HDR color metadata; "
                        "using the decoded SDR range.";
    colorSpaceName = 0;
  }
  if (!preferDma) {
    cached_colorspaces_[frameIndex] = colorSpaceName;
  }
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
  if (OH_PixelmapNative_GetImageInfo(pixelmap, info) != IMAGE_SUCCESS) {
    OH_PixelmapImageInfo_Release(info);
    return;
  }
  if (OH_PixelmapImageInfo_GetWidth(info, &width_) != IMAGE_SUCCESS ||
      OH_PixelmapImageInfo_GetHeight(info, &height_) != IMAGE_SUCCESS ||
      OH_PixelmapImageInfo_GetRowStride(info, &row_stride_) != IMAGE_SUCCESS ||
      OH_PixelmapImageInfo_GetPixelFormat(info, &pixel_format_) !=
          IMAGE_SUCCESS) {
    width_ = height_ = row_stride_ = 0;
    OH_PixelmapImageInfo_Release(info);
    return;
  }
  OH_PixelmapImageInfo_GetAlphaType(info, &alpha_type_);
  dynamic_range_known_ =
      OH_PixelmapImageInfo_GetDynamicRange(info, &is_hdr_) == IMAGE_SUCCESS;
  OH_PixelmapImageInfo_Release(info);
}

Image_ErrorCode OHOSImageGenerator::PixelMapOHOS::ReadPixels(
    uint8_t* dst_buffer,
    uint32_t buffer_size,
    uint32_t row_stride) {
  const uint64_t min_row_bytes_64 =
      static_cast<uint64_t>(width_) * RBGA8888_BYTES;
  if (min_row_bytes_64 > std::numeric_limits<uint32_t>::max()) {
    return IMAGE_BAD_PARAMETER;
  }
  const uint32_t min_row_bytes = static_cast<uint32_t>(min_row_bytes_64);
  if (pixelmap_ == nullptr || dst_buffer == nullptr ||
      row_stride < min_row_bytes || row_stride_ < min_row_bytes ||
      static_cast<uint64_t>(buffer_size) <
          static_cast<uint64_t>(row_stride_) * height_) {
    return IMAGE_BAD_PARAMETER;
  }
  if (pixel_format_ == PIXEL_FORMAT_RGBA_1010102 ||
      row_stride_ != min_row_bytes || row_stride != min_row_bytes) {
    FML_LOG(INFO) << "OHOS PixelMap ReadPixels layout: " << width_ << "x"
                  << height_ << " format=" << pixel_format_
                  << " physical_stride=" << row_stride_
                  << " compact_stride=" << min_row_bytes
                  << " skia_row_bytes=" << row_stride
                  << " read_capacity=" << buffer_size;
  }
  Image_ErrorCode ret_code = IMAGE_SUCCESS;
  uint8_t* temp_dst_buffer = dst_buffer;

  std::unique_ptr<uint8_t[]> tempBuffer;
  const bool needs_staging =
      row_stride_ != min_row_bytes || row_stride != min_row_bytes;
  if (needs_staging) {
    tempBuffer = std::make_unique<uint8_t[]>(buffer_size);
    temp_dst_buffer = tempBuffer.get();
  }
  if (temp_dst_buffer != nullptr) {
    size_t dst_size = buffer_size;
    ret_code =
        OH_PixelmapNative_ReadPixels(pixelmap_, temp_dst_buffer, &dst_size);
    if (ret_code == IMAGE_SUCCESS &&
        dst_size < static_cast<uint64_t>(min_row_bytes) * height_) {
      return IMAGE_BAD_PARAMETER;
    }
  }
  if (needs_staging && temp_dst_buffer != nullptr) {
    if (ret_code == IMAGE_SUCCESS) {
      // OpenHarmony PixelMap::ReadPixels strips allocator padding and writes
      // tightly packed rows. The PixelMap's physical row_stride_ is only the
      // source storage pitch; using it to walk this temporary output skips
      // bytes between rows and produces a garbled image whenever DMA rows are
      // padded. Re-stride from tight ReadPixels output into Skia's rowBytes.
      for (uint32_t i = 0; i < height_; i++) {
        memcpy(dst_buffer + static_cast<size_t>(row_stride) * i,
               temp_dst_buffer + static_cast<size_t>(min_row_bytes) * i,
               min_row_bytes);
      }
    }
  }
  return ret_code;
}

}  // namespace flutter
