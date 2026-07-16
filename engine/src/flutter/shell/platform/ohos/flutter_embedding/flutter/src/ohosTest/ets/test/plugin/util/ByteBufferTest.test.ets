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
      byteBuffer.writeBool(false);
      let result = byteBuffer.readBool();
      expect(result).assertFalse();
      done();
    })

    it('TestByteBufferGetInt8', 0, async (done: Function) => {
      byteBuffer.setInt8(byteOffset, 10);
      let result = byteBuffer.getInt8(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadInt8', 0, async (done: Function) => {
      byteBuffer.writeInt8(byteOffset);
      let result = byteBuffer.readInt8();
      expect(result).assertEqual(0);
      done();
    })

    it('TestByteBufferGetUint8', 0, async (done: Function) => {
      byteBuffer.setUint8(byteOffset, 10);
      let result = byteBuffer.getUint8(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint8', 0, async (done: Function) => {
      byteBuffer.writeUint8(byteOffset);
      let result = byteBuffer.readUint8();
      expect(result).assertEqual(0);
      done();
    })

    it('TestByteBufferGetInt16', 0, async (done: Function) => {
      byteBuffer.setInt16(byteOffset, 10);
      let result = byteBuffer.getInt16(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadInt16', 0, async (done: Function) => {
      byteBuffer.writeInt16(byteOffset);
      let result = byteBuffer.readInt16();
      expect(result).assertEqual(0);
      done();
    })

    it('TestByteBufferGetUint16', 0, async (done: Function) => {
      byteBuffer.setUint16(byteOffset, 10);
      let result = byteBuffer.getUint16(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint16', 0, async (done: Function) => {
      byteBuffer.writeUint16(byteOffset);
      let result = byteBuffer.readUint16();
      expect(result).assertEqual(0);
      done();
    })

    it('TestByteBufferGetInt32', 0, async (done: Function) => {
      byteBuffer.setInt32(byteOffset, 10);
      let result = byteBuffer.getInt32(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadInt32', 0, async (done: Function) => {
      byteBuffer.writeInt32(byteOffset);
      let result = byteBuffer.readInt32();
      expect(result).assertEqual(0);
      done();
    })

    it('TestByteBufferGetUint32', 0, async (done: Function) => {
      byteBuffer.setUint32(byteOffset, 10);
      let result = byteBuffer.getUint32(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint32', 0, async (done: Function) => {
      byteBuffer.writeUint32(byteOffset);
      let result = byteBuffer.readUint32();
      expect(result).assertEqual(0);
      done();
    })

    it('TestByteBufferGetFloat32', 0, async (done: Function) => {
      byteBuffer.setFloat32(byteOffset, 10);
      let result = byteBuffer.getFloat32(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadFloat32', 0, async (done: Function) => {
      byteBuffer.writeFloat32(byteOffset);
      let result = byteBuffer.readFloat32();
      expect(result).assertEqual(0);
      done();
    })

    it('TestByteBufferGetFloat64', 0, async (done: Function) => {
      byteBuffer.setFloat64(byteOffset, 10);
      let result = byteBuffer.getFloat64(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadFloat64', 0, async (done: Function) => {
      byteBuffer.writeFloat64(byteOffset);
      let result = byteBuffer.readFloat64();
      expect(result).assertEqual(0);
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
      const value: bigint = BigInt(9223372036854775807);
      byteBuffer.writeBigInt64(value);
      let result = byteBuffer.readBigInt64();
      expect(result).assertEqual(0n);
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
      const value: bigint = BigInt(9223372036854775807);
      byteBuffer.writeBigUint64(value);
      let result = byteBuffer.readBigUint64();
      expect(result).assertEqual(0n);
      done();
    })

    it('TestByteBufferGetInt64', 0, async (done: Function) => {
      byteBuffer.setInt64(byteOffset, 10);
      let result = byteBuffer.getInt64(byteOffset);
      expect(result).assertEqual(10n);
      done();
    })

    it('TestByteBufferReadInt64', 0, async (done: Function) => {
      byteBuffer.writeInt64(10);
      let result = byteBuffer.readInt64();
      expect(result).assertEqual(0n);
      done();
    })

    it('TestByteBufferGetUint64', 0, async (done: Function) => {
      byteBuffer.setUint64(byteOffset, 10);
      let result = byteBuffer.getUint64(byteOffset);
      expect(result).assertEqual(10);
      done();
    })

    it('TestByteBufferReadUint64', 0, async (done: Function) => {
      byteBuffer.writeUint64(10);
      let result = byteBuffer.readUint64();
      expect(result).assertEqual(0);
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
      const value: Uint8Array = new Uint8Array();
      byteBuffer.writeUint8Array(value);
      let result = byteBuffer.readUint8Array();
      expect(result).assertInstanceOf('Uint8Array');
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
      const value: Uint16Array = new Uint16Array();
      byteBuffer.writeUint16Array(value);
      let result = byteBuffer.readUint16Array();
      expect(result).assertInstanceOf('Uint16Array');
      done();
    })

    it('TestByteBufferGetString', 0, async (done: Function) => {
      byteBuffer.setString(byteOffset, 'test');
      let result = byteBuffer.getString(byteOffset);
      expect(result).not().assertNull();
      done();
    })

    it('TestByteBufferReadString', 0, async (done: Function) => {
      byteBuffer.writeString('test');
      let result = byteBuffer.readString();
      expect(result).assertUndefined();
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
