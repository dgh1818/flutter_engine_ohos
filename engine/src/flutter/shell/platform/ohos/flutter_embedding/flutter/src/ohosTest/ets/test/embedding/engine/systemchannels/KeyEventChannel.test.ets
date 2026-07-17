import { describe, it, expect, Level } from '@ohos/hypium';
import KeyEventChannel, { SimulateKeyEvent, EventResponseHandler } from '../../../../../../main/ets/embedding/engine/systemchannels/KeyEventChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';

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
  });
}
