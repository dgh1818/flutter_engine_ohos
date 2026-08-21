/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/windowing/ohos_window_dialog.h"

namespace flutter {

void OHOSWindowDialog::RequestWindowHost() {
  if (host_kind() == WindowHostKind::kUiAbility) {
    // Modeless: same UIAbility host path as Regular.
    RequestUiAbilityHost();
    return;
  }
  // Modal: generic sub-window host (ETS applies the modal semantics).
  OHOSWindow::RequestWindowHost();
}

}  // namespace flutter
