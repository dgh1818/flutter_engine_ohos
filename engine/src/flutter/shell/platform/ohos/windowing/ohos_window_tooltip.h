/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TOOLTIP_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TOOLTIP_H_

#include "flutter/shell/platform/ohos/windowing/ohos_window_anchored.h"

namespace flutter {

/// A Tooltip: positioner-anchored and non-focusable; behavior lives in
/// OHOSWindowAnchored, surface styling is applied by the ETS host by
/// archetype.
class OHOSWindowTooltip : public OHOSWindowAnchored {
 public:
  using OHOSWindowAnchored::OHOSWindowAnchored;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_TOOLTIP_H_
