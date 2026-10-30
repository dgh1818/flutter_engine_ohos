import { describe, it, expect, Level } from '@ohos/hypium';
import MethodChannel, { IncomingMethodCallHandler, IncomingResultHandler, MethodCallHandler, MethodResult } from '../../../../../main/ets/plugin/common/MethodChannel';
import { BinaryMessenger, BinaryMessageHandler, BinaryReply, TaskQueue, TaskQueueOptions } from '../../../../../main/ets/plugin/common/BinaryMessenger';
import SendableBinaryMessageHandler from '../../../../../main/ets/plugin/common/SendableBinaryMessageHandler';
import MethodCodec from '../../../../../main/ets/plugin/common/MethodCodec';
import FlutterException from '../../../../../main/ets/plugin/common/FlutterException';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';
import Any from '../../../../../main/ets/plugin/common/Any';

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

class SuccessHandler implements MethodCallHandler {
  receivedMethod: string = '';
  resultValue: Any = null;
  onMethodCall(call: MethodCall, result: MethodResult): void {
    this.receivedMethod = call.method;
    result.success(this.resultValue);
  }
}

class ErrorHandler implements MethodCallHandler {
  errorCode: string = 'ERR_CODE';
  errorMessage: string = 'error happened';
  errorDetails: Any = null;
  onMethodCall(call: MethodCall, result: MethodResult): void {
    result.error(this.errorCode, this.errorMessage, this.errorDetails);
  }
}

class NotImplementedHandler implements MethodCallHandler {
  onMethodCall(call: MethodCall, result: MethodResult): void {
    result.notImplemented();
  }
}

class ThrowingHandler implements MethodCallHandler {
  onMethodCall(call: MethodCall, result: MethodResult): void {
    throw new Error('handler exception');
  }
}

class FlutterThrowingHandler implements MethodCallHandler {
  onMethodCall(call: MethodCall, result: MethodResult): void {
    throw new FlutterException('invalid_args', 'bad argument', 'extra');
  }
}

class FakeMethodCodec implements MethodCodec {
  private successResult: Any = null;
  private errorCodec: string = '';
  private errorMessage: string = '';
  private errorDetails: Any = null;
  capturedErrMsg: string = '';

  setSuccessResult(value: Any): void {
    this.successResult = value;
  }

  setErrorInfo(code: string, message: string, details: Any): void {
    this.errorCodec = code;
    this.errorMessage = message;
    this.errorDetails = details;
  }

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
    const buffer: ArrayBuffer = new ArrayBuffer(1);
    const view: Uint8Array = new Uint8Array(buffer);
    view[0] = 1;
    return buffer;
  }

  decodeEnvelope(envelope: ArrayBuffer): Any {
    const view: Uint8Array = new Uint8Array(envelope);
    if (view[0] === 0) {
      return this.successResult;
    }
    throw new FlutterException(this.errorCodec, this.errorMessage, this.errorDetails);
  }
}

export default function methodChannelTest() {
  const codec = new FakeMethodCodec();

  describe('IncomingMethodCallHandler.onMessage', () => {
    it('decodes method call and passes to handler', Level.LEVEL1, () => {
      const handler = new SuccessHandler();
      handler.resultValue = 'ok';
      const wrapper = new IncomingMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('testMethod', null));
      wrapper.onMessage(message, reply);
      expect(handler.receivedMethod).assertEqual('testMethod');
    });

    it('replies with success envelope when handler calls success', Level.LEVEL1, () => {
      const handler = new SuccessHandler();
      handler.resultValue = 'success result';
      codec.setSuccessResult('success result');
      const wrapper = new IncomingMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('test', null));
      wrapper.onMessage(message, reply);
      expect(reply.replyData).not().assertNull();
      const decoded: Any = codec.decodeEnvelope(reply.replyData!);
      expect(decoded).assertEqual('success result');
    });

    it('replies with error envelope when handler calls error', Level.LEVEL1, () => {
      const handler = new ErrorHandler();
      handler.errorCode = 'E001';
      handler.errorMessage = 'something broke';
      handler.errorDetails = 'detail info';
      const wrapper = new IncomingMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('test', null));
      wrapper.onMessage(message, reply);
      expect(reply.replyData).not().assertNull();
    });

    it('replies with null when handler calls notImplemented', Level.LEVEL1, () => {
      const handler = new NotImplementedHandler();
      const wrapper = new IncomingMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('test', null));
      wrapper.onMessage(message, reply);
      expect(reply.replyData).assertNull();
    });

    it('replies with error envelope when handler throws exception', Level.LEVEL1, () => {
      const handler = new ThrowingHandler();
      const wrapper = new IncomingMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('test', null));
      wrapper.onMessage(message, reply);
      expect(reply.replyData).not().assertNull();
    });

    // Regression test: a handler throwing FlutterException must not degrade
    // the error message to "[object Object]".
    it('replies with real message when handler throws FlutterException', Level.LEVEL1, () => {
      const handler = new FlutterThrowingHandler();
      const wrapper = new IncomingMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      wrapper.onMessage(codec.encodeMethodCall(new MethodCall('test', null)), reply);
      expect(reply.replyData).not().assertNull();
      expect(new Uint8Array(reply.replyData!)[0]).assertEqual(1);
      expect(codec.capturedErrMsg).assertEqual('bad argument');
    });

    it('passes correct method name to handler', Level.LEVEL1, () => {
      const handler = new SuccessHandler();
      handler.resultValue = null;
      const wrapper = new IncomingMethodCallHandler(handler, codec);
      const reply = new FakeReply();
      const message = codec.encodeMethodCall(new MethodCall('myMethod', null));
      wrapper.onMessage(message, reply);
      expect(handler.receivedMethod).assertEqual('myMethod');
    });
  });

  describe('IncomingResultHandler.reply', () => {
    it('calls notImplemented when reply is null', Level.LEVEL0, () => {
      const result = new FakeResult();
      const handler = new IncomingResultHandler(result, codec);
      handler.reply(null);
      expect(result.notImplCalled).assertEqual(true);
    });

    it('calls success when reply is success envelope', Level.LEVEL1, () => {
      const result = new FakeResult();
      codec.setSuccessResult('result value');
      const handler = new IncomingResultHandler(result, codec);
      const envelope = codec.encodeSuccessEnvelope(null);
      handler.reply(envelope);
      expect(result.successCalled).assertEqual(true);
      expect(result.successValue).assertEqual('result value');
    });

    // Regression test for the e.getMessage() bug previously documented here.
    it('calls error when reply is error envelope', Level.LEVEL1, () => {
      const result = new FakeResult();
      codec.setErrorInfo('ERR_CODE', 'something broke', 'detail info');
      const handler = new IncomingResultHandler(result, codec);
      const envelope = codec.encodeErrorEnvelope('ERR_CODE', 'something broke', 'detail info');
      handler.reply(envelope);
      expect(result.errorCalled).assertEqual(true);
      expect(result.errorCode).assertEqual('ERR_CODE');
      expect(result.errorMessage).assertEqual('something broke');
      expect(result.errorDetails).assertEqual('detail info');
    });
  });

  describe('MethodChannel.public_API', () => {
    it('constructor stores name and codec', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new MethodChannel(messenger, 'myChannel', codec);
      channel.invokeMethod('test', null);
      expect(messenger.sendChannel).assertEqual('myChannel');
    });

    it('invokeMethod passes null callback when no callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new MethodChannel(messenger, 'c1', codec);
      channel.invokeMethod('m', 'arg', undefined);
      expect(messenger.capturedReplyCallback).assertEqual(null);
    });

    it('invokeMethod wraps callback in IncomingResultHandler when callback provided', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new MethodChannel(messenger, 'c2', codec);
      const cb: MethodResult = new FakeResult();
      channel.invokeMethod('m', 'arg', cb);
      expect(messenger.capturedReplyCallback).not().assertEqual(null);
      expect(messenger.capturedReplyCallback).not().assertEqual(cb);
    });

    it('setMethodCallHandler delegates to messenger with correct channel', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new MethodChannel(messenger, 'c3', codec);
      channel.setMethodCallHandler(new SuccessHandler());
      expect(messenger.handlerChannel).assertEqual('c3');
      expect(messenger.capturedHandler).not().assertEqual(null);
    });

    it('setMethodCallHandler(null) deregisters', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new MethodChannel(messenger, 'c4', codec);
      channel.setMethodCallHandler(null);
      expect(messenger.capturedHandler).assertEqual(null);
    });

    it('resizeChannelBuffer does not throw under FakeMessenger', Level.LEVEL1, () => {
      const messenger = new FakeMessenger();
      const channel = new MethodChannel(messenger, 'c5', codec);
      let threw = false;
      try {
        channel.resizeChannelBuffer(2048);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });
}
