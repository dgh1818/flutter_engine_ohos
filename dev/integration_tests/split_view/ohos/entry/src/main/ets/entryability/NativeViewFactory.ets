/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import PlatformViewFactory from '@ohos/flutter_ohos/src/main/ets/plugin/platform/PlatformViewFactory';
import { NativeViewPage } from './NativeViewPage';
import common from '@ohos.app.ability.common';
import PlatformView from '@ohos/flutter_ohos/src/main/ets/plugin/platform/PlatformView';
import { StandardMessageCodec } from '@ohos/flutter_ohos';

/**
 * Factory that creates [NativeViewPage] instances for the platform view
 * type `split_view/native_view`.
 */
export class NativeViewFactory extends PlatformViewFactory {

  constructor() {
    super(new StandardMessageCodec());
  }

  public create(context: common.Context, viewId: number, args: Object): PlatformView {
    return new NativeViewPage(context, viewId, args);
  }
}
