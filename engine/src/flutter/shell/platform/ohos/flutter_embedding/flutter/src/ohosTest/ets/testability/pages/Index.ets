/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { FlutterPage } from '../../../../main/ets/embedding/ohos/FlutterPage';

let storage = LocalStorage.getShared();

@Entry(storage)
@Component
struct Index {
  @StorageProp('flutterViewId') flutterViewId: string = '';

  build() {
    Column() {
      if (this.flutterViewId.length > 0) {
        FlutterPage({ viewId: this.flutterViewId })
      } else {
        Text('Waiting for Flutter engine...')
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
      }
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
  }
}
