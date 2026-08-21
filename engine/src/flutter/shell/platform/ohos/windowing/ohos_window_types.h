/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TYPES_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TYPES_H_

#include <cstdint>

#include "shell/platform/common/windowing.h"

namespace flutter {

// Wire structs for the Dart FFI windowing contract. The Dart bindings in
// `_window_ohos.dart` must line up field-for-field with these structs.

struct FlutterWindowSize {
  double width;
  double height;
};

struct FlutterWindowRect {
  double left;
  double top;
  double width;
  double height;
};

struct FlutterWindowConstraints {
  double min_width;
  double min_height;
  double max_width;
  double max_height;
};

struct FlutterWindowCreationRequest {
  bool has_size;
  struct FlutterWindowSize size;
  bool has_constraints;
  struct FlutterWindowConstraints constraints;
  // Authoritative "has a parent" discriminator (modal dialog / tooltip /
  // popup). parent_view_id == 0 is a LEGITIMATE parent (view 0 is the adopted
  // main window), so the id alone can NOT mean "no parent".
  bool has_parent;
  int64_t parent_view_id;
  void (*on_should_close)();
  void (*on_will_close)();
  void (*notify_listeners)();
  // Positioner for sized-to-content windows; coordinates are logical px.
  FlutterWindowRect* (*on_get_window_position)(
      const FlutterWindowSize& child_size,
      const FlutterWindowRect& parent_rect,
      const FlutterWindowRect& output_rect);
};

// The kind of OHOS host window: kUiAbility (task-center card; Regular and
// *modeless* dialogs) vs kSubWindow (windowStage.createSubWindow; modal
// dialogs, tooltips and popups).
enum class WindowHostKind {
  kUiAbility,
  kSubWindow,
};

// Semantic window type from the Dart API; with has_parent it resolves the
// [WindowHostKind]. Values are initialized from the shared WindowArchetype
// (cannot drift) and forwarded to ETS as the `archetype` int (0=Regular,
// 1=Dialog, 2=Tooltip, 3=Popup) — NEVER renumber.
enum class WindowType {
  kRegular = static_cast<int>(WindowArchetype::kRegular),
  kDialog = static_cast<int>(WindowArchetype::kDialog),
  kTooltip = static_cast<int>(WindowArchetype::kTooltip),
  kPopup = static_cast<int>(WindowArchetype::kPopup),
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TYPES_H_
