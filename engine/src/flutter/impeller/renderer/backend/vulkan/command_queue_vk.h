// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_COMMAND_QUEUE_VK_H_
#define FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_COMMAND_QUEUE_VK_H_

#include <cstdint>
#include <vector>
#include "impeller/renderer/command_queue.h"

namespace impeller {

class ContextVK;

namespace vk {
class Semaphore;
}  // namespace vk

class CommandQueueVK : public CommandQueue {
 public:
  explicit CommandQueueVK(const std::weak_ptr<ContextVK>& context);

  ~CommandQueueVK() override;

  fml::Status Submit(
      const std::vector<std::shared_ptr<CommandBuffer>>& buffers,
      const CompletionCallback& completion_callback = {}) override;

  void AddNextSemaphores(
      vk::Semaphore& wait_semaphore,
      vk::Semaphore& signal_semaphore,
      const CompletionCallback& completion_callback = {},
      const CompletionCallback& submit_callback = {}) override;

 private:
  std::weak_ptr<ContextVK> context_;
  std::vector<vk::Semaphore> next_signal_semaphores_;
  std::vector<vk::Semaphore> next_wait_semaphores_;
  std::vector<CompletionCallback> next_semaphore_completion_callbacks_;
  std::vector<CompletionCallback> next_semaphore_submit_callbacks_;

  CommandQueueVK(const CommandQueueVK&) = delete;

  CommandQueueVK& operator=(const CommandQueueVK&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_RENDERER_BACKEND_VULKAN_COMMAND_QUEUE_VK_H_
