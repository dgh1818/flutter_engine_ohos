import { describe, it, expect, Level } from '@ohos/hypium';
import BackgroundMethodChannel, { IncomingSendableResultHandler, IncomingSendableMethodCallHandler } from '../../../../../main/ets/plugin/common/BackgroundMethodChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import SendableMethodCodec from '../../../../../main/ets/plugin/common/SendableMethodCodec';
import SendableMethodCallHandler from '../../../../../main/ets/plugin/common/SendableMethodCallHandler';
import { MethodResult } from '../../../../../main/ets/plugin/common/MethodChannel';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';
import Any from '../../../../../main/ets/plugin/common/Any';
import FlutterException from '../../../../../main/ets/plugin/common/FlutterException';

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

class FakeResult implements MethodResult {
  successCalled: boolean = false;
  successValue: Any = null;
  errorCalled: boolean = false;
  errorCode: string = '';
  errorMessage: string = '';
  errorDetails: Any = null;
  notImplCalled: boolean = false;

  success(result: Any): void {
    this.successCalled = true;
    this.successValue = result;
  }
  error(errorCode: string, errorMessage: string, errorDetails: Any): void {
    this.errorCalled = true;
    this.errorCode = errorCode;
    this.errorMessage = errorMessage;
    this.errorDetails = errorDetails;
  }
  notImplemented(): void {
    this.notImplCalled = true;
  }
}

@Sendable
class FakeSendableMethodCodec implements SendableMethodCodec {
  errorCode: string = '';
  errorMessage: string = '';
  errorDetails: string = '';
  capturedErrMsg: string = '';
  capturedErrStack: string = '';

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
    this.capturedErrMsg = errorMessage;
    this.capturedErrStack = errorStacktrace;
    const buffer: ArrayBuffer = new ArrayBuffer(1);
    const view: Uint8Array = new Uint8Array(buffer);
    view[0] = 1;
    return buffer;
  }

  decodeEnvelope(envelope: ArrayBuffer): Any {
    const view: Uint8Array = new Uint8Array(envelope);
    if (view[0] === 0) {
      return null;
    }
    throw new FlutterException(this.errorCode, this.errorMessage, this.errorDetails);
  }
}

@Sendable
class SuccessSendableHandler implements SendableMethodCallHandler {
  receivedMethod: string = '';

  onMethodCall(call: MethodCall, result: MethodResult, ...args: Object[]): void {
    this.receivedMethod = call.method;
    result.success(null);
  }
}

@Sendable
class NotImplementedSendableHandler implements SendableMethodCallHandler {
  onMethodCall(call: MethodCall, result: MethodResult, ...args: Object[]): void {
    result.notImplemented();
  }
}

@Sendable
class ThrowingSendableHandler implements SendableMethodCallHandler {
  onMethodCall(call: MethodCall, result: MethodResult, ...args: Object[]): void {
    throw new Error('sendable handler exception');
  }
}

@Sendable
class FlutterThrowingSendableHandler implements SendableMethodCallHandler {
  onMethodCall(call: MethodCall, result: MethodResult, ...args: Object[]): void {
    const e: FlutterException = new FlutterException('invalid_args', 'bad argument', 'extra');
    e.stack = 'simulated stack';
    throw e;
  }
}

export default function backgroundMethodChannelTest() {
  const codec = new FakeSendableMethodCodec();

  describe('BackgroundMethodChannel.constructor', () => {
    it('does not throw with valid args', Level.LEVEL0, () => {
      const messenger = new FakeMessenger();
      let threw = false;
      try {
        new BackgroundMethodChannel(messenger, 'test_channel', codec);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('BackgroundMethodChannel.setMethodCallHandler', () => {
    it('registers non-null handler with messenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundMethodChannel(messenger, 'test_channel', codec);
      channel.setMethodCallHandler(new SuccessSendableHandler());
      expect(messenger.handlerChannel).assertEqual('test_channel');
      expect(messenger.capturedHandler).not().assertNull();
    });

    it('deregisters when handler is null', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundMethodChannel(messenger, 'test_channel', codec);
      channel.setMethodCallHandler(null);
      expect(messenger.capturedHandler).assertNull();
    });
  });

  describe('BackgroundMethodChannel.invokeMethod', () => {
    it('calls messenger.send with correct channel name', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundMethodChannel(messenger, 'my_channel', codec);
      channel.invokeMethod('testMethod', null);
      expect(messenger.sendChannel).assertEqual('my_channel');
    });

    it('passes null callback when no callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundMethodChannel(messenger, 'test_channel', codec);
      channel.invokeMethod('test', null);
      expect(messenger.capturedReplyCallback).assertNull();
    });

    it('passes non-null callback when callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new BackgroundMethodChannel(messenger, 'test_channel', codec);
      const result = new FakeResult();
      channel.invokeMethod('test', null, result);
      expect(messenger.capturedReplyCallback).not().assertNull();
    });
  });

  describe('IncomingSendableMethodCallHandler.onMessage', () => {
    it('decodes method call and passes to handler', Level.LEVEL1, () => {
      const handler = new SuccessSendableHandler();
      const wrapper = new IncomingSendableMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('testMethod', null));
      wrapper.onMessage(message, reply);
      expect(handler.receivedMethod).assertEqual('testMethod');
    });

    it('replies with success envelope when handler calls success', Level.LEVEL1, () => {
      const handler = new SuccessSendableHandler();
      const wrapper = new IncomingSendableMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      wrapper.onMessage(codec.encodeMethodCall(new MethodCall('test', null)), reply);
      expect(reply.replyData).not().assertNull();
    });

    it('replies with empty buffer when handler calls notImplemented', Level.LEVEL1, () => {
      const handler = new NotImplementedSendableHandler();
      const wrapper = new IncomingSendableMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      wrapper.onMessage(codec.encodeMethodCall(new MethodCall('test', null)), reply);
      expect(reply.replyData).not().assertNull();
      expect(reply.replyData!.byteLength).assertEqual(0);
    });

    // Regression test for the e.getMessage() bug previously documented here.
    it('replies with error envelope when handler throws exception', Level.LEVEL1, () => {
      const handler = new ThrowingSendableHandler();
      const wrapper = new IncomingSendableMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      wrapper.onMessage(codec.encodeMethodCall(new MethodCall('test', null)), reply);
      expect(reply.replyData).not().assertNull();
      expect(new Uint8Array(reply.replyData!)[0]).assertEqual(1);
    });

    // Regression test: a handler throwing FlutterException must not degrade the
    // error message to "[object Object]" nor drop its stack.
    it('replies with real message and stack when handler throws FlutterException', Level.LEVEL1, () => {
      const handler = new FlutterThrowingSendableHandler();
      const wrapper = new IncomingSendableMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      wrapper.onMessage(codec.encodeMethodCall(new MethodCall('test', null)), reply);
      expect(reply.replyData).not().assertNull();
      expect(new Uint8Array(reply.replyData!)[0]).assertEqual(1);
      expect(codec.capturedErrMsg).assertEqual('bad argument');
      expect(codec.capturedErrStack).assertEqual('simulated stack');
    });
  });

  describe('IncomingSendableResultHandler.reply', () => {
    it('calls notImplemented when reply is null', Level.LEVEL0, () => {
      const result = new FakeResult();
      const handler = new IncomingSendableResultHandler(result, codec);
      handler.reply(null);
      expect(result.notImplCalled).assertEqual(true);
    });

    it('calls success when reply is non-null', Level.LEVEL1, () => {
      const result = new FakeResult();
      const handler = new IncomingSendableResultHandler(result, codec);
      const envelope = codec.encodeSuccessEnvelope(null);
      handler.reply(envelope);
      expect(result.successCalled).assertEqual(true);
    });

    // Regression test for the error envelope path of reply().
    it('calls error when reply is error envelope', Level.LEVEL1, () => {
      const result = new FakeResult();
      codec.errorCode = 'ERR_CODE';
      codec.errorMessage = 'something broke';
      codec.errorDetails = 'detail info';
      const handler = new IncomingSendableResultHandler(result, codec);
      const envelope = codec.encodeErrorEnvelope('ERR_CODE', 'something broke', 'detail info');
      handler.reply(envelope);
      expect(result.errorCalled).assertEqual(true);
      expect(result.errorCode).assertEqual('ERR_CODE');
      expect(result.errorMessage).assertEqual('something broke');
      expect(result.errorDetails).assertEqual('detail info');
    });
  });
}
