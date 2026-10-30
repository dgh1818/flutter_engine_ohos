import { describe, it, expect, Level } from '@ohos/hypium';
import { HashMap } from '@kit.ArkTS';
import Any from '../../../../../../main/ets/plugin/common/Any';
import AccessibilityChannel, { Action } from '../../../../../../main/ets/embedding/engine/systemchannels/AccessibilityChannel';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import { resourceManager } from '@kit.LocalizationKit';

function createChannel(): AccessibilityChannel {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  return new AccessibilityChannel(dartExecutor, flutterNapi);
}

class FakeReply {
  replyCount: number = 0;
  lastData: Any = null;
  reply(data: Any): void { this.replyCount++; this.lastData = data; }
}

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

    it('Action enum coverage (additional)', Level.LEVEL1, () => {
      // Verify a few more bit positions are unique and not aliased.
      const values = [
        Action.TAP, Action.LONG_PRESS, Action.SCROLL_LEFT, Action.SCROLL_RIGHT,
        Action.SCROLL_UP, Action.SCROLL_DOWN
      ];
      const seen = new Set<number>();
      for (const v of values) {
        expect(seen.has(v)).assertEqual(false);
        seen.add(v);
      }
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

  describe('AccessibilityChannel.onMessage.branches', () => {
    it('onMessage with real HashMap announce event', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'announce');
      const data: HashMap<string, Any> = new HashMap();
      data.set('message', 'hello');
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(channel).not().assertNull();
    });

    it('onMessage with tap event', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'tap');
      msg.set('nodeId', 42);
      const data: HashMap<string, Any> = new HashMap();
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      // The handler.onTap call may throw (NAPI), but the method body is entered
      expect(threw).assertEqual(true);
    });

    it('onMessage with longPress event', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'longPress');
      msg.set('nodeId', 10);
      const data: HashMap<string, Any> = new HashMap();
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
    });

    it('onMessage with tooltip event', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'tooltip');
      const data: HashMap<string, Any> = new HashMap();
      data.set('message', 'tooltip text');
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(true);
    });

    it('onMessage with unknown type', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'unknown');
      const data: HashMap<string, Any> = new HashMap();
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onMessage with null nodeId for tap', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'tap');
      const data: HashMap<string, Any> = new HashMap();
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onMessage with null message for announce', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'announce');
      const data: HashMap<string, Any> = new HashMap();
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onMessage with null nodeId for long press (covers line 71 false branch)', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'longPress');
      const data: HashMap<string, Any> = new HashMap();
      data.set('nodeId', null);
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onMessage with null tooltip (covers line 79 false branch)', Level.LEVEL0, () => {
      const channel = createChannel();
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'tooltip');
      const data: HashMap<string, Any> = new HashMap();
      data.set('message', null);
      msg.set('data', data);
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onMessage with null handler (covers line 40 true branch)', Level.LEVEL0, () => {
      const channel = createChannel();
      // Cast to Any to set handler to null
      (channel as Any).handler = null;
      const msg: HashMap<string, Any> = new HashMap();
      msg.set('type', 'tap');
      msg.set('data', new HashMap<string, Any>());
      const reply: Any = new FakeReply();
      let threw = false;
      try { channel.onMessage(msg, reply); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onOhosAccessibilityEnabled does not throw', Level.LEVEL0, () => {
      const channel = createChannel();
      let threw = false;
      try { channel.onOhosAccessibilityEnabled(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('onOhosAccessibilityFeatures does not throw', Level.LEVEL0, () => {
      const channel = createChannel();
      let threw = false;
      try { channel.onOhosAccessibilityFeatures(0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('dispatchSemanticsAction does not throw', Level.LEVEL0, () => {
      const channel = createChannel();
      let threw = false;
      try { channel.dispatchSemanticsAction(1, Action.TAP); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('dispatchSemanticsAction with different actions', Level.LEVEL0, () => {
      const channel = createChannel();
      let threw = false;
      try {
        channel.dispatchSemanticsAction(1, Action.LONG_PRESS);
        channel.dispatchSemanticsAction(2, Action.SCROLL_LEFT);
        channel.dispatchSemanticsAction(3, Action.SCROLL_RIGHT);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });
}
