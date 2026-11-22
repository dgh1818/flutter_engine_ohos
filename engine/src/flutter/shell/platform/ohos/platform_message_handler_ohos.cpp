/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "flutter/shell/platform/ohos/platform_message_handler_ohos.h"
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <unordered_set>
#include "flutter/fml/make_copyable.h"
#include "rapidjson/document.h"

namespace flutter {
namespace {

constexpr char kInspectorSidecarChannel[] =
    "flutter/ohos_inspector_sidecar_experiment";
constexpr size_t kInspectorSidecarMaxBytes = 1024 * 1024;
constexpr size_t kInspectorSidecarMaxNodes = 10000;

bool HasExactMembers(const rapidjson::Value& object,
                     std::initializer_list<const char*> names) {
  if (!object.IsObject() || object.MemberCount() != names.size()) {
    return false;
  }
  for (const char* name : names) {
    size_t count = 0;
    for (auto member = object.MemberBegin(); member != object.MemberEnd();
         ++member) {
      if (member->name.GetStringLength() == std::strlen(name) &&
          std::memcmp(member->name.GetString(), name, std::strlen(name)) == 0) {
        ++count;
      }
    }
    if (count != 1) {
      return false;
    }
  }
  return true;
}

bool ParseInspectorSidecarNode(const rapidjson::Value& node,
                               std::unordered_set<int32_t>& ids,
                               InspectorSidecarNode* parsed) {
  if (!HasExactMembers(node, {"id", "fontSize", "backgroundColor"}) ||
      !node["id"].IsInt() || node["id"].GetInt() < 0 ||
      !ids.insert(node["id"].GetInt()).second) {
    return false;
  }
  const auto& font_size = node["fontSize"];
  const auto& background_color = node["backgroundColor"];
  if ((!font_size.IsNull() &&
       (!font_size.IsNumber() || !std::isfinite(font_size.GetDouble()) ||
        font_size.GetDouble() < 0)) ||
      (!background_color.IsNull() && !background_color.IsUint())) {
    return false;
  }
  *parsed = {node["id"].GetInt(),
             font_size.IsNull() ? std::nan("") : font_size.GetDouble(),
             background_color.IsNull()
                 ? -1
                 : static_cast<int64_t>(background_color.GetUint())};
  return true;
}

}  // namespace

bool ParseInspectorSidecar(const char* data,
                           size_t size,
                           InspectorSidecarUpdate* update) {
  if (!update || !data || size == 0 || size > kInspectorSidecarMaxBytes ||
      std::memchr(data, '\0', size) != nullptr) {
    return false;
  }
  rapidjson::Document document;
  document.Parse<rapidjson::kParseValidateEncodingFlag |
                 rapidjson::kParseIterativeFlag>(data, size);
  if (document.HasParseError() ||
      !HasExactMembers(document,
                       {"version", "viewId", "epoch", "sequence", "nodes"}) ||
      !document["version"].IsInt() || document["version"].GetInt() != 1 ||
      !document["viewId"].IsInt64() || document["viewId"].GetInt64() != 0 ||
      !document["epoch"].IsUint64() || !document["sequence"].IsUint64() ||
      !document["nodes"].IsArray() ||
      document["nodes"].Size() > kInspectorSidecarMaxNodes) {
    return false;
  }
  InspectorSidecarUpdate parsed{
      document["epoch"].GetUint64(), document["sequence"].GetUint64(), {}};
  parsed.nodes.reserve(document["nodes"].Size());
  std::unordered_set<int32_t> ids;
  for (const auto& node : document["nodes"].GetArray()) {
    InspectorSidecarNode parsed_node;
    if (!ParseInspectorSidecarNode(node, ids, &parsed_node)) {
      return false;
    }
    parsed.nodes.push_back(parsed_node);
  }
  *update = std::move(parsed);
  return true;
}

bool ValidateInspectorSidecar(const char* data, size_t size) {
  InspectorSidecarUpdate update;
  return ParseInspectorSidecar(data, size, &update);
}

PlatformMessageHandlerOHOS::PlatformMessageHandlerOHOS(
    const std::shared_ptr<PlatformViewOHOSNapi>& napi_facede,
    fml::RefPtr<fml::TaskRunner> platform_task_runner)
    : napi_facade_(napi_facede),
      platform_task_runner_(std::move(platform_task_runner)) {}

void PlatformMessageHandlerOHOS::SetSemanticsBridge(
    std::weak_ptr<SemanticsBridge> bridge,
    std::shared_ptr<std::mutex> mutex) {
  std::lock_guard lock(bridge_config_mutex_);
  bridge_ = std::move(bridge);
  bridge_mutex_ = std::move(mutex);
}

void PlatformMessageHandlerOHOS::OnPreEngineRestart() {
  std::weak_ptr<SemanticsBridge> bridge;
  std::shared_ptr<std::mutex> mutex;
  {
    std::lock_guard lock(bridge_config_mutex_);
    ++(*sidecar_generation_);
    bridge = bridge_;
    mutex = bridge_mutex_;
  }
  if (mutex) {
    std::lock_guard lock(*mutex);
    if (auto live_bridge = bridge.lock()) {
      live_bridge->ResetInspectorSidecar();
    }
  }
}

void PlatformMessageHandlerOHOS::HandleInspectorSidecar(
    PlatformMessage& message) {
  const uint64_t generation = sidecar_generation_->load();
  const auto& data = message.data();
  InspectorSidecarUpdate update;
  if (ParseInspectorSidecar(reinterpret_cast<const char*>(data.GetMapping()),
                            data.GetSize(), &update)) {
    std::weak_ptr<SemanticsBridge> bridge;
    std::shared_ptr<std::mutex> mutex;
    {
      std::lock_guard lock(bridge_config_mutex_);
      bridge = bridge_;
      mutex = bridge_mutex_;
    }
    platform_task_runner_->PostTask([bridge, mutex, generation,
                                     current = sidecar_generation_,
                                     update = std::move(update)]() {
      if (!mutex) {
        return;
      }
      std::lock_guard lock(*mutex);
      if (generation != current->load()) {
        return;
      }
      if (auto live_bridge = bridge.lock()) {
        live_bridge->ApplyInspectorSidecar(update);
      }
    });
  } else {
    FML_DLOG(WARNING) << "Rejected invalid OHOS inspector sidecar";
  }
  if (auto response = message.response()) {
    response->CompleteEmpty();
  }
}

void PlatformMessageHandlerOHOS::HandlePlatformMessage(
    std::unique_ptr<flutter::PlatformMessage> message) {
  if (message->channel() == kInspectorSidecarChannel) {
    HandleInspectorSidecar(*message);
    return;
  }
  int response_id = next_response_id_.fetch_add(1);
  if (auto response = message->response()) {
    std::lock_guard lock(pending_responses_mutex_);
    pending_responses_[response_id] = response;
  }

  platform_task_runner_->PostTask(
      fml::MakeCopyable([response_id = response_id, napi_facede = napi_facade_,
                         message = std::move(message)]() mutable {
        napi_facede->FlutterViewHandlePlatformMessage(response_id,
                                                      std::move(message));
      }));
}

void PlatformMessageHandlerOHOS::InvokePlatformMessageResponseCallback(
    int response_id,
    std::unique_ptr<fml::Mapping> mapping) {
  if (!response_id) {
    return;
  }
  // TODO(gaaclarke): Move the jump to the ui thread here from
  // PlatformMessageResponseDart so we won't need to use a mutex anymore.
  fml::RefPtr<flutter::PlatformMessageResponse> message_response;
  {
    std::lock_guard lock(pending_responses_mutex_);
    auto it = pending_responses_.find(response_id);
    if (it == pending_responses_.end()) {
      return;
    }
    message_response = std::move(it->second);
    pending_responses_.erase(it);
  }

  message_response->Complete(std::move(mapping));
}

void PlatformMessageHandlerOHOS::InvokePlatformMessageEmptyResponseCallback(
    int response_id) {
  if (!response_id) {
    return;
  }
  fml::RefPtr<flutter::PlatformMessageResponse> message_response;
  {
    std::lock_guard lock(pending_responses_mutex_);
    auto it = pending_responses_.find(response_id);
    if (it == pending_responses_.end()) {
      return;
    }
    message_response = std::move(it->second);
    pending_responses_.erase(it);
  }
  message_response->CompleteEmpty();
}

}  // namespace flutter