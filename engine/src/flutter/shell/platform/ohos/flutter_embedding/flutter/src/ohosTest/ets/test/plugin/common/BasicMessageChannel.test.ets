import { describe, it, expect, Level } from '@ohos/hypium';
import BasicMessageChannel, { Reply, MessageHandler } from '../../../../../main/ets/plugin/common/BasicMessageChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import MessageCodec from '../../../../../main/ets/plugin/common/MessageCodec';

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

class FakeMessageCodec implements MessageCodec<string> {
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

class FakeMessageHandler implements MessageHandler<string> {
  receivedMessage: string = '';
  capturedReply: Reply<string> | null = null;
  shouldThrow: boolean = false;

  onMessage(message: string, reply: Reply<string>): void {
    this.receivedMessage = message;
    this.capturedReply = reply;
    if (this.shouldThrow) {
      throw new Error('handler error');
    }
  }
}

class FakeReply implements BinaryReply {
  replyData: ArrayBuffer | null = null;
  reply(data: ArrayBuffer | null): void {
    this.replyData = data;
  }
}

export default function basicMessageChannelTest() {
  const codec = new FakeMessageCodec();

  describe('BasicMessageChannel.constructor', () => {
    it('does not throw with valid args', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      let threw = false;
      try {
        new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('BasicMessageChannel.send', () => {
    it('calls messenger.send with correct channel name', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'my_channel', codec);
      channel.send('hello');
      expect(messenger.sendChannel).assertEqual('my_channel');
    });

    it('encodes message before sending', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.send('test message');
      expect(messenger.sendMessage).not().assertNull();
    });

    it('passes null callback when no callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.send('hello');
      expect(messenger.capturedReplyCallback).assertNull();
    });

    it('passes non-null callback when callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      let replyValue: string = '';
      channel.send('hello', (reply: string) => { replyValue = reply; });
      expect(messenger.capturedReplyCallback).not().assertNull();
    });
  });

  describe('BasicMessageChannel.setMessageHandler', () => {
    it('registers non-null handler with messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(new FakeMessageHandler());
      expect(messenger.capturedHandler).not().assertNull();
    });

    it('deregisters when handler is null', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(null);
      expect(messenger.capturedHandler).assertNull();
    });

    it('passes correct channel name to messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'my_channel', codec);
      channel.setMessageHandler(new FakeMessageHandler());
      expect(messenger.handlerChannel).assertEqual('my_channel');
    });
  });

  describe('IncomingMessageHandler.onMessage', () => {
    it('decodes message and passes to handler', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const handler = new FakeMessageHandler();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(handler);

      const binaryHandler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      const reply = new FakeReply();
      const message = codec.encodeMessage('hello world');
      binaryHandler.onMessage(message, reply);

      expect(handler.receivedMessage).assertEqual('hello world');
    });

    it('reply encodes and sends via callback', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const handler = new FakeMessageHandler();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(handler);

      const binaryHandler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      const reply = new FakeReply();
      binaryHandler.onMessage(codec.encodeMessage('request'), reply);

      expect(handler.capturedReply).not().assertNull();
      handler.capturedReply!.reply('response data');
      expect(reply.replyData).not().assertNull();
    });

    it('sends empty buffer when handler throws', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const handler = new FakeMessageHandler();
      handler.shouldThrow = true;
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      channel.setMessageHandler(handler);

      const binaryHandler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      const reply = new FakeReply();
      binaryHandler.onMessage(codec.encodeMessage('trigger'), reply);

      expect(reply.replyData).not().assertNull();
      expect(reply.replyData!.byteLength).assertEqual(0);
    });
  });

  describe('IncomingReplyHandler.reply', () => {
    it('calls callback with decoded message', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      let replyValue: string = '';
      channel.send('hello', (reply: string) => { replyValue = reply; });

      const replyHandler: BinaryReply = messenger.capturedReplyCallback as BinaryReply;
      replyHandler.reply(codec.encodeMessage('reply data'));

      expect(replyValue).assertEqual('reply data');
    });

    it('calls callback with empty string when reply is null', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BasicMessageChannel<string>(messenger, 'test_channel', codec);
      let replyValue: string = 'not set';
      channel.send('hello', (reply: string) => { replyValue = reply; });

      const replyHandler: BinaryReply = messenger.capturedReplyCallback as BinaryReply;
      replyHandler.reply(null);

      expect(replyValue).assertEqual('');
    });
  });
}
