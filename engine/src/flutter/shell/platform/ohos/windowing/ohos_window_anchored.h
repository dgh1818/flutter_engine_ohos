/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_ANCHORED_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_ANCHORED_H_

#include "flutter/shell/platform/ohos/windowing/ohos_window.h"

namespace flutter {

/// Positioner-anchored sub-window (tooltip / popup), the OHOS analogue of
/// win32's HostWindowTooltip. The window keeps the size it was born with
/// (explicit request size, else the min constraint — see
/// OHOSWindow::GetSubWindowBirthSize) and is placed by the Dart positioner;
/// the ETS host (AnchoredWindowHost) anchors it to its trigger widget.
class OHOSWindowAnchored : public OHOSWindow {
 public:
  using OHOSWindow::OHOSWindow;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_ANCHORED_H_
