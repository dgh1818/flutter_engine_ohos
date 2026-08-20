/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/build_config.h"

#include "gtest/gtest.h"
#include "impeller/base/validation.h"
#include "impeller/core/formats.h"
#include "impeller/core/texture_descriptor.h"
#include "impeller/renderer/backend/vulkan/blit_pass_vk.h"
#include "impeller/renderer/backend/vulkan/test/mock_vulkan.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(BlitPassVKOhosTest, AddCopiesNullDestinationFails) {
  ScopedValidationDisable scope;
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  auto cmd_buffer = context->CreateCommandBuffer();
  ASSERT_NE(cmd_buffer, nullptr);
  auto blit_pass = cmd_buffer->CreateBlitPass();
  ASSERT_NE(blit_pass, nullptr);

  std::vector<BufferToTextureCopy> copies;
  EXPECT_FALSE(blit_pass->AddCopies(std::move(copies), nullptr));
}

TEST(BlitPassVKOhosTest, AddCopiesEmptyVectorSucceeds) {
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  auto cmd_buffer = context->CreateCommandBuffer();
  ASSERT_NE(cmd_buffer, nullptr);
  auto blit_pass = cmd_buffer->CreateBlitPass();
  ASSERT_NE(blit_pass, nullptr);

  TextureDescriptor dst_desc;
  dst_desc.storage_mode = StorageMode::kDevicePrivate;
  dst_desc.format = PixelFormat::kR8G8B8A8UNormInt;
  dst_desc.size = {100, 100};
  auto dst = context->GetResourceAllocator()->CreateTexture(dst_desc);
  ASSERT_TRUE(dst);

  std::vector<BufferToTextureCopy> copies;
  EXPECT_TRUE(blit_pass->AddCopies(std::move(copies), dst));
}

TEST(BlitPassVKOhosTest, AddCopiesInvalidSliceNoBufferFails) {
  ScopedValidationDisable scope;
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  auto cmd_buffer = context->CreateCommandBuffer();
  ASSERT_NE(cmd_buffer, nullptr);
  auto blit_pass = cmd_buffer->CreateBlitPass();
  ASSERT_NE(blit_pass, nullptr);

  TextureDescriptor dst_desc;
  dst_desc.storage_mode = StorageMode::kDevicePrivate;
  dst_desc.format = PixelFormat::kR8G8B8A8UNormInt;
  dst_desc.size = {100, 100};
  auto dst = context->GetResourceAllocator()->CreateTexture(dst_desc);
  ASSERT_TRUE(dst);

  std::vector<BufferToTextureCopy> copies;
  BufferView empty_source;
  copies.push_back(
      {std::move(empty_source), IRect::MakeLTRB(0, 0, 10, 10), 0u, 25u});
  EXPECT_FALSE(blit_pass->AddCopies(std::move(copies), dst));
}

TEST(BlitPassVKOhosTest, AddCopiesInvalidMipLevelNoBufferFails) {
  ScopedValidationDisable scope;
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  auto cmd_buffer = context->CreateCommandBuffer();
  ASSERT_NE(cmd_buffer, nullptr);
  auto blit_pass = cmd_buffer->CreateBlitPass();
  ASSERT_NE(blit_pass, nullptr);

  TextureDescriptor dst_desc;
  dst_desc.storage_mode = StorageMode::kDevicePrivate;
  dst_desc.format = PixelFormat::kR8G8B8A8UNormInt;
  dst_desc.size = {100, 100};
  auto dst = context->GetResourceAllocator()->CreateTexture(dst_desc);
  ASSERT_TRUE(dst);

  std::vector<BufferToTextureCopy> copies;
  BufferView empty_source;
  copies.push_back(
      {std::move(empty_source), IRect::MakeLTRB(0, 0, 10, 10), 1u, 0u});
  EXPECT_FALSE(blit_pass->AddCopies(std::move(copies), dst));
}

TEST(BlitPassVKOhosTest, AddCopiesOutOfBoundsRegionNoBufferFails) {
  ScopedValidationDisable scope;
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  auto cmd_buffer = context->CreateCommandBuffer();
  ASSERT_NE(cmd_buffer, nullptr);
  auto blit_pass = cmd_buffer->CreateBlitPass();
  ASSERT_NE(blit_pass, nullptr);

  TextureDescriptor dst_desc;
  dst_desc.storage_mode = StorageMode::kDevicePrivate;
  dst_desc.format = PixelFormat::kR8G8B8A8UNormInt;
  dst_desc.size = {100, 100};
  auto dst = context->GetResourceAllocator()->CreateTexture(dst_desc);
  ASSERT_TRUE(dst);

  std::vector<BufferToTextureCopy> copies;
  BufferView empty_source;
  copies.push_back(
      {std::move(empty_source), IRect::MakeLTRB(0, 0, 200, 200), 0u, 0u});
  EXPECT_FALSE(blit_pass->AddCopies(std::move(copies), dst));
}

TEST(BlitPassVKOhosTest, AddCopiesNegativeRegionFails) {
  ScopedValidationDisable scope;
  auto const context = MockVulkanContextBuilder().Build();
  ASSERT_NE(context, nullptr);

  auto cmd_buffer = context->CreateCommandBuffer();
  ASSERT_NE(cmd_buffer, nullptr);
  auto blit_pass = cmd_buffer->CreateBlitPass();
  ASSERT_NE(blit_pass, nullptr);

  TextureDescriptor dst_desc;
  dst_desc.storage_mode = StorageMode::kDevicePrivate;
  dst_desc.format = PixelFormat::kR8G8B8A8UNormInt;
  dst_desc.size = {100, 100};
  auto dst = context->GetResourceAllocator()->CreateTexture(dst_desc);
  ASSERT_TRUE(dst);

  std::vector<BufferToTextureCopy> copies;
  BufferView empty_source;
  copies.push_back(
      {std::move(empty_source), IRect::MakeLTRB(-1, -1, 10, 10), 0u, 0u});
  EXPECT_FALSE(blit_pass->AddCopies(std::move(copies), dst));
}

}  // namespace testing
}  // namespace impeller

#endif  // defined(FML_OS_OHOS)
