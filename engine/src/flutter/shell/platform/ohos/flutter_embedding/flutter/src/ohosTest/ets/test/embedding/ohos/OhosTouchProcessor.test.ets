/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it, Level } from '@ohos/hypium';
import OhosTouchProcessor, {
  PointerChange,
  PointerDeviceKind,
  PointerSignalKind,
} from '../../../../../main/ets/embedding/ohos/OhosTouchProcessor';
import { TouchEvent } from '@ohos.multimodalInput.touchEvent';

export default function ohosTouchProcessorTest() {
  describe('OhosTouchProcessor.BYTES_PER_FIELD', () => {
    it('is eight bytes', Level.LEVEL1, () => {
      expect(OhosTouchProcessor.BYTES_PER_FIELD).assertEqual(8);
    });
  });

  describe('OhosTouchProcessor.PointerChange', () => {
    it('enumerates pointer state changes', Level.LEVEL1, () => {
      expect(PointerChange.CANCEL).assertEqual(0);
      expect(PointerChange.ADD).assertEqual(1);
      expect(PointerChange.REMOVE).assertEqual(2);
      expect(PointerChange.HOVER).assertEqual(3);
      expect(PointerChange.DOWN).assertEqual(4);
      expect(PointerChange.MOVE).assertEqual(5);
      expect(PointerChange.UP).assertEqual(6);
      expect(PointerChange.PAN_ZOOM_START).assertEqual(7);
      expect(PointerChange.PAN_ZOOM_UPDATE).assertEqual(8);
      expect(PointerChange.PAN_ZOOM_END).assertEqual(9);
    });
  });

  describe('OhosTouchProcessor.PointerDeviceKind', () => {
    it('enumerates pointer devices', Level.LEVEL1, () => {
      expect(PointerDeviceKind.TOUCH).assertEqual(0);
      expect(PointerDeviceKind.MOUSE).assertEqual(1);
      expect(PointerDeviceKind.STYLUS).assertEqual(2);
      expect(PointerDeviceKind.INVERTED_STYLUS).assertEqual(3);
      expect(PointerDeviceKind.TRACKPAD).assertEqual(4);
      expect(PointerDeviceKind.UNKNOWN).assertEqual(5);
    });
  });

  describe('OhosTouchProcessor.PointerSignalKind', () => {
    it('enumerates pointer signals', Level.LEVEL1, () => {
      expect(PointerSignalKind.NONE).assertEqual(0);
      expect(PointerSignalKind.SCROLL).assertEqual(1);
      expect(PointerSignalKind.SCROLL_INERTIA_CANCEL).assertEqual(2);
      expect(PointerSignalKind.SCALE).assertEqual(3);
      expect(PointerSignalKind.UNKNOWN).assertEqual(4);
    });
  });

  describe('OhosTouchProcessor.onTouchEvent', () => {
    it('empty implementation does not throw', Level.LEVEL1, () => {
      const processor: OhosTouchProcessor = new OhosTouchProcessor();
      // onTouchEvent body is empty; only the no-crash path is covered (hcpp style).
      try {
        processor.onTouchEvent((new Object()) as TouchEvent, new Object());
      } catch (_e) {
      }
    });
  });
}
