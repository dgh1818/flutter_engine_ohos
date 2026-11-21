/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_INSPECTOR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_INSPECTOR_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <arkui/native_node.h>  // ArkUI_NodeHandle (NDK, API 12+)

#include "flutter/shell/platform/ohos/accessibility/ohos_semantics_node.h"

namespace flutter {

class SemanticsTree;
class SemanticsBridge;

// Read-only view over the extended semantics tree, consumed by the Inspector
// JSON serializer. Deliberately decoupled from ArkUI: while
// OH_ArkUI_InspectorProviderRegisterCallback is not yet available in public
// SDKs, unit tests drive the serializer with mock implementations of this
// interface.
class InspectorTreeSource {
 public:
  virtual ~InspectorTreeSource() = default;

  virtual int32_t RootId() const = 0;

  // Looks a node up by id; returns false when the id is absent.
  virtual bool NodeAt(int32_t id, SemanticsNodeExtend** node) const = 0;

  virtual std::vector<int32_t> ChildIdsInTraversalOrder(
      SemanticsNodeExtend& node) const = 0;
};

// Serializes `source` into an OHOS Inspector JSON array. Each node contains
// $type, $ID, type, $rect, $debugLine, $attrs, and $children. Rect coordinates
// have two decimal places in the format "[left, top],[right,bottom]".
// The attributes always include clickable and scrollable. Font size is in
// logical pixels with one decimal place and omitted when NaN. Background color
// is uppercase #AARRGGBB and omitted for the -1 sentinel.
// Node count, depth, and byte budgets protect against malformed or cyclic
// trees; truncation is marked on a retained root. If the root itself exceeds
// the byte budget, the result is an empty array.
std::string BuildInspectorTreeJson(const InspectorTreeSource& source);

// Adapts the OHOS semantics tree to the serializer's read-only view. Callers
// must synchronize access to the tree with semantics updates.
class SemanticsTreeInspectorSource : public InspectorTreeSource {
 public:
  explicit SemanticsTreeInspectorSource(SemanticsTree* tree) : tree_(tree) {}

  int32_t RootId() const override;
  bool NodeAt(int32_t id, SemanticsNodeExtend** node) const override;
  std::vector<int32_t> ChildIdsInTraversalOrder(
      SemanticsNodeExtend& node) const override;

 private:
  SemanticsTree* tree_;
};

// Callback shape required by ArkUI
// (OH_ArkUI_NativeModule_RegisterInspectorTreeGetter, interface_sdk_c !6571,
// @since 26.1.0).
using InspectorTreeGetterFn = void (*)(void* userData, char** inspectorTree);

class InspectorSnapshot {
 public:
  InspectorSnapshot();
  void Publish(std::string json);
  void Publish(const InspectorTreeSource& source);
  std::shared_ptr<const std::string> Read(
      ArkUI_IntOffset xcomponent_offset = {}) const;

 private:
  struct Data;
  std::shared_ptr<const Data> data_;
};

class InspectorCallbackContext {
 public:
  explicit InspectorCallbackContext(std::shared_ptr<InspectorSnapshot> snapshot,
                                    ArkUI_NodeHandle node = nullptr);
  std::shared_ptr<InspectorSnapshot> Read() const;
  ArkUI_IntOffset ReadXComponentOffset() const;
  void Deactivate();

 private:
  std::shared_ptr<InspectorSnapshot> snapshot_;
  const ArkUI_NodeHandle node_;
  const std::thread::id registration_thread_;
};

// Caller must hold the bridge mutex while creating or publishing a snapshot.
void PublishInspectorSnapshot(SemanticsBridge* bridge);
bool InspectorTreeGetterAvailable();
int32_t RegisterInspectorTreeGetter(
    ArkUI_NodeHandle node,
    std::shared_ptr<InspectorSnapshot> snapshot);
int32_t UnregisterInspectorTreeGetter(ArkUI_NodeHandle node);
void InspectorTreeGetterTrampoline(void* userData, char** inspectorTree);

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_ACCESSIBILITY_OHOS_SEMANTICS_INSPECTOR_H_
