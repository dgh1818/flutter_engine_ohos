/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_DIALOG_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_DIALOG_H_

#include "flutter/shell/platform/ohos/windowing/ohos_window.h"

namespace flutter {

/// A Dialog window. kUiAbility (no parent) = MODELESS, a task-center card
/// like Regular; kSubWindow (has parent) = MODAL, and the ETS side applies
/// WINDOW_MODALITY + owner-centered placement from the archetype.
class OHOSWindowDialog : public OHOSWindow {
 public:
  using OHOSWindow::OHOSWindow;

  void RequestWindowHost() override;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_DIALOG_H_
