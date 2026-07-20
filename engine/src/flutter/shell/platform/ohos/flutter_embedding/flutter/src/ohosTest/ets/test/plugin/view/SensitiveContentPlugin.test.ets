import { describe, it, expect, Level } from '@ohos/hypium';
import {
  SENSITIVE_CONTENT_SENSITIVITY,
  NOT_SENSITIVE_CONTENT_SENSITIVITY,
} from '../../../../../main/ets/embedding/engine/systemchannels/SensitiveContentChannel';

// SensitiveContentPlugin constructor requires SensitiveContentChannel,
// which requires DartExecutor (NAPI dependency). Cannot construct plugin instance.
// Test only exported constants. Instance methods (isSupported, getContentSensitivity,
// destroy) require import mock to fake the channel — not yet available.

export default function sensitiveContentPluginTest() {
  describe('SensitiveContentPlugin.constants', () => {
    it('SENSITIVE_CONTENT_SENSITIVITY equals 1', Level.LEVEL0, () => {
      expect(SENSITIVE_CONTENT_SENSITIVITY).assertEqual(1);
    });

    it('NOT_SENSITIVE_CONTENT_SENSITIVITY equals 2', Level.LEVEL0, () => {
      expect(NOT_SENSITIVE_CONTENT_SENSITIVITY).assertEqual(2);
    });
  });
}
