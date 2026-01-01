/*
 * Copyright 2025 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_MPF_DECODER_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_MPF_DECODER_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include <multimedia/image_framework/image/image_common.h>

struct OH_PixelmapNative;
struct OH_ImageSourceNative;

namespace flutter {

constexpr float kDefaultGainmapHeadroom = 4.0f;

struct MpfGainmapInfo {
  size_t offset = 0;
  size_t size = 0;
};

struct MpfGainmapInitResult {
  OH_ImageSourceNative* gainmap_image_source = nullptr;
  std::vector<uint8_t> gainmap_data;
  float headroom = kDefaultGainmapHeadroom;
};

struct MpfGainmapComposeInput {
  OH_ImageSourceNative* base_source = nullptr;
  OH_ImageSourceNative* gainmap_source = nullptr;
  int width = 0;
  int height = 0;
  float rotate_degree = 0.0f;
  bool need_flip = false;
  float headroom = kDefaultGainmapHeadroom;
  void* pixels = nullptr;
  size_t row_bytes = 0;
};

struct MpfGainmapComposeResult {
  bool success = false;
  bool used_vpe = false;
  OH_PixelmapNative* hdr_pixelmap = nullptr;
};

bool ParseMpfGainmapInfo(const uint8_t* data,
                         size_t size,
                         MpfGainmapInfo* out);

bool ParseGainmapHeadroomFromXmp(const uint8_t* data,
                                 size_t size,
                                 float* out_headroom);

Image_ErrorCode CreateImageSourceFromDataMaybeWithUserBuffer(
    uint8_t* data,
    size_t datalength,
    OH_ImageSourceNative** image_source,
    bool* used_user_buffer);

bool InitMpfGainmapFromData(const uint8_t* data,
                            size_t size,
                            size_t gainmap_offset,
                            size_t gainmap_size,
                            MpfGainmapInitResult* out);

OH_PixelmapNative* CopyPixelmapToDmaRGBA8888(OH_PixelmapNative* src);

OH_PixelmapNative* ExpandSingleChannelToDmaRGBA8888(OH_PixelmapNative* src);

bool ComposeGainmapToRGBA1010102(OH_PixelmapNative* sdr,
                                 OH_PixelmapNative* gainmap,
                                 uint8_t* dst,
                                 size_t row_bytes,
                                 float headroom);

bool ComposeMpfGainmap(const MpfGainmapComposeInput& input,
                       MpfGainmapComposeResult* out);

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_MPF_DECODER_H_
