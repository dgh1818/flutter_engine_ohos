/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

import { ByteBuffer } from '../../util/ByteBuffer';
import FlutterException from './FlutterException';
import Any from './Any';
import MethodCall from './MethodCall';
import SendableMethodCodec from './SendableMethodCodec';
import SendableStandardMessageCodec from './SendableStandardMessageCodec';

/**
 * A {@link SendableMethodCodec} using the Flutter standard binary encoding.
 *
 * <p>This codec is guaranteed to be compatible with the corresponding <a
 * href="https://api.flutter.dev/flutter/services/StandardMethodCodec-class.html">StandardMethodCodec</a>
 * on the Dart side. These parts of the Flutter SDK are evolved synchronously.
 *
 * <p>Values supported as method arguments and result payloads are those supported by {@link
 * StandardMessageCodec}.
 */
@Sendable
export default class SendableStandardMethodCodec implements SendableMethodCodec {
  private static TAG: string = "SendableStandardMethodCodec";
  public static INSTANCE: SendableStandardMethodCodec = new SendableStandardMethodCodec(SendableStandardMessageCodec.INSTANCE);

  private messageCodec: SendableStandardMessageCodec;

  /** Creates a new method codec based on the specified message codec. */
  constructor(messageCodec: SendableStandardMessageCodec) {
    this.messageCodec = messageCodec;
  }

  encodeMethodCall(methodCall: MethodCall): ArrayBuffer {
    const stream = ByteBuffer.from(new ArrayBuffer(1024));
    this.messageCodec.writeValue(stream, methodCall.method);
    this.messageCodec.writeValue(stream, methodCall.args);
    return stream.buffer;
  }

  decodeMethodCall(methodCall: ArrayBuffer): MethodCall {
    const buffer = ByteBuffer.from(methodCall);
    const method: Any = this.messageCodec.readValue(buffer);
    const args: Any = this.messageCodec.readValue(buffer);
    if (typeof method == 'string' && !buffer.hasRemaining()) {
      return new MethodCall(method, args);
    }
    throw new Error("Method call corrupted");
  }

  encodeSuccessEnvelope(result: Any): ArrayBuffer {
    const stream = ByteBuffer.from(new ArrayBuffer(1024));
    stream.writeInt8(0);
    this.messageCodec.writeValue(stream, result);
    return stream.buffer;
  }

  encodeErrorEnvelope(errorCode: string, errorMessage: string, errorDetails: Any): ArrayBuffer {
    const stream = ByteBuffer.from(new ArrayBuffer(1024));
    stream.writeInt8(1);
    this.messageCodec.writeValue(stream, errorCode);
    this.messageCodec.writeValue(stream, errorMessage);
    if (errorDetails instanceof Error) {
      this.messageCodec.writeValue(stream, errorDetails.stack);
    } else {
      this.messageCodec.writeValue(stream, errorDetails);
    }
    return stream.buffer;
  }

  encodeErrorEnvelopeWithStacktrace(errorCode: string, errorMessage: string, errorDetails: Any, errorStacktrace: string): ArrayBuffer {
    const stream = ByteBuffer.from(new ArrayBuffer(1024));
    stream.writeInt8(1);
    this.messageCodec.writeValue(stream, errorCode);
    this.messageCodec.writeValue(stream, errorMessage);
    if (errorDetails instanceof Error) {
      this.messageCodec.writeValue(stream, errorDetails.stack);
    } else {
      this.messageCodec.writeValue(stream, errorDetails);
    }
    this.messageCodec.writeValue(stream, errorStacktrace);
    return stream.buffer;
  }

  decodeEnvelope(envelope: ArrayBuffer): Any {
    const buffer = ByteBuffer.from(envelope);
    const flag = buffer.readInt8();
    switch (flag) {
      case 0: {
        const result: Any = this.messageCodec.readValue(buffer);
        if (!buffer.hasRemaining()) {
          return result;
        }
        // Falls through intentionally.
      }
      case 1: {
        const code: Any = this.messageCodec.readValue(buffer);
        const message: Any = this.messageCodec.readValue(buffer);
        const details: Any = this.messageCodec.readValue(buffer);
        if (typeof code == 'string' && (message == null || typeof message == 'string') && !buffer.hasRemaining()) {
          throw new FlutterException(code, message, details);
        }
      }
    }
    throw new Error("Envelope corrupted");
  }
}