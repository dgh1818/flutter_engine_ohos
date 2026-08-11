import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import MouseCursorChannel from '../../../../../main/ets/embedding/engine/systemchannels/MouseCursorChannel';
import MouseCursorPlugin from '../../../../../main/ets/plugin/mouse/MouseCursorPlugin';
import resourceManager from '@ohos.resourceManager';

function createPlugin(windowId: number): MouseCursorPlugin {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  const channel: MouseCursorChannel = new MouseCursorChannel(dartExecutor);
  return new MouseCursorPlugin(windowId, channel);
}

export default function mouseCursorPluginTest() {
  describe('MouseCursorPlugin.construct', () => {
    it('constructsWithValidWindowId', Level.LEVEL1, () => {
      const plugin: MouseCursorPlugin = createPlugin(0);
      expect(plugin).not().assertNull();
    });

    it('constructsWithNegativeWindowId', Level.LEVEL1, () => {
      const plugin: MouseCursorPlugin = createPlugin(-1);
      expect(plugin).not().assertNull();
    });
  });

  describe('MouseCursorPlugin.destroy', () => {
    it('destroysWithoutCrash', Level.LEVEL1, () => {
      const plugin: MouseCursorPlugin = createPlugin(0);
      plugin.destroy();
    });
  });

  describe('MouseCursorPlugin.activateSystemCursor.negativeWindowId', () => {
    it('returnsEarlyForNegativeWindowId', Level.LEVEL1, () => {
      const plugin: MouseCursorPlugin = createPlugin(-1);
      plugin.activateSystemCursor('click');
    });
  });

  describe('MouseCursorPlugin.activateSystemCursor.validWindowId', () => {
    it('doesNotCrashForKnownKind', Level.LEVEL1, () => {
      const plugin: MouseCursorPlugin = createPlugin(0);
      plugin.activateSystemCursor('basic');
    });

    it('doesNotCrashForUnknownKind', Level.LEVEL1, () => {
      const plugin: MouseCursorPlugin = createPlugin(0);
      plugin.activateSystemCursor('nonexistent_kind');
    });

    it('doesNotCrashForEmptyKind', Level.LEVEL1, () => {
      const plugin: MouseCursorPlugin = createPlugin(0);
      plugin.activateSystemCursor('');
    });
  });
}
