/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import Any from '../../../../../main/ets/plugin/common/Any';
import StringCodec from '../../../../../main/ets/plugin/common/StringCodec';
import PlatformView from '../../../../../main/ets/plugin/platform/PlatformView';
import PlatformViewFactory from '../../../../../main/ets/plugin/platform/PlatformViewFactory';
import { decodePlatformViewCreationParams } from '../../../../../main/ets/plugin/platform/PlatformViewParamsUtil';
import { ByteBuffer } from '../../../../../main/ets/util/ByteBuffer';

class FakePlatformViewFactory extends PlatformViewFactory {
  constructor() {
    super(StringCodec.INSTANCE);
  }

  create(context: common.Context, viewId: number, args: Any): PlatformView {
    throw new Error('FakePlatformViewFactory.create should not be called');
  }
}

export default function PlatformViewParamsUtilTest() {
  describe('PlatformViewParamsUtil.decodePlatformViewCreationParams', () => {
    const factory: FakePlatformViewFactory = new FakePlatformViewFactory();

    it('null_params_returns_null', 0, () => {
      const decoded: Any = decodePlatformViewCreationParams(factory, null, 1);
      expect(decoded).assertNull();
    });

    it('empty_uint8array_returns_null', 0, () => {
      const emptyBytes: Uint8Array = new Uint8Array(0);
      const decoded: Any = decodePlatformViewCreationParams(factory, emptyBytes, 2);
      expect(decoded).assertNull();
    });

    it('empty_arraybuffer_returns_null', 0, () => {
      const emptyBuffer: ArrayBuffer = new ArrayBuffer(0);
      const decoded: Any = decodePlatformViewCreationParams(factory, emptyBuffer, 3);
      expect(decoded).assertNull();
    });

    it('empty_bytebuffer_returns_null', 0, () => {
      const emptyBuffer: ByteBuffer = ByteBuffer.from(new ArrayBuffer(0));
      const decoded: Any = decodePlatformViewCreationParams(factory, emptyBuffer, 4);
      expect(decoded).assertNull();
    });

    it('arraybuffer_decodes_string_codec_message', 0, () => {
      const encoded: ArrayBuffer = StringCodec.INSTANCE.encodeMessage('hcpp-args') as ArrayBuffer;
      const decoded: Any = decodePlatformViewCreationParams(factory, encoded, 5);
      expect(decoded).assertEqual('hcpp-args');
    });

    it('uint8array_decodes_string_codec_message', 0, () => {
      const encoded: ArrayBuffer = StringCodec.INSTANCE.encodeMessage('via-u8') as ArrayBuffer;
      const asBytes: Uint8Array = new Uint8Array(encoded);
      const decoded: Any = decodePlatformViewCreationParams(factory, asBytes, 6);
      expect(decoded).assertEqual('via-u8');
    });

    it('bytebuffer_decodes_string_codec_message', 0, () => {
      const encoded: ArrayBuffer = StringCodec.INSTANCE.encodeMessage('via-bb') as ArrayBuffer;
      const byteBuffer: ByteBuffer = ByteBuffer.from(encoded);
      const decoded: Any = decodePlatformViewCreationParams(factory, byteBuffer, 7);
      expect(decoded).assertEqual('via-bb');
    });

    it('uint8array_subarray_offset_still_decodes', 0, () => {
      const encoded: ArrayBuffer = StringCodec.INSTANCE.encodeMessage('offset') as ArrayBuffer;
      const padded: Uint8Array = new Uint8Array(encoded.byteLength + 4);
      padded.set(new Uint8Array(encoded), 2);
      const slice: Uint8Array = padded.subarray(2, 2 + encoded.byteLength);
      const decoded: Any = decodePlatformViewCreationParams(factory, slice, 8);
      expect(decoded).assertEqual('offset');
    });
  });
}
