import { describe, it, expect, Level } from '@ohos/hypium';
import KeyEmbedderResponder from '../../../../../main/ets/embedding/ohos/KeyEmbedderResponder';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import KeyboardMap, { KeyPair, ModifierGoal } from '../../../../../main/ets/embedding/ohos/KeyboardMap';

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

export default function keyEmbedderResponderTest() {
  describe('KeyEmbedderResponder.constructor', () => {
    it('does not throw with valid BinaryMessenger', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      let threw = false;
      try {
        new KeyEmbedderResponder(messenger);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('KeyEmbedderResponder.updatePressingKeys', () => {
    it('records pressed key', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const responder = new KeyEmbedderResponder(messenger);
      responder.updatePressingKeys(0x000700e0, 0x0200000100);
    });

    it('releases pressed key', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const responder = new KeyEmbedderResponder(messenger);
      responder.updatePressingKeys(0x000700e0, 0x0200000100);
      responder.updatePressingKeys(0x000700e0, null);
    });

    it('does not throw for unknown key release', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const responder = new KeyEmbedderResponder(messenger);
      let threw = false;
      try {
        responder.updatePressingKeys(0x99999999, null);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('KeyEmbedderResponder.getPressedKeys', () => {
    it('returns empty map initially', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      const responder = new KeyEmbedderResponder(messenger);
      const keys: Map<number, number> = responder.getPressedKeys();
      expect(keys.size).assertEqual(0);
    });

    it('returns pressed key after update', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const responder = new KeyEmbedderResponder(messenger);
      responder.updatePressingKeys(0x000700e0, 0x0200000100);
      const keys: Map<number, number> = responder.getPressedKeys();
      expect(keys.size).assertEqual(1);
      expect(keys.get(0x000700e0)).assertEqual(0x0200000100);
    });

    it('returns empty after press then release', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const responder = new KeyEmbedderResponder(messenger);
      responder.updatePressingKeys(0x000700e0, 0x0200000100);
      responder.updatePressingKeys(0x000700e0, null);
      const keys: Map<number, number> = responder.getPressedKeys();
      expect(keys.size).assertEqual(0);
    });

    it('tracks multiple pressed keys independently', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const responder = new KeyEmbedderResponder(messenger);
      responder.updatePressingKeys(0x000700e0, 0x0200000100);
      responder.updatePressingKeys(0x000700e1, 0x0200000102);
      const keys: Map<number, number> = responder.getPressedKeys();
      expect(keys.size).assertEqual(2);
      expect(keys.get(0x000700e0)).assertEqual(0x0200000100);
      expect(keys.get(0x000700e1)).assertEqual(0x0200000102);
    });
  });

  // SKIPPED: handleKeyEvent requires KeyEvent from @kit.InputKit (system API type).
  // synchronizeModifierKey requires KeyEvent parameter.
  // synthesizeEvent calls this.messenger.send which triggers NAPI via KeyData.toBytes.
  // These methods need import mock for KeyEvent type and KeyData serialization.
  // Will be added after mock infrastructure is ready.
}
