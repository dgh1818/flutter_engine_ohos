/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import TouchEventProcessor from '../../../../../main/ets/embedding/ohos/TouchEventProcessor';
import Any from '../../../../../main/ets/plugin/common/Any';
import { CustomTouchEvent, CustomTouchObject } from '../../../../../main/ets/plugin/platform/CustomTouchEvent';
import display from '@ohos.display';

function makeTouch(x: number, y: number): CustomTouchObject {
  return new CustomTouchObject(
    0,  // type
    0,  // id
    x, y,  // displayX, displayY
    x, y,  // windowX, windowY
    x, y,  // screenX, screenY
    x, y   // x, y (local)
  );
}

function makeEvent(touches: Array<CustomTouchObject>, changed: Array<CustomTouchObject>): CustomTouchEvent {
  return new CustomTouchEvent(
    0,         // type
    touches,   // touches
    changed,   // changedTouches
    0,         // timestamp
    0,         // source
    0,         // pressure
    0,         // tiltX
    0,         // tiltY
    0          // sourceTool
  );
}

export default function touchEventProcessorTest() {
  describe('TouchEventProcessor.getInstance', () => {
    it('returns a non-null instance', Level.LEVEL0, () => {
      const instance = TouchEventProcessor.getInstance();
      expect(instance !== null).assertEqual(true);
    });

    it('returns the same instance on multiple calls (singleton)', Level.LEVEL0, () => {
      const a = TouchEventProcessor.getInstance();
      const b = TouchEventProcessor.getInstance();
      expect(a).assertEqual(b);
    });
  });

  describe('TouchEventProcessor.checkHitPlatformView', () => {
    it('returns true when point is inside the view bounds', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(0, 0, 100, 100, 50, 50);
      expect(r).assertEqual(true);
    });

    it('returns true when point is at the left edge', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(0, 0, 100, 100, 0, 50);
      expect(r).assertEqual(true);
    });

    it('returns true when point is at the right edge', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(0, 0, 100, 100, 100, 50);
      expect(r).assertEqual(true);
    });

    it('returns true when point is at the top edge', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(0, 0, 100, 100, 50, 0);
      expect(r).assertEqual(true);
    });

    it('returns true when point is at the bottom edge', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(0, 0, 100, 100, 50, 100);
      expect(r).assertEqual(true);
    });

    it('returns false when point is outside (left of view)', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(10, 10, 100, 100, 5, 50);
      expect(r).assertEqual(false);
    });

    it('returns false when point is outside (right of view)', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(10, 10, 100, 100, 200, 50);
      expect(r).assertEqual(false);
    });

    it('returns false when point is outside (above view)', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(10, 10, 100, 100, 50, 5);
      expect(r).assertEqual(false);
    });

    it('returns false when point is outside (below view)', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      const r = instance.checkHitPlatformView(10, 10, 100, 100, 50, 200);
      expect(r).assertEqual(false);
    });

    it('returns true with non-zero origin view', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      // view at (50, 50) with size 100x100
      const r = instance.checkHitPlatformView(50, 50, 100, 100, 100, 100);
      expect(r).assertEqual(true);
    });

    it('returns false for point just outside non-zero origin view', Level.LEVEL1, () => {
      const instance = TouchEventProcessor.getInstance();
      // view at (50, 50) with size 100x100 → bounds [50..150] x [50..150]
      // point (151, 100) → x=151 is just outside
      const r = instance.checkHitPlatformView(50, 50, 100, 100, 151, 100);
      expect(r).assertEqual(false);
    });
  });

  describe('TouchEventProcessor.applyAffineTransform.static', () => {
    it('returns early when affineTransform is undefined', Level.LEVEL1, () => {
      const touch = makeTouch(10, 20);
      const event = makeEvent(new Array<CustomTouchObject>(touch), new Array<CustomTouchObject>(touch));
      // Should not throw
      TouchEventProcessor.applyAffineTransform(event, undefined);
      // Touch x/y unchanged
      expect(touch.x).assertEqual(10);
      expect(touch.y).assertEqual(20);
    });

    it('returns early when affineTransform has fewer than 6 elements', Level.LEVEL1, () => {
      const touch = makeTouch(10, 20);
      const event = makeEvent(new Array<CustomTouchObject>(touch), new Array<CustomTouchObject>(touch));
      TouchEventProcessor.applyAffineTransform(event, [1, 0, 0, 0, 1]);
      expect(touch.x).assertEqual(10);
      expect(touch.y).assertEqual(20);
    });

    it('returns early when coefficients contain NaN', Level.LEVEL1, () => {
      const touch = makeTouch(10, 20);
      const event = makeEvent(new Array<CustomTouchObject>(touch), new Array<CustomTouchObject>(touch));
      TouchEventProcessor.applyAffineTransform(event, [NaN, 0, 0, 0, 1, 0]);
      expect(touch.x).assertEqual(10);
      expect(touch.y).assertEqual(20);
    });

    it('returns early when coefficients contain Infinity', Level.LEVEL1, () => {
      const touch = makeTouch(10, 20);
      const event = makeEvent(new Array<CustomTouchObject>(touch), new Array<CustomTouchObject>(touch));
      TouchEventProcessor.applyAffineTransform(event, [Infinity, 0, 0, 0, 1, 0]);
      expect(touch.x).assertEqual(10);
      expect(touch.y).assertEqual(20);
    });

    it('applies identity transform (no change)', Level.LEVEL1, () => {
      const touch = makeTouch(10, 20);
      const event = makeEvent(new Array<CustomTouchObject>(touch), new Array<CustomTouchObject>(touch));
      // Identity: scaleX=1, shearY=0, tx=0, shearX=0, scaleY=1, ty=0
      TouchEventProcessor.applyAffineTransform(event, [1, 0, 0, 0, 1, 0]);
      expect(touch.x).assertEqual(10);
      expect(touch.y).assertEqual(20);
    });

    it('applies translation transform', Level.LEVEL1, () => {
      const touch = makeTouch(10, 20);
      const event = makeEvent(new Array<CustomTouchObject>(touch), new Array<CustomTouchObject>(touch));
      // Translation: tx=5, ty=10 (with identity scale/shear)
      TouchEventProcessor.applyAffineTransform(event, [1, 0, 5, 0, 1, 10]);
      // x_new = invScaleX*x + invShearX*y + invTx = 1*10 + 0*20 + 5 = 15
      // y_new = invShearY*x + invScaleY*y + invTy = 0*10 + 1*20 + 10 = 30
      expect(touch.x).assertEqual(15);
      expect(touch.y).assertEqual(30);
    });

    it('applies scale transform', Level.LEVEL1, () => {
      const touch = makeTouch(10, 20);
      const event = makeEvent(new Array<CustomTouchObject>(touch), new Array<CustomTouchObject>(touch));
      // Scale: scaleX=2, scaleY=3
      TouchEventProcessor.applyAffineTransform(event, [2, 0, 0, 0, 3, 0]);
      expect(touch.x).assertEqual(20);
      expect(touch.y).assertEqual(60);
    });

    it('applies transform to all touch objects in touches and changedTouches', Level.LEVEL1, () => {
      const t1 = makeTouch(10, 20);
      const t2 = makeTouch(30, 40);
      const event = makeEvent(new Array<CustomTouchObject>(t1, t2), new Array<CustomTouchObject>(t1, t2));
      // Translation: tx=100, ty=0
      TouchEventProcessor.applyAffineTransform(event, [1, 0, 100, 0, 1, 0]);
      expect(t1.x).assertEqual(110);
      expect(t2.x).assertEqual(130);
    });
  });

  describe('TouchEventProcessor.getInstance.and.static.methods', () => {
    it('getInstance returns singleton', Level.LEVEL1, () => {
      const a = TouchEventProcessor.getInstance();
      const b = TouchEventProcessor.getInstance();
      expect(a === b).assertEqual(true);
    });

    it('postTouchEvent with empty FlutterViewList does not throw', Level.LEVEL1, () => {
      let threw = false;
      try { TouchEventProcessor.getInstance().postTouchEvent(new Array<string>()); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('postMouseEvent with empty FlutterViewList does not throw', Level.LEVEL1, () => {
      let threw = false;
      try { TouchEventProcessor.getInstance().postMouseEvent(new Array<string>()); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('postAxisEvent with empty FlutterViewList does not throw', Level.LEVEL1, () => {
      let threw = false;
      try { TouchEventProcessor.getInstance().postAxisEvent(new Array<string>()); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('CustomTouchObject with no touches returns empty recordCount', Level.LEVEL1, () => {
      const e = new CustomTouchEvent(0, new Array<CustomTouchObject>(), new Array<CustomTouchObject>(), 0, 0, 0, 0, 0, 0);
      expect(e.touches.length).assertEqual(0);
      expect(e.changedTouches.length).assertEqual(0);
    });

    it('CustomTouchObject with one touch', Level.LEVEL1, () => {
      const t = new CustomTouchObject(0, 1, 10, 20, 30, 40, 50, 60, 70, 80);
      const e = new CustomTouchEvent(0, new Array<CustomTouchObject>(t), new Array<CustomTouchObject>(t), 0, 0, 0, 0, 0, 0);
      expect(e.touches.length).assertEqual(1);
      expect(e.changedTouches.length).assertEqual(1);
    });
  });
  describe('TouchEventProcessor.constructCustomTouchEvent', () => {
    it('constructs touch event without throwing', Level.LEVEL0, () => {
      const proc = TouchEventProcessor.getInstance();
      let threw = false;
      try { proc.constructCustomTouchEvent([], 0, 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('constructs touch event with valid strings', Level.LEVEL0, () => {
      const proc = TouchEventProcessor.getInstance();
      let threw = false;
      let result: Any = null;
      try { result = proc.constructCustomTouchEvent(['1', '0', '0', '1', '0', '0', '0', '0', '0', '0', '0', '0', '0'], 0, 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('TouchEventProcessor.postTouchEvent', () => {
    it('does not throw with empty strings', Level.LEVEL0, () => {
      const proc = TouchEventProcessor.getInstance();
      let threw = false;
      try { proc.postTouchEvent([]); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('TouchEventProcessor.postMouseEvent', () => {
    it('does not throw with empty strings', Level.LEVEL0, () => {
      const proc = TouchEventProcessor.getInstance();
      let threw = false;
      try { proc.postMouseEvent([]); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('TouchEventProcessor.postAxisEvent', () => {
    it('does not throw with empty strings', Level.LEVEL0, () => {
      const proc = TouchEventProcessor.getInstance();
      let threw = false;
      try { proc.postAxisEvent([]); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('TouchEventProcessor.constructMouseEvent', () => {
    it('does not throw with empty strings', Level.LEVEL0, () => {
      const proc = TouchEventProcessor.getInstance();
      let threw = false;
      try { proc.constructMouseEvent([], 0, 0); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  // ===== Supplement (2026-08-29): packet decoding, dispatch paths and hit tests =====

  /**
   * Builds a single-point touch packet. Layout mirrors decodeTouchPacket:
   * numPoint, changed id/screenX/screenY/x/y/type/size/force/deviceId/timeStamp,
   * then per point id/x/y/localX/localY/type/size/force/deviceId/isPx,
   * then toolType/tiltX/tiltY.
   */
  const mkTouchStrings = (density: number, type: string): Array<string> => {
    const arr: Array<string> = [
      '1',                                  // numPoint
      '7',                                  // changed id
      String(100 * density),                // changed screenX (px)
      String(50 * density),                 // changed screenY (px)
      String((30 + 10) * density),          // changed x (px, left=10)
      String((40 + 20) * density),          // changed y (px, top=20)
      type,                                 // changed type
      '1.5',                                // changed size
      '0.6',                                // changed force
      '9',                                  // changed deviceId
      '12345',                              // changed timeStamp
      '7',                                  // point id
      String((25 + 10) * density),          // point x (px)
      String((35 + 20) * density),          // point y (px)
      String((25 + 10) * density),          // point local x (px, left=10)
      String((35 + 20) * density),          // point local y (px, top=20)
      type,                                 // point type
      '1.5',                                // point size
      '0.6',                                // point force
      '9',                                  // point deviceId
      '1',                                  // point isPx flag
      '2',                                  // toolType
      '3',                                  // tiltX
      '4'                                   // tiltY
    ];
    return arr;
  };

  describe('TouchEventProcessor.constructCustomTouchEvent', () => {
    it('parses a single-point packet into a CustomTouchEvent', Level.LEVEL0, () => {
      const density: number = display.getDefaultDisplaySync().densityPixels;
      const strings = mkTouchStrings(density, '0');
      const evt: CustomTouchEvent = TouchEventProcessor.getInstance().constructCustomTouchEvent(strings, 20, 10);
      expect(evt.type).assertEqual(0);
      expect(evt.changedTouches.length).assertEqual(1);
      expect(evt.touches.length).assertEqual(1);
      expect(evt.touches[0].id).assertEqual(7);
      expect(Math.abs(evt.touches[0].x - 25) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.touches[0].y - 35) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.pressure - 0.6) < 1e-6).assertEqual(true);
      expect(evt.tiltX).assertEqual(3);
      expect(evt.tiltY).assertEqual(4);
      expect(evt.sourceTool).assertEqual(2);
    });

    it('parses the non-px flag variant without changing local coordinates', Level.LEVEL0, () => {
      const density: number = display.getDefaultDisplaySync().densityPixels;
      const strings = mkTouchStrings(density, '1');
      strings[20] = '0';
      const evt: CustomTouchEvent = TouchEventProcessor.getInstance().constructCustomTouchEvent(strings, 20, 10);
      expect(evt.type).assertEqual(1);
      expect(Math.abs(evt.touches[0].x - 25) < 1e-6).assertEqual(true);
    });

    // On windowed devices (e.g. desktops) the packet's screenX/screenY
    // are window-relative (modal title bar origin) while x/y are
    // XComponent-relative, so the global fields must be derived from the
    // local x/y plus the view offset instead of the packet screenX/screenY.
    it('derives global coordinates from local x/y plus the view offset', Level.LEVEL0, () => {
      const density: number = display.getDefaultDisplaySync().densityPixels;
      const strings = mkTouchStrings(density, '0');
      const evt: CustomTouchEvent = TouchEventProcessor.getInstance().constructCustomTouchEvent(strings, 20, 10);
      // changed point: local (30, 40) + view offset (10, 20) = (40, 60)
      expect(Math.abs(evt.changedTouches[0].displayX - 40) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.changedTouches[0].displayY - 60) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.changedTouches[0].windowX - 40) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.changedTouches[0].windowY - 60) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.changedTouches[0].screenX - 40) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.changedTouches[0].screenY - 60) < 1e-6).assertEqual(true);
      // touch point: local (25, 35) + view offset (10, 20) = (35, 55)
      expect(Math.abs(evt.touches[0].displayX - 35) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.touches[0].displayY - 55) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.touches[0].windowX - 35) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.touches[0].windowY - 55) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.touches[0].screenX - 35) < 1e-6).assertEqual(true);
      expect(Math.abs(evt.touches[0].screenY - 55) < 1e-6).assertEqual(true);
    });
  });

}
