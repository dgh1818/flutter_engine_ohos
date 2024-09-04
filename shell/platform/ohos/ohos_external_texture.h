/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_H_

#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <atomic>

#include "flutter/common/graphics/texture.h"

#include "image_lru.h"

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

  bool SetPixelMapAsProducer(NativePixelMap* pixelMap);

 protected:
  OHNativeWindowBuffer* GetConsumerNativeBuffer(int* fence_fd);

  void ReleaseConsumerNativeBuffer(OHNativeWindowBuffer* buffer, int fence_fd);

  virtual void SetGPUFence(int* fence_fd) = 0;
  virtual void WaitGPUFence(int fence_fd) { close(fence_fd); }
  virtual void GPUResourceDestroy() = 0;

  virtual sk_sp<flutter::DlImage> CreateDlImage(
      PaintContext& context,
      const SkRect& bounds,
      NativeBufferKey key,
      OHNativeWindowBuffer* nw_buffer) = 0;

  ImageLRU image_lru_ = ImageLRU();

 private:
  sk_sp<flutter::DlImage> GetNextDrawImage(PaintContext& context,
                                           const SkRect& bounds);

  bool CreateProducerNativeBuffer(int width, int height);

  bool CopyDataToNativeBuffer(const unsigned char* src,
                              int width,
                              int height,
                              int stride);

  void GetNewTransformBound(SkM44& transform, SkRect& bounds);

  enum class AttachmentState { kUninitialized, kAttached, kDetached };

  AttachmentState state_ = AttachmentState::kUninitialized;

  // bool new_frame_ready_ = false;

  uint64_t producer_surface_id_ = 0;

  int producer_nativewindow_width_ = 0;
  int producer_nativewindow_height_ = 0;
  OHNativeWindow* producer_nativewindow_ = nullptr;
  OHNativeWindowBuffer* producer_nativewindow_buffer_ = nullptr;

  OHNativeWindowBuffer* last_native_window_buffer_ = nullptr;
  int last_fence_fd_ = -1;

  std::atomic<int64_t> now_paint_frame_seq_num_ = 0;

  std::atomic<int64_t> now_new_frame_seq_num_ = 0;

  OH_NativeImage* native_image_source_ = nullptr;

  SkMatrix transform_;

  sk_sp<flutter::DlImage> old_dl_image_;

  OH_OnFrameAvailableListener frame_listener_;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalTexture);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_H_