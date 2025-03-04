// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_CAPABILITIES_VK_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_CAPABILITIES_VK_H_

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "impeller/base/backend_cast.h"
#include "impeller/renderer/backend/vulkan/vk.h"
#include "impeller/renderer/capabilities.h"

namespace impeller {

class ContextVK;

//------------------------------------------------------------------------------
/// @brief      A device extension available on all platforms. Without the
///             presence of these extensions, context creation will fail.
///
enum class RequiredCommonDeviceExtensionVK : uint32_t {
  //----------------------------------------------------------------------------
  /// For displaying content in the window system.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_swapchain.html
  ///
  kKHRSwapchain,

  kLast,
};

//------------------------------------------------------------------------------
/// @brief      A device extension available on all Android platforms. Without
///             the presence of these extensions on Android, context creation
///             will fail.
///
///             Platform agnostic code can still check if these Android
///             extensions are present.
///
enum class RequiredAndroidDeviceExtensionVK : uint32_t {
  //----------------------------------------------------------------------------
  /// For importing hardware buffers used in external texture composition.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_ANDROID_external_memory_android_hardware_buffer.html
  ///
  kANDROIDExternalMemoryAndroidHardwareBuffer,

  //----------------------------------------------------------------------------
  /// Dependency of kANDROIDExternalMemoryAndroidHardwareBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_sampler_ycbcr_conversion.html
  ///
  kKHRSamplerYcbcrConversion,

  //----------------------------------------------------------------------------
  /// Dependency of kANDROIDExternalMemoryAndroidHardwareBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_memory.html
  ///
  kKHRExternalMemory,

  //----------------------------------------------------------------------------
  /// Dependency of kANDROIDExternalMemoryAndroidHardwareBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_queue_family_foreign.html
  ///
  kEXTQueueFamilyForeign,

  //----------------------------------------------------------------------------
  /// Dependency of kANDROIDExternalMemoryAndroidHardwareBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_dedicated_allocation.html
  ///
  kKHRDedicatedAllocation,

  kLast,
};

//------------------------------------------------------------------------------
/// @brief      A device extension available on all OHOS platforms. Without
///             the presence of these extensions on OHOS, context creation
///             will fail.
///
///             Platform agnostic code can still check if these OHOS
///             extensions are present.
///
enum class RequiredOHOSDeviceExtensionVK : uint32_t {
  //----------------------------------------------------------------------------
  /// For importing hardware buffers used in external texture composition.
  ///
  /// https://developer.huawei.com/consumer/cn/doc/harmonyos-references-V5/vulkan__ohos_8h-V5
  ///
  kOHOSNativeBuffer,

  //----------------------------------------------------------------------------
  /// Dependency of kOHOSNativeBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_sampler_ycbcr_conversion.html
  ///
  kKHRSamplerYcbcrConversion,

  //----------------------------------------------------------------------------
  /// Dependency of kOHOSNativeBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_memory.html
  ///
  kOHOSExternalMemory,

  //----------------------------------------------------------------------------
  /// Dependency of kOHOSNativeBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_queue_family_foreign.html
  ///
  kEXTQueueFamilyForeign,

  //----------------------------------------------------------------------------
  /// Dependency of kOHOSNativeBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_external_semaphore_fd.html
  ///
  kKHRExternalSemaphoreFd,

  //----------------------------------------------------------------------------
  /// Dependency of kOHOSNativeBuffer.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_dedicated_allocation.html
  ///
  kKHRDedicatedAllocation,

  kLast,
};

//------------------------------------------------------------------------------
/// @brief      A device extension enabled if available. Subsystems cannot
///             assume availability and must check if these extensions are
///             available.
///
/// @see        `CapabilitiesVK::HasExtension`.
///
enum class OptionalDeviceExtensionVK : uint32_t {
  //----------------------------------------------------------------------------
  /// To instrument and profile PSO creation.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_pipeline_creation_feedback.html
  ///
  kEXTPipelineCreationFeedback,

  //----------------------------------------------------------------------------
  /// To enable context creation on MoltenVK. A non-conformant Vulkan
  /// implementation.
  ///
  /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_subset.html
  ///
  kVKKHRPortabilitySubset,

  kLast,
};

//------------------------------------------------------------------------------
/// @brief      The Vulkan layers and extensions wrangler.
///
class CapabilitiesVK final : public Capabilities,
                             public BackendCast<CapabilitiesVK, Capabilities> {
 public:
  explicit CapabilitiesVK(bool enable_validations);

  ~CapabilitiesVK();

  bool IsValid() const;

  bool AreValidationsEnabled() const;

  bool HasExtension(RequiredCommonDeviceExtensionVK ext) const;

  bool HasExtension(RequiredAndroidDeviceExtensionVK ext) const;

  bool HasExtension(RequiredOHOSDeviceExtensionVK ext) const;

  bool HasExtension(OptionalDeviceExtensionVK ext) const;

  std::optional<std::vector<std::string>> GetEnabledLayers() const;

  std::optional<std::vector<std::string>> GetEnabledInstanceExtensions() const;

  std::optional<std::vector<std::string>> GetEnabledDeviceExtensions(
      const vk::PhysicalDevice& physical_device) const;

  using PhysicalDeviceFeatures =
      vk::StructureChain<vk::PhysicalDeviceFeatures2,
                         vk::PhysicalDeviceSamplerYcbcrConversionFeaturesKHR>;

  std::optional<PhysicalDeviceFeatures> GetEnabledDeviceFeatures(
      const vk::PhysicalDevice& physical_device) const;

  [[nodiscard]] bool SetPhysicalDevice(
      const vk::PhysicalDevice& physical_device);

  const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const;

  void SetOffscreenFormat(PixelFormat pixel_format) const;

  // |Capabilities|
  bool SupportsOffscreenMSAA() const override;

  // |Capabilities|
  bool SupportsImplicitResolvingMSAA() const override;

  // |Capabilities|
  bool SupportsSSBO() const override;

  // |Capabilities|
  bool SupportsBufferToTextureBlits() const override;

  // |Capabilities|
  bool SupportsTextureToTextureBlits() const override;

  // |Capabilities|
  bool SupportsFramebufferFetch() const override;

  // |Capabilities|
  bool SupportsCompute() const override;

  // |Capabilities|
  bool SupportsComputeSubgroups() const override;

  // |Capabilities|
  bool SupportsReadFromResolve() const override;

  // |Capabilities|
  bool SupportsDecalSamplerAddressMode() const override;

  // |Capabilities|
  bool SupportsDeviceTransientTextures() const override;

  // |Capabilities|
  PixelFormat GetDefaultColorFormat() const override;

  // |Capabilities|
  PixelFormat GetDefaultStencilFormat() const override;

  // |Capabilities|
  PixelFormat GetDefaultDepthStencilFormat() const override;

  // |Capabilities|
  PixelFormat GetDefaultGlyphAtlasFormat() const override;

 private:
  bool validations_enabled_ = false;
  std::map<std::string, std::set<std::string>> exts_;
  std::set<RequiredCommonDeviceExtensionVK> required_common_device_extensions_;
  std::set<RequiredAndroidDeviceExtensionVK>
      required_android_device_extensions_;
  std::set<RequiredOHOSDeviceExtensionVK> required_ohos_device_extensions_;
  std::set<OptionalDeviceExtensionVK> optional_device_extensions_;
#ifdef OHOS_PLATFORM
  // This format is set during swapchain initialization and is used for creating
  // offscreen textures. On OHOS, offscreen textures are created before the
  // swapchain is initialized due to pipeline preloading. In such cases, the
  // texture format is undefined, violating Vulkan specifications. To prevent
  // this, a default value is assigned.
  mutable PixelFormat default_color_format_ = PixelFormat::kR8G8B8A8UNormInt;
#else
  mutable PixelFormat default_color_format_ = PixelFormat::kUnknown;
#endif
  PixelFormat default_stencil_format_ = PixelFormat::kUnknown;
  PixelFormat default_depth_stencil_format_ = PixelFormat::kUnknown;
  vk::PhysicalDeviceProperties device_properties_;
  bool supports_compute_subgroups_ = false;
  bool supports_device_transient_textures_ = false;
  bool is_valid_ = false;

  bool HasExtension(const std::string& ext) const;

  bool HasLayer(const std::string& layer) const;

  CapabilitiesVK(const CapabilitiesVK&) = delete;

  CapabilitiesVK& operator=(const CapabilitiesVK&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_CAPABILITIES_VK_H_
