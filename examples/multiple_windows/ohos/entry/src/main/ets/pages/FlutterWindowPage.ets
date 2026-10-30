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
  @LocalStorageLink('surfaceWidth') surfaceWidth: number = 0;
  @LocalStorageLink('surfaceHeight') surfaceHeight: number = 0;
  @LocalStorageLink('windowWidth') windowWidth: number = 0;
  @LocalStorageLink('windowHeight') windowHeight: number = 0;
  @LocalStorageLink('contentWidth') contentWidth: number = 0;
  @LocalStorageLink('contentHeight') contentHeight: number = 0;

  private fitScale(content: number, win: number): number {
    if (content <= 0 || win <= 0 || win >= content) {
      return 1;
    }
    return win / content;
  }

  build() {
    Column() {
      // Sized-to-content sub-windows (tooltip/popup) center their content
      // with transparent slack. FlutterPage has no transparency prop (that
      // lives on the internal FlutterSurface, driven by HCPP mode); injecting
      // xComponentColor=Transparent keeps the XComponent see-through after
      // the first frame instead of flipping to the default black.
      FlutterPage({ viewId: this.viewId, xComponentColor: Color.Transparent })
    }
    .width(this.surfaceWidth > 0 ? `${this.surfaceWidth}px` : '100%')
    .height(this.surfaceHeight > 0 ? `${this.surfaceHeight}px` : '100%')
    .scale({
      x: this.fitScale(this.contentWidth, this.windowWidth),
      y: this.fitScale(this.contentHeight, this.windowHeight),
      centerX: 0,
      centerY: 0,
    })
  }
}
