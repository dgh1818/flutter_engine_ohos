/*
 * Copyright 2025 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "flutter/shell/platform/ohos/mpf_decoder.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <multimedia/video_processing_engine/image_processing.h>
#include <native_color_space_manager/native_color_space_manager.h>

#include "flutter/fml/platform/ohos/dynamic_library_loader.h"
#include "fml/logging.h"

namespace flutter {
namespace {

constexpr uint32_t kRgba8888Bytes = 4;
constexpr uint32_t kRowStrideAlignment = 64;
constexpr float kGainmapEpsilon = 1.0f / 64.0f;
constexpr float kHlgA = 0.17883277f;
constexpr float kHlgB = 0.28466892f;
constexpr float kHlgC = 0.55991073f;

std::once_flag g_lut_once;
std::array<float, 256> g_srgb_to_linear = {};
std::array<float, 256> g_gain_pow2 = {};

class OhosImageSourceLoader {
  using CreateFromDataWithUserBufferFunc = Image_ErrorCode (*)(
      uint8_t* data,
      size_t datalength,
      OH_ImageSourceNative** image_source);

 public:
  OhosImageSourceLoader();
  ~OhosImageSourceLoader() = default;
  static std::shared_ptr<OhosImageSourceLoader> GetInstance();
  Image_ErrorCode CreateFromDataWithUserBuffer(uint8_t* data,
                                               size_t datalength,
                                               OH_ImageSourceNative** image_source);

 private:
  static constexpr char kImageSourceLibName[] = "libimage_source.so";
  bool is_valid_ = false;
  std::unique_ptr<DynamicLibraryLoader> loader_;
  CreateFromDataWithUserBufferFunc create_from_data_with_user_buffer_ = nullptr;
};

std::shared_ptr<OhosImageSourceLoader> g_image_source_loader = nullptr;
std::once_flag g_image_source_loader_init;

uint32_t AlignTo(uint32_t value, uint32_t alignment) {
  if (alignment == 0) {
    return value;
  }
  return ((value + alignment - 1) / alignment) * alignment;
}

void InitGainmapLuts() {
  for (int i = 0; i < 256; i++) {
    const float c = static_cast<float>(i) / 255.0f;
    g_srgb_to_linear[i] =
        (c <= 0.04045f) ? (c / 12.92f)
                        : powf((c + 0.055f) / 1.055f, 2.4f);
    const float gain = static_cast<float>(i) / 128.0f;
    g_gain_pow2[i] = powf(2.0f, gain);
  }
}

std::shared_ptr<OhosImageSourceLoader> OhosImageSourceLoader::GetInstance() {
  std::call_once(g_image_source_loader_init, [] {
    g_image_source_loader = std::make_shared<OhosImageSourceLoader>();
  });
  return g_image_source_loader;
}

OhosImageSourceLoader::OhosImageSourceLoader()
    : loader_(std::make_unique<DynamicLibraryLoader>(kImageSourceLibName)) {
  std::vector<SymbolInfo> symbols = {
      {"OH_ImageSourceNative_CreateFromDataWithUserBuffer",
       reinterpret_cast<void**>(&create_from_data_with_user_buffer_), 20},
  };
  is_valid_ = loader_->LoadSymbols(symbols);
}

Image_ErrorCode OhosImageSourceLoader::CreateFromDataWithUserBuffer(
    uint8_t* data,
    size_t datalength,
    OH_ImageSourceNative** image_source) {
  if (!is_valid_ || create_from_data_with_user_buffer_ == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  return create_from_data_with_user_buffer_(data, datalength, image_source);
}

float HlgOetf(float l) {
  if (l <= 0.0f) {
    return 0.0f;
  }
  if (l <= (1.0f / 12.0f)) {
    return sqrtf(3.0f * l);
  }
  return kHlgA * logf(12.0f * l - kHlgB) + kHlgC;
}

uint32_t PackRGBA1010102(uint32_t r, uint32_t g, uint32_t b) {
  return (r & 0x3FFu) | ((g & 0x3FFu) << 10) | ((b & 0x3FFu) << 20) |
         (0x3u << 30);
}

struct PixelmapInfo {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t row_stride = 0;
  int32_t pixel_format = 0;
};

bool GetPixelmapInfo(OH_PixelmapNative* pixelmap, PixelmapInfo* out) {
  if (pixelmap == nullptr || out == nullptr) {
    return false;
  }

  OH_Pixelmap_ImageInfo* info = nullptr;
  OH_PixelmapImageInfo_Create(&info);
  if (info == nullptr) {
    return false;
  }

  OH_PixelmapNative_GetImageInfo(pixelmap, info);
  OH_PixelmapImageInfo_GetWidth(info, &out->width);
  OH_PixelmapImageInfo_GetHeight(info, &out->height);
  OH_PixelmapImageInfo_GetRowStride(info, &out->row_stride);
  OH_PixelmapImageInfo_GetPixelFormat(info, &out->pixel_format);
  OH_PixelmapImageInfo_Release(info);
  return out->width > 0 && out->height > 0 && out->row_stride > 0;
}

struct PixelmapHandle {
  OH_PixelmapNative* pixelmap = nullptr;
  PixelmapInfo info;

  explicit PixelmapHandle(OH_PixelmapNative* pm) : pixelmap(pm) {
    if (pixelmap != nullptr) {
      GetPixelmapInfo(pixelmap, &info);
    }
  }

  ~PixelmapHandle() {
    if (pixelmap != nullptr) {
      OH_PixelmapNative_Release(pixelmap);
    }
  }

  PixelmapHandle(const PixelmapHandle&) = delete;
  PixelmapHandle& operator=(const PixelmapHandle&) = delete;

  PixelmapHandle(PixelmapHandle&& other) noexcept {
    pixelmap = other.pixelmap;
    info = other.info;
    other.pixelmap = nullptr;
  }

  PixelmapHandle& operator=(PixelmapHandle&& other) noexcept {
    if (this != &other) {
      if (pixelmap != nullptr) {
        OH_PixelmapNative_Release(pixelmap);
      }
      pixelmap = other.pixelmap;
      info = other.info;
      other.pixelmap = nullptr;
    }
    return *this;
  }

  bool IsValid() const { return pixelmap != nullptr && info.width > 0; }

  OH_PixelmapNative* Release() {
    OH_PixelmapNative* released = pixelmap;
    pixelmap = nullptr;
    return released;
  }
};

bool IsJpegStart(const uint8_t* data, size_t size, size_t offset) {
  return data != nullptr && offset + 2 <= size && data[offset] == 0xFF &&
         data[offset + 1] == 0xD8;
}

bool ReadU16(const uint8_t* data,
             size_t size,
             size_t offset,
             uint16_t* out) {
  if (data == nullptr || out == nullptr || offset + 2 > size) {
    return false;
  }
  *out = (static_cast<uint16_t>(data[offset]) << 8) |
         static_cast<uint16_t>(data[offset + 1]);
  return true;
}

bool ReadU32(const uint8_t* data,
             size_t size,
             size_t offset,
             uint32_t* out) {
  if (data == nullptr || out == nullptr || offset + 4 > size) {
    return false;
  }
  *out = (static_cast<uint32_t>(data[offset]) << 24) |
         (static_cast<uint32_t>(data[offset + 1]) << 16) |
         (static_cast<uint32_t>(data[offset + 2]) << 8) |
         static_cast<uint32_t>(data[offset + 3]);
  return true;
}

uint32_t TiffTypeSize(uint16_t type) {
  switch (type) {
    case 1:   // BYTE
    case 2:   // ASCII
    case 7:   // UNDEFINED
      return 1;
    case 3:   // SHORT
      return 2;
    case 4:   // LONG
    case 9:   // SLONG
      return 4;
    case 5:   // RATIONAL
    case 10:  // SRATIONAL
      return 8;
    default:
      return 0;
  }
}

bool ResolveMpfImageOffset(const uint8_t* data,
                           size_t size,
                           size_t mpf_payload_offset,
                           size_t tiff_offset,
                           uint32_t entry_offset,
                           uint32_t entry_size,
                           size_t* resolved_offset) {
  if (resolved_offset == nullptr || entry_size == 0) {
    return false;
  }

  const size_t candidates[] = {
      static_cast<size_t>(entry_offset),
      mpf_payload_offset + static_cast<size_t>(entry_offset),
      tiff_offset + static_cast<size_t>(entry_offset),
  };

  for (size_t candidate : candidates) {
    if (candidate > size || entry_size > size - candidate) {
      continue;
    }
    if (IsJpegStart(data, size, candidate)) {
      *resolved_offset = candidate;
      return true;
    }
  }
  return false;
}

bool TryParseMpfPayload(const uint8_t* data,
                        size_t size,
                        size_t mpf_payload_offset,
                        size_t payload_length,
                        MpfGainmapInfo* out) {
  if (data == nullptr || out == nullptr || payload_length < 4) {
    return false;
  }

  const size_t tiff_offset = mpf_payload_offset + 4;
  const size_t tiff_size = payload_length - 4;
  if (tiff_offset > size || tiff_size < 8 || tiff_size > size - tiff_offset) {
    return false;
  }

  const uint8_t* tiff = data + tiff_offset;

  uint16_t magic = 0;
  if (!ReadU16(tiff, tiff_size, 2, &magic) || magic != 0x2A) {
    return false;
  }

  uint32_t ifd_offset = 0;
  if (!ReadU32(tiff, tiff_size, 4, &ifd_offset)) {
    return false;
  }
  if (ifd_offset + 2 > tiff_size) {
    return false;
  }

  uint16_t entry_count = 0;
  if (!ReadU16(tiff, tiff_size, ifd_offset, &entry_count)) {
    return false;
  }

  const size_t entries_start = ifd_offset + 2;
  if (entries_start + static_cast<size_t>(entry_count) * 12 > tiff_size) {
    return false;
  }

  bool found_mp_entry = false;
  uint32_t mp_entry_offset = 0;
  uint32_t mp_entry_byte_count = 0;
  bool found_num_images = false;
  uint32_t num_images = 0;

  for (uint16_t i = 0; i < entry_count; i++) {
    const size_t entry_offset = entries_start + static_cast<size_t>(i) * 12;
    uint16_t tag = 0;
    uint16_t type = 0;
    uint32_t count = 0;
    uint32_t value = 0;
    if (!ReadU16(tiff, tiff_size, entry_offset, &tag) ||
        !ReadU16(tiff, tiff_size, entry_offset + 2, &type) ||
        !ReadU32(tiff, tiff_size, entry_offset + 4, &count) ||
        !ReadU32(tiff, tiff_size, entry_offset + 8, &value)) {
      return false;
    }

    if (tag == 0xB001 && type == 4 && count == 1) {
      num_images = value;
      found_num_images = true;
    }

    if (tag == 0xB002) {
      const uint32_t type_size = TiffTypeSize(type);
      if (type_size == 0) {
        continue;
      }
      mp_entry_byte_count = count * type_size;
      if (mp_entry_byte_count <= 4) {
        mp_entry_offset = static_cast<uint32_t>(entry_offset + 8);
      } else {
        mp_entry_offset = value;
      }
      found_mp_entry = true;
    }
  }

  if (!found_mp_entry || mp_entry_byte_count < 32) {
    return false;
  }

  if (mp_entry_offset > tiff_size ||
      mp_entry_byte_count > tiff_size - mp_entry_offset) {
    return false;
  }

  size_t entry_count_total = mp_entry_byte_count / 16;
  if (found_num_images && num_images > 0) {
    entry_count_total = std::min<size_t>(entry_count_total, num_images);
  }
  if (entry_count_total < 2) {
    return false;
  }

  const size_t entry_offset = mp_entry_offset + 16;
  uint32_t entry_size = 0;
  uint32_t entry_data_offset = 0;
  if (!ReadU32(tiff, tiff_size, entry_offset + 4, &entry_size) ||
      !ReadU32(tiff, tiff_size, entry_offset + 8, &entry_data_offset)) {
    return false;
  }
  if (entry_size == 0) {
    return false;
  }

  size_t resolved_offset = 0;
  if (!ResolveMpfImageOffset(data, size, mpf_payload_offset, tiff_offset,
                             entry_data_offset, entry_size, &resolved_offset)) {
    return false;
  }

  out->offset = resolved_offset;
  out->size = entry_size;
  return true;
}

}  // namespace

bool ParseGainmapHeadroomFromXmp(const uint8_t* data,
                                 size_t size,
                                 float* out_headroom) {
  if (data == nullptr || size == 0 || out_headroom == nullptr) {
    return false;
  }

  constexpr char kKey[] = "HDRGainMapHeadroom";
  constexpr size_t kKeyLen = sizeof(kKey) - 1;

  auto is_number_char = [](uint8_t c) -> bool {
    return std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == '-' ||
           c == '+' || c == 'e' || c == 'E';
  };

  const uint8_t* begin = data;
  const uint8_t* end = data + size;
  const uint8_t* search_start = begin;

  while (search_start < end) {
    const uint8_t* found =
        std::search(search_start, end, kKey, kKey + kKeyLen);
    if (found == end) {
      return false;
    }

    const uint8_t* p = found + kKeyLen;
    while (p < end && !is_number_char(*p)) {
      p++;
    }
    if (p >= end) {
      return false;
    }

    char buf[64] = {};
    size_t idx = 0;
    while (p < end && idx + 1 < sizeof(buf) && is_number_char(*p)) {
      buf[idx++] = static_cast<char>(*p);
      p++;
    }
    buf[idx] = '\0';

    char* parse_end = nullptr;
    float value = strtof(buf, &parse_end);
    if (parse_end != buf && std::isfinite(value) && value > 0.0f) {
      *out_headroom = value;
      return true;
    }

    search_start = found + kKeyLen;
  }

  return false;
}

Image_ErrorCode CreateImageSourceFromDataMaybeWithUserBuffer(
    uint8_t* data,
    size_t datalength,
    OH_ImageSourceNative** image_source,
    bool* used_user_buffer) {
  if (used_user_buffer != nullptr) {
    *used_user_buffer = false;
  }
  if (image_source == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *image_source = nullptr;

  auto loader = OhosImageSourceLoader::GetInstance();
  if (loader != nullptr) {
    Image_ErrorCode err =
        loader->CreateFromDataWithUserBuffer(data, datalength, image_source);
    if (err == IMAGE_SUCCESS && *image_source != nullptr) {
      if (used_user_buffer != nullptr) {
        *used_user_buffer = true;
      }
      return err;
    }
  }

  return OH_ImageSourceNative_CreateFromData(data, datalength, image_source);
}

bool InitMpfGainmapFromData(const uint8_t* data,
                            size_t size,
                            size_t gainmap_offset,
                            size_t gainmap_size,
                            MpfGainmapInitResult* out) {
  if (data == nullptr || out == nullptr || gainmap_size == 0 ||
      gainmap_offset >= size || gainmap_size > size - gainmap_offset) {
    return false;
  }

  out->gainmap_data.clear();
  out->gainmap_image_source = nullptr;
  out->headroom = kDefaultGainmapHeadroom;

  float parsed_headroom = 0.0f;
  if (ParseGainmapHeadroomFromXmp(data, size, &parsed_headroom)) {
    out->headroom = parsed_headroom;
    FML_LOG(ERROR) << "[MPF] gainmap headroom: " << out->headroom;
  } else {
    FML_LOG(ERROR) << "[MPF] gainmap headroom not found; default="
                   << out->headroom;
  }

  out->gainmap_data.resize(gainmap_size);
  std::memcpy(out->gainmap_data.data(), data + gainmap_offset, gainmap_size);

  OH_ImageSourceNative* gainmap_source = nullptr;
  bool used_user_buffer = false;
  Image_ErrorCode err = CreateImageSourceFromDataMaybeWithUserBuffer(
      out->gainmap_data.data(), out->gainmap_data.size(), &gainmap_source,
      &used_user_buffer);
  if (err != IMAGE_SUCCESS || gainmap_source == nullptr) {
    FML_LOG(ERROR) << "[MPF] Create gainmap ImageSource failed:" << err;
    out->gainmap_data.clear();
    return false;
  }

  out->gainmap_image_source = gainmap_source;

  OH_ImageSource_Info* gainmap_info = nullptr;
  OH_ImageSourceInfo_Create(&gainmap_info);
  if (gainmap_info != nullptr) {
    uint32_t gainmap_width = 0;
    uint32_t gainmap_height = 0;
    OH_ImageSourceNative_GetImageInfo(gainmap_source, 0, gainmap_info);
    OH_ImageSourceInfo_GetWidth(gainmap_info, &gainmap_width);
    OH_ImageSourceInfo_GetHeight(gainmap_info, &gainmap_height);
    FML_LOG(ERROR) << "[MPF] gainmap source size: " << gainmap_width << "x"
                   << gainmap_height << " bytes:" << gainmap_size;
    OH_ImageSourceInfo_Release(gainmap_info);
  } else {
    FML_LOG(ERROR) << "[MPF] gainmap source size: info create failed";
  }

  return true;
}

OH_PixelmapNative* CopyPixelmapToDmaRGBA8888(OH_PixelmapNative* src) {
  PixelmapInfo info;
  if (!GetPixelmapInfo(src, &info)) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: invalid source";
    return nullptr;
  }

  if (info.pixel_format != PIXEL_FORMAT_RGBA_8888) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: unexpected format "
                   << info.pixel_format;
    return nullptr;
  }

  const uint32_t tight_stride = info.width * kRgba8888Bytes;
  FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: tight_stride="
                 << tight_stride << " src_stride=" << info.row_stride;
  if (info.row_stride < tight_stride) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: src_stride too small "
                   << info.row_stride << " < " << tight_stride;
    return nullptr;
  }

  std::vector<uint8_t> src_buf(
      static_cast<size_t>(info.row_stride) * info.height);
  size_t src_size = src_buf.size();
  Image_ErrorCode r =
      OH_PixelmapNative_ReadPixels(src, src_buf.data(), &src_size);
  if (r != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: ReadPixels failed:" << r;
    return nullptr;
  }

  OH_Pixelmap_InitializationOptions* opt = nullptr;
  Image_ErrorCode e = OH_PixelmapInitializationOptions_Create(&opt);
  if (e != IMAGE_SUCCESS || opt == nullptr) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: options create failed:"
                   << e;
    return nullptr;
  }

  OH_PixelmapInitializationOptions_SetWidth(opt, info.width);
  OH_PixelmapInitializationOptions_SetHeight(opt, info.height);
  OH_PixelmapInitializationOptions_SetPixelFormat(opt, PIXEL_FORMAT_RGBA_8888);
  OH_PixelmapInitializationOptions_SetAlphaType(opt,
                                                PIXELMAP_ALPHA_TYPE_OPAQUE);
  const uint32_t desired_stride = AlignTo(tight_stride, kRowStrideAlignment);
  FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: desired_stride="
                 << desired_stride;
  Image_ErrorCode stride_err =
      OH_PixelmapInitializationOptions_SetRowStride(
          opt, static_cast<int32_t>(desired_stride));
  if (stride_err != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: SetRowStride failed:"
                   << stride_err << " stride:" << desired_stride;
  }

  OH_PixelmapNative* dst_pm = nullptr;
  e = OH_PixelmapNative_CreateEmptyPixelmapUsingAllocator(
      opt, IMAGE_ALLOCATOR_MODE_DMA, &dst_pm);
  OH_PixelmapInitializationOptions_Release(opt);
  if (e != IMAGE_SUCCESS || dst_pm == nullptr) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: create DMA pixelmap failed:"
                   << e;
    return nullptr;
  }

  PixelmapInfo dst_info;
  if (!GetPixelmapInfo(dst_pm, &dst_info)) {
    OH_PixelmapNative_Release(dst_pm);
    return nullptr;
  }
  FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: dst_stride="
                 << dst_info.row_stride;
  if (dst_info.row_stride < tight_stride) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: dst_stride too small "
                   << dst_info.row_stride << " < " << tight_stride;
    OH_PixelmapNative_Release(dst_pm);
    return nullptr;
  }
  if (dst_info.row_stride % kRowStrideAlignment != 0) {
    FML_LOG(ERROR)
        << "[MPF] CopyPixelmapToDmaRGBA8888: dst_stride not 64-aligned "
        << dst_info.row_stride;
  }

  std::vector<uint8_t> dst_buf(
      static_cast<size_t>(dst_info.row_stride) * info.height, 0);
  for (uint32_t y = 0; y < info.height; y++) {
    memcpy(dst_buf.data() + static_cast<size_t>(dst_info.row_stride) * y,
           src_buf.data() + static_cast<size_t>(info.row_stride) * y,
           tight_stride);
  }

  Image_ErrorCode wret =
      OH_PixelmapNative_WritePixels(dst_pm, dst_buf.data(), dst_buf.size());
  if (wret != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] CopyPixelmapToDmaRGBA8888: WritePixels failed:"
                   << wret;
    OH_PixelmapNative_Release(dst_pm);
    return nullptr;
  }

  return dst_pm;
}

OH_PixelmapNative* ExpandSingleChannelToDmaRGBA8888(OH_PixelmapNative* src) {
  PixelmapInfo info;
  if (!GetPixelmapInfo(src, &info)) {
    FML_LOG(ERROR) << "[MPF] ExpandSingleChannelToDmaRGBA8888: invalid source";
    return nullptr;
  }
  if (info.row_stride < info.width) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: src_stride too small "
        << info.row_stride << " width " << info.width;
    return nullptr;
  }

  std::vector<uint8_t> src_buf(
      static_cast<size_t>(info.row_stride) * info.height);
  size_t src_size = src_buf.size();
  Image_ErrorCode r =
      OH_PixelmapNative_ReadPixels(src, src_buf.data(), &src_size);
  if (r != IMAGE_SUCCESS) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: ReadPixels failed:" << r;
    return nullptr;
  }

  OH_Pixelmap_InitializationOptions* opt = nullptr;
  Image_ErrorCode e = OH_PixelmapInitializationOptions_Create(&opt);
  if (e != IMAGE_SUCCESS || opt == nullptr) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: options create failed:"
        << e;
    return nullptr;
  }

  OH_PixelmapInitializationOptions_SetWidth(opt, info.width);
  OH_PixelmapInitializationOptions_SetHeight(opt, info.height);
  OH_PixelmapInitializationOptions_SetPixelFormat(opt, PIXEL_FORMAT_RGBA_8888);
  OH_PixelmapInitializationOptions_SetAlphaType(opt,
                                                PIXELMAP_ALPHA_TYPE_OPAQUE);
  const uint32_t tight_stride = info.width * kRgba8888Bytes;
  const uint32_t desired_stride = AlignTo(tight_stride, kRowStrideAlignment);
  Image_ErrorCode stride_err =
      OH_PixelmapInitializationOptions_SetRowStride(
          opt, static_cast<int32_t>(desired_stride));
  if (stride_err != IMAGE_SUCCESS) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: SetRowStride failed:"
        << stride_err << " stride:" << desired_stride;
  }

  OH_PixelmapNative* dst_pm = nullptr;
  e = OH_PixelmapNative_CreateEmptyPixelmapUsingAllocator(
      opt, IMAGE_ALLOCATOR_MODE_DMA, &dst_pm);
  OH_PixelmapInitializationOptions_Release(opt);
  if (e != IMAGE_SUCCESS || dst_pm == nullptr) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: create DMA pixelmap failed:"
        << e;
    return nullptr;
  }

  PixelmapInfo dst_info;
  if (!GetPixelmapInfo(dst_pm, &dst_info)) {
    OH_PixelmapNative_Release(dst_pm);
    return nullptr;
  }

  FML_LOG(ERROR)
      << "[MPF] ExpandSingleChannelToDmaRGBA8888: tight_stride="
      << tight_stride << " src_stride=" << info.row_stride;
  if (dst_info.row_stride < tight_stride) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: dst_stride too small "
        << dst_info.row_stride << " < " << tight_stride;
    OH_PixelmapNative_Release(dst_pm);
    return nullptr;
  }
  FML_LOG(ERROR)
      << "[MPF] ExpandSingleChannelToDmaRGBA8888: dst_stride="
      << dst_info.row_stride;
  if (dst_info.row_stride % kRowStrideAlignment != 0) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: dst_stride not 64-aligned "
        << dst_info.row_stride;
  }

  std::vector<uint8_t> dst_buf(
      static_cast<size_t>(dst_info.row_stride) * info.height, 0);
  for (uint32_t y = 0; y < info.height; y++) {
    const uint8_t* src_row =
        src_buf.data() + static_cast<size_t>(info.row_stride) * y;
    uint8_t* dst_row =
        dst_buf.data() + static_cast<size_t>(dst_info.row_stride) * y;
    for (uint32_t x = 0; x < info.width; x++) {
      const uint8_t v = src_row[x];
      uint8_t* dst_px = dst_row + static_cast<size_t>(x) * kRgba8888Bytes;
      dst_px[0] = v;
      dst_px[1] = v;
      dst_px[2] = v;
      dst_px[3] = 0xFF;
    }
  }

  Image_ErrorCode wret =
      OH_PixelmapNative_WritePixels(dst_pm, dst_buf.data(), dst_buf.size());
  if (wret != IMAGE_SUCCESS) {
    FML_LOG(ERROR)
        << "[MPF] ExpandSingleChannelToDmaRGBA8888: WritePixels failed:" << wret;
    OH_PixelmapNative_Release(dst_pm);
    return nullptr;
  }

  return dst_pm;
}

bool ComposeGainmapToRGBA1010102(OH_PixelmapNative* sdr,
                                 OH_PixelmapNative* gainmap,
                                 uint8_t* dst,
                                 size_t row_bytes,
                                 float headroom) {
  if (sdr == nullptr || gainmap == nullptr || dst == nullptr) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: invalid input";
    return false;
  }

  PixelmapInfo sdr_info;
  PixelmapInfo gain_info;
  if (!GetPixelmapInfo(sdr, &sdr_info) ||
      !GetPixelmapInfo(gainmap, &gain_info)) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: invalid pixelmap info";
    return false;
  }
  if (sdr_info.pixel_format != PIXEL_FORMAT_RGBA_8888 ||
      gain_info.pixel_format != PIXEL_FORMAT_RGBA_8888) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: unsupported pixel format sdr="
                   << sdr_info.pixel_format
                   << " gainmap=" << gain_info.pixel_format;
    return false;
  }
  if (sdr_info.width != gain_info.width ||
      sdr_info.height != gain_info.height) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: size mismatch sdr="
                   << sdr_info.width << "x" << sdr_info.height
                   << " gainmap=" << gain_info.width << "x"
                   << gain_info.height;
    return false;
  }
  const uint32_t tight_stride = sdr_info.width * kRgba8888Bytes;
  if (row_bytes < tight_stride) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: row_bytes too small "
                   << row_bytes << " < " << tight_stride;
    return false;
  }
  if (sdr_info.row_stride < tight_stride ||
      gain_info.row_stride < tight_stride) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: src stride too small sdr="
                   << sdr_info.row_stride
                   << " gainmap=" << gain_info.row_stride;
    return false;
  }

  const size_t tight_size =
      static_cast<size_t>(tight_stride) * sdr_info.height;
  std::vector<uint8_t> sdr_buf(tight_size);
  size_t sdr_size = sdr_buf.size();
  Image_ErrorCode sdr_ret =
      OH_PixelmapNative_ReadPixels(sdr, sdr_buf.data(), &sdr_size);
  if (sdr_ret != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: read sdr failed " << sdr_ret;
    return false;
  }

  std::vector<uint8_t> gain_buf(tight_size);
  size_t gain_size = gain_buf.size();
  Image_ErrorCode gain_ret =
      OH_PixelmapNative_ReadPixels(gainmap, gain_buf.data(), &gain_size);
  if (gain_ret != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] ComposeGainmap: read gainmap failed " << gain_ret;
    return false;
  }

  std::call_once(g_lut_once, InitGainmapLuts);

  const float safe_headroom =
      (std::isfinite(headroom) && headroom > 0.0f) ? headroom
                                                   : kDefaultGainmapHeadroom;
  const float inv_headroom = 1.0f / safe_headroom;

  for (uint32_t y = 0; y < sdr_info.height; y++) {
    const uint8_t* sdr_row =
        sdr_buf.data() + static_cast<size_t>(tight_stride) * y;
    const uint8_t* gain_row =
        gain_buf.data() + static_cast<size_t>(tight_stride) * y;
    uint32_t* dst_row =
        reinterpret_cast<uint32_t*>(dst + static_cast<size_t>(row_bytes) * y);

    for (uint32_t x = 0; x < sdr_info.width; x++) {
      const uint8_t sr = sdr_row[x * 4 + 0];
      const uint8_t sg = sdr_row[x * 4 + 1];
      const uint8_t sb = sdr_row[x * 4 + 2];

      float r = g_srgb_to_linear[sr];
      float g = g_srgb_to_linear[sg];
      float b = g_srgb_to_linear[sb];

      const uint8_t gr = gain_row[x * 4 + 0];
      const uint8_t gg = gain_row[x * 4 + 1];
      const uint8_t gb = gain_row[x * 4 + 2];

      const float gain_r = g_gain_pow2[gr];
      const float gain_g = g_gain_pow2[gg];
      const float gain_b = g_gain_pow2[gb];

      r = gain_r * (r + kGainmapEpsilon) - kGainmapEpsilon;
      g = gain_g * (g + kGainmapEpsilon) - kGainmapEpsilon;
      b = gain_b * (b + kGainmapEpsilon) - kGainmapEpsilon;

      if (r < 0.0f) r = 0.0f;
      if (g < 0.0f) g = 0.0f;
      if (b < 0.0f) b = 0.0f;

      float r_norm = r * inv_headroom;
      float g_norm = g * inv_headroom;
      float b_norm = b * inv_headroom;

      if (r_norm > 1.0f) r_norm = 1.0f;
      if (g_norm > 1.0f) g_norm = 1.0f;
      if (b_norm > 1.0f) b_norm = 1.0f;

      float r_hlg = HlgOetf(r_norm);
      float g_hlg = HlgOetf(g_norm);
      float b_hlg = HlgOetf(b_norm);

      if (r_hlg < 0.0f) r_hlg = 0.0f;
      if (g_hlg < 0.0f) g_hlg = 0.0f;
      if (b_hlg < 0.0f) b_hlg = 0.0f;

      uint32_t r10 = static_cast<uint32_t>(r_hlg * 1023.0f + 0.5f);
      uint32_t g10 = static_cast<uint32_t>(g_hlg * 1023.0f + 0.5f);
      uint32_t b10 = static_cast<uint32_t>(b_hlg * 1023.0f + 0.5f);

      if (r10 > 1023u) r10 = 1023u;
      if (g10 > 1023u) g10 = 1023u;
      if (b10 > 1023u) b10 = 1023u;

      dst_row[x] = PackRGBA1010102(r10, g10, b10);
    }
  }

  return true;
}

bool ComposeMpfGainmap(const MpfGainmapComposeInput& input,
                       MpfGainmapComposeResult* out) {
  if (out == nullptr) {
    return false;
  }
  *out = {};

  if (input.pixels == nullptr) {
    return false;
  }
  if (input.base_source == nullptr || input.gainmap_source == nullptr) {
    FML_LOG(ERROR) << "[MPF] gainmap image_source is nullptr";
    return false;
  }
  if (input.width <= 0 || input.height <= 0) {
    return false;
  }
  if (input.row_bytes <
      static_cast<size_t>(input.width) * kRgba8888Bytes) {
    FML_LOG(ERROR) << "[MPF] row_bytes too small:" << input.row_bytes
                   << " width:" << input.width;
    return false;
  }
  if (input.row_bytes % kRowStrideAlignment != 0) {
    FML_LOG(ERROR)
        << "[MPF] output row_bytes not 64-aligned: " << input.row_bytes;
  }

  auto CreatePixelMapForSource = [&](OH_ImageSourceNative* image_source,
                                     bool is_gainmap) -> PixelmapHandle {
    if (!image_source) {
      return PixelmapHandle(nullptr);
    }

    OH_DecodingOptions* opts = nullptr;
    Image_ErrorCode err = OH_DecodingOptions_Create(&opts);
    if (err != IMAGE_SUCCESS || !opts) {
      return PixelmapHandle(nullptr);
    }

    Image_Size size = {static_cast<uint32_t>(input.width),
                       static_cast<uint32_t>(input.height)};
    OH_DecodingOptions_SetDesiredSize(opts, &size);
    OH_DecodingOptions_SetDesiredDynamicRange(opts, IMAGE_DYNAMIC_RANGE_SDR);
    OH_DecodingOptions_SetRotate(opts, input.rotate_degree);
    OH_DecodingOptions_SetIndex(opts, 0);

    const char* label = is_gainmap ? "gainmap" : "sdr";
    if (!is_gainmap) {
      OH_PixelmapNative* dma_pm = nullptr;
      err = OH_ImageSourceNative_CreatePixelmapUsingAllocator(
          image_source, opts, IMAGE_ALLOCATOR_TYPE_DMA, &dma_pm);
      if (err == IMAGE_SUCCESS && dma_pm) {
        if (input.need_flip) {
          OH_PixelmapNative_Flip(dma_pm, input.need_flip, false);
        }
        PixelmapHandle dma_handle(dma_pm);
        if (dma_handle.IsValid() &&
            dma_handle.info.pixel_format == PIXEL_FORMAT_RGBA_8888) {
          FML_LOG(ERROR) << "[MPF] " << label
                         << " DMA decode success size:"
                         << dma_handle.info.width << "x"
                         << dma_handle.info.height
                         << " stride:" << dma_handle.info.row_stride
                         << " format:" << dma_handle.info.pixel_format;
          OH_DecodingOptions_Release(opts);
          return dma_handle;
        }
        if (dma_handle.IsValid()) {
          FML_LOG(ERROR) << "[MPF] " << label
                         << " DMA decode format mismatch, fallback to CPU:"
                         << dma_handle.info.pixel_format;
        }
      } else {
        FML_LOG(ERROR) << "[MPF] " << label << " DMA decode failed:" << err;
      }
    }

    OH_DecodingOptions_SetPixelFormat(opts, PIXEL_FORMAT_RGBA_8888);

    OH_PixelmapNative* cpu_pm = nullptr;
    Image_ErrorCode cpu_err =
        OH_ImageSourceNative_CreatePixelmap(image_source, opts, &cpu_pm);
    OH_DecodingOptions_Release(opts);
    if (cpu_err != IMAGE_SUCCESS || !cpu_pm) {
      FML_LOG(ERROR) << "[MPF] CreatePixelMapForSource: CPU decode failed:"
                     << cpu_err;
      return PixelmapHandle(nullptr);
    }
    if (input.need_flip) {
      OH_PixelmapNative_Flip(cpu_pm, input.need_flip, false);
    }
    PixelmapHandle cpu_handle(cpu_pm);
    if (!cpu_handle.IsValid()) {
      return PixelmapHandle(nullptr);
    }
    FML_LOG(ERROR) << "[MPF] " << label << " CPU decode pixel format: "
                   << cpu_handle.info.pixel_format;
    if (is_gainmap) {
      FML_LOG(ERROR) << "[MPF] gainmap CPU decode uses CPU pixelmap";
      return cpu_handle;
    }

    OH_PixelmapNative* converted_pm = CopyPixelmapToDmaRGBA8888(cpu_pm);
    if (converted_pm == nullptr) {
      return PixelmapHandle(nullptr);
    }
    return PixelmapHandle(converted_pm);
  };

  PixelmapHandle base_pixelmap =
      CreatePixelMapForSource(input.base_source, false);
  PixelmapHandle gainmap_pixelmap =
      CreatePixelMapForSource(input.gainmap_source, true);
  if (!base_pixelmap.IsValid() || !gainmap_pixelmap.IsValid()) {
    return false;
  }

  if (ComposeGainmapToRGBA1010102(
          base_pixelmap.pixelmap, gainmap_pixelmap.pixelmap,
          static_cast<uint8_t*>(input.pixels), input.row_bytes,
          input.headroom)) {
    FML_LOG(ERROR) << "[MPF] CPU compose success";
    out->success = true;
    return true;
  }
  FML_LOG(ERROR) << "[MPF] CPU compose failed, fallback to VPE";

  auto ColorSpaceNameToString = [](int name) -> const char* {
    switch (name) {
      case DISPLAY_P3:
        return "DISPLAY_P3";
      case DISPLAY_P3_LIMIT:
        return "DISPLAY_P3_LIMIT";
      case P3_HLG:
        return "P3_HLG";
      case P3_PQ:
        return "P3_PQ";
      case SRGB:
        return "SRGB";
      case BT2020_HLG:
        return "BT2020_HLG";
      case BT2020_PQ:
        return "BT2020_PQ";
      case BT709:
        return "BT709";
      default:
        return "UNKNOWN";
    }
  };

  auto LogPixelmapInfo = [&](const char* label,
                             const PixelmapHandle& pixelmap) {
    if (!pixelmap.IsValid()) {
      FML_LOG(ERROR) << "[MPF] " << label << " pixelmap: null";
      return;
    }
    if (pixelmap.info.row_stride % kRowStrideAlignment != 0) {
      FML_LOG(ERROR)
          << "[MPF] " << label << " pixelmap stride not 64-aligned: "
          << pixelmap.info.row_stride;
    }
    OH_NativeColorSpaceManager* color_space = nullptr;
    int color_space_name = 0;
    Image_ErrorCode cs_ret = OH_PixelmapNative_GetColorSpaceNative(
        pixelmap.pixelmap, &color_space);
    if (cs_ret == IMAGE_SUCCESS && color_space != nullptr) {
      color_space_name =
          OH_NativeColorSpaceManager_GetColorSpaceName(color_space);
    }
    FML_LOG(ERROR) << "[MPF] " << label
                   << " pixelmap size:" << pixelmap.info.width << "*"
                   << pixelmap.info.height
                   << " stride:" << pixelmap.info.row_stride
                   << " format:" << pixelmap.info.pixel_format
                   << " colorspace:" << ColorSpaceNameToString(color_space_name)
                   << "(" << color_space_name << ")";
    if (color_space != nullptr) {
      OH_NativeColorSpaceManager_Destroy(color_space);
    }
  };

  auto GetColorSpaceName = [&](OH_PixelmapNative* pixelmap) -> int {
    if (pixelmap == nullptr) {
      return 0;
    }
    OH_NativeColorSpaceManager* color_space = nullptr;
    int color_space_name = 0;
    Image_ErrorCode cs_ret =
        OH_PixelmapNative_GetColorSpaceNative(pixelmap, &color_space);
    if (cs_ret == IMAGE_SUCCESS && color_space != nullptr) {
      color_space_name =
          OH_NativeColorSpaceManager_GetColorSpaceName(color_space);
    }
    if (color_space != nullptr) {
      OH_NativeColorSpaceManager_Destroy(color_space);
    }
    return color_space_name;
  };

  OH_Pixelmap_InitializationOptions* options = nullptr;
  Image_ErrorCode err = OH_PixelmapInitializationOptions_Create(&options);
  if (err != IMAGE_SUCCESS || options == nullptr) {
    FML_LOG(ERROR) << "[MPF] Create Pixelmap options failed:" << err;
    return false;
  }
  OH_PixelmapInitializationOptions_SetWidth(
      options, static_cast<uint32_t>(input.width));
  OH_PixelmapInitializationOptions_SetHeight(
      options, static_cast<uint32_t>(input.height));
  OH_PixelmapInitializationOptions_SetPixelFormat(
      options, PIXEL_FORMAT_RGBA_1010102);
  OH_PixelmapInitializationOptions_SetAlphaType(options,
                                                PIXELMAP_ALPHA_TYPE_OPAQUE);
  const uint32_t hdr_tight_stride =
      static_cast<uint32_t>(input.width) * kRgba8888Bytes;
  const uint32_t hdr_aligned_stride =
      AlignTo(hdr_tight_stride, kRowStrideAlignment);
  FML_LOG(ERROR) << "[MPF] hdr tight_stride=" << hdr_tight_stride
                 << " aligned_stride=" << hdr_aligned_stride;
  Image_ErrorCode hdr_stride_err =
      OH_PixelmapInitializationOptions_SetRowStride(
          options, static_cast<int32_t>(hdr_aligned_stride));
  if (hdr_stride_err != IMAGE_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] Set HDR row_stride failed: " << hdr_stride_err
                   << " stride:" << hdr_aligned_stride;
  }

  OH_PixelmapNative* hdr_pixelmap = nullptr;
  err = OH_PixelmapNative_CreateEmptyPixelmapUsingAllocator(
      options, IMAGE_ALLOCATOR_MODE_DMA, &hdr_pixelmap);
  OH_PixelmapInitializationOptions_Release(options);
  if (err != IMAGE_SUCCESS || hdr_pixelmap == nullptr) {
    FML_LOG(ERROR) << "[MPF] Create HDR Pixelmap failed:" << err;
    return false;
  }
  PixelmapHandle hdr_handle(hdr_pixelmap);

  {
    OH_Pixelmap_ImageInfo* hdr_info = nullptr;
    OH_PixelmapImageInfo_Create(&hdr_info);
    if (hdr_info != nullptr) {
      uint32_t hdr_stride = 0;
      OH_PixelmapNative_GetImageInfo(hdr_handle.pixelmap, hdr_info);
      OH_PixelmapImageInfo_GetRowStride(hdr_info, &hdr_stride);
      if (hdr_stride % kRowStrideAlignment != 0) {
        FML_LOG(ERROR) << "[MPF] hdr pixelmap stride not 64-aligned: "
                       << hdr_stride;
      }
      OH_PixelmapImageInfo_Release(hdr_info);
    }
  }

  OH_NativeColorSpaceManager* color_space_sdr =
      OH_NativeColorSpaceManager_CreateFromName(DISPLAY_P3);
  OH_NativeColorSpaceManager* color_space_hdr =
      OH_NativeColorSpaceManager_CreateFromName(BT2020_HLG);
  if (color_space_sdr != nullptr) {
    OH_PixelmapNative_SetColorSpaceNative(base_pixelmap.pixelmap,
                                          color_space_sdr);
    OH_PixelmapNative_SetColorSpaceNative(gainmap_pixelmap.pixelmap,
                                          color_space_sdr);
    OH_PixelmapNative_SetColorSpaceNative(hdr_handle.pixelmap, color_space_hdr);
  }

  auto SetHdrMetadataType = [&](const char* label,
                                OH_PixelmapNative* pixelmap,
                                OH_Pixelmap_HdrMetadataType type) {
    if (pixelmap == nullptr) {
      return;
    }
    OH_Pixelmap_HdrMetadataValue value = {};
    value.type = type;
    Image_ErrorCode meta_err =
        OH_PixelmapNative_SetMetadata(pixelmap, HDR_METADATA_TYPE, &value);
    if (meta_err != IMAGE_SUCCESS) {
      FML_LOG(ERROR) << "[MPF] Set metadata type failed for " << label << ": "
                     << meta_err;
    }
  };

  auto SetGainmapMetadataDefaults = [&](OH_PixelmapNative* pixelmap,
                                        float headroom) {
    if (pixelmap == nullptr) {
      return;
    }
    OH_Pixelmap_HdrMetadataValue value = {};
    value.gainmapMetadata.writerVersion = 1;
    value.gainmapMetadata.miniVersion = 1;
    value.gainmapMetadata.gainmapChannelNum = 3;
    value.gainmapMetadata.useBaseColorFlag = true;
    value.gainmapMetadata.baseHeadroom = 1.0f;
    value.gainmapMetadata.alternateHeadroom = headroom;
    for (int i = 0; i < 3; i++) {
      value.gainmapMetadata.gainmapMax[i] = 1.0f;
      value.gainmapMetadata.gainmapMin[i] = 0.0f;
      value.gainmapMetadata.gamma[i] = 1.0f;
      value.gainmapMetadata.baselineOffset[i] = 0.0f;
      value.gainmapMetadata.alternateOffset[i] = 0.0f;
    }
    Image_ErrorCode meta_err =
        OH_PixelmapNative_SetMetadata(pixelmap, HDR_GAINMAP_METADATA, &value);
    if (meta_err != IMAGE_SUCCESS) {
      FML_LOG(ERROR) << "[MPF] Set gainmap metadata failed: " << meta_err;
    }
  };

  SetHdrMetadataType("sdr", base_pixelmap.pixelmap, HDR_METADATA_TYPE_BASE);
  SetHdrMetadataType("gainmap", gainmap_pixelmap.pixelmap,
                     HDR_METADATA_TYPE_GAINMAP);
  SetHdrMetadataType("hdr", hdr_handle.pixelmap, HDR_METADATA_TYPE_ALTERNATE);
  SetGainmapMetadataDefaults(gainmap_pixelmap.pixelmap, input.headroom);

  LogPixelmapInfo("sdr", base_pixelmap);
  LogPixelmapInfo("gainmap", gainmap_pixelmap);

  {
    OH_NativeBuffer* native_buffer = nullptr;
    Image_ErrorCode dma_ret = OH_PixelmapNative_GetNativeBuffer(
        base_pixelmap.pixelmap, &native_buffer);
    FML_LOG(ERROR) << "[MPF] sdr GetNativeBuffer: " << dma_ret
                   << " nativeBuffer=" << (native_buffer != nullptr);
    uint32_t byte_count = 0;
    Image_ErrorCode byte_ret =
        OH_PixelmapNative_GetByteCount(base_pixelmap.pixelmap, &byte_count);
    FML_LOG(ERROR) << "[MPF] sdr GetByteCount: " << byte_ret
                   << " bytes:" << byte_count;
  }
  {
    OH_NativeBuffer* native_buffer = nullptr;
    Image_ErrorCode dma_ret = OH_PixelmapNative_GetNativeBuffer(
        gainmap_pixelmap.pixelmap, &native_buffer);
    FML_LOG(ERROR) << "[MPF] gainmap GetNativeBuffer: " << dma_ret
                   << " nativeBuffer=" << (native_buffer != nullptr);
    uint32_t byte_count = 0;
    Image_ErrorCode byte_ret = OH_PixelmapNative_GetByteCount(
        gainmap_pixelmap.pixelmap, &byte_count);
    FML_LOG(ERROR) << "[MPF] gainmap GetByteCount: " << byte_ret
                   << " bytes:" << byte_count;
  }
  {
    OH_NativeBuffer* native_buffer = nullptr;
    Image_ErrorCode dma_ret = OH_PixelmapNative_GetNativeBuffer(
        hdr_handle.pixelmap, &native_buffer);
    FML_LOG(ERROR) << "[MPF] hdr GetNativeBuffer: " << dma_ret
                   << " nativeBuffer=" << (native_buffer != nullptr);
    uint32_t byte_count = 0;
    Image_ErrorCode byte_ret =
        OH_PixelmapNative_GetByteCount(hdr_handle.pixelmap, &byte_count);
    FML_LOG(ERROR) << "[MPF] hdr GetByteCount: " << byte_ret
                   << " bytes:" << byte_count;
  }

  ImageProcessing_ColorSpaceInfo src_info = {
      HDR_METADATA_TYPE_BASE,
      GetColorSpaceName(base_pixelmap.pixelmap),
      base_pixelmap.info.pixel_format,
  };
  ImageProcessing_ColorSpaceInfo gain_info = {
      HDR_METADATA_TYPE_GAINMAP,
      GetColorSpaceName(gainmap_pixelmap.pixelmap),
      gainmap_pixelmap.info.pixel_format,
  };
  ImageProcessing_ColorSpaceInfo dst_info = {
      HDR_METADATA_TYPE_ALTERNATE,
      GetColorSpaceName(hdr_handle.pixelmap),
      PIXEL_FORMAT_RGBA_1010102,
  };
  bool compose_supported =
      OH_ImageProcessing_IsCompositionSupported(&src_info, &gain_info,
                                                &dst_info);
  FML_LOG(ERROR)
      << "[MPF] Compose support: " << compose_supported
      << " src(cs=" << ColorSpaceNameToString(src_info.colorSpace)
      << " fmt=" << src_info.pixelFormat
      << " meta=" << src_info.metadataType << ")"
      << " gain(cs=" << ColorSpaceNameToString(gain_info.colorSpace)
      << " fmt=" << gain_info.pixelFormat
      << " meta=" << gain_info.metadataType << ")"
      << " dst(cs=" << ColorSpaceNameToString(dst_info.colorSpace)
      << " fmt=" << dst_info.pixelFormat
      << " meta=" << dst_info.metadataType << ")";
  if (!compose_supported) {
    if (color_space_sdr != nullptr && color_space_hdr != nullptr) {
      OH_NativeColorSpaceManager_Destroy(color_space_sdr);
      OH_NativeColorSpaceManager_Destroy(color_space_hdr);
    }
    return false;
  }

  ImageProcessing_ErrorCode proc_ret =
      OH_ImageProcessing_InitializeEnvironment();
  if (proc_ret != IMAGE_PROCESSING_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] ImageProcessing init failed:" << proc_ret;
    if (color_space_sdr != nullptr && color_space_hdr != nullptr) {
      OH_NativeColorSpaceManager_Destroy(color_space_sdr);
      OH_NativeColorSpaceManager_Destroy(color_space_hdr);
    }
    return false;
  }

  OH_ImageProcessing* instance = nullptr;
  proc_ret =
      OH_ImageProcessing_Create(&instance, IMAGE_PROCESSING_TYPE_COMPOSITION);
  if (proc_ret != IMAGE_PROCESSING_SUCCESS || instance == nullptr) {
    FML_LOG(ERROR) << "[MPF] ImageProcessing create failed:" << proc_ret;
    OH_ImageProcessing_DeinitializeEnvironment();
    if (color_space_sdr != nullptr && color_space_hdr != nullptr) {
      OH_NativeColorSpaceManager_Destroy(color_space_sdr);
      OH_NativeColorSpaceManager_Destroy(color_space_hdr);
    }
    return false;
  }

  proc_ret = OH_ImageProcessing_Compose(
      instance, base_pixelmap.pixelmap, gainmap_pixelmap.pixelmap,
      hdr_handle.pixelmap);
  if (proc_ret != IMAGE_PROCESSING_SUCCESS) {
    FML_LOG(ERROR) << "[MPF] ImageProcessing compose failed:" << proc_ret;
  }

  OH_ImageProcessing_Destroy(instance);
  OH_ImageProcessing_DeinitializeEnvironment();

  if (color_space_sdr != nullptr && color_space_hdr != nullptr) {
    OH_NativeColorSpaceManager_Destroy(color_space_sdr);
    OH_NativeColorSpaceManager_Destroy(color_space_hdr);
  }

  if (proc_ret != IMAGE_PROCESSING_SUCCESS) {
    return false;
  }

  out->success = true;
  out->used_vpe = true;
  out->hdr_pixelmap = hdr_handle.Release();
  return true;
}

bool ParseMpfGainmapInfo(const uint8_t* data,
                         size_t size,
                         MpfGainmapInfo* out) {
  if (data == nullptr || out == nullptr) {
    return false;
  }
  if (!IsJpegStart(data, size, 0)) {
    return false;
  }

  size_t offset = 2;
  while (offset + 4 <= size) {
    if (data[offset] != 0xFF) {
      offset++;
      continue;
    }

    while (offset < size && data[offset] == 0xFF) {
      offset++;
    }
    if (offset >= size) {
      break;
    }

    const uint8_t marker = data[offset++];
    if (marker == 0xD9 || marker == 0xDA) {
      break;
    }
    if (marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7) ||
        marker == 0x01) {
      continue;
    }

    if (offset + 2 > size) {
      break;
    }
    const uint16_t segment_length =
        (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;
    if (segment_length < 2) {
      break;
    }

    const size_t payload_length = segment_length - 2;
    if (offset + payload_length > size) {
      break;
    }

    if (marker == 0xE2 && payload_length >= 4 &&
        memcmp(data + offset, "MPF\0", 4) == 0) {
      if (TryParseMpfPayload(data, size, offset, payload_length, out)) {
        FML_LOG(ERROR) << "[MPF] Parsed MPF APP2 at offset " << offset
                       << " length " << payload_length;
        return true;
      }
    }

    offset += payload_length;
  }

  // Some files place MPF APP2 after the EOI marker; scan for MPF signatures.
  constexpr uint8_t kMpfSignature[] = {'M', 'P', 'F', '\0'};
  for (size_t i = 0; i + sizeof(kMpfSignature) <= size; i++) {
    if (memcmp(data + i, kMpfSignature, sizeof(kMpfSignature)) != 0) {
      continue;
    }
    if (i < 4) {
      continue;
    }
    const size_t marker_offset = i - 4;
    if (data[marker_offset] != 0xFF || data[marker_offset + 1] != 0xE2) {
      continue;
    }
    const uint16_t segment_length =
        (static_cast<uint16_t>(data[marker_offset + 2]) << 8) |
        data[marker_offset + 3];
    if (segment_length < 2) {
      continue;
    }
    const size_t payload_length = segment_length - 2;
    if (i + payload_length > size) {
      continue;
    }
    if (TryParseMpfPayload(data, size, i, payload_length, out)) {
      FML_LOG(ERROR) << "[MPF] Parsed MPF APP2 after EOI at offset " << i
                     << " length " << payload_length;
      return true;
    }
  }

  return false;
}

}  // namespace flutter
