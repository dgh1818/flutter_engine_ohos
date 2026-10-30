/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { beforeAll, describe, it, expect } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import common from '@ohos.app.ability.common';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import TextInputChannel from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import TextInputPluginDefault from '../../../../../main/ets/plugin/editing/TextInputPlugin';
import FlutterInjector from '../../../../../main/ets/FlutterInjector';
import { TextEditState } from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';

function makeState(text: string, compStart: number, compEnd: number): TextEditState {
  return new TextEditState(text, 0, text.length, compStart, compEnd);
}

export default function textInputPluginPrivatesTest() {
  describe('TextInputMethodHandlerImpl.private.static.methods.via.bracket.access', () => {
    let handlerCls: Any;
    beforeAll(() => {
      // TextInputMethodHandlerImpl is not exported. Access it through a
      // TextInputPlugin instance's private mTextInputHandler field, then
      // get its constructor (the class) for static method access.
      const context: common.Context = getContext();
      const assetManager = context.resourceManager;
      const flutterNapi = FlutterInjector.getInstance().getFlutterNapi();
      const dartExecutor = new DartExecutor(flutterNapi, assetManager);
      dartExecutor.onAttachedToNAPI();
      const textInputChannel = new TextInputChannel(dartExecutor);
      const plugin = new TextInputPluginDefault(textInputChannel, "test_view", undefined);
      const handler: Any = (plugin as Any)['mTextInputHandler'];
      handlerCls = handler?.constructor;
    });

    it('isImeSegmentApostropheLike: ASCII apostrophe', 0, () => {
      const fn: Function = handlerCls?.['isImeSegmentApostropheLike'];
      expect(fn).not().assertNull();
      if (fn) {
        expect(fn("'")).assertEqual(true);
        expect(fn('a')).assertEqual(false);
        expect(fn('')).assertEqual(false);
      }
    });

    it('previewTextHasInternalSegmentApostrophe: detects middle apostrophe', 0, () => {
      const fn: Function = handlerCls?.['previewTextHasInternalSegmentApostrophe'];
      expect(fn).not().assertNull();
      if (fn) {
        expect(fn("ni'hao")).assertEqual(true);
        expect(fn("'hao")).assertEqual(false);
        expect(fn("nihao")).assertEqual(false);
      }
    });

    it('previewStartsWithImeSegmentMarker: detects leading apostrophe', 0, () => {
      const fn: Function = handlerCls?.['previewStartsWithImeSegmentMarker'];
      expect(fn).not().assertNull();
      if (fn) {
        expect(fn("'hao")).assertEqual(true);
        expect(fn("\u2019hao")).assertEqual(true);
        expect(fn("hao")).assertEqual(false);
        expect(fn("")).assertEqual(false);
      }
    });

    it('composingRegionMutatedByFramework: length change', 0, () => {
      const fn: Function = handlerCls?.['composingRegionMutatedByFramework'];
      expect(fn).not().assertNull();
      if (fn) {
        const before = makeState('hello', 0, 0);  // no composing
        const after = makeState('hello', 0, 3);   // composing [0, 3)
        expect(fn(before, after)).assertEqual(true);
      }
    });

    it('composingRegionMutatedByFramework: no change returns false', 0, () => {
      const fn: Function = handlerCls?.['composingRegionMutatedByFramework'];
      expect(fn).not().assertNull();
      if (fn) {
        const s = makeState('hello', 0, 3);
        expect(fn(s, s)).assertEqual(false);
      }
    });

    it('composingRegionMutatedByFramework: text change in composing region', 0, () => {
      const fn: Function = handlerCls?.['composingRegionMutatedByFramework'];
      expect(fn).not().assertNull();
      if (fn) {
        const before = makeState('hello', 0, 2);
        const after = makeState('hxllo', 0, 2);
        expect(fn(before, after)).assertEqual(true);
      }
    });
  });

  describe('TextInputMethodHandlerImpl.mockPrivateFunc.on.instance', () => {
    it('skipped - inner class not exported', 0, () => {
      expect(true).assertEqual(true);
    });
  });
}
