// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_OHOS_OHB_TEXTURE_SOURCE_VK_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_OHOS_OHB_TEXTURE_SOURCE_VK_H_

#include "flutter/fml/macros.h"
#include "impeller/geometry/size.h"
#include "impeller/renderer/backend/vulkan/formats_vk.h"
#include "impeller/renderer/backend/vulkan/texture_source_vk.h"
#include "impeller/renderer/backend/vulkan/vk.h"
#include "impeller/renderer/backend/vulkan/yuv_conversion_vk.h"

#include "flutter/fml/macros.h"
#include "flutter/fml/native_library.h"

#include <native_window/external_window.h>

namespace impeller {

class ContextVK;

class OHBTextureSourceVK final : public TextureSourceVK {
 public:
  OHBTextureSourceVK(const std::shared_ptr<ContextVK>& context,
                     OHNativeWindowBuffer* native_window_buffer);

  // |TextureSourceVK|
  ~OHBTextureSourceVK() override;

  // |TextureSourceVK|
  vk::Image GetImage() const override;

  // |TextureSourceVK|
  vk::ImageView GetImageView() const override;

  // |TextureSourceVK|
  vk::ImageView GetRenderTargetView() const override;

  bool IsValid() const;

  // |TextureSourceVK|
  bool IsSwapchainImage() const override;

  // |TextureSourceVK|
  std::shared_ptr<YUVConversionVK> GetYUVConversion() const override;

 private:
  vk::UniqueDeviceMemory device_memory_ = {};
  vk::UniqueImage image_ = {};
  vk::UniqueImageView image_view_ = {};
  std::shared_ptr<YUVConversionVK> yuv_conversion_ = {};
  bool needs_yuv_conversion_ = false;
  bool is_valid_ = false;

  OHBTextureSourceVK(const OHBTextureSourceVK&) = delete;

  OHBTextureSourceVK& operator=(const OHBTextureSourceVK&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_OHOS_OHB_TEXTURE_SOURCE_VK_H_
