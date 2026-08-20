/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_REGULAR_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_REGULAR_H_

#include "flutter/shell/platform/ohos/windowing/ohos_window.h"

namespace flutter {

/// A Regular window: always a UIAbility host. No owner/modality semantics —
/// closing one Regular window never closes another.
class OHOSWindowRegular : public OHOSWindow {
 public:
  using OHOSWindow::OHOSWindow;

  // Adopt the EntryAbility (view 0) or spawn a RegularWindowAbility.
  void RequestWindowHost() override;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_REGULAR_H_
