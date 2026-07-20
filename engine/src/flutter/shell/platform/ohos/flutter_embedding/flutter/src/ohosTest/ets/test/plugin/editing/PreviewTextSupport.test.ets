import { describe, it, expect, Level } from '@ohos/hypium';
import { isPreviewTextSupported } from '../../../../../main/ets/plugin/editing/PreviewTextSupport';

export default function previewTextSupportTest() {
  describe('PreviewTextSupport.isPreviewTextSupported', () => {
    it('returns a boolean', Level.LEVEL0, () => {
      const result = isPreviewTextSupported();
      expect(typeof result === 'boolean').assertEqual(true);
    });
  });
}
