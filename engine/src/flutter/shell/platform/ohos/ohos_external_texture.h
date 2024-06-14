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

#include <multimedia/image_framework/image_mdk.h>
#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>

#include "flutter/common/graphics/texture.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

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

  bool SetPixelMapAsProducer(NativePixelMap* pixelMap);

 protected:
  OH_NativeBuffer* GetConsumerNativeBuffer(int* fence_fd);

  void ReleaseConsumerNativeBuffer(OH_NativeBuffer* buffer, int fence_fd);

  virtual sk_sp<flutter::DlImage> GetNextDrawImage();

  virtual void GPUResourceDestroy();

  ImageLRU image_lru_ = ImageLRU();

 private:
  bool CreateProducerNativeBuffer(int width, int height);

  bool CopyDataToNativeBuffer(const char* src,
                              int width,
                              int height,
                              int stride);

  //   void UpdateTransform();

  enum class AttachmentState { kUninitialized, kAttached, kDetached };

  AttachmentState state_ = AttachmentState::kUninitialized;

  bool new_frame_ready_ = false;

  uint64_t producer_surface_id_;

  int producer_nativewindow_width_;
  int producer_nativewindow_height_;
  OHNativeWindow* producer_nativewindow_;
  OH_NativeBuffer* producer_nativebuffer_;
  OHNativeWindowBuffer* producer_nativewindowbuffer_;

  OH_NativeImage* native_image_source_;

  NativePixelMap* pixelmap_source_;

  OHNativeWindowBuffer* pixelmap_buffer_;

  SkMatrix transform_;

  OhosPixelMapInfos pixelmap_info_;

  sk_sp<flutter::DlImage> old_dl_image_;

  FML_DISALLOW_COPY_AND_ASSIGN(OHOSExternalTexture);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_EXTERNAL_TEXTURE_H_