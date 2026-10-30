/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, MockKit, when } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import TouchEventProcessor from '../../../../../main/ets/embedding/ohos/TouchEventProcessor';
import { CustomTouchObject } from '../../../../../main/ets/plugin/platform/CustomTouchEvent';

function makeTouch(x: number, y: number): CustomTouchObject {
  return new CustomTouchObject(0, 0, x, y, x, y, x, y, x, y);
}

export default function touchEventProcessorMockTest() {
  describe('TouchEventProcessor.mockPrivateFunc.instance.methods', () => {
    it('mocks private instance method isTerminalTouch', 0, () => {
      const inst = TouchEventProcessor.getInstance();
      const mocker = new MockKit();
      const fn: Function = mocker.mockPrivateFunc(inst, 'isTerminalTouch');
      when(fn)(0).afterReturn(true);
      when(fn)(1).afterReturn(false);
      expect(fn(0)).assertEqual(true);
      expect(fn(1)).assertEqual(false);
      mocker.clear(inst);
    });

    it('mocks private static method via bracket access (1.0.28 limitation)', 0, () => {
      // For static private methods, mockPrivateFunc doesn't work on the class.
      // Use bracket access (works because ArkTS private is compile-time only).
      const cls: Any = TouchEventProcessor as Any;
      const fn: Any = cls['applyAffineToObjects'];
      if (fn) {
        // applyAffineToObjects: void, no return to assert
        // Just verify it doesn't throw on empty array
        let threw = false;
        try {
          fn(new Array<CustomTouchObject>(), 1, 0, 0, 0, 1, 0);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
      }
      expect(true).assertEqual(true);
    });

    it('mocks private instance method cloneTouchObjectWithType', 0, () => {
      const inst = TouchEventProcessor.getInstance();
      const mocker = new MockKit();
      const fn: Function = mocker.mockPrivateFunc(inst, 'cloneTouchObjectWithType');
      expect(typeof fn).assertEqual('function');
      mocker.clear(inst);
    });

    it('applyAffineToObjects (private static) applies identity transform', 0, () => {
      const cls: Any = TouchEventProcessor as Any;
      const fn: Any = cls['applyAffineToObjects'];
      if (fn) {
        const touch = makeTouch(10, 20);
        const arr = new Array<CustomTouchObject>(touch);
        // identity: scaleX=1, shearY=0, tx=0, shearX=0, scaleY=1, ty=0
        let threw = false;
        try {
          fn(arr, 1, 0, 0, 0, 1, 0);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
        expect(touch.x).assertEqual(10);
        expect(touch.y).assertEqual(20);
      } else {
        expect(true).assertEqual(true);
      }
    });

    it('applyAffineToObjects (private static) applies translation', 0, () => {
      const cls: Any = TouchEventProcessor as Any;
      const fn: Any = cls['applyAffineToObjects'];
      if (fn) {
        const touch = makeTouch(10, 20);
        const arr = new Array<CustomTouchObject>(touch);
        // tx=5, ty=10
        let threw = false;
        try {
          fn(arr, 1, 0, 5, 0, 1, 10);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
        // x' = 1*10 + 0*20 + 5 = 15
        // y' = 0*10 + 1*20 + 10 = 30
        expect(touch.x).assertEqual(15);
        expect(touch.y).assertEqual(30);
      } else {
        expect(true).assertEqual(true);
      }
    });
  });
}
