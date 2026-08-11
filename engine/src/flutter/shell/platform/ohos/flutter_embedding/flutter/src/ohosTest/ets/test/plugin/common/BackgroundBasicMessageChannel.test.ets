import { describe, it, expect, Level } from '@ohos/hypium';
import BackgroundBasicMessageChannel from '../../../../../main/ets/plugin/common/BackgroundBasicMessageChannel';
import { Reply } from '../../../../../main/ets/plugin/common/BasicMessageChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import SendableMessageCodec from '../../../../../main/ets/plugin/common/SendableMessageCodec';
import SendableMessageHandler from '../../../../../main/ets/plugin/common/SendableMessageHandler';

class FakeTaskQueue implements TaskQueue {}

class FakeMessenger implements BinaryMessenger {
  handlerChannel: string = '';
  capturedHandler: Object | null = null;
  sendChannel: string = '';
  sendMessage: ArrayBuffer | null = null;
  capturedReplyCallback: Object | null = null;

  makeBackgroundTaskQueue(options?: TaskQueueOptions): TaskQueue {
    return new FakeTaskQueue();
  }

  send(channel: String, message: ArrayBuffer | null, callback?: BinaryReply | null): void {
    this.sendChannel = channel as string;
    this.sendMessage = message;
    this.capturedReplyCallback = callback ?? null;
  }

  setMessageHandler(channel: String, handler: BinaryMessageHandler | SendableBinaryMessageHandler | null,
    taskQueue?: TaskQueue, ...args: Object[]): void {
    this.handlerChannel = channel as string;
    this.capturedHandler = handler;
  }
}

class FakeReply implements BinaryReply {
  replyData: ArrayBuffer | null = null;
  reply(data: ArrayBuffer | null): void {
    this.replyData = data;
  }
}

@Sendable
class FakeSendableMessageCodec implements SendableMessageCodec<string> {
  encodeMessage(message: string): ArrayBuffer {
    const buffer: ArrayBuffer = new ArrayBuffer(message.length + 1);
    const view: Uint8Array = new Uint8Array(buffer);
    for (let i = 0; i < message.length; i++) {
      view[i] = message.charCodeAt(i);
    }
    view[message.length] = 0;
    return buffer;
  }

  decodeMessage(message: ArrayBuffer | null): string {
    if (message == null) {
      return '';
    }
    const view: Uint8Array = new Uint8Array(message);
    let result: string = '';
    for (let i = 0; i < view.length; i++) {
      if (view[i] === 0) break;
      result += String.fromCharCode(view[i]);
    }
    return result;
  }
}

interface ReplyCapture {
  replyValue: string;
  called: boolean;
}

@Sendable
class SuccessSendableMessageHandler implements SendableMessageHandler<string> {
  receivedMessage: string = '';

  onMessage(message: string, reply: Reply<string>): void {
    this.receivedMessage = message;
    reply.reply('response');
  }
}

@Sendable
class ThrowingSendableMessageHandler implements SendableMessageHandler<string> {
  onMessage(message: string, reply: Reply<string>): void {
    throw new Error('handler error');
  }
}

export default function backgroundBasicMessageChannelTest() {
  const codec = new FakeSendableMessageCodec();

  describe('BackgroundBasicMessageChannel.constructor', () => {
    it('does not throw with valid args', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      let threw = false;
      try {
        new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('BackgroundBasicMessageChannel.send', () => {
    it('calls messenger.send with correct channel name', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'my_channel', codec);
      channel.send('hello');
      expect(messenger.sendChannel).assertEqual('my_channel');
    });

    it('encodes message before sending', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.send('test message');
      expect(messenger.sendMessage).not().assertNull();
    });

    it('passes null callback when no callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.send('hello');
      expect(messenger.capturedReplyCallback).assertNull();
    });

    it('passes non-null callback when callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      let replyValue: string = '';
      channel.send('hello', (reply: string) => { replyValue = reply; });
      expect(messenger.capturedReplyCallback).not().assertNull();
    });
  });

  describe('BackgroundBasicMessageChannel.setMessageHandler', () => {
    it('registers non-null handler with messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(new SuccessSendableMessageHandler());
      expect(messenger.capturedHandler).not().assertNull();
    });

    it('deregisters when handler is null', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(null);
      expect(messenger.capturedHandler).assertNull();
    });

    it('passes correct channel name to messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'my_channel', codec);
      channel.setMessageHandler(new SuccessSendableMessageHandler());
      expect(messenger.handlerChannel).assertEqual('my_channel');
    });
  });

  describe('IncomingSendableMessageHandler.onMessage', () => {
    it('decodes message and passes to handler', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const handler = new SuccessSendableMessageHandler();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(handler);

      const binaryHandler: SendableBinaryMessageHandler = messenger.capturedHandler as SendableBinaryMessageHandler;
      const reply = new FakeReply();
      const message = codec.encodeMessage('hello world');
      binaryHandler.onMessage(message, reply);

      expect(handler.receivedMessage).assertEqual('hello world');
    });

    it('reply encodes and sends via callback', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const handler = new SuccessSendableMessageHandler();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(handler);

      const binaryHandler: SendableBinaryMessageHandler = messenger.capturedHandler as SendableBinaryMessageHandler;
      const reply = new FakeReply();
      binaryHandler.onMessage(codec.encodeMessage('request'), reply);

      expect(reply.replyData).not().assertNull();
    });

    it('sends empty buffer when handler throws', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const handler = new ThrowingSendableMessageHandler();
      const channel = new BackgroundBasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(handler);

      const binaryHandler: SendableBinaryMessageHandler = messenger.capturedHandler as SendableBinaryMessageHandler;
      const reply = new FakeReply();
      binaryHandler.onMessage(codec.encodeMessage('trigger'), reply);

      expect(reply.replyData).not().assertNull();
      expect(reply.replyData!.byteLength).assertEqual(0);
    });
  });
}
