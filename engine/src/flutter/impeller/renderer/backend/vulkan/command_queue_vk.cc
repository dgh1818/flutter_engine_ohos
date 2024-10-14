// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fml/status.h"

#include "impeller/renderer/backend/vulkan/command_queue_vk.h"

#include "impeller/base/validation.h"
#include "impeller/renderer/backend/vulkan/command_buffer_vk.h"
#include "impeller/renderer/backend/vulkan/command_encoder_vk.h"
#include "impeller/renderer/backend/vulkan/context_vk.h"
#include "impeller/renderer/backend/vulkan/fence_waiter_vk.h"
#include "impeller/renderer/backend/vulkan/tracked_objects_vk.h"
#include "impeller/renderer/command_buffer.h"

namespace impeller {

CommandQueueVK::CommandQueueVK(const std::weak_ptr<ContextVK>& context)
    : context_(context) {}

CommandQueueVK::~CommandQueueVK() = default;

fml::Status CommandQueueVK::Submit(
    const std::vector<std::shared_ptr<CommandBuffer>>& buffers,
    const CompletionCallback& completion_callback) {
  if (buffers.empty()) {
    return fml::Status(fml::StatusCode::kInvalidArgument,
                       "No command buffers provided.");
  }
  // Success or failure, you only get to submit once.
  fml::ScopedCleanupClosure reset([&]() {
    if (completion_callback) {
      completion_callback(CommandBuffer::Status::kError);
      for (const auto& callback : next_semaphore_completion_callbacks_) {
        if (callback) {
          callback(CommandBuffer::Status::kError);
        }
      }
      next_semaphore_completion_callbacks_.clear();
      for (const auto& callback : next_semaphore_submit_callbacks_) {
        if (callback) {
          callback(CommandBuffer::Status::kError);
        }
      }
      next_semaphore_submit_callbacks_.clear();
      next_wait_semaphores_.clear();
      next_signal_semaphores_.clear();
    }
  });

  std::vector<vk::CommandBuffer> vk_buffers;
  std::vector<std::shared_ptr<TrackedObjectsVK>> tracked_objects;
  vk_buffers.reserve(buffers.size());
  tracked_objects.reserve(buffers.size());
  for (const std::shared_ptr<CommandBuffer>& buffer : buffers) {
    auto encoder = CommandBufferVK::Cast(*buffer).GetEncoder();
    if (!encoder->EndCommandBuffer()) {
      return fml::Status(fml::StatusCode::kCancelled,
                         "Failed to end command buffer.");
    }
    tracked_objects.push_back(encoder->tracked_objects_);
    vk_buffers.push_back(encoder->GetCommandBuffer());
    encoder->Reset();
  }

  auto context = context_.lock();
  if (!context) {
    VALIDATION_LOG << "Device lost.";
    return fml::Status(fml::StatusCode::kCancelled, "Device lost.");
  }
  auto [fence_result, fence] = context->GetDevice().createFenceUnique({});
  if (fence_result != vk::Result::eSuccess) {
    VALIDATION_LOG << "Failed to create fence: " << vk::to_string(fence_result);
    return fml::Status(fml::StatusCode::kCancelled, "Failed to create fence.");
  }

  vk::SubmitInfo submit_info;
  // Add this to wait and signal semaphores.
  std::vector<vk::PipelineStageFlags> all_wait_stage;
  if (next_wait_semaphores_.size() > 0) {
    submit_info.setWaitSemaphores(next_wait_semaphores_);
    for (size_t i = 0; i < next_wait_semaphores_.size(); i++) {
      all_wait_stage.push_back(vk::PipelineStageFlagBits::eAllCommands);
    }
    submit_info.setWaitDstStageMask(all_wait_stage);
  }
  if (next_signal_semaphores_.size() > 0) {
    submit_info.setSignalSemaphores(next_signal_semaphores_);
  }
  submit_info.setCommandBuffers(vk_buffers);
  auto status = context->GetGraphicsQueue()->Submit(submit_info, *fence);
  next_wait_semaphores_.clear();
  next_signal_semaphores_.clear();
  if (status != vk::Result::eSuccess) {
    VALIDATION_LOG << "Failed to submit queue: " << vk::to_string(status);
    return fml::Status(fml::StatusCode::kCancelled, "Failed to submit queue: ");
  }

  // Submit will proceed, call callback with true when it is done and do not
  // call when `reset` is collected.
  auto added_fence = context->GetFenceWaiter()->AddFence(
      std::move(fence),
      [completion_callback,
       next_callbacks = next_semaphore_completion_callbacks_,
       tracked_objects = std::move(tracked_objects)]() mutable {
        // Ensure tracked objects are destructed before calling any final
        // callbacks.
        tracked_objects.clear();
        if (completion_callback) {
          completion_callback(CommandBuffer::Status::kCompleted);
        }
        for (const auto& callback : next_callbacks) {
          if (callback) {
            callback(CommandBuffer::Status::kCompleted);
          }
        }
      });
  if (!added_fence) {
    return fml::Status(fml::StatusCode::kCancelled, "Failed to add fence.");
  }
  reset.Release();

  // Callback invoked when the task is submitted
  next_semaphore_completion_callbacks_.clear();
  for (const auto& callback : next_semaphore_submit_callbacks_) {
    if (callback) {
      callback(CommandBuffer::Status::kCompleted);
    }
  }
  next_semaphore_submit_callbacks_.clear();
  return fml::Status();
}

void CommandQueueVK::AddNextSemaphores(
    vk::Semaphore& wait_semaphore,
    vk::Semaphore& signal_semaphore,
    const CompletionCallback& completion_callback,
    const CompletionCallback& submit_callback) {
  if (wait_semaphore) {
    next_wait_semaphores_.push_back(wait_semaphore);
  }
  if (signal_semaphore) {
    next_signal_semaphores_.push_back(signal_semaphore);
  }
  if (completion_callback) {
    next_semaphore_completion_callbacks_.push_back(completion_callback);
  }
  if (submit_callback) {
    next_semaphore_submit_callbacks_.push_back(submit_callback);
  }
};

}  // namespace impeller
