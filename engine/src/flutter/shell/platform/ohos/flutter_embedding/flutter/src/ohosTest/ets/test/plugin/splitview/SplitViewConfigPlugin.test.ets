import { describe, it, expect, Level } from '@ohos/hypium';
import SplitViewConfigPlugin from '../../../../../main/ets/plugin/splitview/SplitViewConfigPlugin';
import MethodCall from '../../../../../main/ets/plugin/common/MethodCall';
import { MethodResult } from '../../../../../main/ets/plugin/common/MethodChannel';
import Any from '../../../../../main/ets/plugin/common/Any';

class FakeResult implements MethodResult {
  successCalled: boolean = false;
  successValue: Any = null;
  notImplCalled: boolean = false;

  success(result: Any): void {
    this.successCalled = true;
    this.successValue = result;
  }
  error(errorCode: string, errorMessage: string, errorDetails: Any): void {}
  notImplemented(): void {
    this.notImplCalled = true;
  }
}

export default function splitViewConfigPluginTest() {
  describe('SplitViewConfigPlugin.getUniqueClassName', () => {
    it('returns correct class name', Level.LEVEL0, () => {
      const plugin = new SplitViewConfigPlugin();
      expect(plugin.getUniqueClassName()).assertEqual('SplitViewConfigPlugin');
    });
  });

  describe('SplitViewConfigPlugin.getAppIconBase64', () => {
    it('returns empty string when context is null', Level.LEVEL0, () => {
      const plugin = new SplitViewConfigPlugin();
      expect(plugin.getAppIconBase64()).assertEqual('');
    });
  });

  describe('SplitViewConfigPlugin.onMethodCall', () => {
    it('calls success with empty config when context is null', Level.LEVEL1, () => {
      const plugin = new SplitViewConfigPlugin();
      const result = new FakeResult();
      const call = new MethodCall('getSplitViewConfig', null);
      plugin.onMethodCall(call, result);
      expect(result.successCalled).assertEqual(true);
      expect(result.successValue).not().assertNull();
    });

    it('calls notImplemented for unknown method', Level.LEVEL1, () => {
      const plugin = new SplitViewConfigPlugin();
      const result = new FakeResult();
      const call = new MethodCall('unknownMethod', null);
      plugin.onMethodCall(call, result);
      expect(result.notImplCalled).assertEqual(true);
      expect(result.successCalled).assertEqual(false);
    });
  });
}
