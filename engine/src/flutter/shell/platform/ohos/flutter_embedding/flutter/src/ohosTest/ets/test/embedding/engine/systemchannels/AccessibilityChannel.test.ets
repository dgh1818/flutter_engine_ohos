import { describe, it, expect, Level } from '@ohos/hypium';
import { Action } from '../../../../../../main/ets/embedding/engine/systemchannels/AccessibilityChannel';

// AccessibilityChannel constructor requires DartExecutor + FlutterNapi (NAPI).
// Cannot construct channel instance. Test only the exported Action enum.

export default function accessibilityChannelTest() {
  describe('AccessibilityChannel.Action', () => {
    it('TAP equals 1', Level.LEVEL0, () => {
      expect(Action.TAP).assertEqual(1);
    });

    it('LONG_PRESS equals 2', Level.LEVEL0, () => {
      expect(Action.LONG_PRESS).assertEqual(2);
    });

    it('SCROLL_LEFT equals 4', Level.LEVEL0, () => {
      expect(Action.SCROLL_LEFT).assertEqual(4);
    });

    it('SCROLL_RIGHT equals 8', Level.LEVEL0, () => {
      expect(Action.SCROLL_RIGHT).assertEqual(8);
    });

    it('SCROLL_UP equals 16', Level.LEVEL0, () => {
      expect(Action.SCROLL_UP).assertEqual(16);
    });

    it('SCROLL_DOWN equals 32', Level.LEVEL0, () => {
      expect(Action.SCROLL_DOWN).assertEqual(32);
    });

    it('INCREASE equals 64', Level.LEVEL0, () => {
      expect(Action.INCREASE).assertEqual(64);
    });

    it('DECREASE equals 128', Level.LEVEL0, () => {
      expect(Action.DECREASE).assertEqual(128);
    });

    it('SHOW_ON_SCREEN equals 256', Level.LEVEL0, () => {
      expect(Action.SHOW_ON_SCREEN).assertEqual(256);
    });

    it('COPY equals 4096', Level.LEVEL1, () => {
      expect(Action.COPY).assertEqual(4096);
    });

    it('CUT equals 8192', Level.LEVEL1, () => {
      expect(Action.CUT).assertEqual(8192);
    });

    it('PASTE equals 16384', Level.LEVEL1, () => {
      expect(Action.PASTE).assertEqual(16384);
    });

    it('DISMISS equals 262144', Level.LEVEL1, () => {
      expect(Action.DISMISS).assertEqual(262144);
    });

    it('SET_NEXT equals 2097152', Level.LEVEL1, () => {
      expect(Action.SET_NEXT).assertEqual(2097152);
    });

    it('TAP and LONG_PRESS are different bits', Level.LEVEL0, () => {
      expect(Action.TAP & Action.LONG_PRESS).assertEqual(0);
    });

    it('TAP and COPY are different bits', Level.LEVEL1, () => {
      expect(Action.TAP & Action.COPY).assertEqual(0);
    });

    it('all scroll actions combine correctly', Level.LEVEL1, () => {
      const combined: number = Action.SCROLL_LEFT | Action.SCROLL_RIGHT | Action.SCROLL_UP | Action.SCROLL_DOWN;
      expect(combined).assertEqual(60);
    });
  });
}
