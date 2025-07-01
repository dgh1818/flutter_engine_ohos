/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * All rights reserved. Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE_KHZG file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_H_

#include <multimedia/image_framework/image/pixelmap_native.h>
#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <atomic>

#include "flutter/common/graphics/texture.h"

#include "image_lru.h"
#include "include/core/SkRect.h"

namespace flutter {

class OHOSExternalTexture : public flutter::Texture {
 public:
  explicit OHOSExternalTexture(int64_t id,
                               OH_OnFrameAvailableListener listener);

  ~OHOSExternalTexture() override;

  void Paint(PaintContext& context,
             const SkRect& bounds,
             bool freeze,
             DlImageSampling sampling) override;

  void OnGrContextCreated() override;

  void OnGrContextDestroyed() override;

  void MarkNewFrameAvailable() override;

  void OnTextureUnregistered() override;

  uint64_t GetProducerSurfaceId();

  uint64_t GetProducerWindowId();

  bool SetPixelMapAsProducer(NativePixelMap* pixelMap,
                             OH_NativeBuffer* pixelMap_native_buffer);

  void SetBackGroundColor(uint32_t color);

  bool SetProducerWindowSize(int width, int height);

  void NotifyResizing(int width, int height);

  // Replace the original native_image_source_ with the external native_image.
  // This must be called on the raster thread to avoid concurrent operations
  // on native_image_source_, as the OnFrameAvailable callback for native_image
  // may be triggered immediately (the callback relies on native_image_source_
  // within the raster thread).
  bool SetExternalNativeImage(OH_NativeImage* native_image);

  uint64_t Reset(bool need_surfaceId);

  static void DefaultOnFrameAvailable(void* native_image_ptr);

  static void ReleaseWindowBuffer(OH_NativeImage* native_image,
                                  OHNativeWindowBuffer* buffer,
                                  int* fence_fd);

  static bool FenceIsSignal(int fence_fd);

  // Check if the fd is a valid sync file.
  static bool FdIsValid(int fd);

  static bool GetWindowBufferConfig(OHNativeWindowBuffer* buffer,
                                    OH_NativeBuffer** native_buffer,
                                    OH_NativeBuffer_Config* config,
                                    uint32_t* id);

 protected:
  OHNativeWindowBuffer* GetConsumerNativeBuffer(int* fence_fd);

  virtual void SetGPUFence(OHNativeWindowBuffer* window_buffer,
                           int* fence_fd) = 0;
  virtual void WaitGPUFence(int fence_fd) { close(fence_fd); }
  virtual void GPUResourceDestroy() = 0;

  virtual sk_sp<flutter::DlImage> CreateDlImage(
      PaintContext& context,
      const SkRect& bounds,
      NativeBufferKey key,
      OH_NativeBuffer_Config& config,
      OHNativeWindowBuffer* nw_buffer) = 0;

  virtual void DeleteBufferGPUResource(NativeBufferKey key) = 0;

  ImageLRU image_lru_ = ImageLRU();

 private:
  sk_sp<flutter::DlImage> GetNextDrawImage(PaintContext& context,
                                           const SkRect& bounds);

  sk_sp<flutter::DlImage> GetOldDlImage(PaintContext& context,
                                        const SkRect& bounds);

  void SetOldDlImage(sk_sp<flutter::DlImage> old_image);

  bool CopyDataToPixelMapBuffer(const unsigned char* src,
                                int width,
                                int height,
                                int stride,
                                int pixelmap_format);

  bool CreatePixelMapBuffer(int width, int height, int pixel_format);

  void DestroyNativeImageSource();

  void DestroyPixelMapBuffer();

  SkRect UpdateWindowSize(OHNativeWindowBuffer* buffer);

  bool SetWindowSize(OHNativeWindow* window, int width, int height);

  bool SetWindowFormat(OHNativeWindow* window, int format);

  bool CPUWaitFence(int fence_fd, uint32_t timeout);

  bool SetNativeWindowCPUAccess(OHNativeWindow* window, bool cpuAccess);

  void GetNewTransformBound(SkM44& transform, SkRect& bounds);

  uint64_t producer_surface_id_ = 0;

  bool producer_has_frame_ = false;
  int producer_nativewindow_width_ = 0;
  int producer_nativewindow_height_ = 0;
  OHNativeWindow* producer_nativewindow_ = nullptr;
  OHNativeWindowBuffer* pixelmap_buffer_ = nullptr;
  OH_NativeBuffer* pixelmap_native_buffer_ = nullptr;
  bool background_color_enable_ = false;
  uint32_t background_color_ = 0x00000000;  // 透明

  OHNativeWindowBuffer* last_native_window_buffer_ = nullptr;
  int last_fence_fd_ = -1;

  std::atomic<int64_t> now_paint_frame_seq_num_ = 0;

  std::atomic<int64_t> now_new_frame_seq_num_ = 0;

  bool source_is_external_ = false;
  OH_NativeImage* native_image_source_ = nullptr;

  SkMatrix transform_;

  sk_sp<flutter::DlImage> old_dl_image_;
  SkRect old_buffer_bounds_ = {};
  SkRect old_draw_bounds_ = {};
  int size_change_frames_ = 0;
  std::atomic<bool> size_is_changing_ = false;
  bool draw_size_has_changed_ = true;
  bool buffer_size_has_changed_ = true;

  OHNativeWindowBuffer* size_change_buffer_ = nullptr;
  int size_change_buffer_fence_fd_ = -1;

  OH_OnFrameAvailableListener frame_listener_;

  bool is_emulator_ = false;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalTexture);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_H_