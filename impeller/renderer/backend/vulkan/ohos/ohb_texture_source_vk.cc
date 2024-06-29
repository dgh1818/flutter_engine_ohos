// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/vulkan/ohos/ohb_texture_source_vk.h"
#include "impeller/core/formats.h"
#include "impeller/renderer/backend/vulkan/allocator_vk.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"
#include "impeller/renderer/backend/vulkan/yuv_conversion_library_vk.h"

#include <native_buffer/native_buffer.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_ohos.h>
#include <cstdint>

namespace impeller {

using ONBProperties = vk::StructureChain<vk::NativeBufferPropertiesOHOS,
                                         vk::NativeBufferFormatPropertiesOHOS>;

static PixelFormat ToPixelFormat(int32_t format) {
  if (format < 0 || format > NATIVEBUFFER_PIXEL_FMT_RGBA_1010102) {
    return PixelFormat::kR8G8B8A8UNormInt;
  }
  OH_NativeBuffer_Format format_spec =
      static_cast<OH_NativeBuffer_Format>(format);
  switch (format_spec) {
    case OH_NativeBuffer_Format::NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
      return PixelFormat::kR8G8B8A8UNormInt;
    case OH_NativeBuffer_Format::NATIVEBUFFER_PIXEL_FMT_BGRA_8888:
      return PixelFormat::kB8G8R8A8UNormInt;
    default:
      // Not understood by the rest of Impeller. Use a placeholder but create
      // the native image and image views using the right external format.
      break;
  }
  return PixelFormat::kR8G8B8A8UNormInt;
}

static TextureDescriptor CreateTextureDescriptorFromNativeWindowBuffer(
    OHNativeWindowBuffer* native_window_buffer) {
  OH_NativeBuffer_Config nativebuffer_config;
  OH_NativeBuffer* native_buffer;
  TextureDescriptor descriptor;
  descriptor.size = {0, 0};
  int ret = OH_NativeBuffer_FromNativeWindowBuffer(native_window_buffer,
                                                   &native_buffer);
  if (ret != 0) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL get OH_NativeBuffer error:" << ret;
    return descriptor;
  }
  OH_NativeBuffer_GetConfig(native_buffer, &nativebuffer_config);
  descriptor.format = ToPixelFormat(nativebuffer_config.format);
  descriptor.size =
      ISize{nativebuffer_config.width, nativebuffer_config.height};
  descriptor.storage_mode = StorageMode::kDevicePrivate;
  descriptor.type = TextureType::kTexture2D;
  descriptor.mip_count = 1;
  descriptor.sample_count = SampleCount::kCount1;
  descriptor.compression_type = CompressionType::kLossless;
  return descriptor;
}

static vk::UniqueImage CreateVkImage(
    const vk::Device& device,
    OH_NativeBuffer* native_buffer,
    const vk::NativeBufferFormatPropertiesOHOS& onb_format) {
  VkExternalFormatOHOS externalFormat{
      VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_OHOS,
      nullptr,
      0,
  };
  if (onb_format.format == vk::Format::eUndefined) {
    externalFormat.externalFormat = onb_format.externalFormat;
  }

  VkExternalMemoryImageCreateInfo ext_mem_info{
      VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, &externalFormat,
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OHOS_NATIVE_BUFFER_BIT_OHOS};

  OH_NativeBuffer_Config nativebuffer_config;
  OH_NativeBuffer_GetConfig(native_buffer, &nativebuffer_config);

  vk::ImageUsageFlags usage_flags = vk::ImageUsageFlagBits::eSampled;
  if (nativebuffer_config.usage & NATIVEBUFFER_USAGE_HW_RENDER) {
    usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;
  }

  vk::ImageCreateInfo image_info;
  image_info.pNext = &ext_mem_info;
  image_info.flags = vk::ImageCreateFlags(0);
  image_info.imageType = vk::ImageType::e2D;
  image_info.extent.width = nativebuffer_config.width;
  image_info.extent.height = nativebuffer_config.height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = onb_format.format;
  image_info.tiling = vk::ImageTiling::eOptimal;
  image_info.initialLayout = vk::ImageLayout::eUndefined;
  image_info.usage = usage_flags;
  image_info.samples = vk::SampleCountFlagBits::e1;
  image_info.sharingMode = vk::SharingMode::eExclusive;

  auto image_result = device.createImageUnique(image_info);
  if (image_result.result != vk::Result::eSuccess) {
    return {};
  }
  return std::move(image_result.value);
}

static vk::UniqueDeviceMemory AllocateDeviceMemorty(
    const std::shared_ptr<ContextVK>& context,
    const vk::Image& image,
    OH_NativeBuffer* native_buffer,
    const vk::NativeBufferPropertiesOHOS& ohb_props) {
  vk::Device device = context->GetDevice();
  vk::PhysicalDevice physical_device = context->GetPhysicalDevice();
  vk::PhysicalDeviceMemoryProperties memory_properties;
  physical_device.getMemoryProperties(&memory_properties);
  int memory_type_index = AllocatorVK::FindMemoryTypeIndex(
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_properties);
  if (memory_type_index < 0) {
    VALIDATION_LOG << "Could not find memory type of external image.";
    return {};
  }

  FML_LOG(INFO) << "find memory index " << memory_type_index;

  VkImportNativeBufferInfoOHOS nb_info;
  nb_info.sType = VK_STRUCTURE_TYPE_IMPORT_NATIVE_BUFFER_INFO_OHOS;
  nb_info.buffer = native_buffer;
  nb_info.pNext = nullptr;

  VkMemoryDedicatedAllocateInfo ded_alloc_info;
  ded_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
  ded_alloc_info.image = image;
  ded_alloc_info.buffer = VK_NULL_HANDLE;
  ded_alloc_info.pNext = &nb_info;

  VkMemoryAllocateInfo alloc_info;
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = ohb_props.allocationSize;
  alloc_info.memoryTypeIndex = memory_type_index;
  alloc_info.pNext = &ded_alloc_info;

  auto device_memory = device.allocateMemoryUnique(alloc_info);

  if (device_memory.result != vk::Result::eSuccess) {
    FML_LOG(ERROR) << "allocateMemoryUnique failed";
    return {};
  }
  return std::move(device_memory.value);
}

static std::shared_ptr<YUVConversionVK> CreateYUVConversion(
    const ContextVK& context,
    const vk::NativeBufferFormatPropertiesOHOS& onb_format) {
  YUVConversionDescriptorVK conversion_chain;
  auto& ycbcr_info = conversion_chain.get();
  ycbcr_info.format = onb_format.format;
  ycbcr_info.ycbcrModel = onb_format.suggestedYcbcrModel;
  ycbcr_info.ycbcrRange = onb_format.suggestedYcbcrRange;
  ycbcr_info.components = onb_format.samplerYcbcrConversionComponents;
  ycbcr_info.xChromaOffset = onb_format.suggestedXChromaOffset;
  ycbcr_info.yChromaOffset = onb_format.suggestedYChromaOffset;
  ycbcr_info.chromaFilter = vk::Filter::eNearest;
  ycbcr_info.forceExplicitReconstruction = false;

  if (ycbcr_info.format == vk::Format::eUndefined) {
    auto& external_format = conversion_chain.get<vk::ExternalFormatOHOS>();
    external_format.externalFormat = onb_format.externalFormat;
    FML_LOG(INFO) << "set yuv external_format " << (onb_format.externalFormat);
  } else {
    conversion_chain.unlink<vk::ExternalFormatOHOS>();
    FML_LOG(INFO) << "not set yuv external_format";
  }

  return context.GetYUVConversionLibrary()->GetConversion(conversion_chain);
}

static vk::UniqueImageView CreateVkImageView(
    const vk::Device& device,
    const vk::Image& image,
    const vk::SamplerYcbcrConversion& yuv_conversion,
    const vk::NativeBufferFormatPropertiesOHOS& onb_format) {
  vk::StructureChain<vk::ImageViewCreateInfo, vk::SamplerYcbcrConversionInfo>
      view_chain;
  auto& view_info = view_chain.get();
  view_info.image = image;
  view_info.viewType = vk::ImageViewType::e2D;
  view_info.format = onb_format.format;
  view_info.components.r = vk::ComponentSwizzle::eIdentity;
  view_info.components.g = vk::ComponentSwizzle::eIdentity;
  view_info.components.b = vk::ComponentSwizzle::eIdentity;
  view_info.components.a = vk::ComponentSwizzle::eIdentity;
  view_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (view_info.format == vk::Format::eUndefined) {
    view_chain.get<vk::SamplerYcbcrConversionInfo>().conversion =
        yuv_conversion;
  } else {
    view_chain.unlink<vk::SamplerYcbcrConversionInfo>();
    FML_LOG(ERROR) << "unlink yuv sampler ";
  }
  auto view_result = device.createImageViewUnique(view_info);
  if (view_result.result != vk::Result::eSuccess) {
    return {};
  }
  return std::move(view_result.value);
}

OHBTextureSourceVK::OHBTextureSourceVK(
    const std::shared_ptr<ContextVK>& context,
    OHNativeWindowBuffer* native_window_buffer)
    : TextureSourceVK(
          CreateTextureDescriptorFromNativeWindowBuffer(native_window_buffer)) {
  is_valid_ = false;
  if (!native_window_buffer) {
    return;
  }

  vk::Device device = context->GetDevice();
  OH_NativeBuffer* native_buffer = nullptr;

  int ret = OH_NativeBuffer_FromNativeWindowBuffer(native_window_buffer,
                                                   &native_buffer);
  if (ret != 0 || native_buffer == nullptr) {
    FML_LOG(ERROR) << "OHOSExternalTextureGL get OH_NativeBuffer error:" << ret;
    return;
  }

  ONBProperties onb_props;
  auto get_ret =
      device.getNativeBufferPropertiesOHOS(native_buffer, &onb_props.get());
  if (get_ret != vk::Result::eSuccess) {
    FML_LOG(ERROR) << "getNativeBufferPropertiesOHOS faile " << int(get_ret);
    return;
  }

  const auto& onb_format =
      onb_props.get<vk::NativeBufferFormatPropertiesOHOS>();

  FML_LOG(INFO) << "onb_format  " << int(onb_format.format) << " external "
                << int(onb_format.externalFormat) << " allocSize "
                << onb_props.get().allocationSize;
  auto image = CreateVkImage(device, native_buffer, onb_format);
  if (!image) {
    FML_LOG(ERROR) << "create vkimage faile";
    return;
  }

  auto device_memory = AllocateDeviceMemorty(context, image_.get(),
                                             native_buffer, onb_props.get());
  if (!device_memory) {
    FML_LOG(ERROR) << "allocateDeviceMemorty failed";
    return;
  }

  auto bind_ret = device.bindImageMemory(image.get(), device_memory.get(), 0);
  if (bind_ret != vk::Result::eSuccess) {
    FML_LOG(ERROR) << "bindImageMemory failed " << int(bind_ret);
    return;
  }

  auto yuv_conversion = CreateYUVConversion(*context, onb_format);

  auto image_view = CreateVkImageView(
      device, image.get(), yuv_conversion->GetConversion(), onb_format);
  if (!image_view) {
    FML_LOG(ERROR) << "CreateVkImageView failed";
    return;
  }
  needs_yuv_conversion_ = onb_format.format == vk::Format::eUndefined;
  device_memory_ = std::move(device_memory);
  image_ = std::move(image);
  yuv_conversion_ = std::move(yuv_conversion);
  image_view_ = std::move(image_view);
  is_valid_ = true;
}

OHBTextureSourceVK::~OHBTextureSourceVK() {
  // Vulkan resources are automatically destroyed by vk::Unique* destructors.
}

vk::Image OHBTextureSourceVK::GetImage() const {
  return image_.get();
}

vk::ImageView OHBTextureSourceVK::GetImageView() const {
  return image_view_.get();
}

vk::ImageView OHBTextureSourceVK::GetRenderTargetView() const {
  return image_view_.get();  // Assuming same view can be used for render target
}

bool OHBTextureSourceVK::IsValid() const {
  return is_valid_;
}

bool OHBTextureSourceVK::IsSwapchainImage() const {
  return false;
}

std::shared_ptr<YUVConversionVK> OHBTextureSourceVK::GetYUVConversion() const {
  return needs_yuv_conversion_ ? yuv_conversion_ : nullptr;
}
}  // namespace impeller
