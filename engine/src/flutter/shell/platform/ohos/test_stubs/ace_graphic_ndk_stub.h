/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_TESTING_ACE_GRAPHIC_NDK_STUB_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_TESTING_ACE_GRAPHIC_NDK_STUB_H_

#include <stdint.h>

#ifdef __cplusplus

struct GraphicStubState {
  uint32_t fail_mask = 0u;
  int32_t geometry_width = 0;
  int32_t geometry_height = 0;
  int32_t buffer_format = 12;
  int32_t source_type = 0;
  int32_t buffer_queue_size = 0;
  int32_t touch_type = 0;
  int32_t touch_id = 0;
  int32_t touch_num_points = 0;
  int32_t mouse_button = 0;
  int32_t mouse_action = 0;
  const char* framework_type = "";
  int engaged = 0;
  int from_native_window_buffer_fail = 0;
  int vsync_request_fail = 0;
  int64_t last_present_ts = 0;
};

extern GraphicStubState g_graphic_stub;

extern "C" void UpdateFromNativeWindowBufferFail(int fail);

extern "C" void StubArkuiSetActionArgument(const char* key, const char* value);
extern "C" void StubArkuiResetActionArguments(void);
extern "C" void StubXcompFailNextGetXComponentId(int32_t ret);

enum GraphicStubKnob {
  kStubFailNativeImageCreate = 1 << 0,
  kStubFailAcquireNativeWindow = 1 << 1,
  kStubFailWindowHandleOpt = 1 << 2,
  kStubFailFrameAvailableListener = 1 << 3,
  kStubFailReleaseWindowBuffer = 1 << 4,
  kStubFailRequestBuffer = 1 << 5,
  kStubFailGetBufferHandle = 1 << 6,
  kStubFailFlushBuffer = 1 << 7,
  kStubAcquireBufferSuccess = 1 << 8,
  kStubBufferHandleBadFd = 1 << 9,
  kStubFailFromNativeWindowBuffer = 1 << 10,
  kStubFailTouchTilt = 1 << 11,
  kStubFailSetAppFrameworkType = 1 << 12,
  kStubFailAttachBuffer = 1 << 13,
  kStubAcquireBufferFailWithBuffer = 1 << 14,
  kStubFailGetTouchEvent = 1 << 15,
  kStubFailGetMouseEvent = 1 << 16,
  kStubFailGetTouchEventSourceType = 1 << 17,
  kStubFailSetNeedSoftKeyboard = 1 << 18,
  kStubFailGetXComponentSize = 1 << 19,
  kStubFailNativeObjectReference = 1 << 20,
  kStubFailNativeObjectUnreference = 1 << 21,
  kStubFailGetTouchPointToolType = 1 << 22,
};

class GraphicStubKnobGuard {
 public:
  GraphicStubKnobGuard() {
    g_graphic_stub = GraphicStubState{};
    g_graphic_stub.engaged = 1;
  }
  ~GraphicStubKnobGuard() { g_graphic_stub = GraphicStubState{}; }
};

#define g_stub_graphic_fail_mask g_graphic_stub.fail_mask
#define g_stub_geometry_width g_graphic_stub.geometry_width
#define g_stub_geometry_height g_graphic_stub.geometry_height
#define g_stub_buffer_format g_graphic_stub.buffer_format
#define g_stub_source_type g_graphic_stub.source_type
#define g_stub_buffer_queue_size g_graphic_stub.buffer_queue_size
#define g_stub_framework_type g_graphic_stub.framework_type
#define g_stub_graphic_engaged g_graphic_stub.engaged
#define g_from_native_window_buffer_fail \
  g_graphic_stub.from_native_window_buffer_fail
#define g_stub_vsync_request_fail g_graphic_stub.vsync_request_fail

#else  // !__cplusplus

extern int g_stub_graphic_engaged_c_dummy;

#endif  // __cplusplus

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_TESTING_ACE_GRAPHIC_NDK_STUB_H_
