import { describe, it, expect, Level } from '@ohos/hypium';
import { CustomTouchEvent, CustomTouchObject } from '../../../../../main/ets/plugin/platform/CustomTouchEvent';

export default function customTouchEventTest() {
  describe('CustomTouchObject.constructor', () => {
    it('assigns all fields correctly', Level.LEVEL0, () => {
      const obj = new CustomTouchObject(0, 1, 10, 20, 30, 40, 50, 60, 70, 80);
      expect(obj.type).assertEqual(0);
      expect(obj.id).assertEqual(1);
      expect(obj.displayX).assertEqual(10);
      expect(obj.displayY).assertEqual(20);
      expect(obj.windowX).assertEqual(30);
      expect(obj.windowY).assertEqual(40);
      expect(obj.screenX).assertEqual(50);
      expect(obj.screenY).assertEqual(60);
      expect(obj.x).assertEqual(70);
      expect(obj.y).assertEqual(80);
    });

    it('handles zero values', Level.LEVEL0, () => {
      const obj = new CustomTouchObject(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      expect(obj.id).assertEqual(0);
      expect(obj.x).assertEqual(0);
      expect(obj.y).assertEqual(0);
    });
  });

  describe('CustomTouchEvent.constructor', () => {
    it('assigns all fields correctly', Level.LEVEL0, () => {
      const touches: CustomTouchObject[] = new Array<CustomTouchObject>();
      const changed: CustomTouchObject[] = new Array<CustomTouchObject>();
      const event = new CustomTouchEvent(0, touches, changed, 1000, 0, 0.5, 10, 20, 0);
      expect(event.type).assertEqual(0);
      expect(event.touches).assertEqual(touches);
      expect(event.changedTouches).assertEqual(changed);
      expect(event.timestamp).assertEqual(1000);
      expect(event.source).assertEqual(0);
      expect(event.pressure).assertEqual(0.5);
      expect(event.tiltX).assertEqual(10);
      expect(event.tiltY).assertEqual(20);
      expect(event.sourceTool).assertEqual(0);
    });

    it('has stopPropagation as a function', Level.LEVEL0, () => {
      const touches: CustomTouchObject[] = new Array<CustomTouchObject>();
      const changed: CustomTouchObject[] = new Array<CustomTouchObject>();
      const event = new CustomTouchEvent(0, touches, changed, 0, 0, 0, 0, 0, 0);
      let threw = false;
      try {
        event.stopPropagation();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('has preventDefault as a function', Level.LEVEL0, () => {
      const touches: CustomTouchObject[] = new Array<CustomTouchObject>();
      const changed: CustomTouchObject[] = new Array<CustomTouchObject>();
      const event = new CustomTouchEvent(0, touches, changed, 0, 0, 0, 0, 0, 0);
      let threw = false;
      try {
        event.preventDefault();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('initializes target field', Level.LEVEL0, () => {
      const touches: CustomTouchObject[] = new Array<CustomTouchObject>();
      const changed: CustomTouchObject[] = new Array<CustomTouchObject>();
      const event = new CustomTouchEvent(0, touches, changed, 0, 0, 0, 0, 0, 0);
      expect(event.target).not().assertNull();
    });

    it('stores touch objects in arrays', Level.LEVEL1, () => {
      const touch: CustomTouchObject = new CustomTouchObject(0, 1, 0, 0, 0, 0, 0, 0, 50, 60);
      const touches: CustomTouchObject[] = [touch];
      const changed: CustomTouchObject[] = [touch];
      const event = new CustomTouchEvent(0, touches, changed, 100, 0, 1.0, 0, 0, 0);
      expect(event.touches.length).assertEqual(1);
      expect(event.changedTouches.length).assertEqual(1);
      expect(event.touches[0].id).assertEqual(1);
      expect(event.touches[0].x).assertEqual(50);
    });
  });

  describe('CustomTouchEvent.getModifierKeyState', () => {
    it('throws not implemented', Level.LEVEL1, () => {
      const touches: CustomTouchObject[] = new Array<CustomTouchObject>();
      const changed: CustomTouchObject[] = new Array<CustomTouchObject>();
      const event = new CustomTouchEvent(0, touches, changed, 0, 0, 0, 0, 0, 0);
      let threw = false;
      try {
        event.getModifierKeyState(new Array<string>());
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });

  describe('CustomTouchEvent.getHistoricalPoints', () => {
    it('throws not implemented', Level.LEVEL1, () => {
      const touches: CustomTouchObject[] = new Array<CustomTouchObject>();
      const changed: CustomTouchObject[] = new Array<CustomTouchObject>();
      const event = new CustomTouchEvent(0, touches, changed, 0, 0, 0, 0, 0, 0);
      let threw = false;
      try {
        event.getHistoricalPoints();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(true);
    });
  });
}
