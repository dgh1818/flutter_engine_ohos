/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { FlutterPage } from '@ohos/flutter_ohos'

// Host page for a non-implicit (sub-window) view: viewId arrives via
// LocalStorage, and FlutterPage's XComponent carries id=viewId so the native
// surface callback routes to NotifyCreateForView.
let storage = LocalStorage.getShared()

@Entry(storage)
@Component
struct FlutterWindowPage {
  @LocalStorageLink('viewId') viewId: string = "";

  build() {
    Column() {
      // Sized-to-content sub-windows (tooltip/popup) center their content
      // with transparent slack. FlutterPage has no transparency prop (that
      // lives on the internal FlutterSurface, driven by HCPP mode); injecting
      // xComponentColor=Transparent keeps the XComponent see-through after
      // the first frame instead of flipping to the default black.
      FlutterPage({ viewId: this.viewId, xComponentColor: Color.Transparent })
    }
  }
}
