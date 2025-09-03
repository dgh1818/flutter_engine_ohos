// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_SURFACE_CONTEXT_VK_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_SURFACE_CONTEXT_VK_H_

#include <memory>

#include "impeller/base/backend_cast.h"
#include "impeller/core/runtime_types.h"
#include "impeller/renderer/backend/vulkan/vk.h"
#include "impeller/renderer/command_queue.h"
#include "impeller/renderer/context.h"

#ifdef FML_OS_OHOS
#include <native_window/external_window.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_ohos.h>
#define VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS 1000451000
#endif

namespace impeller {

class ContextVK;
class Surface;
class SwapchainVK;

/// For Vulkan, there is both a ContextVK that implements Context and a
/// SurfaceContextVK that also implements Context and takes a ContextVK as its
/// parent. There is a one to many relationship between ContextVK and
/// SurfaceContextVK.
///
/// Most operations in this class are delegated to the parent ContextVK.
/// This class specifically manages swapchains and creation of VkSurfaces on
/// Android. By maintaining the swapchain this way, it is possible to have
/// multiple surfaces sharing the same ContextVK without stepping on each
/// other's swapchains.
class SurfaceContextVK : public Context,
                         public BackendCast<SurfaceContextVK, Context> {
 public:
  explicit SurfaceContextVK(const std::shared_ptr<ContextVK>& parent);

  // |Context|
  ~SurfaceContextVK() override;

  // |Context|
  BackendType GetBackendType() const override;

  // |Context|
  std::string DescribeGpuModel() const override;

  // |Context|
  bool IsValid() const override;

  // |Context|
  std::shared_ptr<Allocator> GetResourceAllocator() const override;

  // |Context|
  std::shared_ptr<ShaderLibrary> GetShaderLibrary() const override;

  // |Context|
  std::shared_ptr<SamplerLibrary> GetSamplerLibrary() const override;

  // |Context|
  std::shared_ptr<PipelineLibrary> GetPipelineLibrary() const override;

  // |Context|
  std::shared_ptr<CommandBuffer> CreateCommandBuffer() const override;

  // |Context|
  const std::shared_ptr<const Capabilities>& GetCapabilities() const override;

  // |Context|
  std::shared_ptr<CommandQueue> GetCommandQueue() const override;

  // |Context|
  std::shared_ptr<const IdleWaiter> GetIdleWaiter() const override;

  // |Context|
  RuntimeStageBackend GetRuntimeStageBackend() const override;

  // |Context|
  bool SubmitOnscreen(std::shared_ptr<CommandBuffer> cmd_buffer) override;

  // |Context|
  void Shutdown() override;

  [[nodiscard]] bool SetWindowSurface(vk::UniqueSurfaceKHR surface,
                                      const ISize& size);

  void ClearSwapchain();

  [[nodiscard]] bool SetSwapchain(std::shared_ptr<SwapchainVK> swapchain);

  std::unique_ptr<Surface> AcquireNextSurface();

  int GetCurrentImageIndex();

  void SetRenderArea(std::optional<IRect> area);

  /// @brief Performs frame incrementing processes like AcquireNextSurface but
  ///        without the surface.
  ///
  /// Used by the embedder.h implementations.
  void MarkFrameEnd();

  /// @brief Mark the current swapchain configuration as dirty, forcing it to be
  ///        recreated on the next frame.
  void UpdateSurfaceSize(const ISize& size) const;

  /// @brief Can be called when the surface is destroyed to reduce memory usage.
  void TeardownSwapchain();

  // |Context|
  void InitializeCommonlyUsedShadersIfNeeded() const override;

#ifdef FML_OS_OHOS
  vk::UniqueSurfaceKHR CreateOHOSSurface(OHNativeWindow* window) const;
#endif  // FML_OS_OHOS

  // |Context|
  void DisposeThreadLocalCachedResources() override;

  const vk::Device& GetDevice() const;

  const std::shared_ptr<ContextVK>& GetParent() const;

  bool EnqueueCommandBuffer(
      std::shared_ptr<CommandBuffer> command_buffer) override;

  bool FlushCommandBuffers() override;

  bool GetAndResetChangedFlag() const {
    bool ret = swapchain_changed_;
    swapchain_changed_ = false;
    return ret;
  }

 private:
  std::shared_ptr<ContextVK> parent_;
  std::shared_ptr<KHRSwapchainVK> swapchain_;
  mutable bool swapchain_changed_ = true;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_SURFACE_CONTEXT_VK_H_
