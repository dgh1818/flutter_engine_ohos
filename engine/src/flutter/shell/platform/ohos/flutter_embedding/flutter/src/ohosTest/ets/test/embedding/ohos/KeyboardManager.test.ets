import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import KeyboardManager from '../../../../../main/ets/embedding/ohos/KeyboardManager';
import TextInputPlugin from '../../../../../main/ets/plugin/editing/TextInputPlugin';
import TextInputChannel from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import resourceManager from '@ohos.resourceManager';
import { common } from '@kit.AbilityKit';

function createKeyboardManager(): KeyboardManager {
  const context: common.Context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  const engine: FlutterEngine = new FlutterEngine(context, null, flutterNapi, null);
  const textInputChannel: TextInputChannel = new TextInputChannel(dartExecutor);
  const textInputPlugin: TextInputPlugin = new TextInputPlugin(textInputChannel, '0');
  return new KeyboardManager(engine, textInputPlugin);
}

export default function keyboardManagerTest() {
  describe('KeyboardManager.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const manager: KeyboardManager = createKeyboardManager();
      expect(manager).not().assertNull();
    });
  });

  describe('KeyboardManager.getKeyboardState', () => {
    it('returnsEmptyMapInitially', Level.LEVEL1, () => {
      const manager: KeyboardManager = createKeyboardManager();
      const state: Map<number, number> = manager.getKeyboardState();
      expect(state).not().assertNull();
      expect(state.size).assertEqual(0);
    });
  });
}
