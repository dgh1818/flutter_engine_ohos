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

    it('appliesSensitiveLevel_doesNotCrash', Level.LEVEL1, () => {
      // SENSITIVE != the default NOT_SENSITIVE level, so the early-return guard
      // is skipped and the window privacy-mode toggle path executes.
      const plugin: SensitiveContentPlugin = createPlugin();
      let threw = false;
      try {
        plugin.setContentSensitivity(SENSITIVE_CONTENT_SENSITIVITY);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      // currentContentSensitivity is only assigned inside the async
      // getLastWindow -> setWindowPrivacyMode promise (SensitiveContentPlugin.ets:66-68),
      // so synchronously after the call the requested level must NOT be reflected yet.
      expect(plugin.getContentSensitivity()).assertEqual(NOT_SENSITIVE_CONTENT_SENSITIVITY);
    });

    it('returnsEarlyForUnknownValue', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.setContentSensitivity(999);
      // unknown values default to NOT_SENSITIVE_CONTENT_SENSITIVITY
      expect(plugin.getContentSensitivity()).assertEqual(NOT_SENSITIVE_CONTENT_SENSITIVITY);
    });
  });

  describe('SensitiveContentPlugin.setContentSensitivity.sensitive', () => {
    it('handlesSENSITIVE_CONTENT_SENSITIVITY (window NAPI may fail)', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      let threw = false;
      try {
        plugin.setContentSensitivity(SENSITIVE_CONTENT_SENSITIVITY);
      } catch (e) {
        threw = true;
      }
      // The plugin itself shouldn't throw - NAPI failures are caught internally
      expect(threw).assertEqual(false);
    });
  });

  describe('SensitiveContentPlugin.setContentSensitivity.transitions', () => {
    it('handlesSENSITIVE_then_NOT_SENSITIVE', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.setContentSensitivity(SENSITIVE_CONTENT_SENSITIVITY);
      // After the first call, currentContentSensitivity may or may not have
      // been updated (depends on NAPI). Just verify no throw on second call.
      let threw = false;
      try {
        plugin.setContentSensitivity(NOT_SENSITIVE_CONTENT_SENSITIVITY);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('handlesNOT_SENSITIVE_then_SENSITIVE', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.setContentSensitivity(NOT_SENSITIVE_CONTENT_SENSITIVITY);
      let threw = false;
      try {
        plugin.setContentSensitivity(SENSITIVE_CONTENT_SENSITIVITY);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('SensitiveContentPlugin.destroy', () => {
    it('destroysWithoutCrash', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.destroy();
    });

    it('canBeCalledMultipleTimes', Level.LEVEL1, () => {
      const plugin: SensitiveContentPlugin = createPlugin();
      plugin.destroy();
      let threw = false;
      try {
        plugin.destroy();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });
}
