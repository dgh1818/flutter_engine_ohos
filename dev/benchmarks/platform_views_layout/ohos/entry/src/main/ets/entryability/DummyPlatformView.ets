import PlatformView, { Params } from '@ohos/flutter_ohos/src/main/ets/plugin/platform/PlatformView';
import common from '@ohos.app.ability.common';

@Component
struct DummyComponent {
  build() {
    Text("DummyPlatformView").fontSize(72).backgroundColor(0xffffff)
      .width('100%').height('100%')
  }
}

@Builder
function DummyBuilder(params: Params) {
  DummyComponent()
}

export class DummyPlatformView extends PlatformView {
  constructor(context: common.Context, viewId: number, args: Object) {
    super()
  }

  getView(): WrappedBuilder<[Params]> {
    return new WrappedBuilder(DummyBuilder)
  }

  dispose(): void {
  }
}