import { describe, it, expect, Level } from '@ohos/hypium';
import { PreviewTextMergeContext } from '../../../../../main/ets/plugin/editing/PreviewTextMergeContext';

export default function previewTextMergeContextTest() {
  describe('PreviewTextMergeContext.defaults', () => {
    it('initializes imeText to empty string', Level.LEVEL0, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.imeText).assertEqual('');
    });

    it('initializes prevPreview to empty string', Level.LEVEL0, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.prevPreview).assertEqual('');
    });

    it('initializes prevLen to zero', Level.LEVEL0, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.prevLen).assertEqual(0);
    });

    it('initializes hadActivePreview to false', Level.LEVEL0, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.hadActivePreview).assertEqual(false);
    });

    it('initializes oldLeft to zero', Level.LEVEL1, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.oldLeft).assertEqual(0);
    });

    it('initializes selStart and selEnd to zero', Level.LEVEL1, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.selStart).assertEqual(0);
      expect(ctx.selEnd).assertEqual(0);
    });

    it('initializes textForMerge to empty string', Level.LEVEL1, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.textForMerge).assertEqual('');
    });

    it('initializes imeFullBodyMode to false', Level.LEVEL1, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.imeFullBodyMode).assertEqual(false);
    });

    it('initializes previewRangeSpliceUsed to false', Level.LEVEL1, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.previewRangeSpliceUsed).assertEqual(false);
    });

    it('initializes effectiveText to empty string', Level.LEVEL1, () => {
      const ctx = new PreviewTextMergeContext();
      expect(ctx.effectiveText).assertEqual('');
    });

    it('allows field updates after construction', Level.LEVEL1, () => {
      const ctx = new PreviewTextMergeContext();
      ctx.imeText = 'hello';
      ctx.selStart = 2;
      ctx.selEnd = 4;
      expect(ctx.imeText).assertEqual('hello');
      expect(ctx.selStart).assertEqual(2);
      expect(ctx.selEnd).assertEqual(4);
    });
  });
}
