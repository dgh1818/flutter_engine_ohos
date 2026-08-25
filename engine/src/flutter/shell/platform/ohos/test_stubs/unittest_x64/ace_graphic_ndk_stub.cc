/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#include "native_buffer/native_buffer.h"
#include "native_image/native_image.h"
#include "native_vsync/native_vsync.h"
#include "native_window/external_window.h"

namespace {
char g_dummy_buffer;
char g_dummy_window;
char g_dummy_window_buffer;
char g_dummy_vsync;
char g_map_memory[4096];
BufferHandle g_dummy_handle = {};
OH_NativeBuffer_Config g_last_config = {};
}  // namespace

extern "C" {

OH_NativeBuffer* OH_NativeBuffer_Alloc(const OH_NativeBuffer_Config* config) {
  if (config != nullptr) {
    g_last_config = *config;
  }
  return reinterpret_cast<OH_NativeBuffer*>(&g_dummy_buffer);
}

int32_t OH_NativeBuffer_Unreference(OH_NativeBuffer* /*buffer*/) {
  return 0;
}

void OH_NativeBuffer_GetConfig(OH_NativeBuffer* /*buffer*/,
                               OH_NativeBuffer_Config* config) {
  if (config != nullptr) {
    *config = g_last_config;
  }
}

int32_t OH_NativeBuffer_Map(OH_NativeBuffer* /*buffer*/, void** virAddr) {
  if (virAddr != nullptr) {
    *virAddr = g_map_memory;
  }
  return 0;
}

int32_t OH_NativeBuffer_Unmap(OH_NativeBuffer* /*buffer*/) {
  return 0;
}

uint32_t OH_NativeBuffer_GetSeqNum(OH_NativeBuffer* /*buffer*/) {
  return 1;
}

int32_t OH_NativeBuffer_FromNativeWindowBuffer(
    OHNativeWindowBuffer* /*nativeWindowBuffer*/,
    OH_NativeBuffer** buffer) {
  if (buffer != nullptr) {
    *buffer = reinterpret_cast<OH_NativeBuffer*>(&g_dummy_buffer);
  }
  return 0;
}

OHNativeWindowBuffer* OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
    OH_NativeBuffer* /*nativeBuffer*/) {
  return reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
}

void OH_NativeWindow_DestroyNativeWindowBuffer(
    OHNativeWindowBuffer* /*buffer*/) {}

int32_t OH_NativeWindow_NativeWindowRequestBuffer(
    OHNativeWindow* /*window*/,
    OHNativeWindowBuffer** buffer,
    int* fenceFd) {
  if (buffer != nullptr) {
    *buffer = reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
  }
  if (fenceFd != nullptr) {
    *fenceFd = -1;
  }
  return 0;
}

int32_t OH_NativeWindow_NativeWindowFlushBuffer(OHNativeWindow* /*window*/,
                                                OHNativeWindowBuffer* /*buffer*/,
                                                int /*fenceFd*/,
                                                Region /*region*/) {
  return 0;
}

int32_t OH_NativeWindow_NativeWindowHandleOpt(OHNativeWindow* /*window*/,
                                              int /*code*/,
                                              ...) {
  return 0;
}

BufferHandle* OH_NativeWindow_GetBufferHandleFromNative(
    OHNativeWindowBuffer* /*buffer*/) {
  return &g_dummy_handle;
}

int32_t OH_NativeWindow_NativeObjectReference(void* /*obj*/) {
  return 0;
}

int32_t OH_NativeWindow_NativeObjectUnreference(void* /*obj*/) {
  return 0;
}

int32_t OH_NativeWindow_NativeWindowAttachBuffer(
    OHNativeWindow* /*window*/,
    OHNativeWindowBuffer* /*buffer*/) {
  return 0;
}

OH_NativeImage* OH_NativeImage_Create(uint32_t /*textureId*/,
                                      uint32_t /*textureTarget*/) {
  // Distinct pointers: SetExternalNativeImage no-ops when
  // native_image == native_image_source_ (ohos_external_texture.cpp).
  return reinterpret_cast<OH_NativeImage*>(new char);
}

OHNativeWindow* OH_NativeImage_AcquireNativeWindow(OH_NativeImage* /*image*/) {
  return reinterpret_cast<OHNativeWindow*>(&g_dummy_window);
}

int32_t OH_NativeImage_GetSurfaceId(OH_NativeImage* /*image*/,
                                    uint64_t* surfaceId) {
  if (surfaceId != nullptr) {
    *surfaceId = 1;
  }
  return 0;
}

int32_t OH_NativeImage_SetOnFrameAvailableListener(
    OH_NativeImage* /*image*/,
    OH_OnFrameAvailableListener /*listener*/) {
  return 0;
}

int32_t OH_NativeImage_UnsetOnFrameAvailableListener(
    OH_NativeImage* /*image*/) {
  return 0;
}

void OH_NativeImage_Destroy(OH_NativeImage** image) {
  if (image != nullptr && *image != nullptr) {
    delete reinterpret_cast<char*>(*image);
    *image = nullptr;
  }
}

int32_t OH_NativeImage_GetTransformMatrixV2(OH_NativeImage* /*image*/,
                                            float matrix[16]) {
  if (matrix != nullptr) {
    for (int i = 0; i < 16; ++i) {
      matrix[i] = 0.0f;
    }
  }
  return 0;
}

int32_t OH_NativeImage_AcquireNativeWindowBuffer(
    OH_NativeImage* /*image*/,
    OHNativeWindowBuffer** nativeWindowBuffer,
    int* fenceFd) {
  // Empty queue. Returning success + a dummy buffer would spin forever in
  // SetExternalNativeImage's drain loop (ohos_external_texture.cpp).
  if (nativeWindowBuffer != nullptr) {
    *nativeWindowBuffer = nullptr;
  }
  if (fenceFd != nullptr) {
    *fenceFd = -1;
  }
  return 1;
}

int32_t OH_NativeImage_ReleaseNativeWindowBuffer(
    OH_NativeImage* /*image*/,
    OHNativeWindowBuffer* /*nativeWindowBuffer*/,
    int /*fenceFd*/) {
  return 0;
}

OH_NativeVSync* OH_NativeVSync_Create(const char* /*name*/,
                                      unsigned int /*length*/) {
  return reinterpret_cast<OH_NativeVSync*>(&g_dummy_vsync);
}

void OH_NativeVSync_Destroy(OH_NativeVSync* /*nativeVsync*/) {}

int OH_NativeVSync_RequestFrameWithMultiCallback(
    OH_NativeVSync* /*nativeVsync*/,
    OH_NativeVSync_FrameCallback /*callback*/,
    void* /*data*/) {
  return 0;
}

int OH_NativeVSync_GetPeriod(OH_NativeVSync* /*nativeVsync*/,
                             long long* period) {
  if (period != nullptr) {
    *period = 16666667;
  }
  return 0;
}

}  // extern "C"
