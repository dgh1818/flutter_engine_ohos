/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_POPUP_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_POPUP_H_

#include "flutter/shell/platform/ohos/windowing/ohos_window_anchored.h"

namespace flutter {

/// A Popup: engine-side identical to a tooltip (OHOSWindowAnchored), but
/// stays focusable — the ETS host distinguishes them by archetype.
class OHOSWindowPopup : public OHOSWindowAnchored {
 public:
  using OHOSWindowAnchored::OHOSWindowAnchored;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_OHOS_WINDOWING_OHOS_WINDOW_POPUP_H_
