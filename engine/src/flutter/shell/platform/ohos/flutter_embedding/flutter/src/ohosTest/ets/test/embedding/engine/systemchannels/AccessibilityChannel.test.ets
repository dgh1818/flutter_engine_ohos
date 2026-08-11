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

    it('MOVE_CURSOR_FORWARD_BY_CHARACTER equals 512', Level.LEVEL0, () => {
      expect(Action.MOVE_CURSOR_FORWARD_BY_CHARACTER).assertEqual(512);
    });

    it('MOVE_CURSOR_BACKWARD_BY_CHARACTER equals 1024', Level.LEVEL0, () => {
      expect(Action.MOVE_CURSOR_BACKWARD_BY_CHARACTER).assertEqual(1024);
    });

    it('SET_SELECTION equals 2048', Level.LEVEL0, () => {
      expect(Action.SET_SELECTION).assertEqual(2048);
    });

    it('DID_GAIN_ACCESSIBILITY_FOCUS equals 32768', Level.LEVEL0, () => {
      expect(Action.DID_GAIN_ACCESSIBILITY_FOCUS).assertEqual(32768);
    });

    it('DID_LOSE_ACCESSIBILITY_FOCUS equals 65536', Level.LEVEL0, () => {
      expect(Action.DID_LOSE_ACCESSIBILITY_FOCUS).assertEqual(65536);
    });

    it('CUSTOM_ACTION equals 131072', Level.LEVEL0, () => {
      expect(Action.CUSTOM_ACTION).assertEqual(131072);
    });

    it('MOVE_CURSOR_FORWARD_BY_WORD equals 524288', Level.LEVEL0, () => {
      expect(Action.MOVE_CURSOR_FORWARD_BY_WORD).assertEqual(524288);
    });

    it('MOVE_CURSOR_BACKWARD_BY_WORD equals 1048576', Level.LEVEL0, () => {
      expect(Action.MOVE_CURSOR_BACKWARD_BY_WORD).assertEqual(1048576);
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
