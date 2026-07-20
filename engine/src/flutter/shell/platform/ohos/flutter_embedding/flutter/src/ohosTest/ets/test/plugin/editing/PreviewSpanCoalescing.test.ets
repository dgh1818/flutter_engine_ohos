import { describe, it, expect, Level } from '@ohos/hypium';
import { PreviewSpanCoalescing } from '../../../../../main/ets/plugin/editing/PreviewSpanCoalescing';

export default function previewSpanCoalescingTest() {
  describe('PreviewSpanCoalescing.syncCacheRightToPreviewLength', () => {
    it('returns unchanged cache when preview grows', Level.LEVEL0, () => {
      const result = PreviewSpanCoalescing.syncCacheRightToPreviewLength('hello', 0, 5, 3);
      expect(result.cache).assertEqual('hello');
      expect(result.rightIdx).assertEqual(5);
    });

    it('removes stale units when preview shrinks', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.syncCacheRightToPreviewLength('hello', 0, 2, 5);
      expect(result.cache).assertEqual('he');
      expect(result.rightIdx).assertEqual(2);
    });

    it('clamps rightIdx to cache length', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.syncCacheRightToPreviewLength('abc', 0, 10, 3);
      expect(result.rightIdx).assertEqual(3);
    });

    it('clamps rightIdx to leftIdx minimum', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.syncCacheRightToPreviewLength('abc', 2, 0, 3);
      expect(result.rightIdx).assertEqual(2);
    });

    it('handles empty cache', Level.LEVEL0, () => {
      const result = PreviewSpanCoalescing.syncCacheRightToPreviewLength('', 0, 0, 0);
      expect(result.cache).assertEqual('');
      expect(result.rightIdx).assertEqual(0);
    });

    it('removes middle portion when preview shrinks with offset', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.syncCacheRightToPreviewLength('abcdef', 2, 1, 5);
      expect(result.cache).assertEqual('abcf');
      expect(result.rightIdx).assertEqual(3);
    });
  });

  describe('PreviewSpanCoalescing.repairPreviewSpanIfDrifted', () => {
    it('returns unchanged when span matches and rightIdx correct', Level.LEVEL0, () => {
      const result = PreviewSpanCoalescing.repairPreviewSpanIfDrifted('hello', 0, 5, 'hello');
      expect(result.cache).assertEqual('hello');
      expect(result.rightIdx).assertEqual(5);
    });

    it('fixes rightIdx when span matches but rightIdx lags', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.repairPreviewSpanIfDrifted('hello', 0, 3, 'hello');
      expect(result.cache).assertEqual('hello');
      expect(result.rightIdx).assertEqual(5);
    });

    it('rewrites span when text drifts', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.repairPreviewSpanIfDrifted('hello', 0, 5, 'world');
      expect(result.cache).assertEqual('world');
      expect(result.rightIdx).assertEqual(5);
    });

    it('returns unchanged when preview is empty', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.repairPreviewSpanIfDrifted('hello', 0, 3, '');
      expect(result.cache).assertEqual('hello');
      expect(result.rightIdx).assertEqual(3);
    });

    it('returns unchanged when leftIdx equals rightIdx and preview empty', Level.LEVEL0, () => {
      const result = PreviewSpanCoalescing.repairPreviewSpanIfDrifted('hello', 2, 2, '');
      expect(result.cache).assertEqual('hello');
      expect(result.rightIdx).assertEqual(2);
    });

    it('uses prefix suffix when cache starts with preview', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.repairPreviewSpanIfDrifted('hello world', 0, 5, 'hello');
      expect(result.cache).assertEqual('hello world');
      expect(result.rightIdx).assertEqual(5);
    });
  });

  describe('PreviewSpanCoalescing.overwriteSpanIfTextMismatch', () => {
    it('returns unchanged when span matches expected', Level.LEVEL0, () => {
      const result = PreviewSpanCoalescing.overwriteSpanIfTextMismatch('hello', 0, 5, 'hello');
      expect(result.cache).assertEqual('hello');
      expect(result.rightIdx).assertEqual(5);
      expect(result.changed).assertEqual(false);
    });

    it('overwrites span when text differs', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.overwriteSpanIfTextMismatch('hello', 0, 5, 'world');
      expect(result.cache).assertEqual('world');
      expect(result.rightIdx).assertEqual(5);
      expect(result.changed).assertEqual(true);
    });

    it('overwrites middle span preserving prefix and suffix', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.overwriteSpanIfTextMismatch('abcdef', 2, 5, 'XYZ');
      expect(result.cache).assertEqual('abXYZf');
      expect(result.rightIdx).assertEqual(5);
      expect(result.changed).assertEqual(true);
    });

    it('handles empty expected', Level.LEVEL1, () => {
      const result = PreviewSpanCoalescing.overwriteSpanIfTextMismatch('abcdef', 2, 4, '');
      expect(result.cache).assertEqual('abef');
      expect(result.rightIdx).assertEqual(2);
      expect(result.changed).assertEqual(true);
    });
  });
}
