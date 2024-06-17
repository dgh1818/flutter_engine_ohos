// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/vulkan/ohos/ohb_texture_source_vk.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"
#include "impeller/renderer/backend/vulkan/yuv_conversion_library_vk.h"

#include <native_buffer/native_buffer.h>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_ohos.h>

#define VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_OHOS 1000452005
#define VK_EXTERNAL_MEMORY_HANDLE_TYPE_OHOS_NATIVE_BUFFER_BIT_OHOS 0x00002000
#define VK_STRUCTURE_TYPE_IMPORT_NATIVE_BUFFER_INFO_OHOS 1000452003
#define VK_STRUCTURE_TYPE_NATIVE_BUFFER_FORMAT_PROPERTIES_OHOS 1000452002
#define VK_STRUCTURE_TYPE_NATIVE_BUFFER_PROPERTIES_OHOS 1000452001

namespace impeller {

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
      break;
  }
  return PixelFormat::kR8G8B8A8UNormInt;
}

TextureDescriptor CreateTextureDescriptorFromBufferHandle(
    const BufferHandle& handle) {
  TextureDescriptor descriptor;
  descriptor.format = ToPixelFormat(handle.format);
  descriptor.size = ISize{handle.width, handle.height};
  descriptor.storage_mode = StorageMode::kDevicePrivate;
  descriptor.type = TextureType::kTexture2D;
  descriptor.mip_count = 1;
  descriptor.sample_count = SampleCount::kCount1;
  descriptor.compression_type = CompressionType::kLossless;
  return descriptor;
}

static vk::UniqueImage CreateVkImage(
    const vk::Device& device,
    const BufferHandle& buffer_handle,
    const VkNativeBufferFormatPropertiesOHOS& ohb_format) {
  vk::ImageCreateInfo image_info = {};

  image_info.imageType = vk::ImageType::e2D;
  image_info.extent.width = buffer_handle.width;
  image_info.extent.height = buffer_handle.height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = (vk::Format)ohb_format.format;
  image_info.tiling = vk::ImageTiling::eOptimal;
  image_info.initialLayout = vk::ImageLayout::eUndefined;
  vk::ImageUsageFlags usage_flags = vk::ImageUsageFlagBits::eSampled;
  if (image_info.format != vk::Format::eUndefined) {
    usage_flags = usage_flags | vk::ImageUsageFlagBits::eTransferSrc |
                  vk::ImageUsageFlagBits::eTransferDst;
  }
  image_info.usage = usage_flags;
  image_info.samples = vk::SampleCountFlagBits::e1;
  image_info.sharingMode = vk::SharingMode::eExclusive;
  image_info.flags = {};
  image_info.queueFamilyIndexCount = 0;
  image_info.pQueueFamilyIndices = 0;

  VkExternalFormatOHOS externalFormat;
  externalFormat.sType =
      (VkStructureType)VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_OHOS;
  externalFormat.pNext = nullptr;
  externalFormat.externalFormat = 0;

  if (image_info.format == vk::Format::eUndefined) {
    externalFormat.externalFormat = ohb_format.externalFormat;
  }

  const VkExternalMemoryImageCreateInfo ext_mem_info{
      VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, &externalFormat,
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OHOS_NATIVE_BUFFER_BIT_OHOS};

  auto image_result = device.createImageUnique(image_info);
  if (image_result.result != vk::Result::eSuccess) {
    return {};
  }
  return std::move(image_result.value);
}

static vk::UniqueDeviceMemory AllocateDeviceMemorty(
    const std::shared_ptr<ContextVK>& context,
    const vk::Image& image,
    OHNativeWindowBuffer* hardware_buffer,
    const VkNativeBufferPropertiesOHOS& ohb_props,
    const fml::RefPtr<fml::NativeLibrary>& vulkan_dylib) {
  vk::Device device = context->GetDevice();
  vk::PhysicalDevice physical_device = context->GetPhysicalDevice();

  VkPhysicalDeviceMemoryProperties2 physical_device_mem_props;
  physical_device_mem_props.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
  physical_device_mem_props.pNext = nullptr;
  auto vk_call =
      vulkan_dylib->ResolveFunction<PFN_vkGetPhysicalDeviceMemoryProperties2>(
          "vkGetPhysicalDeviceMemoryProperties2");
  if (!vk_call.has_value()) {
    return {};
  }
  vk_call.value()(physical_device, &physical_device_mem_props);

  uint32_t mem_type_cnt =
      physical_device_mem_props.memoryProperties.memoryTypeCount;

  bool found = false;
  uint32_t found_index = 0;
  const auto& mem_props = physical_device_mem_props.memoryProperties;

  for (uint32_t i = 0; i < mem_type_cnt; ++i) {
    if (ohb_props.memoryTypeBits & (1 << i)) {
      uint32_t supported_flags = mem_props.memoryTypes[i].propertyFlags &
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      if (supported_flags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        found = true;
        found_index = i;
        break;
      }
    }
  }

  if (!found) {
    // TODO, ERROR
    return {};
  }
  VkImportNativeBufferInfoOHOS nb_info;
  nb_info.sType =
      (VkStructureType)VK_STRUCTURE_TYPE_IMPORT_NATIVE_BUFFER_INFO_OHOS;
  nb_info.pNext = nullptr;
  nb_info.buffer = nullptr;
  int res =
      OH_NativeBuffer_FromNativeWindowBuffer(hardware_buffer, &nb_info.buffer);
  if (res != 0 || nb_info.buffer == nullptr) {
    // TODO, ERROR
    return {};
  }

  VkMemoryDedicatedAllocateInfo ded_alloc_info;
  ded_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
  ded_alloc_info.pNext = &nb_info;
  ded_alloc_info.image = image;
  ded_alloc_info.buffer = VK_NULL_HANDLE;

  VkMemoryAllocateInfo alloc_info;
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext = &ded_alloc_info;
  alloc_info.allocationSize = ohb_props.allocationSize;
  alloc_info.memoryTypeIndex = found_index;

  auto device_memory = device.allocateMemoryUnique(alloc_info);

  if (device_memory.result != vk::Result::eSuccess) {
    // TOOD, ERROR
    return {};
  }
  return std::move(device_memory.value);
}

bool BindImageMemory(const vk::Device& device,
                     const vk::Image& image,
                     const vk::DeviceMemory& memory,
                     const fml::RefPtr<fml::NativeLibrary>& vulkan_dylib) {
  VkBindImageMemoryInfo bind_image_info;
  bind_image_info.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
  bind_image_info.pNext = nullptr;
  bind_image_info.image = image;
  bind_image_info.memory = memory;
  bind_image_info.memoryOffset = 0;
  auto vk_call = vulkan_dylib->ResolveFunction<PFN_vkBindImageMemory2>(
      "vkBindImageMemory2");
  if (!vk_call.has_value()) {
    return {};
  }
  auto res = vk_call.value()(device, 1, &bind_image_info);
  if (res != VK_SUCCESS) {
    // TODO, ERROR
    return false;
  }
  return true;
}

static std::shared_ptr<YUVConversionVK> CreateYUVConversion(
    const ContextVK& context,
    const VkNativeBufferFormatPropertiesOHOS& ohb_format) {
  YUVConversionDescriptorVK desc;
  auto& ycbcr_info = desc.get();
  // TODO,
  // 确认是否需要externalFormat字段，在YUVConversionDescriptorVK里面，安卓会有一个专属字段
  // src/flutter/impeller/renderer/backend/vulkan/yuv_conversion_vk.h/cc
  // 通过修改其externalFormat，适配OHOS的ohb_format.externalFormat
  // 或者直接把VkExternalFormatOHOS，放到ycbcr_info的pNext？
  ycbcr_info.pNext = nullptr;
  ycbcr_info.format = (vk::Format)ohb_format.format;
  ycbcr_info.ycbcrModel =
      (vk::SamplerYcbcrModelConversion)ohb_format.suggestedYcbcrModel;
  ycbcr_info.ycbcrRange = (vk::SamplerYcbcrRange)ohb_format.suggestedYcbcrRange;
  ycbcr_info.components =
      (vk::ComponentMapping)ohb_format.samplerYcbcrConversionComponents;
  ycbcr_info.xChromaOffset =
      (vk::ChromaLocation)ohb_format.suggestedXChromaOffset;
  ycbcr_info.yChromaOffset =
      (vk::ChromaLocation)ohb_format.suggestedYChromaOffset;
  ycbcr_info.chromaFilter =
      vk::Filter::eNearest;  // TODO，check the value VK_FILTER_LINERA？
  ycbcr_info.forceExplicitReconstruction = false;

  // if (ycbcr_info.format == vk::Format::eUndefined) {
  //   auto& external_format = desc.get<VkExternalFormatOHOS>();
  //   external_format.externalFormat = ohb_format.externalFormat;
  // } else {
  //   desc.unlink<VkExternalFormatOHOS>();
  // }

  return context.GetYUVConversionLibrary()->GetConversion(desc);
}

static vk::UniqueImageView CreateVkImageView(
    const vk::Device& device,
    const vk::Image& image,
    const vk::SamplerYcbcrConversion& yuv_conversion,
    const VkNativeBufferFormatPropertiesOHOS& ohb_format,
    const BufferHandle& buffer_handle) {
  vk::StructureChain<vk::ImageViewCreateInfo, vk::SamplerYcbcrConversionInfo>
      view_chain;
  auto& view_info = view_chain.get();
  view_info.pNext = nullptr;
  view_info.image = image;
  view_info.viewType = vk::ImageViewType::e2D;
  view_info.format = (vk::Format)ohb_format.format;
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
  }
  auto view_result = device.createImageViewUnique(view_info);
  if (view_result.result != vk::Result::eSuccess) {
    return {};
  }
  return std::move(view_result.value);
}

OHBTextureSourceVK::OHBTextureSourceVK(
    const std::shared_ptr<ContextVK>& context,
    OHNativeWindowBuffer* hardware_buffer)
    : TextureSourceVK(CreateTextureDescriptorFromBufferHandle(
          *OH_NativeWindow_GetBufferHandleFromNative(hardware_buffer))),
      vulkan_dylib_(fml::NativeLibrary::Create("libvulkan.so")) {
  is_valid_ = false;
  if (!hardware_buffer) {
    return;
  }
  const auto& buffer_handle =
      *OH_NativeWindow_GetBufferHandleFromNative(hardware_buffer);

  vk::Device device = context->GetDevice();
  vk::PhysicalDevice physical_device = context->GetPhysicalDevice();

  VkNativeBufferPropertiesOHOS ohb_props;
  VkNativeBufferFormatPropertiesOHOS ohb_format;
  ohb_format.sType =
      (VkStructureType)VK_STRUCTURE_TYPE_NATIVE_BUFFER_FORMAT_PROPERTIES_OHOS;
  ohb_format.pNext = nullptr;
  ohb_props.sType =
      (VkStructureType)VK_STRUCTURE_TYPE_NATIVE_BUFFER_PROPERTIES_OHOS;
  ohb_props.pNext = &ohb_format;
  auto vk_call =
      vulkan_dylib_->ResolveFunction<PFN_vkGetNativeBufferPropertiesOHOS>(
          "vkGetNativeBufferPropertiesOHOS");
  if (!vk_call.has_value()) {
    return;
  }
  OH_NativeBuffer* native_buffer;
  int res =
      OH_NativeBuffer_FromNativeWindowBuffer(hardware_buffer, &native_buffer);
  if (res != 0 || native_buffer == nullptr) {
    // TODO, ERROR
    return;
  }

  auto vk_res = vk_call.value()(device, native_buffer, &ohb_props);
  if (vk_res != VK_SUCCESS) {
    return;
  }

  auto image = CreateVkImage(device, buffer_handle, ohb_format);
  if (!image) {
    return;
  }

  auto device_memory = AllocateDeviceMemorty(
      context, image_.get(), hardware_buffer, ohb_props, vulkan_dylib_);
  if (!device_memory) {
    return;
  }

  if (!BindImageMemory(device, image.get(), device_memory.get(),
                       vulkan_dylib_)) {
    return;
  }

  auto yuv_conversion = CreateYUVConversion(*context, ohb_format);

  auto image_view =
      CreateVkImageView(device, image.get(), yuv_conversion->GetConversion(),
                        ohb_format, buffer_handle);
  if (!image_view) {
    return;
  }

  needs_yuv_conversion_ = ohb_format.format == VK_FORMAT_UNDEFINED;
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
  return yuv_conversion_;
}
}  // namespace impeller
