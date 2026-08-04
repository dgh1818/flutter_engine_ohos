/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
import { afterAll, describe, expect, it } from '@ohos/hypium';
import { buffer } from '@kit.ArkTS';
import { ByteBuffer } from '../../../../../main/ets/util/ByteBuffer';

let byteBuffer: ByteBuffer;
let byteOffset: number;

export default function ByteBufferTest() {
  describe('ByteBufferTest', () => {
    it('TestByteBufferFrom', 0, async (done: Function) => {
      const blobValue: buffer.Blob = new buffer.Blob(['name', 'age', 'sex']);
      const message: ArrayBuffer = await blobValue.arrayBuffer();
      const result = ByteBuffer.from(message);
      expect(result.byteLength).assertEqual(10);
      done();
    })

    it('TestByteBufferByteOffset', 0, async (done: Function) => {
      const blobValue: buffer.Blob = new buffer.Blob(['name', 'age', 'sex']);
      const message: ArrayBuffer = await blobValue.arrayBuffer();
      byteBuffer = ByteBuffer.from(message);
      byteOffset = byteBuffer.byteOffset;
      expect(byteOffset).assertEqual(0);
      done();
    })

    it('TestByteBufferByteLength', 0, async (done: Function) => {
      const result = byteBuffer.byteLength;
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferBytesRemaining', 0, async (done: Function) => {
      const result = byteBuffer.bytesRemaining;
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferBuffer', 0, async (done: Function) => {
      const result = byteBuffer.buffer;
      expect(result).assertInstanceOf('ArrayBuffer');
      done();
    })

    it('TestByteBufferHasRemaining', 0, async (done: Function) => {
      const result = byteBuffer.hasRemaining();
      expect(result).assertTrue();
      done();
    })

    it('TestByteBufferSkip', 0, async (done: Function) => {
      const result = byteBuffer.byteLength;
      expect(result).assertEqual(10);
      byteBuffer.skip(result);
      byteBuffer.checkWriteCapacity(10);
      done();
    })

    it('TestByteBufferGetBool', 0, async (done: Function) => {
      byteBuffer.setBool(byteOffset, true);
      let result = byteBuffer.getBool(byteOffset);
      expect(result).assertTrue();
      done();
    })

    it('TestByteBufferReadBool', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(8));
      buffer.writeBool(true);
      buffer.writeBool(false);
      buffer.reset();
      expect(buffer.readBool()).assertTrue();
      expect(buffer.readBool()).assertFalse();
      done();
    })

    it('TestByteBufferGetInt8', 0, async (done: Function) => {
      byteBuffer.setInt8(byteOffset, 10);
      let result = byteBuffer.getInt8(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadInt8', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(16));
      buffer.writeInt8(-128);
      buffer.writeInt8(127);
      buffer.writeInt8(0);
      buffer.reset();
      expect(buffer.readInt8()).assertEqual(-128);
      expect(buffer.readInt8()).assertEqual(127);
      expect(buffer.readInt8()).assertEqual(0);
      done();
    })

    it('TestByteBufferGetUint8', 0, async (done: Function) => {
      byteBuffer.setUint8(byteOffset, 10);
      let result = byteBuffer.getUint8(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint8', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(16));
      buffer.writeUint8(0);
      buffer.writeUint8(255);
      buffer.writeUint8(128);
      buffer.reset();
      expect(buffer.readUint8()).assertEqual(0);
      expect(buffer.readUint8()).assertEqual(255);
      expect(buffer.readUint8()).assertEqual(128);
      done();
    })

    it('TestByteBufferGetInt16', 0, async (done: Function) => {
      byteBuffer.setInt16(byteOffset, 10);
      let result = byteBuffer.getInt16(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadInt16', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(32));
      buffer.writeInt16(-32768);
      buffer.writeInt16(32767);
      buffer.writeInt16(0, true);
      buffer.reset();
      expect(buffer.readInt16()).assertEqual(-32768);
      expect(buffer.readInt16()).assertEqual(32767);
      expect(buffer.readInt16(true)).assertEqual(0);
      done();
    })

    it('TestByteBufferGetUint16', 0, async (done: Function) => {
      byteBuffer.setUint16(byteOffset, 10);
      let result = byteBuffer.getUint16(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint16', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(32));
      buffer.writeUint16(0);
      buffer.writeUint16(65535);
      buffer.writeUint16(32768, true);
      buffer.reset();
      expect(buffer.readUint16()).assertEqual(0);
      expect(buffer.readUint16()).assertEqual(65535);
      expect(buffer.readUint16(true)).assertEqual(32768);
      done();
    })

    it('TestByteBufferGetInt32', 0, async (done: Function) => {
      byteBuffer.setInt32(byteOffset, 10);
      let result = byteBuffer.getInt32(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadInt32', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(64));
      buffer.writeInt32(-2147483648);
      buffer.writeInt32(2147483647);
      buffer.writeInt32(0, true);
      buffer.reset();
      expect(buffer.readInt32()).assertEqual(-2147483648);
      expect(buffer.readInt32()).assertEqual(2147483647);
      expect(buffer.readInt32(true)).assertEqual(0);
      done();
    })

    it('TestByteBufferGetUint32', 0, async (done: Function) => {
      byteBuffer.setUint32(byteOffset, 10);
      let result = byteBuffer.getUint32(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint32', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(64));
      buffer.writeUint32(0);
      buffer.writeUint32(4294967295);
      buffer.writeUint32(2147483648, true);
      buffer.reset();
      expect(buffer.readUint32()).assertEqual(0);
      expect(buffer.readUint32()).assertEqual(4294967295);
      expect(buffer.readUint32(true)).assertEqual(2147483648);
      done();
    })

    it('TestByteBufferGetFloat32', 0, async (done: Function) => {
      byteBuffer.setFloat32(byteOffset, 10);
      let result = byteBuffer.getFloat32(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadFloat32', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(64));
      buffer.writeFloat32(1.5);
      buffer.writeFloat32(-2.25);
      buffer.writeFloat32(0.0, true);
      buffer.reset();
      expect(buffer.readFloat32()).assertEqual(1.5);
      expect(buffer.readFloat32()).assertEqual(-2.25);
      expect(buffer.readFloat32(true)).assertEqual(0.0);
      done();
    })

    it('TestByteBufferGetFloat64', 0, async (done: Function) => {
      byteBuffer.setFloat64(byteOffset, 10);
      let result = byteBuffer.getFloat64(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadFloat64', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(128));
      buffer.writeFloat64(3.141592653589793);
      buffer.writeFloat64(-2.718281828459045);
      buffer.writeFloat64(0.0, true);
      buffer.reset();
      expect(buffer.readFloat64()).assertEqual(3.141592653589793);
      expect(buffer.readFloat64()).assertEqual(-2.718281828459045);
      expect(buffer.readFloat64(true)).assertEqual(0.0);
      done();
    })

    it('TestByteBufferGetBigInt64', 0, async (done: Function) => {
      const value: bigint = BigInt(9223372036854775807);
      byteBuffer.setBigInt64(byteOffset, value);
      let result = byteBuffer.getBigInt64(byteOffset);
      expect(result).not().assertNull();
      done();
    })

    it('TestByteBufferReadBigInt64', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(128));
      const value1: bigint = BigInt("9223372036854775807");
      const value2: bigint = BigInt("-9223372036854775808");
      const value3: bigint = BigInt(0);
      buffer.writeBigInt64(value1);
      buffer.writeBigInt64(value2);
      buffer.writeBigInt64(value3, true);
      buffer.reset();
      expect(buffer.readBigInt64()).assertEqual(value1);
      expect(buffer.readBigInt64()).assertEqual(value2);
      expect(buffer.readBigInt64(true)).assertEqual(value3);
      done();
    })

    it('TestByteBufferGetBigUint64', 0, async (done: Function) => {
      const value: bigint = BigInt(9223372036854775807);
      byteBuffer.setBigUint64(byteOffset, value);
      let result = byteBuffer.getBigUint64(byteOffset);
      expect(result).not().assertNull();
      done();
    })

    it('TestByteBufferReadBigUint64', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(128));
      const value1: bigint = BigInt("9223372036854775807");
      const value2: bigint = BigInt("18446744073709551615");
      const value3: bigint = BigInt(0);
      buffer.writeBigUint64(value1);
      buffer.writeBigUint64(value2);
      buffer.writeBigUint64(value3, true);
      buffer.reset();
      expect(buffer.readBigUint64()).assertEqual(value1);
      expect(buffer.readBigUint64()).assertEqual(value2);
      expect(buffer.readBigUint64(true)).assertEqual(value3);
      done();
    })

    it('TestByteBufferGetInt64', 0, async (done: Function) => {
      byteBuffer.setInt64(byteOffset, 10);
      let result = byteBuffer.getInt64(byteOffset);
      expect(result).assertEqual(10n);
      done();
    })

    it('TestByteBufferReadInt64', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(128));
      buffer.writeInt64(9007199254740991);
      buffer.writeInt64(-9007199254740991);
      buffer.writeInt64(0, true);
      buffer.reset();
      expect(buffer.readInt64()).assertEqual(BigInt(9007199254740991));
      expect(buffer.readInt64()).assertEqual(BigInt(-9007199254740991));
      expect(buffer.readInt64(true)).assertEqual(BigInt(0));
      done();
    })

    it('TestByteBufferGetUint64', 0, async (done: Function) => {
      byteBuffer.setUint64(byteOffset, 10);
      let result = byteBuffer.getUint64(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint64', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(128));
      buffer.writeUint64(0);
      buffer.writeUint64(9007199254740991);
      buffer.writeUint64(123456789, true);
      buffer.reset();
      expect(buffer.readUint64()).assertEqual(0);
      expect(buffer.readUint64()).assertEqual(9007199254740991);
      expect(buffer.readUint64(true)).assertEqual(123456789);
      done();
    })

    it('TestByteBufferGetUint8Array', 0, async (done: Function) => {
      const value: Uint8Array = new Uint8Array();
      byteBuffer.setUint8Array(byteOffset, value);
      let result = byteBuffer.getUint8Array(byteOffset);
      expect(result).assertInstanceOf('Uint8Array');
      done();
    })

    it('TestByteBufferReadUint8Array', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(32));
      const value1: Uint8Array = new Uint8Array([1, 2, 3, 4, 5]);
      const value2: Uint8Array = new Uint8Array([255, 128, 0]);
      buffer.writeUint8Array(value1);
      buffer.writeUint8Array(value2);
      buffer.reset();
      const result1 = buffer.readUint8Array(5);
      expect(result1.length).assertEqual(5);
      expect(result1[0]).assertEqual(1);
      expect(result1[4]).assertEqual(5);
      const result2 = buffer.readUint8Array(3);
      expect(result2.length).assertEqual(3);
      expect(result2[0]).assertEqual(255);
      expect(result2[2]).assertEqual(0);
      done();
    })

    it('TestByteBufferGetUint16Array', 0, async (done: Function) => {
      const value: Uint16Array = new Uint16Array();
      byteBuffer.setUint16Array(byteOffset, value);
      let result = byteBuffer.getUint16Array(byteOffset);
      expect(result).assertInstanceOf('Uint16Array');
      done();
    })

    it('TestByteBufferReadUint16Array', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(64));
      const value1: Uint16Array = new Uint16Array([100, 200, 300]);
      const value2: Uint16Array = new Uint16Array([0, 65535, 32768]);
      buffer.writeUint16Array(value1);
      buffer.writeUint16Array(value2);
      buffer.reset();
      const result1 = buffer.readUint16Array(6);
      expect(result1.length).assertEqual(3);
      expect(result1[0]).assertEqual(100);
      expect(result1[2]).assertEqual(300);
      const result2 = buffer.readUint16Array(6);
      expect(result2.length).assertEqual(3);
      expect(result2[0]).assertEqual(0);
      expect(result2[1]).assertEqual(65535);
      done();
    })

    it('TestByteBufferGetString', 0, async (done: Function) => {
      byteBuffer.setString(byteOffset, 'test');
      let result = byteBuffer.getString(byteOffset);
      expect(result).not().assertNull();
      done();
    })

    it('TestByteBufferReadString', 0, async (done: Function) => {
      const buffer = ByteBuffer.from(new ArrayBuffer(64));
      buffer.writeString('hello');
      buffer.writeString('world');
      buffer.reset();
      expect(buffer.readString(5)).assertEqual('hello');
      expect(buffer.readString(5)).assertEqual('world');
      done();
    })

    it('TestByteBufferToString', 0, async (done: Function) => {
      let result = byteBuffer.toString();
      expect(result).not().assertNull();
      done();
    })

    afterAll(() => {
      byteBuffer.reset();
      byteBuffer.clear();
    })
  })

}
