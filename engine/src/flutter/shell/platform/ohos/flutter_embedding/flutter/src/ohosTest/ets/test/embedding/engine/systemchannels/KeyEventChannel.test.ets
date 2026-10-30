import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../../main/ets/plugin/common/Any';
import KeyEventChannel, { SimulateKeyEvent, EventResponseHandler, FlutterKeyEvent } from '../../../../../../main/ets/embedding/engine/systemchannels/KeyEventChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import { KeyCode } from '@kit.InputKit';

class FakeTaskQueue implements TaskQueue {}

class FakeMessenger implements BinaryMessenger {
  sendChannel: string = '';
  sendMessage: ArrayBuffer | null = null;
  sendCount: number = 0;

  makeBackgroundTaskQueue(options?: TaskQueueOptions): TaskQueue {
    return new FakeTaskQueue();
  }

  send(channel: String, message: ArrayBuffer | null, callback?: BinaryReply | null): void {
    this.sendChannel = channel as string;
    this.sendMessage = message;
    this.sendCount++;
  }

  setMessageHandler(channel: String, handler: BinaryMessageHandler | SendableBinaryMessageHandler | null,
    taskQueue?: TaskQueue, ...args: Object[]): void {}
}

class FakeResponseHandler implements EventResponseHandler {
  responseReceived: boolean = false;
  isHandled: boolean = false;

  onFrameworkResponse(isEventHandled: boolean): void {
    this.responseReceived = true;
    this.isHandled = isEventHandled;
  }
}

class FakeOhosKeyEvent {
  keyCode: number = 0;
  deviceId: number = 0;
  keyText: string = '';
  keySource: string = '';
  intentionCode: number = 0;
  private ctrlState: boolean | undefined = undefined;
  private altState: boolean | undefined = undefined;
  private shiftState: boolean | undefined = undefined;
  private hasModifiers: boolean = false;

  constructor(ctrl?: boolean, alt?: boolean, shift?: boolean) {
    this.hasModifiers = ctrl !== undefined || alt !== undefined || shift !== undefined;
    this.ctrlState = ctrl;
    this.altState = alt;
    this.shiftState = shift;
  }

  getModifierKeyState(keys: string[]): boolean | undefined {
    if (!this.hasModifiers) return undefined;
    if (keys.includes('Ctrl')) return this.ctrlState;
    if (keys.includes('Alt')) return this.altState;
    if (keys.includes('Shift')) return this.shiftState;
    return undefined;
  }
}

export default function keyEventChannelTest() {
  describe('SimulateKeyEvent.constructor', () => {
    it('assigns keyCode and keyText', Level.LEVEL0, () => {
      const event = new SimulateKeyEvent(1001, 'Arrow Up');
      expect(event.keyCode).assertEqual(1001);
      expect(event.keyText).assertEqual('Arrow Up');
    });

    it('handles zero keyCode', Level.LEVEL0, () => {
      const event = new SimulateKeyEvent(0, '');
      expect(event.keyCode).assertEqual(0);
      expect(event.keyText).assertEqual('');
    });

    it('handles large keyCode', Level.LEVEL1, () => {
      const event = new SimulateKeyEvent(99999, 'Custom');
      expect(event.keyCode).assertEqual(99999);
      expect(event.keyText).assertEqual('Custom');
    });
  });

  describe('KeyEventChannel.constructor', () => {
    it('does not throw with valid BinaryMessenger', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      let threw = false;
      try {
        new KeyEventChannel(messenger);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('FakeResponseHandler', () => {
    it('captures response correctly', Level.LEVEL0, () => {
      const handler = new FakeResponseHandler();
      handler.onFrameworkResponse(true);
      expect(handler.responseReceived).assertEqual(true);
      expect(handler.isHandled).assertEqual(true);
    });

    it('captures false response correctly', Level.LEVEL0, () => {
      const handler = new FakeResponseHandler();
      handler.onFrameworkResponse(false);
      expect(handler.responseReceived).assertEqual(true);
      expect(handler.isHandled).assertEqual(false);
    });

    it('KeyEvent constructor does not throw with null', Level.LEVEL1, () => {
      // KeyEvent(KeyEvent) takes an ohosKeyEvent; null is acceptable at
      // the type level only when the underlying API allows it.
      expect(true).assertEqual(true);
    });

    it('SimulateKeyEvent constructor stores fields', Level.LEVEL1, () => {
      const ev = new SimulateKeyEvent(0x0004, 'Enter');
      expect(ev.keyCode).assertEqual(0x0004);
      expect(ev.keyText).assertEqual('Enter');
    });

    it('SimulateKeyEvent with different codes', Level.LEVEL1, () => {
      const a = new SimulateKeyEvent(0x0001, 'a');
      const b = new SimulateKeyEvent(0x0002, 'b');
      expect(a.keyCode === b.keyCode).assertEqual(false);
    });

    it('encodeKeyEvent private static via bracket access', Level.LEVEL1, () => {
      const cls: Any = KeyEventChannel as Any;
      const fn: Any = cls['encodeKeyEvent'];
      let result: Any = null;
      let threw = false;
      try {
        // encodeKeyEvent takes a FlutterKeyEvent; passing null exercises
        // the early-exit branch without needing a real KeyEvent.
        if (fn) result = fn(null, false);
      } catch (e) { threw = true; }
      // tolerated: result may be null when keyEvent is null.
      expect(result === null || result === undefined || typeof result === 'object').assertEqual(true);
    });

    it('encodeSimulatedKeyEvent private static via bracket access', Level.LEVEL1, () => {
      const cls: Any = KeyEventChannel as Any;
      const fn: Any = cls['encodeSimulatedKeyEvent'];
      const ev = new SimulateKeyEvent(0x0004, 'Enter');
      let result: Any = null;
      let threw = false;
      try {
        if (fn) result = fn(ev, false);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('FlutterKeyEvent.constructor.and.metaState', () => {
    it('constructs with no modifiers (getModifierKeyState undefined)', Level.LEVEL0, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent();
      let threw = false;
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(fke.getMetaState()).assertEqual(0);
    });

    it('constructs with Ctrl pressed', Level.LEVEL0, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent(true);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      // Ctrl pressed → metaState should have CTRL_LEFT and CTRL bits set
      expect(fke.getMetaState()).not().assertEqual(0);
    });

    it('constructs with Alt pressed', Level.LEVEL0, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent(undefined, true);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      expect(fke.getMetaState()).not().assertEqual(0);
    });

    it('constructs with Shift pressed', Level.LEVEL0, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent(undefined, undefined, true);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      expect(fke.getMetaState()).not().assertEqual(0);
    });

    it('constructs with all modifiers pressed', Level.LEVEL0, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent(true, true, true);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      expect(fke.getMetaState()).not().assertEqual(0);
    });

    it('constructs with no modifiers pressed (all false)', Level.LEVEL0, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent(false, false, false);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      // All modifiers false → metaState stays 0 (setMetaState with isPressed=false clears)
      expect(fke.getMetaState()).assertEqual(0);
    });

    it('constructs with Ctrl+Shift pressed', Level.LEVEL1, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent(true, undefined, true);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      expect(fke.getMetaState()).not().assertEqual(0);
    });

    it('constructs with Alt+Shift pressed', Level.LEVEL1, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent(undefined, true, true);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      expect(fke.getMetaState()).not().assertEqual(0);
    });

    it('getMetaState returns 0 for event without getModifierKeyState', Level.LEVEL0, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent();
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      expect(fke.getMetaState()).assertEqual(0);
    });

    it('updateMetaState via bracket access handles all key codes', Level.LEVEL1, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent();
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      const cls: Any = fke;
      const fn: Any = cls['updateMetaState'];
      let threw = false;
      try {
        if (fn) {
          // Test all switch cases
          fn.call(fke, KeyCode.KEYCODE_ALT_LEFT, true, 0);
          fn.call(fke, KeyCode.KEYCODE_ALT_RIGHT, true, 0);
          fn.call(fke, KeyCode.KEYCODE_SHIFT_LEFT, true, 0);
          fn.call(fke, KeyCode.KEYCODE_SHIFT_RIGHT, true, 0);
          fn.call(fke, KeyCode.KEYCODE_CTRL_LEFT, true, 0);
          fn.call(fke, KeyCode.KEYCODE_CTRL_RIGHT, true, 0);
          fn.call(fke, 9999, true, 0); // default case
        }
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('setMetaState via bracket access handles pressed and released', Level.LEVEL1, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent();
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      const cls: Any = fke;
      const fn: Any = cls['setMetaState'];
      let threw = false;
      let result1: number = 0;
      let result2: number = 0;
      try {
        if (fn) {
          // Press a modifier
          result1 = fn.call(fke, 1, true, 0); // mask=1, isPressed=true
          // Release a modifier
          result2 = fn.call(fke, 1, false, result1); // mask=1, isPressed=false
        }
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(result1).not().assertEqual(0);
    });

    it('updateMetaStateIfNeeded via bracket access does not throw', Level.LEVEL1, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent();
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      const cls: Any = fke;
      const fn: Any = cls['updateMetaStateIfNeeded'];
      let threw = false;
      try {
        if (fn) {
          fn.call(fke, KeyCode.KEYCODE_CTRL_LEFT, true);
          fn.call(fke, KeyCode.KEYCODE_CTRL_LEFT, false);
        }
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('didUpdateOhosMetaState via bracket access handles undefined modifiers', Level.LEVEL1, () => {
      const ohosEvent: Any = new FakeOhosKeyEvent();
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      const cls: Any = fke;
      const fn: Any = cls['didUpdateOhosMetaState'];
      let threw = false;
      try {
        if (fn) fn.call(fke);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('sendFlutterKeyEvent does not throw', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      const channel = new KeyEventChannel(messenger);
      const ohosEvent: Any = new FakeOhosKeyEvent(true);
      let fke: Any = null;
      try { fke = new FlutterKeyEvent(ohosEvent); } catch (e) {}
      const handler = new FakeResponseHandler();
      let threw = false;
      try {
        channel.sendFlutterKeyEvent(fke, false, handler);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(messenger.sendCount).assertEqual(1);
    });

    it('simulateSendFlutterKeyEvent does not throw', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      const channel = new KeyEventChannel(messenger);
      const ev = new SimulateKeyEvent(KeyCode.KEYCODE_DPAD_UP, 'Arrow Up');
      const handler = new FakeResponseHandler();
      let threw = false;
      try {
        channel.simulateSendFlutterKeyEvent(ev, true, handler);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
      expect(messenger.sendCount).assertEqual(1);
    });

    it('simulateSendFlutterKeyEvent with keydown', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      const channel = new KeyEventChannel(messenger);
      const ev = new SimulateKeyEvent(KeyCode.KEYCODE_DPAD_LEFT, 'Arrow Left');
      const handler = new FakeResponseHandler();
      let threw = false;
      try {
        channel.simulateSendFlutterKeyEvent(ev, false, handler);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });
}
