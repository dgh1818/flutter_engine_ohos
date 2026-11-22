/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_MESSAGE_HANDLER_OHOS_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_MESSAGE_HANDLER_OHOS_H_
#include <memory>
#include <mutex>
#include <unordered_map>

#include "flutter/fml/task_runner.h"
#include "flutter/lib/ui/window/platform_message.h"
#include "flutter/shell/common/platform_message_handler.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_bridge.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {

// Validates the experimental sidecar without associating it with a semantics
// batch.
bool ParseInspectorSidecar(const char* data,
                           size_t size,
                           InspectorSidecarUpdate* update);
bool ValidateInspectorSidecar(const char* data, size_t size);

class PlatformMessageHandlerOHOS : public PlatformMessageHandler {
 public:
  explicit PlatformMessageHandlerOHOS(
      const std::shared_ptr<PlatformViewOHOSNapi>& napi_facede,
      fml::RefPtr<fml::TaskRunner> platform_task_runner);
  void HandlePlatformMessage(std::unique_ptr<PlatformMessage> message) override;
  void SetSemanticsBridge(std::weak_ptr<SemanticsBridge> bridge,
                          std::shared_ptr<std::mutex> mutex);
  // Called on the platform thread before the new isolate starts.
  void OnPreEngineRestart();
  bool DoesHandlePlatformMessageOnPlatformThread() const override {
    return true;
  }
  void InvokePlatformMessageResponseCallback(
      int response_id,
      std::unique_ptr<fml::Mapping> mapping) override;

  void InvokePlatformMessageEmptyResponseCallback(int response_id) override;

 private:
  void HandleInspectorSidecar(PlatformMessage& message);

  const std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  const fml::RefPtr<fml::TaskRunner> platform_task_runner_;
  std::atomic<int> next_response_id_ = 1;
  std::unordered_map<int, fml::RefPtr<flutter::PlatformMessageResponse>>
      pending_responses_;
  std::mutex pending_responses_mutex_;
  std::mutex bridge_config_mutex_;
  std::weak_ptr<SemanticsBridge> bridge_;
  std::shared_ptr<std::mutex> bridge_mutex_;
  std::shared_ptr<std::atomic<uint64_t>> sidecar_generation_ =
      std::make_shared<std::atomic<uint64_t>>(0);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_MESSAGE_HANDLER_OHOS_H_