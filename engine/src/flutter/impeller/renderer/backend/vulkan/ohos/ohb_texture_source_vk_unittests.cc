/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "impeller/renderer/backend/vulkan/ohos/ohb_texture_source_vk.h"

#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <memory>

#include "flutter/fml/native_library.h"
#include "flutter/testing/testing.h"
#include "gtest/gtest.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"

namespace impeller::ohos::testing {

// OHBTextureSourceVK is a Class-3 (device-only) target: its constructor calls
// OH_NativeBuffer_FromNativeWindowBuffer, OH_NativeBuffer_GetConfig, Vulkan
// device.getNativeBufferPropertiesOHOS, device.createImageUnique,
// device.allocateMemoryUnique, etc. These tests run on the device
// (flutter_ohos_unittests) and require a real Vulkan context plus a real
// OH_NativeBuffer.

namespace {

// Create a Vulkan context by loading libvulkan.so and resolving
// vkGetInstanceProcAddr, mirroring the Android AHB texture source test.
// Returns the ContextVK shared_ptr directly, since OHBTextureSourceVK's
// constructor requires a shared_ptr<ContextVK>.
std::shared_ptr<ContextVK> CreateContext() {
  auto vulkan_dylib = fml::NativeLibrary::Create("libvulkan.so");
  if (!vulkan_dylib) {
    return nullptr;
  }
  auto instance_proc_addr =
      vulkan_dylib->ResolveFunction<PFN_vkGetInstanceProcAddr>(
          "vkGetInstanceProcAddr");
  if (!instance_proc_addr.has_value()) {
    return nullptr;
  }

  ContextVK::Settings settings;
  settings.proc_address_callback = instance_proc_addr.value();
  settings.shader_libraries_data = {};
  settings.enable_validation = false;
  settings.enable_gpu_tracing = false;
  settings.enable_surface_control = false;

  return ContextVK::Create(std::move(settings));
}

// Allocate a real OH_NativeBuffer with the given dimensions and format.
OH_NativeBuffer* AllocNativeBuffer(int32_t width,
                                    int32_t height,
                                    int32_t format) {
  OH_NativeBuffer_Config config;
  config.width = width;
  config.height = height;
  config.format = format;
  config.usage = NATIVEBUFFER_USAGE_HW_RENDER | NATIVEBUFFER_USAGE_CPU_READ |
                 NATIVEBUFFER_USAGE_CPU_WRITE;
  config.stride = 0;
  return OH_NativeBuffer_Alloc(&config);
}

}  // namespace

// ---------------------------------------------------------------------------
// Constructor / IsValid
// ---------------------------------------------------------------------------

// Constructing with a null native_window_buffer yields an invalid source
// (the constructor early-returns when native_window_buffer is null).
TEST(OHBTextureSourceVKTest, NullBufferYieldsInvalid) {
  auto context = CreateContext();
  ASSERT_TRUE(context);

  OHBTextureSourceVK source(context, /*native_window_buffer=*/nullptr,
                            TextureColorSpace::kSRGB);
  EXPECT_FALSE(source.IsValid());
  EXPECT_EQ(source.GetImage(), vk::Image{});
  EXPECT_EQ(source.GetImageView(), vk::ImageView{});
  EXPECT_EQ(source.GetRenderTargetView(), vk::ImageView{});

  context->Shutdown();
}

// Constructing with a real RGBA_8888 native buffer yields a valid source.
TEST(OHBTextureSourceVKTest, CanImportRGBA8888) {
  auto context = CreateContext();
  ASSERT_TRUE(context);

  OH_NativeBuffer* native_buffer =
      AllocNativeBuffer(16, 16, NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
  ASSERT_NE(native_buffer, nullptr);

  OHNativeWindowBuffer* window_buffer =
      OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
  ASSERT_NE(window_buffer, nullptr);

  OHBTextureSourceVK source(context, window_buffer,
                            TextureColorSpace::kSRGB);
  EXPECT_TRUE(source.IsValid());
  // An RGBA_8888 buffer has a known vk::Format, so no YUV conversion is
  // needed.
  EXPECT_EQ(source.GetYUVConversion(), nullptr);
  EXPECT_FALSE(source.IsSwapchainImage());

  // The image and image view should be non-null handles.
  EXPECT_NE(source.GetImage(), vk::Image{});
  EXPECT_NE(source.GetImageView(), vk::ImageView{});
  EXPECT_NE(source.GetRenderTargetView(), vk::ImageView{});

  OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
  OH_NativeBuffer_Unreference(native_buffer);
  context->Shutdown();
}

// ---------------------------------------------------------------------------
// Color space propagation
// ---------------------------------------------------------------------------

// The texture descriptor's color space should match the one passed to the
// constructor. We verify this indirectly by checking that the source is valid
// for each color space value.
TEST(OHBTextureSourceVKTest, AcceptsAllTextureColorSpaces) {
  auto context = CreateContext();
  ASSERT_TRUE(context);

  for (auto color_space : {TextureColorSpace::kSRGB,
                            TextureColorSpace::kDisplayP3,
                            TextureColorSpace::kExtendedSRGB}) {
    OH_NativeBuffer* native_buffer =
        AllocNativeBuffer(8, 8, NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
    ASSERT_NE(native_buffer, nullptr);
    OHNativeWindowBuffer* window_buffer =
        OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
    ASSERT_NE(window_buffer, nullptr);

    OHBTextureSourceVK source(context, window_buffer, color_space);
    EXPECT_TRUE(source.IsValid()) << "color_space=" << static_cast<int>(color_space);

    OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
    OH_NativeBuffer_Unreference(native_buffer);
  }
  context->Shutdown();
}

// ---------------------------------------------------------------------------
// IsSwapchainImage
// ---------------------------------------------------------------------------

// OHBTextureSourceVK is never a swapchain image.
TEST(OHBTextureSourceVKTest, IsNotSwapchainImage) {
  auto context = CreateContext();
  ASSERT_TRUE(context);

  OH_NativeBuffer* native_buffer =
      AllocNativeBuffer(4, 4, NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
  ASSERT_NE(native_buffer, nullptr);
  OHNativeWindowBuffer* window_buffer =
      OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
  ASSERT_NE(window_buffer, nullptr);

  OHBTextureSourceVK source(context, window_buffer);
  EXPECT_TRUE(source.IsValid());
  EXPECT_FALSE(source.IsSwapchainImage());

  OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
  OH_NativeBuffer_Unreference(native_buffer);
  context->Shutdown();
}

// ---------------------------------------------------------------------------
// GetRenderTargetView
// ---------------------------------------------------------------------------

// GetRenderTargetView returns the same view as GetImageView (per the
// implementation, both return image_view_.get()).
TEST(OHBTextureSourceVKTest, RenderTargetViewEqualsImageView) {
  auto context = CreateContext();
  ASSERT_TRUE(context);

  OH_NativeBuffer* native_buffer =
      AllocNativeBuffer(4, 4, NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
  ASSERT_NE(native_buffer, nullptr);
  OHNativeWindowBuffer* window_buffer =
      OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
  ASSERT_NE(window_buffer, nullptr);

  OHBTextureSourceVK source(context, window_buffer);
  ASSERT_TRUE(source.IsValid());
  EXPECT_EQ(source.GetRenderTargetView(), source.GetImageView());

  OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
  OH_NativeBuffer_Unreference(native_buffer);
  context->Shutdown();
}

// ---------------------------------------------------------------------------
// BGRA_8888 format — exercises the ToPixelFormat BGRA branch
// ---------------------------------------------------------------------------

// A BGRA_8888 buffer maps to PixelFormat::kB8G8R8A8UNormInt in ToPixelFormat,
// exercising the second case branch of the switch statement.
TEST(OHBTextureSourceVKTest, CanImportBGRA8888) {
  auto context = CreateContext();
  ASSERT_TRUE(context);

  OH_NativeBuffer* native_buffer =
      AllocNativeBuffer(8, 8, NATIVEBUFFER_PIXEL_FMT_BGRA_8888);
  if (native_buffer == nullptr) {
    GTEST_SKIP() << "Device does not support BGRA_8888 native buffers.";
  }
  OHNativeWindowBuffer* window_buffer =
      OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
  ASSERT_NE(window_buffer, nullptr);

  OHBTextureSourceVK source(context, window_buffer, TextureColorSpace::kSRGB);
  EXPECT_TRUE(source.IsValid()) << "BGRA_8888 buffer should be importable";
  // BGRA_8888 has a known vk::Format, so no YUV conversion is needed.
  EXPECT_EQ(source.GetYUVConversion(), nullptr);

  OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
  OH_NativeBuffer_Unreference(native_buffer);
  context->Shutdown();
}

// ---------------------------------------------------------------------------
// YUV format — exercises the YUV conversion path
// ---------------------------------------------------------------------------

// A YUV-format buffer has vk::Format::eUndefined, so the constructor takes
// the YUV external-format path (CreateYUVConversion + SamplerYcbcrConversion).
// This covers the YUV branches that the RGBA/BGRA tests do not exercise.
TEST(OHBTextureSourceVKTest, CanImportYUVFormat) {
  auto context = CreateContext();
  ASSERT_TRUE(context);

  OH_NativeBuffer* native_buffer =
      AllocNativeBuffer(16, 16, NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP);
  if (native_buffer == nullptr) {
    GTEST_SKIP() << "Device does not support YCRCB_420_SP native buffers.";
  }
  OHNativeWindowBuffer* window_buffer =
      OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
  ASSERT_NE(window_buffer, nullptr);

  OHBTextureSourceVK source(context, window_buffer, TextureColorSpace::kSRGB);
  EXPECT_TRUE(source.IsValid()) << "YUV buffer should be importable";
  // YUV formats have vk::Format::eUndefined, so a YUV conversion should be
  // created and attached.
  EXPECT_NE(source.GetYUVConversion(), nullptr)
      << "YUV format should require a YUV conversion";

  OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
  OH_NativeBuffer_Unreference(native_buffer);
  context->Shutdown();
}

}  // namespace impeller::ohos::testing
