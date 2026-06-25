import PlatformViewFactory from '@ohos/flutter_ohos/src/main/ets/plugin/platform/PlatformViewFactory';
import { DummyPlatformView } from './DummyPlatformView';
import common from '@ohos.app.ability.common';
import PlatformView from '@ohos/flutter_ohos/src/main/ets/plugin/platform/PlatformView';
import {StandardMessageCodec } from '@ohos/flutter_ohos';

export class DummyPlatformViewFactory extends PlatformViewFactory {

  constructor() {
    super(new StandardMessageCodec());
  }

  public create(context: common.Context, viewId: number, args: Object): PlatformView {
    return new DummyPlatformView(context, viewId, args);
  }
}
