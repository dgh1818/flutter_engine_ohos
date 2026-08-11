import {
  FlutterAbility,
  FlutterEngine,
  BasicMessageChannel,
  MethodChannel,
  MethodResult,
  MethodCall,
  JSONMethodCodec,
  StandardMethodCodec,
  StandardMessageCodec,
  MethodCallHandler,
  BinaryCodec,
  StringCodec,
  JSONMessageCodec,
  MessageCodec,
  Reply,
} from '@ohos/flutter_ohos';
import { ByteBuffer } from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';

const DATE_TYPE: number = 128;
const PAIR_TYPE: number = 129;

class Pair {
  left: Object | null = null;
  right: Object | null = null;
}

class ExtendedStandardMessageCodec extends StandardMessageCodec {
  static INSTANCE: ExtendedStandardMessageCodec = new ExtendedStandardMessageCodec();

  writeValue(stream: ByteBuffer, value: Object | null): ByteBuffer {
    if (value instanceof Date) {
      stream.writeInt8(DATE_TYPE);
      stream.writeInt64(value.getTime(), true);
      return stream;
    }
    if (value instanceof Pair) {
      stream.writeInt8(PAIR_TYPE);
      this.writeValue(stream, value.left);
      this.writeValue(stream, value.right);
      return stream;
    }
    return super.writeValue(stream, value);
  }

  readValueOfType(type: number, buffer: ByteBuffer): Object | null {
    if (type === DATE_TYPE) {
      return new Date(Number(buffer.readInt64(true)));
    }
    if (type === PAIR_TYPE) {
      let p = new Pair();
      p.left = super.readValue(buffer);
      p.right = super.readValue(buffer);
      return p;
    }
    return super.readValueOfType(type, buffer);
  }
}

class MessageHandshake<T> {
  private channel: BasicMessageChannel<T>;

  constructor(channel: BasicMessageChannel<T>) {
    this.channel = channel;
  }

  onMessage(message: T, reply: Reply<T>): void {
    let messageEcho: T = this.echo(message);
    this.channel.send(messageEcho, (replyMessage: T): void => {
      this.channel.send(this.echo(replyMessage));
      reply.reply(messageEcho);
    });
  }

  echo(message: T): T {
    if (message instanceof ArrayBuffer) {
      let src = new Uint8Array(message);
      let copy = new ArrayBuffer(src.byteLength);
      new Uint8Array(copy).set(src);
      return copy as T;
    }
    return message;
  }
}

class SuccessMethodResult implements MethodResult {
  private channel: MethodChannel;
  private originalArgs: Object | null;
  private finalResult: MethodResult;

  constructor(channel: MethodChannel, originalArgs: Object | null, finalResult: MethodResult) {
    this.channel = channel;
    this.originalArgs = originalArgs;
    this.finalResult = finalResult;
  }

  success(o: Object | null): void {
    this.channel.invokeMethod('success', o);
    this.finalResult.success(this.originalArgs);
  }

  error(code: string, message: string, details: Object | null): void {
    throw new Error('Should not be called');
  }

  notImplemented(): void {
    throw new Error('Should not be called');
  }
}

class ErrorMethodResult implements MethodResult {
  private channel: MethodChannel;
  private originalArgs: Object | null;
  private finalResult: MethodResult;

  constructor(channel: MethodChannel, originalArgs: Object | null, finalResult: MethodResult) {
    this.channel = channel;
    this.originalArgs = originalArgs;
    this.finalResult = finalResult;
  }

  success(o: Object | null): void {
    throw new Error('Should not be called');
  }

  error(code: string, message: string, details: Object | null): void {
    this.channel.invokeMethod('error', details);
    this.finalResult.error(code, message, this.originalArgs);
  }

  notImplemented(): void {
    throw new Error('Should not be called');
  }
}

class NotImplementedMethodResult implements MethodResult {
  private channel: MethodChannel;
  private finalResult: MethodResult;

  constructor(channel: MethodChannel, finalResult: MethodResult) {
    this.channel = channel;
    this.finalResult = finalResult;
  }

  success(o: Object | null): void {
    throw new Error('Should not be called');
  }

  error(code: string, message: string, details: Object | null): void {
    throw new Error('Should not be called');
  }

  notImplemented(): void {
    this.channel.invokeMethod('notImplemented', null);
    this.finalResult.notImplemented();
  }
}

class MethodCallForwarder implements MethodCallHandler {
  private channel: MethodChannel;

  constructor(channel: MethodChannel) {
    this.channel = channel;
  }

  onMethodCall(call: MethodCall, result: MethodResult): void {
    if (call.method === 'success') {
      this.channel.invokeMethod(call.method, call.args, new SuccessMethodResult(this.channel, call.args, result));
    } else if (call.method === 'error') {
      this.channel.invokeMethod(call.method, call.args, new ErrorMethodResult(this.channel, call.args, result));
    } else {
      this.channel.invokeMethod(call.method, call.args, new NotImplementedMethodResult(this.channel, result));
    }
  }
}

export default class EntryAbility extends FlutterAbility {
  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine);
    GeneratedPluginRegistrant.registerWith(flutterEngine);

    let messenger = flutterEngine.getDartExecutor();

    this.setupMessageHandshake(new BasicMessageChannel(messenger, 'binary-msg', new BinaryCodec(false)));
    this.setupMessageHandshake(new BasicMessageChannel(messenger, 'string-msg', StringCodec.INSTANCE));
    this.setupMessageHandshake(new BasicMessageChannel(messenger, 'json-msg', JSONMessageCodec.INSTANCE));
    this.setupMessageHandshake(new BasicMessageChannel(messenger, 'std-msg', ExtendedStandardMessageCodec.INSTANCE));

    let jsonMethodChannel = new MethodChannel(messenger, 'json-method', JSONMethodCodec.INSTANCE);
    let stdMethodChannel = new MethodChannel(messenger, 'std-method', new StandardMethodCodec(ExtendedStandardMessageCodec.INSTANCE));

    jsonMethodChannel.setMethodCallHandler(new MethodCallForwarder(jsonMethodChannel));
    stdMethodChannel.setMethodCallHandler(new MethodCallForwarder(stdMethodChannel));

    let echoChannel: BasicMessageChannel<Object> = new BasicMessageChannel(messenger, 'std-echo', ExtendedStandardMessageCodec.INSTANCE);
    echoChannel.setMessageHandler({
      onMessage(message: Object, reply: Reply<Object>): void {
        reply.reply(message);
      }
    });
  }

  setupMessageHandshake<T>(channel: BasicMessageChannel<T>): void {
    let handler = new MessageHandshake<T>(channel);
    channel.setMessageHandler({
      onMessage(message: T, reply: Reply<T>): void {
        handler.onMessage(message, reply);
      }
    });
  }
}
