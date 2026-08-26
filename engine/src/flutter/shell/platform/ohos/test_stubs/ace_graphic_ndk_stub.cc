/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdarg.h>
#include "multimedia/image_framework/image/pixelmap_native.h"
#include "multimedia/image_framework/image_pixel_map_mdk.h"
#include "native_buffer/native_buffer.h"
#include "native_image/native_image.h"
#include "native_vsync/native_vsync.h"
#include "native_window/external_window.h"

namespace {
constexpr int kStubFdSize = 4 << 20;
constexpr char kStubFdPath[] = "/data/local/tmp/.stub_graphic_buffer_fd";
int StubSharedFd() {
  static const int kFd = [] {
    int fd = -1;
#if defined(SYS_memfd_create)
    fd = static_cast<int>(::syscall(SYS_memfd_create, "stub_buffer", 0));
#endif
    if (fd >= 0 && (::ftruncate(fd, kStubFdSize) != 0)) {
      fprintf(stderr, "stub fd: memfd ftruncate errno=%d\n", errno);
      ::close(fd);
      fd = -1;
    }
    if (fd < 0) {
      fd = ::open(kStubFdPath, O_CREAT | O_RDWR | O_TRUNC, 0600);
      if (fd >= 0 && (::ftruncate(fd, kStubFdSize) != 0)) {
        fprintf(stderr, "stub fd: file ftruncate errno=%d\n", errno);
        ::close(fd);
        fd = -1;
      } else if (fd < 0) {
        fprintf(stderr, "stub fd: open errno=%d\n", errno);
      }
    }
    if (fd < 0) {
      FILE* file = ::tmpfile();
      fd = file ? ::fileno(file) : -1;
      if (fd >= 0 && (::ftruncate(fd, kStubFdSize) != 0)) {
        fprintf(stderr, "stub fd: tmpfile ftruncate errno=%d\n", errno);
        ::close(fd);
        fd = -1;
      }
    }
    if (fd < 0) {
      fprintf(stderr, "ace_graphic_ndk_stub: all fd sources failed\n");
    }
    return fd;
  }();
  return kFd;
}

}

GraphicStubState g_graphic_stub;

extern "C" {

void UpdateFromNativeWindowBufferFail(int fail) {
  g_from_native_window_buffer_fail = fail;
}

}
#if !defined(OHOS_X64_UNITTEST)
namespace {
char g_dummy_buffer;
char g_dummy_window;
char g_dummy_window_buffer;
}

extern "C" {
int32_t __real_OH_NativeWindow_NativeObjectReference(void*);
int32_t __real_OH_NativeWindow_NativeObjectUnreference(void*);
int32_t __real_OH_NativeWindow_NativeWindowHandleOpt(OHNativeWindow*,
                                                     int,
                                                     ...);
void __real_OH_NativeWindow_DestroyNativeWindowBuffer(OHNativeWindowBuffer*);
int32_t __real_OH_NativeWindow_NativeWindowAttachBuffer(OHNativeWindow*,
                                                        OHNativeWindowBuffer*);
int32_t __real_OH_NativeWindow_NativeWindowFlushBuffer(OHNativeWindow*,
                                                       OHNativeWindowBuffer*,
                                                       int,
                                                       Region);
int32_t __real_OH_NativeWindow_NativeWindowRequestBuffer(OHNativeWindow*,
                                                         OHNativeWindowBuffer**,
                                                         int*);
BufferHandle* __real_OH_NativeWindow_GetBufferHandleFromNative(
    OHNativeWindowBuffer*);
int32_t __real_OH_NativeBuffer_FromNativeWindowBuffer(
    OHNativeWindowBuffer*,
    OH_NativeBuffer**);
OH_NativeImage* __real_OH_NativeImage_Create(uint32_t, uint32_t);
OHNativeWindow* __real_OH_NativeImage_AcquireNativeWindow(OH_NativeImage*);
int32_t __real_OH_NativeImage_SetOnFrameAvailableListener(
    OH_NativeImage*, OH_OnFrameAvailableListener);
int32_t __real_OH_NativeImage_AcquireNativeWindowBuffer(OH_NativeImage*,
                                                        OHNativeWindowBuffer**,
                                                        int*);
int32_t __real_OH_NativeImage_ReleaseNativeWindowBuffer(OH_NativeImage*,
                                                        OHNativeWindowBuffer*,
                                                        int);
void __real_OH_NativeImage_Destroy(OH_NativeImage**);
int32_t __real_OH_PixelMap_GetImageInfo(const NativePixelMap*,
                                        OhosPixelMapInfos*);
OHNativeWindowBuffer* __real_OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
    OH_NativeBuffer*);
int32_t __real_OH_NativeBuffer_Unreference(OH_NativeBuffer*);
int32_t __real_OH_NativeImage_GetSurfaceId(OH_NativeImage*, uint64_t*);
int32_t __real_OH_NativeImage_UnsetOnFrameAvailableListener(OH_NativeImage*);
}

extern "C" {

int32_t __wrap_OH_NativeWindow_NativeObjectReference(void* obj) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeObjectReference(obj);
  }
  return 0;
}

int32_t __wrap_OH_NativeWindow_NativeObjectUnreference(void* obj) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeObjectUnreference(obj);
  }
  return 0;
}

void __wrap_OH_NativeWindow_DestroyNativeWindowBuffer(
    OHNativeWindowBuffer* buffer) {
  if (!g_stub_graphic_engaged) {
    __real_OH_NativeWindow_DestroyNativeWindowBuffer(buffer);
    return;
  }
}

int32_t __wrap_OH_NativeWindow_NativeWindowAttachBuffer(
    OHNativeWindow* window, OHNativeWindowBuffer* buffer) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeWindowAttachBuffer(window, buffer);
  }
  return 0;
}

int32_t __wrap_OH_NativeWindow_NativeWindowFlushBuffer(
    OHNativeWindow* window,
    OHNativeWindowBuffer* buffer,
    int fenceFd,
    Region region) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeWindowFlushBuffer(
        window, buffer, fenceFd, region);
  }
  if (g_stub_graphic_fail_mask & kStubFailFlushBuffer) {
    return 1;
  }
  return 0;
}

int32_t __wrap_OH_NativeWindow_NativeWindowRequestBuffer(
    OHNativeWindow* window,
    OHNativeWindowBuffer** buffer,
    int* fenceFd) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeWindowRequestBuffer(window, buffer,
                                                            fenceFd);
  }
  if (g_stub_graphic_fail_mask & kStubFailRequestBuffer) {
    return 1;
  }
  if (buffer != nullptr) {
    *buffer =
        reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
  }
  if (fenceFd != nullptr) {
    *fenceFd = -1;
  }
  return 0;
}

BufferHandle* __wrap_OH_NativeWindow_GetBufferHandleFromNative(
    OHNativeWindowBuffer* buffer) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_GetBufferHandleFromNative(buffer);
  }
  if (g_stub_graphic_fail_mask & kStubFailGetBufferHandle) {
    return nullptr;
  }
  static BufferHandle handle = {};
  handle.fd = (g_stub_graphic_fail_mask & kStubBufferHandleBadFd)
                  ? -1
                  : StubSharedFd();
  handle.width = 16;
  handle.height = 16;
  handle.stride = 64;
  handle.size = 16 * 64;
  handle.format = g_stub_buffer_format;
  return &handle;
}

int32_t __wrap_OH_NativeBuffer_FromNativeWindowBuffer(
    OHNativeWindowBuffer* nativeWindowBuffer,
    OH_NativeBuffer** buffer) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeBuffer_FromNativeWindowBuffer(nativeWindowBuffer,
                                                         buffer);
  }
  if (g_from_native_window_buffer_fail) {
    return -1;
  }
  if (buffer != nullptr) {
    *buffer = reinterpret_cast<OH_NativeBuffer*>(&g_dummy_buffer);
  }
  return 0;
}

OH_NativeImage* __wrap_OH_NativeImage_Create(uint32_t textureId,
                                             uint32_t textureTarget) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeImage_Create(textureId, textureTarget);
  }
  if (g_stub_graphic_fail_mask & kStubFailNativeImageCreate) {
    return nullptr;
  }
  return reinterpret_cast<OH_NativeImage*>(new char);
}

OHNativeWindow* __wrap_OH_NativeImage_AcquireNativeWindow(
    OH_NativeImage* image) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeImage_AcquireNativeWindow(image);
  }
  if (g_stub_graphic_fail_mask & kStubFailAcquireNativeWindow) {
    return nullptr;
  }
  return reinterpret_cast<OHNativeWindow*>(&g_dummy_window);
}

int32_t __wrap_OH_NativeImage_SetOnFrameAvailableListener(
    OH_NativeImage* image, OH_OnFrameAvailableListener listener) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeImage_SetOnFrameAvailableListener(image, listener);
  }
  if (g_stub_graphic_fail_mask & kStubFailFrameAvailableListener) {
    return 1;
  }
  return 0;
}

int32_t __wrap_OH_NativeImage_AcquireNativeWindowBuffer(
    OH_NativeImage* image,
    OHNativeWindowBuffer** nativeWindowBuffer,
    int* fenceFd) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeImage_AcquireNativeWindowBuffer(
        image, nativeWindowBuffer, fenceFd);
  }
  if (g_stub_graphic_fail_mask & kStubAcquireBufferSuccess) {
    if (nativeWindowBuffer != nullptr) {
      *nativeWindowBuffer =
          reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
    }
    if (fenceFd != nullptr) {
      *fenceFd = -1;
    }
    return 0;
  }
  if (nativeWindowBuffer != nullptr) {
    *nativeWindowBuffer = nullptr;
  }
  if (fenceFd != nullptr) {
    *fenceFd = -1;
  }
  return 1;
}

int32_t __wrap_OH_NativeImage_ReleaseNativeWindowBuffer(
    OH_NativeImage* image,
    OHNativeWindowBuffer* nativeWindowBuffer,
    int fenceFd) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeImage_ReleaseNativeWindowBuffer(
        image, nativeWindowBuffer, fenceFd);
  }
  if (g_stub_graphic_fail_mask & kStubFailReleaseWindowBuffer) {
    return 1;
  }
  return 0;
}

void __wrap_OH_NativeImage_Destroy(OH_NativeImage** image) {
  if (!g_stub_graphic_engaged) {
    __real_OH_NativeImage_Destroy(image);
    return;
  }
  if (image != nullptr && *image != nullptr) {
    delete reinterpret_cast<char*>(*image);
    *image = nullptr;
  }
}

int32_t __wrap_OH_NativeImage_GetSurfaceId(OH_NativeImage* image,
                                           uint64_t* surfaceId) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeImage_GetSurfaceId(image, surfaceId);
  }
  if (surfaceId != nullptr) {
    *surfaceId = 1;
  }
  return 0;
}

int32_t __wrap_OH_NativeImage_UnsetOnFrameAvailableListener(
    OH_NativeImage* image) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeImage_UnsetOnFrameAvailableListener(image);
  }
  return 0;
}

int32_t __wrap_OH_PixelMap_GetImageInfo(const NativePixelMap* native,
                                        OhosPixelMapInfos* info) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_PixelMap_GetImageInfo(native, info);
  }
  return 0;
}

OHNativeWindowBuffer*
__wrap_OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
    OH_NativeBuffer* buffer) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
        buffer);
  }
  return reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
}

int32_t __wrap_OH_NativeBuffer_Unreference(OH_NativeBuffer* buffer) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeBuffer_Unreference(buffer);
  }
  return 0;
}

int32_t __wrap_OH_NativeWindow_NativeWindowHandleOpt(OHNativeWindow* window,
                                                     int code,
                                                     ...) {
  va_list args;
  va_start(args, code);
  int32_t ret;
  if (!g_stub_graphic_engaged) {
    switch (code) {
      case SET_BUFFER_GEOMETRY: {
        int32_t width = va_arg(args, int32_t);
        int32_t height = va_arg(args, int32_t);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code,
                                                           width, height);
        break;
      }
      case GET_BUFFER_GEOMETRY: {
        int32_t* height = va_arg(args, int32_t*);
        int32_t* width = va_arg(args, int32_t*);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code,
                                                           height, width);
        break;
      }
      case SET_FORMAT:
      case SET_STRIDE: {
        int32_t value = va_arg(args, int32_t);
        ret =
            __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, value);
        break;
      }
      case GET_FORMAT:
      case GET_BUFFERQUEUE_SIZE:
      case GET_SOURCE_TYPE: {
        int32_t* value = va_arg(args, int32_t*);
        ret =
            __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, value);
        break;
      }
      case SET_USAGE: {
        uint64_t usage = va_arg(args, uint64_t);
        ret =
            __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, usage);
        break;
      }
      case GET_USAGE: {
        uint64_t* usage = va_arg(args, uint64_t*);
        ret =
            __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, usage);
        break;
      }
      case SET_DESIRED_PRESENT_TIMESTAMP: {
        int64_t ts = va_arg(args, int64_t);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, ts);
        break;
      }
      case SET_APP_FRAMEWORK_TYPE: {
        char* framework_type = va_arg(args, char*);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code,
                                                           framework_type);
        break;
      }
      case GET_APP_FRAMEWORK_TYPE: {
        char** framework_type = va_arg(args, char**);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code,
                                                           framework_type);
        break;
      }
      default:
        ret = 1;
        break;
    }
  } else {
    if (g_stub_graphic_fail_mask & kStubFailWindowHandleOpt) {
      ret = 1;
    } else {
      if (code == GET_BUFFER_GEOMETRY) {
        int32_t* height = va_arg(args, int32_t*);
        int32_t* width = va_arg(args, int32_t*);
        if (height != nullptr) {
          *height = g_stub_geometry_height;
        }
        if (width != nullptr) {
          *width = g_stub_geometry_width;
        }
      }
      ret = 0;
    }
  }
  va_end(args);
  return ret;
}

}
#else  // defined(OHOS_X64_UNITTEST)

namespace {
char g_dummy_buffer;
char g_dummy_window;
char g_dummy_window_buffer;
char g_dummy_vsync;
char g_map_memory[4096];
BufferHandle g_dummy_handle = {};
OH_NativeBuffer_Config g_last_config = {};

}

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
  if (g_from_native_window_buffer_fail) {
    return -1;
  }
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
  if (g_stub_graphic_fail_mask & kStubFailRequestBuffer) {
    return 1;
  }
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
  if (g_stub_graphic_fail_mask & kStubFailFlushBuffer) {
    return 1;
  }
  return 0;
}

int32_t OH_NativeWindow_NativeWindowHandleOpt(OHNativeWindow* /*window*/,
                                              int code,
                                              ...) {
  if (g_stub_graphic_fail_mask & kStubFailWindowHandleOpt) {
    return 1;
  }
  if (code == GET_BUFFER_GEOMETRY) {
    va_list args;
    va_start(args, code);
    int32_t* height = va_arg(args, int32_t*);
    int32_t* width = va_arg(args, int32_t*);
    va_end(args);
    if (height != nullptr) {
      *height = g_stub_geometry_height;
    }
    if (width != nullptr) {
      *width = g_stub_geometry_width;
    }
  }
  return 0;
}

BufferHandle* OH_NativeWindow_GetBufferHandleFromNative(
    OHNativeWindowBuffer* /*buffer*/) {
  if (g_stub_graphic_fail_mask & kStubFailGetBufferHandle) {
    return nullptr;
  }
  g_dummy_handle.fd = (g_stub_graphic_fail_mask & kStubBufferHandleBadFd)
                          ? -1
                          : StubSharedFd();
  g_dummy_handle.width = 16;
  g_dummy_handle.height = 16;
  g_dummy_handle.stride = 64;
  g_dummy_handle.size = 16 * 64;
  g_dummy_handle.format = g_stub_buffer_format;
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
  if (g_stub_graphic_fail_mask & kStubFailNativeImageCreate) {
    return nullptr;
  }
  return reinterpret_cast<OH_NativeImage*>(new char);
}

OHNativeWindow* OH_NativeImage_AcquireNativeWindow(OH_NativeImage* /*image*/) {
  if (g_stub_graphic_fail_mask & kStubFailAcquireNativeWindow) {
    return nullptr;
  }
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
  if (g_stub_graphic_fail_mask & kStubFailFrameAvailableListener) {
    return 1;
  }
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
  if (g_stub_graphic_fail_mask & kStubAcquireBufferSuccess) {
    if (nativeWindowBuffer != nullptr) {
      *nativeWindowBuffer =
          reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
    }
    if (fenceFd != nullptr) {
      *fenceFd = -1;
    }
    return 0;
  }
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
  if (g_stub_graphic_fail_mask & kStubFailReleaseWindowBuffer) {
    return 1;
  }
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

}
#endif  // defined(OHOS_X64_UNITTEST)

