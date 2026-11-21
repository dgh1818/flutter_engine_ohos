/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_inspector.h"

#include <dlfcn.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <unordered_map>

#include "flutter/fml/logging.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_bridge.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_tree.h"

namespace flutter {
namespace {

constexpr int64_t kInspectorBackgroundColorUnset = -1;
constexpr size_t kInspectorInitialJsonCapacity = 16 * 1024;
constexpr size_t kInspectorMaxRetainedContexts = 1024;
constexpr size_t kInspectorMaxNodes = 100000;
constexpr int32_t kInspectorMaxDepth = 512;
constexpr int kInspectorRectDecimalPlaces = 2;
// ArkUI rejects inspector subtree payloads larger than 4 MiB
// (INSPECTOR_EXTERNAL_SUBTREE_MAX_BYTES, arkui_ace_engine
// frameworks/core/components_ng/base/inspector_external_subtree.cpp); the
// budget keeps a margin so the finalized array stays under the limit.
constexpr size_t kInspectorMaxJsonBytes = 4194304;
constexpr size_t kInspectorJsonByteBudget = kInspectorMaxJsonBytes - 4096;

void EscapeJsonString(const std::string& input, std::string& out) {
  for (const char raw : input) {
    const unsigned char c = static_cast<unsigned char>(raw);
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c < 0x20) {
          std::format_to(std::back_inserter(out), "\\u{:04x}",
                         static_cast<unsigned int>(c));
        } else {
          out += raw;
        }
        break;
    }
  }
}

// Rect format matches ArkUI RectF::ToBounds() and the requirement sample
// ("[left, top],[right,bottom]" — space after the first comma only).
void AppendRect(const SkRect& bounds, std::string& out) {
  std::ostringstream rect;
  rect.imbue(std::locale::classic());
  rect << std::fixed << std::setprecision(kInspectorRectDecimalPlaces)
       << "\"$rect\": \"[" << bounds.fLeft << ", " << bounds.fTop << "],["
       << bounds.fRight << "," << bounds.fBottom << "]\"";
  out += rect.str();
}

void AppendAttrs(SemanticsNodeExtend& node, std::string& out) {
  out += "\"$attrs\": {";
  bool first = true;
  const auto comma = [&first, &out]() {
    if (!first) {
      out += ",";
    }
    first = false;
  };

  const std::string text = node.GetAccessibilityText();
  if (!text.empty()) {
    comma();
    out += "\"text\": \"";
    EscapeJsonString(text, out);
    out += "\"";
  }
  comma();
  out += "\"clickable\": ";
  out += node.IsClickable() ? "true" : "false";
  comma();
  out += "\"scrollable\": ";
  out += node.IsScrollable() ? "true" : "false";
  if (!std::isnan(node.fontSize)) {
    comma();
    std::ostringstream font_size;
    font_size.imbue(std::locale::classic());
    font_size << "\"fontSize\": " << std::fixed << std::setprecision(1)
              << node.fontSize;
    out += font_size.str();
  }
  if (node.backgroundColor != kInspectorBackgroundColorUnset) {
    comma();
    out += "\"backgroundColor\": \"#";
    std::format_to(std::back_inserter(out), "{:08X}",
                   static_cast<uint32_t>(node.backgroundColor));
    out += "\"";
  }
  if (!node.identifier.empty()) {
    comma();
    out += "\"identifier\": \"";
    EscapeJsonString(node.identifier, out);
    out += "\"";
  }
  out += "}";
}

// Save the original bounds and their spans in the immutable JSON so the
// getter can translate only the rects without reading the live semantics tree.
struct InspectorRectSpan {
  size_t start;
  size_t end;
  SkRect bounds;
};

struct SerializeState {
  std::vector<InspectorRectSpan>* rects;
  size_t node_count = 0;
  bool truncated = false;
};

void SerializeNode(const InspectorTreeSource& source,
                   SemanticsNodeExtend& node,
                   int32_t depth,
                   SerializeState& state,
                   std::string& out) {
  if (state.node_count >= kInspectorMaxNodes || depth > kInspectorMaxDepth ||
      out.size() >= kInspectorJsonByteBudget) {
    state.truncated = true;
    return;
  }
  ++state.node_count;

  const size_t node_start = out.size();
  out += "{\"$type\": \"";
  EscapeJsonString(std::string(node.componentType), out);
  out += "\",\"$ID\": ";
  out += std::to_string(node.id);
  out += ",\"type\": \"build-in\",";
  const size_t rect_start = out.size();
  AppendRect(node.absoluteRect, out);
  const size_t rect_end = out.size();
  out += ",\"$debugLine\": \"\",";
  AppendAttrs(node, out);
  if (out.size() > kInspectorJsonByteBudget) {
    out.resize(node_start);
    state.truncated = true;
    return;
  }
  if (state.rects) {
    state.rects->push_back({rect_start, rect_end, node.absoluteRect});
  }
  out += ",\"$children\": [";
  const char* separator = "";
  for (const int32_t child_id : source.ChildIdsInTraversalOrder(node)) {
    // Self-references are skipped; longer cycles are bounded by the caps.
    if (child_id == node.id) {
      continue;
    }
    SemanticsNodeExtend* child = nullptr;
    if (!source.NodeAt(child_id, &child) || child == nullptr) {
      continue;
    }
    // A truncated child emits nothing: roll the pending separator back so
    // the array keeps valid JSON (no empty slots, no trailing comma).
    const size_t before = out.size();
    out += separator;
    SerializeNode(source, *child, depth + 1, state, out);
    if (out.size() == before + (separator[0] != '\0' ? 1 : 0)) {
      out.resize(before);
      continue;
    }
    separator = ",";
  }
  out += "]}";
}

std::string BuildInspectorTreeJson(const InspectorTreeSource& source,
                                   std::vector<InspectorRectSpan>* rects) {
  std::string out;
  out.reserve(kInspectorInitialJsonCapacity);
  SerializeState state{rects};
  SemanticsNodeExtend* root = nullptr;
  // ArkUI contract: the payload must be a JSON array of inspector nodes;
  // single-object payloads are rejected and skipped whole.
  out += "[";
  if (source.RootId() >= 0 && source.NodeAt(source.RootId(), &root) &&
      root != nullptr) {
    SerializeNode(source, *root, 0, state, out);
  }
  if (state.truncated) {
    if (!out.empty() && out.back() == '}') {
      out.pop_back();
      out += ",\"truncated\":true}";
    }
  }
  out += "]";
  return out;
}

}  // namespace

std::string BuildInspectorTreeJson(const InspectorTreeSource& source) {
  return BuildInspectorTreeJson(source, nullptr);
}

// ArkUI InspectorTreeGetter integration (interface_sdk_c !6571, since 26.1.0).
// Registration associates a node and user context with the getter callback;
// unregistration uses the same node and callback. The returned JSON must be
// malloc()-allocated because ArkUI releases it with free().

int32_t SemanticsTreeInspectorSource::RootId() const {
  if (tree_ == nullptr) {
    return -1;
  }
  SemanticsNodeExtend* root = tree_->GetRootNode();
  if (root == nullptr) {
    // Flutter semantics convention: the root node id is 0.
    root = tree_->FindNodeById(0);
  }
  return root == nullptr ? -1 : root->id;
}

bool SemanticsTreeInspectorSource::NodeAt(int32_t id,
                                          SemanticsNodeExtend** node) const {
  if (tree_ == nullptr || node == nullptr) {
    return false;
  }
  *node = tree_->FindNodeById(id);
  return *node != nullptr;
}

std::vector<int32_t> SemanticsTreeInspectorSource::ChildIdsInTraversalOrder(
    SemanticsNodeExtend& node) const {
  std::vector<int32_t> ids;
  ids.reserve(node.childrenInTraversalOrderList.size());
  for (const SemanticsNodeExtend* child : node.childrenInTraversalOrderList) {
    if (child != nullptr) {
      ids.push_back(child->id);
    }
  }
  return ids;
}

namespace {

using RegisterInspectorTreeGetterFn = int32_t (*)(ArkUI_NodeHandle,
                                                  void*,
                                                  InspectorTreeGetterFn);
using UnregisterInspectorTreeGetterFn = int32_t (*)(ArkUI_NodeHandle,
                                                    InspectorTreeGetterFn);

struct InspectorApi {
  void* library = nullptr;
  RegisterInspectorTreeGetterFn register_getter = nullptr;
  UnregisterInspectorTreeGetterFn unregister_getter = nullptr;
};

const InspectorApi& LoadInspectorApi() {
  static const InspectorApi* api = [] {
    auto* result = new InspectorApi();
    result->library = dlopen("libace_ndk.z.so", RTLD_NOW | RTLD_LOCAL);
    if (result->library) {
      result->register_getter = reinterpret_cast<RegisterInspectorTreeGetterFn>(
          dlsym(result->library,
                "OH_ArkUI_NativeModule_RegisterInspectorTreeGetter"));
      result->unregister_getter =
          reinterpret_cast<UnregisterInspectorTreeGetterFn>(
              dlsym(result->library,
                    "OH_ArkUI_NativeModule_UnregisterInspectorTreeGetter"));
      if (!result->register_getter || !result->unregister_getter) {
        dlclose(result->library);
        *result = {};
      }
    }
    if (!result->register_getter) {
      FML_LOG(INFO) << "ArkUI Inspector getter API unavailable";
    }
    return result;
  }();
  return *api;
}

struct InspectorRegistrations {
  std::mutex mutex;
  std::unordered_map<ArkUI_NodeHandle,
                     std::shared_ptr<InspectorCallbackContext>>
      nodes;
  std::vector<std::shared_ptr<InspectorCallbackContext>> contexts;
};

InspectorRegistrations& Registrations() {
  static auto* registrations = new InspectorRegistrations();
  return *registrations;
}

bool PinCallbackLibrary() {
  static bool pinned = [] {
    Dl_info info = {};
    return dladdr(reinterpret_cast<void*>(&InspectorTreeGetterTrampoline),
                  &info) != 0 &&
           info.dli_fname &&
           dlopen(info.dli_fname, RTLD_NOW | RTLD_LOCAL) != nullptr;
  }();
  return pinned;
}

}  // namespace

struct InspectorSnapshot::Data {
  std::string json;
  std::vector<InspectorRectSpan> rects;
};

InspectorSnapshot::InspectorSnapshot()
    : data_(std::make_shared<const Data>(Data{"[]", {}})) {}

void InspectorSnapshot::Publish(std::string json) {
  std::atomic_store(&data_,
                    std::make_shared<const Data>(Data{std::move(json), {}}));
}

void InspectorSnapshot::Publish(const InspectorTreeSource& source) {
  Data data;
  data.json = BuildInspectorTreeJson(source, &data.rects);
  std::atomic_store(&data_, std::make_shared<const Data>(std::move(data)));
}

std::shared_ptr<const std::string> InspectorSnapshot::Read(
    ArkUI_IntOffset xcomponent_offset) const {
  auto data = std::atomic_load(&data_);
  const std::shared_ptr<const std::string> original(data, &data->json);
  if ((xcomponent_offset.x == 0 && xcomponent_offset.y == 0) ||
      data->rects.empty()) {
    return original;
  }

  std::string json;
  json.reserve(data->json.size());
  size_t cursor = 0;
  for (const auto& rect : data->rects) {
    json.append(data->json, cursor, rect.start - cursor);
    AppendRect(rect.bounds.makeOffset(xcomponent_offset.x, xcomponent_offset.y),
               json);
    cursor = rect.end;
  }
  json.append(data->json, cursor, std::string::npos);
  if (json.size() > kInspectorMaxJsonBytes) {
    return original;
  }
  return std::make_shared<const std::string>(std::move(json));
}

InspectorCallbackContext::InspectorCallbackContext(
    std::shared_ptr<InspectorSnapshot> snapshot,
    ArkUI_NodeHandle node)
    : snapshot_(std::move(snapshot)),
      node_(node),
      registration_thread_(std::this_thread::get_id()) {}

std::shared_ptr<InspectorSnapshot> InspectorCallbackContext::Read() const {
  return std::atomic_load(&snapshot_);
}

ArkUI_IntOffset InspectorCallbackContext::ReadXComponentOffset() const {
  // The NAPI binding holds the FrameNode until unregistration on this thread.
  // Do not access the ArkUI node if a getter is called from another thread.
  if (!node_ || registration_thread_ != std::this_thread::get_id()) {
    return {};
  }
  ArkUI_IntOffset offset{};
  if (OH_ArkUI_NodeUtils_GetPositionWithTranslateInWindow(node_, &offset) !=
      ARKUI_ERROR_CODE_NO_ERROR) {
    return {};
  }
  return offset;
}

void InspectorCallbackContext::Deactivate() {
  std::atomic_store(&snapshot_, std::shared_ptr<InspectorSnapshot>());
}

void PublishInspectorSnapshot(SemanticsBridge* bridge) {
  if (bridge && bridge->inspector_snapshot_) {
    SemanticsTreeInspectorSource source(&bridge->tree_);
    bridge->inspector_snapshot_->Publish(source);
  }
}

bool InspectorTreeGetterAvailable() {
  const auto& api = LoadInspectorApi();
  return api.register_getter && api.unregister_getter;
}

void InspectorTreeGetterTrampoline(void* userData, char** inspectorTree) {
  if (inspectorTree == nullptr) {
    return;
  }
  *inspectorTree = nullptr;
  auto* context = static_cast<InspectorCallbackContext*>(userData);
  auto snapshot = context ? context->Read() : nullptr;
  auto json =
      snapshot ? snapshot->Read(context->ReadXComponentOffset()) : nullptr;
  *inspectorTree = strdup(json ? json->c_str() : "[]");
  static std::atomic<bool> logged{false};
  if (snapshot && !logged.exchange(true)) {
    FML_LOG(INFO) << "ArkUI Inspector getter invoked";
  }
}

int32_t RegisterInspectorTreeGetterWithApi(
    ArkUI_NodeHandle node,
    std::shared_ptr<InspectorSnapshot> snapshot,
    RegisterInspectorTreeGetterFn register_getter) {
  auto& registrations = Registrations();
  std::shared_ptr<InspectorCallbackContext> context;
  {
    std::lock_guard<std::mutex> lock(registrations.mutex);
    // Successful contexts are retained for late callbacks. Bound memory even
    // if a view is repeatedly reattached.
    if (registrations.nodes.count(node) ||
        registrations.contexts.size() >= kInspectorMaxRetainedContexts) {
      return -1;
    }
    context =
        std::make_shared<InspectorCallbackContext>(std::move(snapshot), node);
    registrations.contexts.push_back(context);
    registrations.nodes[node] = context;
  }
  const int32_t result =
      register_getter(node, context.get(), &InspectorTreeGetterTrampoline);
  if (result != 0) {
    context->Deactivate();
    std::lock_guard<std::mutex> lock(registrations.mutex);
    const auto found = registrations.nodes.find(node);
    if (found != registrations.nodes.end() && found->second == context) {
      registrations.nodes.erase(found);
    }
    // The target ArkUI implementation does not retain userData on failure.
    // Other implementations must satisfy the same contract.
    const auto entry = std::find(registrations.contexts.begin(),
                                 registrations.contexts.end(), context);
    if (entry != registrations.contexts.end()) {
      registrations.contexts.erase(entry);
    }
  }
  FML_LOG(INFO) << "Inspector getter registration result: " << result;
  return result;
}

int32_t RegisterInspectorTreeGetter(
    ArkUI_NodeHandle node,
    std::shared_ptr<InspectorSnapshot> snapshot) {
  const auto& api = LoadInspectorApi();
  if (!api.register_getter || !api.unregister_getter || !node || !snapshot ||
      !PinCallbackLibrary()) {
    return -1;
  }
  return RegisterInspectorTreeGetterWithApi(node, std::move(snapshot),
                                            api.register_getter);
}

size_t InspectorRetainedContextCountForTesting() {
  auto& registrations = Registrations();
  std::lock_guard<std::mutex> lock(registrations.mutex);
  return registrations.contexts.size();
}

int32_t UnregisterInspectorTreeGetter(ArkUI_NodeHandle node) {
  const auto& api = LoadInspectorApi();
  auto& registrations = Registrations();
  std::shared_ptr<InspectorCallbackContext> context;
  {
    std::lock_guard<std::mutex> lock(registrations.mutex);
    const auto found = registrations.nodes.find(node);
    if (!api.unregister_getter || found == registrations.nodes.end()) {
      return -1;
    }
    context = found->second;
  }
  context->Deactivate();
  const int32_t result =
      api.unregister_getter(node, &InspectorTreeGetterTrampoline);
  if (result == 0) {
    std::lock_guard<std::mutex> lock(registrations.mutex);
    const auto found = registrations.nodes.find(node);
    if (found != registrations.nodes.end() && found->second == context) {
      registrations.nodes.erase(found);
    }
  }
  FML_LOG(INFO) << "Inspector getter unregistration result: " << result;
  return result;
}

}  // namespace flutter
