/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/native_interface_accessibility.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdio>
#include <map>
#include <string>
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"
#include "multimedia/image_framework/image/pixelmap_native.h"
#include "multimedia/image_framework/image_pixel_map_mdk.h"
#include "native_buffer/native_buffer.h"
#include "native_image/native_image.h"
#include "native_vsync/native_vsync.h"
#include "native_window/external_window.h"

namespace {
constexpr int kStubFdSize = 4 << 20;

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
      char stub_fd_path[4096];
      snprintf(stub_fd_path, sizeof(stub_fd_path), "%s/.stub_graphic_buffer_fd",
               GetUtTmpDir());
      fd = ::open(stub_fd_path, O_CREAT | O_RDWR | O_TRUNC, 0600);
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

}  // namespace

namespace {
std::map<std::string, std::string>& ArkuiActionArgumentTable() {
  static std::map<std::string, std::string> table;
  return table;
}
}  // namespace

GraphicStubState g_graphic_stub;

extern "C" {

void UpdateFromNativeWindowBufferFail(int fail) {
  g_from_native_window_buffer_fail = fail;
}

void StubArkuiSetActionArgument(const char* key, const char* value) {
  auto& args = ArkuiActionArgumentTable();
  if (key == nullptr || value == nullptr) {
    return;
  }
  args[key] = value;
}

void StubArkuiResetActionArguments(void) {
  ArkuiActionArgumentTable().clear();
}
}
namespace {
char g_dummy_buffer;
char g_dummy_window;
char g_dummy_window_buffer;
}  // namespace

extern "C" {
int32_t __real_OH_NativeWindow_NativeObjectReference(void*);
int32_t __real_OH_NativeWindow_NativeObjectUnreference(void*);
int32_t __real_OH_NativeWindow_NativeWindowHandleOpt(OHNativeWindow*, int, ...);
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
int32_t __real_OH_NativeBuffer_FromNativeWindowBuffer(OHNativeWindowBuffer*,
                                                      OH_NativeBuffer**);
uint32_t __real_OH_NativeBuffer_GetSeqNum(OH_NativeBuffer*);
OH_NativeImage* __real_OH_NativeImage_Create(uint32_t, uint32_t);
OHNativeWindow* __real_OH_NativeImage_AcquireNativeWindow(OH_NativeImage*);
int32_t __real_OH_NativeImage_SetOnFrameAvailableListener(
    OH_NativeImage*,
    OH_OnFrameAvailableListener);
int32_t __real_OH_NativeImage_AcquireNativeWindowBuffer(OH_NativeImage*,
                                                        OHNativeWindowBuffer**,
                                                        int*);
int32_t __real_OH_NativeImage_ReleaseNativeWindowBuffer(OH_NativeImage*,
                                                        OHNativeWindowBuffer*,
                                                        int);
void __real_OH_NativeImage_Destroy(OH_NativeImage**);
int32_t __real_OH_PixelMap_GetImageInfo(const NativePixelMap*,
                                        OhosPixelMapInfos*);
OHNativeWindowBuffer*
__real_OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
    OH_NativeBuffer*);
int32_t __real_OH_NativeBuffer_Unreference(OH_NativeBuffer*);
int32_t __real_OH_NativeImage_GetSurfaceId(OH_NativeImage*, uint64_t*);
int32_t __real_OH_NativeImage_UnsetOnFrameAvailableListener(OH_NativeImage*);
}

extern "C" {

OH_NativeVSync* __wrap_OH_NativeVSync_Create(const char* /*name*/,
                                             unsigned int /*length*/) {
  return reinterpret_cast<OH_NativeVSync*>(0x1234);
}

void __wrap_OH_NativeVSync_Destroy(OH_NativeVSync* /*nativeVsync*/) {}

int32_t __wrap_OH_NativeVSync_GetPeriod(OH_NativeVSync* /*nativeVsync*/,
                                        long long* period) {
  if (period != nullptr) {
    *period = 16666667;
  }
  return 0;
}

int32_t __wrap_OH_NativeVSync_RequestFrameWithMultiCallback(
    OH_NativeVSync* /*nativeVsync*/,
    OH_NativeVSync_FrameCallback /*callback*/,
    void* /*data*/) {
  if (g_graphic_stub.vsync_request_fail) {
    return 1;
  }
  return 0;
}

int32_t __real_OH_NativeXComponent_GetXComponentSize(OH_NativeXComponent*,
                                                     const void*,
                                                     uint64_t*,
                                                     uint64_t*);
int32_t __real_OH_NativeXComponent_SetNeedSoftKeyboard(OH_NativeXComponent*,
                                                       bool);
int32_t __real_OH_NativeXComponent_GetTouchEvent(
    OH_NativeXComponent*,
    const void*,
    OH_NativeXComponent_TouchEvent*);
int32_t __real_OH_NativeXComponent_GetMouseEvent(
    OH_NativeXComponent*,
    const void*,
    OH_NativeXComponent_MouseEvent*);
int32_t __real_OH_NativeXComponent_GetTouchEventSourceType(
    OH_NativeXComponent*,
    int32_t,
    OH_NativeXComponent_EventSourceType*);

int32_t __wrap_OH_NativeXComponent_GetXComponentSize(
    OH_NativeXComponent* component,
    const void* window,
    uint64_t* width,
    uint64_t* height) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_GetXComponentSize(component, window,
                                                        width, height);
  }
  if (g_stub_graphic_fail_mask & kStubFailGetXComponentSize) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  if (width != nullptr) {
    *width = static_cast<uint64_t>(g_graphic_stub.geometry_width);
  }
  if (height != nullptr) {
    *height = static_cast<uint64_t>(g_graphic_stub.geometry_height);
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_SetNeedSoftKeyboard(
    OH_NativeXComponent* component,
    bool needSoftKeyboard) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_SetNeedSoftKeyboard(component,
                                                          needSoftKeyboard);
  }
  if (g_stub_graphic_fail_mask & kStubFailSetNeedSoftKeyboard) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_GetTouchEvent(
    OH_NativeXComponent* component,
    const void* window,
    OH_NativeXComponent_TouchEvent* touchEvent) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_GetTouchEvent(component, window,
                                                    touchEvent);
  }
  if (g_stub_graphic_fail_mask & kStubFailGetTouchEvent) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  if (touchEvent != nullptr) {
    memset(touchEvent, 0, sizeof(*touchEvent));
    touchEvent->type = static_cast<OH_NativeXComponent_TouchEventType>(
        g_graphic_stub.touch_type);
    touchEvent->id = g_graphic_stub.touch_id;
    touchEvent->x = static_cast<float>(g_graphic_stub.geometry_width);
    touchEvent->y = static_cast<float>(g_graphic_stub.geometry_height);
    touchEvent->numPoints = g_graphic_stub.touch_num_points;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_GetMouseEvent(
    OH_NativeXComponent* component,
    const void* window,
    OH_NativeXComponent_MouseEvent* mouseEvent) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_GetMouseEvent(component, window,
                                                    mouseEvent);
  }
  if (g_stub_graphic_fail_mask & kStubFailGetMouseEvent) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  if (mouseEvent != nullptr) {
    memset(mouseEvent, 0, sizeof(*mouseEvent));
    mouseEvent->button = static_cast<OH_NativeXComponent_MouseEventButton>(
        g_graphic_stub.mouse_button);
    mouseEvent->action = static_cast<OH_NativeXComponent_MouseEventAction>(
        g_graphic_stub.mouse_action);
    mouseEvent->x = static_cast<float>(g_graphic_stub.geometry_width);
    mouseEvent->y = static_cast<float>(g_graphic_stub.geometry_height);
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __real_OH_NativeXComponent_GetTouchPointToolType(
    OH_NativeXComponent*,
    int32_t,
    OH_NativeXComponent_TouchPointToolType*);
int32_t __real_OH_NativeXComponent_GetTouchPointTiltX(OH_NativeXComponent*,
                                                      int32_t,
                                                      float*);
int32_t __real_OH_NativeXComponent_GetTouchPointTiltY(OH_NativeXComponent*,
                                                      int32_t,
                                                      float*);

int32_t __wrap_OH_NativeXComponent_GetTouchPointToolType(
    OH_NativeXComponent* component,
    int32_t pointId,
    OH_NativeXComponent_TouchPointToolType* toolType) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_GetTouchPointToolType(component, pointId,
                                                            toolType);
  }
  if (g_stub_graphic_fail_mask & kStubFailGetTouchPointToolType) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  if (toolType != nullptr) {
    *toolType = OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_GetTouchPointTiltX(
    OH_NativeXComponent* component,
    int32_t pointId,
    float* tiltX) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_GetTouchPointTiltX(component, pointId,
                                                         tiltX);
  }
  if (g_stub_graphic_fail_mask & kStubFailTouchTilt) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  if (tiltX != nullptr) {
    *tiltX = 0.0f;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_GetTouchPointTiltY(
    OH_NativeXComponent* component,
    int32_t pointId,
    float* tiltY) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_GetTouchPointTiltY(component, pointId,
                                                         tiltY);
  }
  if (g_stub_graphic_fail_mask & kStubFailTouchTilt) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  if (tiltY != nullptr) {
    *tiltY = 0.0f;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_GetTouchEventSourceType(
    OH_NativeXComponent* component,
    int32_t pointId,
    OH_NativeXComponent_EventSourceType* sourceType) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeXComponent_GetTouchEventSourceType(
        component, pointId, sourceType);
  }
  if (g_stub_graphic_fail_mask & kStubFailGetTouchEventSourceType) {
    return OH_NATIVEXCOMPONENT_RESULT_BAD_PARAMETER;
  }
  if (sourceType != nullptr) {
    *sourceType = g_stub_source_type != 0
                      ? static_cast<OH_NativeXComponent_EventSourceType>(
                            g_stub_source_type)
                      : OH_NATIVEXCOMPONENT_SOURCE_TYPE_TOUCHSCREEN;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_ArkUI_FindAccessibilityActionArgumentByKey(
    void* /*arguments*/,
    const char* key,
    char** value) {
  const auto& table = ArkuiActionArgumentTable();
  const auto it = key != nullptr ? table.find(key) : table.end();
  if (value != nullptr) {
    *value =
        it != table.end() ? const_cast<char*>(it->second.c_str()) : nullptr;
  }
  return 0;
}

ArkUI_AccessibilityElementInfo*
__wrap_OH_ArkUI_AddAndGetAccessibilityElementInfo(
    ArkUI_AccessibilityElementInfoList* /*list*/) {
  static ArkUI_AccessibilityElementInfo* dummy =
      OH_ArkUI_CreateAccessibilityElementInfo();
  return dummy;
}

int32_t __wrap_OH_NativeXComponent_RegisterCallback(
    OH_NativeXComponent* /*component*/,
    OH_NativeXComponent_Callback* /*callback*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_RegisterMouseEventCallback(
    OH_NativeXComponent* /*component*/,
    OH_NativeXComponent_MouseEvent_Callback* /*callback*/) {
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}

int32_t __wrap_OH_NativeXComponent_RegisterUIInputEventCallback(
    OH_NativeXComponent* /*component*/,
    void (*callback)(OH_NativeXComponent*,
                     ArkUI_UIInputEvent*,
                     ArkUI_UIInputEvent_Type),
    ArkUI_UIInputEvent_Type /*type*/) {
  (void)callback;
  return 0;
}

int32_t __wrap_OH_NativeXComponent_GetNativeAccessibilityProvider(
    OH_NativeXComponent* /*component*/,
    ArkUI_AccessibilityProvider** handle) {
  if (handle != nullptr) {
    *handle = nullptr;
  }
  return OH_NATIVEXCOMPONENT_RESULT_FAILED;
}

int32_t __wrap_OH_NativeWindow_NativeObjectReference(void* obj) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeObjectReference(obj);
  }
  if (g_stub_graphic_fail_mask & kStubFailNativeObjectReference) {
    return 1;
  }
  return 0;
}

int32_t __wrap_OH_NativeWindow_NativeObjectUnreference(void* obj) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeObjectUnreference(obj);
  }
  if (g_stub_graphic_fail_mask & kStubFailNativeObjectUnreference) {
    return 1;
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
    OHNativeWindow* window,
    OHNativeWindowBuffer* buffer) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeWindowAttachBuffer(window, buffer);
  }
  if (g_stub_graphic_fail_mask & kStubFailAttachBuffer) {
    return 1;
  }
  return 0;
}

int32_t __wrap_OH_NativeWindow_NativeWindowFlushBuffer(
    OHNativeWindow* window,
    OHNativeWindowBuffer* buffer,
    int fenceFd,
    Region region) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeWindow_NativeWindowFlushBuffer(window, buffer,
                                                          fenceFd, region);
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
    *buffer = reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
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
  handle.fd =
      (g_stub_graphic_fail_mask & kStubBufferHandleBadFd) ? -1 : StubSharedFd();
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

uint32_t __wrap_OH_NativeBuffer_GetSeqNum(OH_NativeBuffer* buffer) {
  if (!g_stub_graphic_engaged) {
    return __real_OH_NativeBuffer_GetSeqNum(buffer);
  }
  return 41;
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
    OH_NativeImage* image,
    OH_OnFrameAvailableListener listener) {
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
  if (g_stub_graphic_fail_mask & kStubAcquireBufferFailWithBuffer) {
    if (nativeWindowBuffer != nullptr) {
      *nativeWindowBuffer =
          reinterpret_cast<OHNativeWindowBuffer*>(&g_dummy_window_buffer);
    }
    if (fenceFd != nullptr) {
      *fenceFd = -1;
    }
    return 1;
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
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, width,
                                                           height);
        break;
      }
      case GET_BUFFER_GEOMETRY: {
        int32_t* height = va_arg(args, int32_t*);
        int32_t* width = va_arg(args, int32_t*);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, height,
                                                           width);
        break;
      }
      case SET_FORMAT:
      case SET_STRIDE: {
        int32_t value = va_arg(args, int32_t);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, value);
        break;
      }
      case GET_FORMAT:
      case GET_BUFFERQUEUE_SIZE:
      case GET_SOURCE_TYPE: {
        int32_t* value = va_arg(args, int32_t*);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, value);
        break;
      }
      case SET_USAGE: {
        uint64_t usage = va_arg(args, uint64_t);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, usage);
        break;
      }
      case GET_USAGE: {
        uint64_t* usage = va_arg(args, uint64_t*);
        ret = __real_OH_NativeWindow_NativeWindowHandleOpt(window, code, usage);
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
        ret = 0;
      } else if (code == GET_SOURCE_TYPE) {
        int32_t* value = va_arg(args, int32_t*);
        if (value != nullptr) {
          *value = g_graphic_stub.source_type;
        }
        ret = 0;
      } else if (code == GET_APP_FRAMEWORK_TYPE) {
        char** framework_type = va_arg(args, char**);
        if (framework_type != nullptr) {
          *framework_type = const_cast<char*>(g_graphic_stub.framework_type);
        }
        ret = 0;
      } else if (code == SET_APP_FRAMEWORK_TYPE) {
        (void)va_arg(args, char*);
        ret = (g_stub_graphic_fail_mask & kStubFailSetAppFrameworkType) ? 1 : 0;
      } else if (code == GET_BUFFERQUEUE_SIZE) {
        int32_t* value = va_arg(args, int32_t*);
        if (value != nullptr) {
          *value = g_graphic_stub.buffer_queue_size;
        }
        ret = 0;
      } else if (code == SET_DESIRED_PRESENT_TIMESTAMP) {
        g_graphic_stub.last_present_ts = va_arg(args, int64_t);
        ret = 0;
      } else {
        ret = 0;
      }
    }
  }
  va_end(args);
  return ret;
}

static int32_t g_fail_get_id_next = 0;

void StubXcompFailNextGetXComponentId(int32_t ret) {
  g_fail_get_id_next = ret;
}

int32_t OH_NativeXComponent_GetXComponentId(OH_NativeXComponent* /*component*/,
                                            char* id,
                                            uint64_t* size) {
  if (g_fail_get_id_next != 0) {
    int32_t ret = g_fail_get_id_next;
    g_fail_get_id_next = 0;
    return ret;
  }
  if (id != nullptr && size != nullptr && *size > 0) {
    id[0] = '\0';
    *size = 0;
  }
  return OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
}
}
