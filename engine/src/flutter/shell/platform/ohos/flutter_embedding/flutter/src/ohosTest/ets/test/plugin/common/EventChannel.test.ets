import { describe, it, expect, Level } from '@ohos/hypium';
import EventChannel, { StreamHandler, EventSink } from '../../../../../main/ets/plugin/common/EventChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import MethodCodec from '../../../../../main/ets/plugin/common/MethodCodec';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';
import Any from '../../../../../main/ets/plugin/common/Any';

class FakeTaskQueue implements TaskQueue {}

class FakeReply implements BinaryReply {
  replyData: ArrayBuffer | null = null;
  reply(data: ArrayBuffer | null): void {
    this.replyData = data;
  }
}

class FakeMessenger implements BinaryMessenger {
  handlerChannel: string = '';
  capturedHandler: Object | null = null;
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
    taskQueue?: TaskQueue, ...args: Object[]): void {
    this.handlerChannel = channel as string;
    this.capturedHandler = handler;
  }
}

class FakeStreamHandler implements StreamHandler {
  listenCalled: boolean = false;
  cancelCalled: boolean = false;
  capturedSink: EventSink | null = null;
  shouldThrow: boolean = false;

  onListen(args: Any, events: EventSink): void {
    this.listenCalled = true;
    this.capturedSink = events;
    if (this.shouldThrow) {
      throw new Error('stream handler error');
    }
  }

  onCancel(args: Any): void {
    this.cancelCalled = true;
  }
}

class FakeMethodCodec implements MethodCodec {
  encodeMethodCall(methodCall: MethodCall): ArrayBuffer {
    const method: string = methodCall.method;
    const buffer: ArrayBuffer = new ArrayBuffer(method.length + 1);
    const view: Uint8Array = new Uint8Array(buffer);
    for (let i = 0; i < method.length; i++) {
      view[i] = method.charCodeAt(i);
    }
    view[method.length] = 0;
    return buffer;
  }

  decodeMethodCall(message: ArrayBuffer): MethodCall {
    const view: Uint8Array = new Uint8Array(message);
    let method: string = '';
    for (let i = 0; i < view.length; i++) {
      if (view[i] === 0) break;
      method += String.fromCharCode(view[i]);
    }
    return new MethodCall(method, null);
  }

  encodeSuccessEnvelope(result: Any): ArrayBuffer {
    const buffer: ArrayBuffer = new ArrayBuffer(1);
    const view: Uint8Array = new Uint8Array(buffer);
    view[0] = 0;
    return buffer;
  }

  encodeErrorEnvelope(errorCode: string, errorMessage: string, errorDetails: Any): ArrayBuffer {
    const buffer: ArrayBuffer = new ArrayBuffer(1);
    const view: Uint8Array = new Uint8Array(buffer);
    view[0] = 1;
    return buffer;
  }

  encodeErrorEnvelopeWithStacktrace(errorCode: string, errorMessage: string, errorDetails: Any,
    errorStacktrace: string): ArrayBuffer {
    const buffer: ArrayBuffer = new ArrayBuffer(1);
    const view: Uint8Array = new Uint8Array(buffer);
    view[0] = 1;
    return buffer;
  }

  decodeEnvelope(envelope: ArrayBuffer): Any {
    return null;
  }
}

export default function eventChannelTest() {
  const codec = new FakeMethodCodec();

  describe('EventChannel.constructor', () => {
    it('does not throw with valid messenger and name', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      let threw = false;
      try {
        new EventChannel(messenger, 'test_channel', codec);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with default codec', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      let threw = false;
      try {
        new EventChannel(messenger, 'test_channel');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('EventChannel.setStreamHandler', () => {
    it('registers non-null handler with messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(new FakeStreamHandler());
      expect(messenger.handlerChannel).assertEqual('test_channel');
      expect(messenger.capturedHandler).not().assertNull();
    });

    // SKIPPED: setStreamHandler type signature is `handler: StreamHandler`
    // (not `| null`), but method body checks `handler == null`.
    // Can't pass null without a cast. Source type bug.

    it('passes correct channel name to messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new EventChannel(messenger, 'my_events', codec);
      channel.setStreamHandler(new FakeStreamHandler());
      expect(messenger.handlerChannel).assertEqual('my_events');
    });
  });

  describe('IncomingStreamRequestHandler.onMessage', () => {
    it('calls onListen for listen method', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const streamHandler = new FakeStreamHandler();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(streamHandler);

      const handler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('listen', null));
      handler.onMessage(message, reply);

      expect(streamHandler.listenCalled).assertEqual(true);
    });

    it('replies with success envelope for listen', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const streamHandler = new FakeStreamHandler();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(streamHandler);

      const handler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('listen', null));
      handler.onMessage(message, reply);

      expect(reply.replyData).not().assertNull();
    });

    it('calls onCancel for cancel method', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const streamHandler = new FakeStreamHandler();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(streamHandler);

      const handler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      const reply = new FakeReply();
      const listenMsg = codec.encodeMethodCall(new MethodCall('listen', null));
      handler.onMessage(listenMsg, new FakeReply());

      const cancelMsg = codec.encodeMethodCall(new MethodCall('cancel', null));
      handler.onMessage(cancelMsg, reply);

      expect(streamHandler.cancelCalled).assertEqual(true);
    });

    it('replies with null for unknown method', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const streamHandler = new FakeStreamHandler();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(streamHandler);

      const handler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('unknown', null));
      handler.onMessage(message, reply);

      expect(reply.replyData).assertNull();
    });
  });

  describe('EventSink', () => {
    it('success sends event via messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const streamHandler = new FakeStreamHandler();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(streamHandler);

      const handler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      handler.onMessage(codec.encodeMethodCall(new MethodCall('listen', null)), new FakeReply());

      expect(streamHandler.capturedSink).not().assertNull();
      const sink: EventSink = streamHandler.capturedSink!;
      messenger.sendCount = 0;
      sink.success('event data');
      expect(messenger.sendCount).assertEqual(1);
      expect(messenger.sendChannel).assertEqual('test_channel');
    });

    it('endOfStream prevents subsequent success', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const streamHandler = new FakeStreamHandler();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(streamHandler);

      const handler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      handler.onMessage(codec.encodeMethodCall(new MethodCall('listen', null)), new FakeReply());

      const sink: EventSink = streamHandler.capturedSink!;
      sink.endOfStream();
      messenger.sendCount = 0;
      sink.success('should be ignored');
      expect(messenger.sendCount).assertEqual(0);
    });

    it('error sends error event via messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const streamHandler = new FakeStreamHandler();
      const channel = new EventChannel(messenger, 'test_channel', codec);
      channel.setStreamHandler(streamHandler);

      const handler: BinaryMessageHandler = messenger.capturedHandler as BinaryMessageHandler;
      handler.onMessage(codec.encodeMethodCall(new MethodCall('listen', null)), new FakeReply());

      const sink: EventSink = streamHandler.capturedSink!;
      messenger.sendCount = 0;
      sink.error('ERR', 'error message', 'details');
      expect(messenger.sendCount).assertEqual(1);
    });
  });
}
