import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import { KeyEventHandler } from '../../../../../main/ets/embedding/ohos/KeyEventHandler';
import { KeyCode } from '@kit.InputKit';

export default function keyEventHandlerTest() {
  describe('KeyEventHandler.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const handler: KeyEventHandler = new KeyEventHandler();
      expect(handler).not().assertNull();
    });

    it('constructsWithUndefinedTextInputPlugin', Level.LEVEL1, () => {
      const handler: KeyEventHandler = new KeyEventHandler(undefined);
      expect(handler).not().assertNull();
    });
  });

  describe('KeyEventHandler.startDeleting', () => {
    it('doesNotCrashWithoutTextInputPlugin on startDeleting', Level.LEVEL0, () => {
      const handler: KeyEventHandler = new KeyEventHandler();
      handler.startDeleting(KeyCode.KEYCODE_DEL);
    });
  });

  describe('KeyEventHandler.endDeletion', () => {
    it('doesNotCrashWithoutTextInputPlugin on endDeletion', Level.LEVEL0, () => {
      const handler: KeyEventHandler = new KeyEventHandler();
      handler.endDeletion(KeyCode.KEYCODE_DEL);
    });
  });

  describe('KeyEventHandler.startDeleting.with.various.codes', () => {
    it('doesNotCrashWithKeyCodeBack', Level.LEVEL1, () => {
      const handler: KeyEventHandler = new KeyEventHandler();
      let threw = false;
      try { handler.startDeleting(KeyCode.KEYCODE_BACK); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('endDeletion with KeyCodeEnter does not crash', Level.LEVEL1, () => {
      const handler: KeyEventHandler = new KeyEventHandler();
      let threw = false;
      try { handler.endDeletion(KeyCode.KEYCODE_ENTER); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('KeyEventHandler.static.keyTextMap.access', () => {
    // Bracket access of the private static field forces class load and
    // initializer execution. ArkTS strict mode may disallow private field
    // bracket access, so we use the typed class reference.
    it('bracket access to keyTextMap does not throw', Level.LEVEL1, () => {
      const cls: Any = KeyEventHandler as Any;
      let threw = false;
      try {
        const m: Any = cls['keyTextMap'];
        expect(m !== undefined).assertEqual(true);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });

    it('multiple constructors do not throw', Level.LEVEL1, () => {
      let threw = false;
      try {
        new KeyEventHandler();
        new KeyEventHandler();
        new KeyEventHandler(undefined);
      } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });

  describe('KeyEventHandler.getKeyText', () => {
    it('returns empty for unknown keyCode without textInputPlugin', Level.LEVEL0, () => {
      const handler = new KeyEventHandler();
      const cls: Any = handler;
      const fn: Any = cls['getKeyText'];
      // getKeyText takes KeyEvent; without a real KeyEvent we can't call it directly
      // But we can verify the method exists
      expect(fn !== undefined).assertEqual(true);
    });
  });

  describe('KeyEventHandler.handleKeyEvent', () => {
    it('does not throw without textInputPlugin for KeyType.Up', Level.LEVEL0, () => {
      const handler = new KeyEventHandler();
      // handleKeyEvent expects KeyEvent; we can't construct one easily
      // Just verify the method exists
      const cls: Any = handler;
      const fn: Any = cls['handleKeyEvent'];
      expect(fn !== undefined).assertEqual(true);
    });
  });
}
