import { describe, it, expect, Level } from '@ohos/hypium';
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
    it('doesNotCrashWithoutTextInputPlugin', Level.LEVEL0, () => {
      const handler: KeyEventHandler = new KeyEventHandler();
      handler.startDeleting(KeyCode.KEYCODE_DEL);
    });
  });

  describe('KeyEventHandler.endDeletion', () => {
    it('doesNotCrashWithoutTextInputPlugin', Level.LEVEL0, () => {
      const handler: KeyEventHandler = new KeyEventHandler();
      handler.endDeletion(KeyCode.KEYCODE_DEL);
    });
  });
}
