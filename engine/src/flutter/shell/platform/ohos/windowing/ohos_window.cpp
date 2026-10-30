/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/windowing/ohos_window.h"

#include <cstdlib>
#include <utility>

namespace flutter {

OHOSWindow::OHOSWindow(int64_t view_id) : view_id_(view_id) {}

// Regular: preferred size + title + callbacks, never a positioner.
OHOSWindow::OHOSWindow(int64_t view_id,
                       const RegularWindowCreationRequest& request)
    : OHOSWindow(view_id) {
  callbacks_ = request.callbacks;
  has_preferred_size_ = request.preferred_size.has_preferred_view_size;
  preferred_size_ = {request.preferred_size.preferred_view_width,
                     request.preferred_size.preferred_view_height};
  // Seeded synchronously inside CreateWindow, while the request's Dart-side
  // `title` pointer is still alive.
  title_cache_ = request.title != nullptr ? request.title : "";
}

// Dialog: same seeds as Regular (no positioner; parentage is the ETS host's
// concern).
OHOSWindow::OHOSWindow(int64_t view_id,
                       const DialogWindowCreationRequest& request)
    : OHOSWindow(view_id) {
  callbacks_ = request.callbacks;
  has_preferred_size_ = request.preferred_size.has_preferred_view_size;
  preferred_size_ = {request.preferred_size.preferred_view_width,
                     request.preferred_size.preferred_view_height};
  title_cache_ = request.title != nullptr ? request.title : "";
}

// Tooltip: positioner + callbacks; no preferred size, no title. The only
// sized-to-content archetype (win32 HostWindowTooltip parity): the view's
// content lays out within the request's constraint RANGE and the host
// window follows the per-frame content size.
OHOSWindow::OHOSWindow(int64_t view_id,
                       const TooltipWindowCreationRequest& request)
    : OHOSWindow(view_id) {
  callbacks_ = request.callbacks;
  on_get_window_position_ = request.get_position_callback;
  sized_to_content_ = true;
  has_constraints_ = request.preferred_constraints.has_view_constraints;
  constraints_ = request.preferred_constraints;
}

OHOSWindow::OHOSWindow(int64_t view_id,
                       const PopupWindowCreationRequest& request)
    : OHOSWindow(view_id) {
  callbacks_ = request.callbacks;
  on_get_window_position_ = request.get_position_callback;
  sized_to_content_ = true;
  has_constraints_ = request.preferred_constraints.has_view_constraints;
  constraints_ = request.preferred_constraints;
}

OHOSWindow::~OHOSWindow() = default;

void OHOSWindow::FireShouldClose() {
  if (callbacks_.on_should_close) {
    callbacks_.on_should_close();
  }
}

void OHOSWindow::FireWillClose() {
  if (callbacks_.on_will_close) {
    callbacks_.on_will_close();
  }
}

void OHOSWindow::FireNotifyListeners() {
  if (callbacks_.notify_listeners) {
    callbacks_.notify_listeners();
  }
}

bool OHOSWindow::GetConstraints(WindowConstraints* out) const {
  if (!sized_to_content_ || !has_constraints_ || out == nullptr) {
    return false;
  }
  *out = constraints_;
  return true;
}

bool OHOSWindow::ComputeWindowPosition(const WindowSize& child_size,
                                       const WindowRect& parent_rect,
                                       const WindowRect& work_area,
                                       WindowRect* out) {
  if (out == nullptr) {
    return false;
  }
  const auto callback = on_get_window_position_;
  if (!callback) {
    return false;
  }
  WindowRect* result = callback(child_size, parent_rect, work_area);
  if (result == nullptr) {
    return false;
  }
  *out = *result;
  free(result);
  return true;
}

}  // namespace flutter
