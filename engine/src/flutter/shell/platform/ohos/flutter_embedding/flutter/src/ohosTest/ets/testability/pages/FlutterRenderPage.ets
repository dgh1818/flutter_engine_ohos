/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { FlutterPage } from '../../../../main/ets/embedding/ohos/FlutterPage';

@Entry
@Component
struct FlutterRenderPage {
  @StorageProp('flutterRenderViewId') flutterViewId: string = '';

  build() {
    Column() {
      FlutterPage({ viewId: this.flutterViewId })
    }
    .width('100%')
    .height('100%')
  }
}
