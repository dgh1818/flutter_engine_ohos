import { describe, it, expect, Level } from '@ohos/hypium';
import { PlatformPluginCallback } from '../../../../../main/ets/plugin/PlatformPlugin';
import { SoundType } from '../../../../../main/ets/embedding/engine/systemchannels/PlatformChannel';

export default function platformPluginTest() {
  describe('PlatformPluginCallback.showBarOrNavigationDefault', () => {
    it('defaults to status and navigation', Level.LEVEL0, () => {
      const callback = new PlatformPluginCallback();
      expect(callback.showBarOrNavigation.length).assertEqual(2);
      expect(callback.showBarOrNavigation[0]).assertEqual('status');
      expect(callback.showBarOrNavigation[1]).assertEqual('navigation');
    });
  });

  describe('PlatformPluginCallback.clipboardHasStrings', () => {
    it('returns false by default', Level.LEVEL0, () => {
      const callback = new PlatformPluginCallback();
      expect(callback.clipboardHasStrings()).assertEqual(false);
    });
  });

  describe('PlatformPluginCallback.playSystemSound', () => {
    it('does not throw for CLICK', Level.LEVEL0, () => {
      const callback = new PlatformPluginCallback();
      let threw = false;
      try {
        callback.playSystemSound(SoundType.CLICK);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw for ALERT', Level.LEVEL1, () => {
      const callback = new PlatformPluginCallback();
      let threw = false;
      try {
        callback.playSystemSound(SoundType.ALERT);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('PlatformPluginCallback.shouldHideWindowDecor', () => {
    it('returns true for default status and navigation', Level.LEVEL0, () => {
      const callback = new PlatformPluginCallback();
      expect(callback.shouldHideWindowDecor()).assertEqual(true);
    });

    it('returns false for status only', Level.LEVEL1, () => {
      const callback = new PlatformPluginCallback();
      const arr: ('status' | 'navigation')[] = ['status'];
      callback.showBarOrNavigation = arr;
      expect(callback.shouldHideWindowDecor()).assertEqual(false);
    });

    it('returns true for navigation only', Level.LEVEL1, () => {
      const callback = new PlatformPluginCallback();
      const arr: ('status' | 'navigation')[] = ['navigation'];
      callback.showBarOrNavigation = arr;
      expect(callback.shouldHideWindowDecor()).assertEqual(true);
    });

    it('returns true for empty array', Level.LEVEL1, () => {
      const callback = new PlatformPluginCallback();
      const arr: ('status' | 'navigation')[] = new Array<'status' | 'navigation'>();
      callback.showBarOrNavigation = arr;
      expect(callback.shouldHideWindowDecor()).assertEqual(true);
    });

    it('returns true for both status and navigation', Level.LEVEL1, () => {
      const callback = new PlatformPluginCallback();
      const arr: ('status' | 'navigation')[] = ['status', 'navigation'];
      callback.showBarOrNavigation = arr;
      expect(callback.shouldHideWindowDecor()).assertEqual(true);
    });
  });
}
