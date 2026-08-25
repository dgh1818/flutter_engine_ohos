/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#include "multimedia/image_framework/image/image_source_native.h"
#include "multimedia/image_framework/image/pixelmap_native.h"
#include "multimedia/image_framework/image_pixel_map_mdk.h"

namespace {

char g_dummy;

}  // namespace

extern "C" {

Image_ErrorCode OH_ImageSourceInfo_Create(OH_ImageSource_Info** info) {
  if (info == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *info = reinterpret_cast<OH_ImageSource_Info*>(&g_dummy);
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceInfo_GetWidth(OH_ImageSource_Info* /*info*/,
                                            uint32_t* width) {
  if (width == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *width = 0;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceInfo_GetHeight(OH_ImageSource_Info* /*info*/,
                                             uint32_t* height) {
  if (height == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *height = 0;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceInfo_GetDynamicRange(OH_ImageSource_Info* /*info*/,
                                                   bool* isHdr) {
  if (isHdr == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *isHdr = false;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceInfo_Release(OH_ImageSource_Info* /*info*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_DecodingOptions_Create(OH_DecodingOptions** options) {
  if (options == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *options = reinterpret_cast<OH_DecodingOptions*>(&g_dummy);
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_DecodingOptions_SetPixelFormat(OH_DecodingOptions* /*options*/,
                                                  int32_t /*pixelFormat*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_DecodingOptions_SetIndex(OH_DecodingOptions* /*options*/,
                                            uint32_t /*index*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_DecodingOptions_SetRotate(OH_DecodingOptions* /*options*/,
                                             float /*rotate*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_DecodingOptions_SetDesiredSize(OH_DecodingOptions* /*options*/,
                                                  Image_Size* /*desiredSize*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_DecodingOptions_SetDesiredDynamicRange(
    OH_DecodingOptions* /*options*/,
    int32_t /*desiredDynamicRange*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_DecodingOptions_Release(OH_DecodingOptions* /*options*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceNative_CreateFromData(uint8_t* /*data*/,
                                                    size_t /*dataSize*/,
                                                    OH_ImageSourceNative** res) {
  if (res == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *res = reinterpret_cast<OH_ImageSourceNative*>(&g_dummy);
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceNative_CreatePixelmap(OH_ImageSourceNative* /*source*/,
                                                    OH_DecodingOptions* /*options*/,
                                                    OH_PixelmapNative** pixelmap) {
  if (pixelmap == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *pixelmap = reinterpret_cast<OH_PixelmapNative*>(&g_dummy);
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceNative_GetDelayTimeList(OH_ImageSourceNative* /*source*/,
                                                      int32_t* /*delayTimeList*/,
                                                      size_t /*size*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceNative_GetImageInfo(OH_ImageSourceNative* /*source*/,
                                                  int32_t /*index*/,
                                                  OH_ImageSource_Info* /*info*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceNative_GetImageProperty(OH_ImageSourceNative* /*source*/,
                                                      Image_String* /*key*/,
                                                      Image_String* /*value*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceNative_GetFrameCount(OH_ImageSourceNative* /*source*/,
                                                   uint32_t* frameCount) {
  if (frameCount == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *frameCount = 0;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_ImageSourceNative_Release(OH_ImageSourceNative* /*source*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapImageInfo_Create(OH_Pixelmap_ImageInfo** info) {
  if (info == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *info = reinterpret_cast<OH_Pixelmap_ImageInfo*>(&g_dummy);
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapImageInfo_GetWidth(OH_Pixelmap_ImageInfo* /*info*/,
                                              uint32_t* width) {
  if (width == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *width = 0;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapImageInfo_GetHeight(OH_Pixelmap_ImageInfo* /*info*/,
                                               uint32_t* height) {
  if (height == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *height = 0;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapImageInfo_GetRowStride(OH_Pixelmap_ImageInfo* /*info*/,
                                                  uint32_t* rowStride) {
  if (rowStride == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *rowStride = 0;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapImageInfo_GetPixelFormat(OH_Pixelmap_ImageInfo* /*info*/,
                                                    int32_t* pixelFormat) {
  if (pixelFormat == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *pixelFormat = 0;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapImageInfo_Release(OH_Pixelmap_ImageInfo* /*info*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapNative_ConvertPixelmapNativeFromNapi(
    napi_env /*env*/,
    napi_value /*pixelmapNapi*/,
    OH_PixelmapNative** pixelmapNative) {
  if (pixelmapNative == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *pixelmapNative = reinterpret_cast<OH_PixelmapNative*>(&g_dummy);
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapNative_ReadPixels(OH_PixelmapNative* /*pixelmap*/,
                                             uint8_t* /*destination*/,
                                             size_t* /*bufferSize*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapNative_GetImageInfo(OH_PixelmapNative* /*pixelmap*/,
                                               OH_Pixelmap_ImageInfo* /*imageInfo*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapNative_Flip(OH_PixelmapNative* /*pixelmap*/,
                                       bool /*shouldFlipHorizontally*/,
                                       bool /*shouldFlipVertically*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapNative_Release(OH_PixelmapNative* /*pixelmap*/) {
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapNative_GetNativeBuffer(OH_PixelmapNative* /*pixelmap*/,
                                                  OH_NativeBuffer** nativeBuffer) {
  if (nativeBuffer == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *nativeBuffer = nullptr;
  return IMAGE_SUCCESS;
}

Image_ErrorCode OH_PixelmapNative_GetColorSpaceNative(
    OH_PixelmapNative* /*pixelmap*/,
    OH_NativeColorSpaceManager** colorSpaceNative) {
  if (colorSpaceNative == nullptr) {
    return IMAGE_BAD_PARAMETER;
  }
  *colorSpaceNative = nullptr;
  return IMAGE_SUCCESS;
}

NativePixelMap* OH_PixelMap_InitNativePixelMap(napi_env /*env*/, napi_value /*source*/) {
  return nullptr;
}

int32_t OH_PixelMap_GetImageInfo(const NativePixelMap* /*native*/,
                                 OhosPixelMapInfos* /*info*/) {
  return 0;
}

int32_t OH_PixelMap_AccessPixels(const NativePixelMap* /*native*/, void** addr) {
  if (addr != nullptr) {
    *addr = nullptr;
  }
  return 0;
}

int32_t OH_PixelMap_UnAccessPixels(const NativePixelMap* /*native*/) {
  return 0;
}

}  // extern "C"
