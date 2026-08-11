import { describe, it, expect, Level } from '@ohos/hypium';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import SensitiveContentChannel, {
  SENSITIVE_CONTENT_SENSITIVITY,
  NOT_SENSITIVE_CONTENT_SENSITIVITY,
} from '../../../../../main/ets/embedding/engine/systemchannels/SensitiveContentChannel';
import SensitiveContentPlugin from '../../../../../main/ets/plugin/view/SensitiveContentPlugin';
import resourceManager from '@ohos.resourceManager';

function createPlugin(): SensitiveContentPlugin {
  const flutterNapi: FlutterNapi = new FlutterNapi();
  const context = getContext();
  const rm: resourceManager.ResourceManager = context.resourceManager;
  const dartExecutor: DartExecutor = new DartExecutor(flutterNapi, rm);
  const channel: SensitiveContentChannel = new SensitiveContentChannel(dartExecutor);
  return new SensitiveContentPlugin(channel);
}

export default function sensitiveContentPluginTest() {
  describe('SensitiveContentPlugin.construct', () => {
    it('constructsWithoutCrash', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      expect(plugin).not().assertNull();
    });
  });

  describe('SensitiveContentPlugin.isSupported', () => {
    it('returnsTrue', Level.LEVEL0, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      expect(plugin.isSupported()).assertEqual(true);
    });
  });

  describe('SensitiveContentPlugin.getContentSensitivity', () => {
    it('returnsNotSensitiveByDefault', Level.LEVEL0, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      expect(plugin.getContentSensitivity()).assertEqual(NOT_SENSITIVE_CONTENT_SENSITIVITY);
    });
  });

  describe('SensitiveContentPlugin.setContentSensitivity.earlyReturn', () => {
    it('returnsEarlyWhenSameAsDefault', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.setContentSensitivity(NOT_SENSITIVE_CONTENT_SENSITIVITY);
      expect(plugin.getContentSensitivity()).assertEqual(NOT_SENSITIVE_CONTENT_SENSITIVITY);
    });

    it('returnsEarlyForUnknownValue', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.setContentSensitivity(999);
      expect(plugin.getContentSensitivity()).assertEqual(NOT_SENSITIVE_CONTENT_SENSITIVITY);
    });
  });

  describe('SensitiveContentPlugin.destroy', () => {
    it('destroysWithoutCrash', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.destroy();
    });
  });
}
