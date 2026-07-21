import { describe, it, expect, Level } from '@ohos/hypium';
import { PlatformViewInfo } from '../../../../../main/ets/embedding/ohos/PlatformViewInfo';
import PlatformView, { Params } from '../../../../../main/ets/plugin/platform/PlatformView';

@Builder
function dummyBuilder(params: Params) {}

class FakePlatformView extends PlatformView {
  getView(): WrappedBuilder<[Params]> {
    return wrapBuilder(dummyBuilder);
  }
  dispose(): void {}
}

export default function platformViewInfoTest() {
  describe('PlatformViewInfo.constructor', () => {
    it('assigns all fields correctly', Level.LEVEL0, () => {
      const view: PlatformView = new FakePlatformView();
      const info = new PlatformViewInfo(view, 'surface-123', 800, 600, 100, 50, Direction.Ltr);
      expect(info.platformView).assertEqual(view);
      expect(info.surfaceId).assertEqual('surface-123');
      expect(info.width).assertEqual(800);
      expect(info.height).assertEqual(600);
      expect(info.top).assertEqual(100);
      expect(info.left).assertEqual(50);
      expect(info.direction).assertEqual(Direction.Ltr);
    });

    it('handles zero dimensions', Level.LEVEL0, () => {
      const view: PlatformView = new FakePlatformView();
      const info = new PlatformViewInfo(view, 'surface-0', 0, 0, 0, 0, Direction.Ltr);
      expect(info.width).assertEqual(0);
      expect(info.height).assertEqual(0);
      expect(info.top).assertEqual(0);
      expect(info.left).assertEqual(0);
    });

    it('handles negative positions', Level.LEVEL1, () => {
      const view: PlatformView = new FakePlatformView();
      const info = new PlatformViewInfo(view, 'surface-neg', 100, 100, -50, -30, Direction.Rtl);
      expect(info.top).assertEqual(-50);
      expect(info.left).assertEqual(-30);
      expect(info.direction).assertEqual(Direction.Rtl);
    });

    it('handles empty surfaceId', Level.LEVEL0, () => {
      const view: PlatformView = new FakePlatformView();
      const info = new PlatformViewInfo(view, '', 200, 200, 10, 10, Direction.Ltr);
      expect(info.surfaceId).assertEqual('');
    });

    it('stores same PlatformView reference', Level.LEVEL1, () => {
      const view: PlatformView = new FakePlatformView();
      const info = new PlatformViewInfo(view, 'surface-ref', 1, 1, 0, 0, Direction.Ltr);
      expect(info.platformView === view).assertEqual(true);
    });

    it('handles large dimensions', Level.LEVEL1, () => {
      const view: PlatformView = new FakePlatformView();
      const info = new PlatformViewInfo(view, 'surface-large', 99999, 99999, 0, 0, Direction.Rtl);
      expect(info.width).assertEqual(99999);
      expect(info.height).assertEqual(99999);
    });
  });
}
