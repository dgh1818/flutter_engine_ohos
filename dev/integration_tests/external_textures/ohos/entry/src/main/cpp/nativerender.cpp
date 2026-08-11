/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <atomic>
#include <cstdint>
#include <hilog/log.h>
#include <native_window/external_window.h>
#include <native_window/buffer_handle.h>
#include <node_api.h>
#include <sys/mman.h>

static const char *TAG = "NativeRender";

static std::atomic<int> frame_count{0};

static constexpr int32_t BORDER_WIDTH = 40;

static napi_value RenderFrame(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  uint64_t window_ptr_val = 0;
  bool lossless = false;
  napi_get_value_bigint_uint64(env, args[0], &window_ptr_val, &lossless);

  if (!lossless || window_ptr_val == 0) {
    return nullptr;
  }

  OHNativeWindow *native_window = reinterpret_cast<OHNativeWindow *>(window_ptr_val);

  int32_t ret;
  OHNativeWindowBuffer *buffer = nullptr;
  int fence_fd = -1;

  ret = OH_NativeWindow_NativeWindowRequestBuffer(native_window, &buffer, &fence_fd);
  if (ret != 0 || buffer == nullptr) {
    OH_LOG_Print(LOG_APP, LOG_WARN, LOG_DOMAIN, TAG, "RequestBuffer failed: %{public}d", ret);
    return nullptr;
  }

  BufferHandle *buffer_handle = OH_NativeWindow_GetBufferHandleFromNative(buffer);
  if (buffer_handle == nullptr) {
    Region region = {nullptr, 0};
    OH_NativeWindow_NativeWindowFlushBuffer(native_window, buffer, -1, region);
    return nullptr;
  }

  void *vaddr = buffer_handle->virAddr;
  bool need_unmap = false;
  if (vaddr == nullptr) {
    vaddr = mmap(nullptr, buffer_handle->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                 buffer_handle->fd, 0);
    if (vaddr == MAP_FAILED) {
      OH_LOG_Print(LOG_APP, LOG_WARN, LOG_DOMAIN, TAG, "mmap failed");
      Region region = {nullptr, 0};
      OH_NativeWindow_NativeWindowFlushBuffer(native_window, buffer, -1, region);
      return nullptr;
    }
    need_unmap = true;
  }

  int count = frame_count.fetch_add(1, std::memory_order_relaxed) + 1;
  uint32_t color = 0xFF000000 | ((count * 17) & 0xFF) << 16 |
                   ((count * 31) & 0xFF) << 8 | ((count * 47) & 0xFF);

  uint32_t *pixels = reinterpret_cast<uint32_t *>(vaddr);
  int32_t stride = buffer_handle->stride / 4;
  int32_t width = buffer_handle->width;
  int32_t height = buffer_handle->height;

  for (int32_t y = 0; y < height; y++) {
    for (int32_t x = 0; x < width; x++) {
      if (y < BORDER_WIDTH || y >= height - BORDER_WIDTH || x < BORDER_WIDTH || x >= width - BORDER_WIDTH) {
        pixels[y * stride + x] = color;
      } else {
        pixels[y * stride + x] = 0xFF000000;
      }
    }
  }

  if (need_unmap) {
    munmap(vaddr, buffer_handle->size);
  }

  Region region = {nullptr, 0};
  ret = OH_NativeWindow_NativeWindowFlushBuffer(native_window, buffer, -1, region);
  if (ret != 0) {
    OH_LOG_Print(LOG_APP, LOG_WARN, LOG_DOMAIN, TAG, "FlushBuffer failed: %{public}d", ret);
  }

  return nullptr;
}

static napi_value ResetFrameCount(napi_env env, napi_callback_info info) {
  frame_count.store(0, std::memory_order_relaxed);
  return nullptr;
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      {"renderFrame", nullptr, RenderFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"resetFrameCount", nullptr, ResetFrameCount, nullptr, nullptr, nullptr, napi_default, nullptr},
  };
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  return exports;
}

static napi_module nativerender_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "nativerender",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterModule(void) {
  napi_module_register(&nativerender_module);
}
