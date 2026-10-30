/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TYPES_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TYPES_H_

#include <cstdint>
#include <string>

#include "shell/platform/common/windowing.h"

namespace flutter {

// Window-creation request types, one per archetype.
struct WindowSizeRequest {
  bool has_preferred_view_size = false;
  double preferred_view_width;
  double preferred_view_height;
};

// Min/max content size constraints (LOGICAL px; max 0 = unbounded).
struct WindowConstraints {
  bool has_view_constraints = false;
  double view_min_width;
  double view_min_height;
  double view_max_width;
  double view_max_height;
};

// Guards the Dart FFI mirror `_Constraints` in
// `packages/flutter/lib/src/widgets/_window_ohos.dart` (leading bool + 7
// bytes padding + 4 doubles). Keep both sides in sync.
static_assert(sizeof(WindowConstraints) == 40,
              "Dart FFI struct _Constraints is out of sync");

// Lifecycle notifications back into Dart; each is null-guarded.
struct WindowLifecycleCallbacks {
  void (*on_should_close)();
  void (*on_will_close)();
  void (*notify_listeners)();
};

// Coordinates are in physical pixels.
struct WindowRect {
  double left;
  double top;
  double width;
  double height;
};

// Sizes are in physical pixels.
struct WindowSize {
  double width;
  double height;
};

constexpr int64_t kNoParentViewId = -1;

struct RegularWindowCreationRequest {
  WindowSizeRequest preferred_size;
  WindowConstraints preferred_constraints;
  const char* title;
  WindowLifecycleCallbacks callbacks;
};

struct DialogWindowCreationRequest {
  WindowSizeRequest preferred_size;
  WindowConstraints preferred_constraints;
  const char* title;
  int64_t parent_view_id = kNoParentViewId;
  WindowLifecycleCallbacks callbacks;
};

typedef WindowRect* (*GetWindowPositionCallback)(const WindowSize& child_size,
                                                 const WindowRect& parent_rect,
                                                 const WindowRect& output_rect);

struct TooltipWindowCreationRequest {
  WindowConstraints preferred_constraints;
  int64_t parent_view_id = kNoParentViewId;
  GetWindowPositionCallback get_position_callback;
  WindowLifecycleCallbacks callbacks;
};

struct PopupWindowCreationRequest {
  WindowConstraints preferred_constraints;
  int64_t parent_view_id = kNoParentViewId;
  GetWindowPositionCallback get_position_callback;
  WindowLifecycleCallbacks callbacks;
};

// ---- Internal ----

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
