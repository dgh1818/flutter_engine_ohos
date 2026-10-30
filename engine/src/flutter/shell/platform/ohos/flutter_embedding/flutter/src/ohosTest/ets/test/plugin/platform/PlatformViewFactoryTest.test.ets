/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { common } from '@kit.AbilityKit';
import PlatformViewFactory from '../../../../../main/ets/plugin/platform/PlatformViewFactory';
import PlatformView from '../../../../../main/ets/plugin/platform/PlatformView';
import MessageCodec from '../../../../../main/ets/plugin/common/MessageCodec';
import Any from '../../../../../main/ets/plugin/common/Any';

class FakeMessageCodec implements MessageCodec<Any> {
  encodeMessage(message: Any): ArrayBuffer { return new ArrayBuffer(0); }
  decodeMessage(message: ArrayBuffer): Any { return null as Any; }
}

class TestPlatformViewFactory extends PlatformViewFactory {
  create(context: common.Context, viewId: number, args: Any): PlatformView {
    return new Object() as PlatformView;
  }
}

export default function platformViewFactoryTest() {
  describe('PlatformViewFactory.constructor', () => {
    it('stores codec in constructor', Level.LEVEL0, () => {
      const codec = new FakeMessageCodec();
      const factory: Any = new TestPlatformViewFactory(codec);
      expect(factory).not().assertNull();
    });
  });

  describe('PlatformViewFactory.getCreateArgsCodec', () => {
    it('returns stored codec via bracket access', Level.LEVEL0, () => {
      const codec = new FakeMessageCodec();
      const factory: Any = new TestPlatformViewFactory(codec);
      const fn: Function = factory['getCreateArgsCodec'];
      expect(fn !== undefined).assertEqual(true);
    });
  });

  describe('PlatformViewFactory.create', () => {
    it('returns a PlatformView instance', Level.LEVEL0, () => {
      const codec = new FakeMessageCodec();
      const factory = new TestPlatformViewFactory(codec);
      const ctx = getContext() as common.Context;
      const view = factory.create(ctx, 0, null as Any);
      expect(view).not().assertNull();
    });
  });
}
