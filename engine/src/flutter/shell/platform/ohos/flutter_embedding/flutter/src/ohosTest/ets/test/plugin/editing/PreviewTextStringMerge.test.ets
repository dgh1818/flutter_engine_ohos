import { describe, it, expect, Level } from '@ohos/hypium';
import { PreviewTextStringMerge } from '../../../../../main/ets/plugin/editing/PreviewTextStringMerge';

export default function previewTextStringMergeTest() {
  describe('PreviewTextStringMerge.previewBareForTailShrinkCompare', () => {
    it('strips all apostrophes', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewBareForTailShrinkCompare("ab'cd")).assertEqual('abcd');
    });

    it('returns unchanged when no apostrophes', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewBareForTailShrinkCompare('abcd')).assertEqual('abcd');
    });

    it('returns empty for all apostrophes', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.previewBareForTailShrinkCompare("'''")).assertEqual('');
    });

    it('handles empty string', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewBareForTailShrinkCompare('')).assertEqual('');
    });

    it('handles consecutive apostrophes', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.previewBareForTailShrinkCompare("a''b''c")).assertEqual('abc');
    });
  });

  describe('PreviewTextStringMerge.isImePreviewApostropheLikeChar', () => {
    it('returns true for ASCII apostrophe', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.isImePreviewApostropheLikeChar("'")).assertEqual(true);
    });

    it('returns true for U+2019 right single quote', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.isImePreviewApostropheLikeChar(String.fromCharCode(0x2019))).assertEqual(true);
    });

    it('returns true for U+02BC modifier letter apostrophe', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.isImePreviewApostropheLikeChar(String.fromCharCode(0x02BC))).assertEqual(true);
    });

    it('returns true for U+FF07 fullwidth apostrophe', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.isImePreviewApostropheLikeChar(String.fromCharCode(0xFF07))).assertEqual(true);
    });

    it('returns false for letter', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.isImePreviewApostropheLikeChar('a')).assertEqual(false);
    });

    it('returns false for empty string', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.isImePreviewApostropheLikeChar('')).assertEqual(false);
    });

    it('returns false for digit', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.isImePreviewApostropheLikeChar('1')).assertEqual(false);
    });
  });

  describe('PreviewTextStringMerge.getFullBufferPinyinRangeIfPureLatin', () => {
    it('returns range for pure ASCII letters', Level.LEVEL0, () => {
      const result = PreviewTextStringMerge.getFullBufferPinyinRangeIfPureLatin('hello');
      expect(result).not().assertNull();
      expect(result!.start).assertEqual(0);
      expect(result!.end).assertEqual(5);
    });

    it('returns range for letters with apostrophes', Level.LEVEL1, () => {
      const result = PreviewTextStringMerge.getFullBufferPinyinRangeIfPureLatin("a'b'c");
      expect(result).not().assertNull();
      expect(result!.end).assertEqual(5);
    });

    it('returns null for empty string', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.getFullBufferPinyinRangeIfPureLatin('')).assertNull();
    });

    it('returns null when contains CJK', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.getFullBufferPinyinRangeIfPureLatin('abc你好')).assertNull();
    });

    it('returns null when contains digit', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.getFullBufferPinyinRangeIfPureLatin('abc123')).assertNull();
    });

    it('returns range for uppercase letters', Level.LEVEL0, () => {
      const result = PreviewTextStringMerge.getFullBufferPinyinRangeIfPureLatin('ABC');
      expect(result).not().assertNull();
      expect(result!.end).assertEqual(3);
    });
  });

  describe('PreviewTextStringMerge.previewHasConsecutiveImeApostropheRun', () => {
    it('returns true for double apostrophe', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewHasConsecutiveImeApostropheRun("a''b")).assertEqual(true);
    });

    it('returns false for single apostrophes', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewHasConsecutiveImeApostropheRun("a'b'c")).assertEqual(false);
    });

    it('returns false for no apostrophes', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewHasConsecutiveImeApostropheRun('abc')).assertEqual(false);
    });

    it('returns false for empty string', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewHasConsecutiveImeApostropheRun('')).assertEqual(false);
    });

    it('returns true for mixed apostrophe types', Level.LEVEL1, () => {
      const text = "'" + String.fromCharCode(0x2019);
      expect(PreviewTextStringMerge.previewHasConsecutiveImeApostropheRun(text)).assertEqual(true);
    });
  });

  describe('PreviewTextStringMerge.previewIntroducesNewConsecutiveApostropheRun', () => {
    it('returns true when candidate has run but prev does not', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewIntroducesNewConsecutiveApostropheRun("a'b", "a''b")).assertEqual(true);
    });

    it('returns false when both have run', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.previewIntroducesNewConsecutiveApostropheRun("a''b", "a''c")).assertEqual(false);
    });

    it('returns false when neither has run', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewIntroducesNewConsecutiveApostropheRun("a'b", "a'c")).assertEqual(false);
    });

    it('returns false when candidate has no run', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewIntroducesNewConsecutiveApostropheRun("a''b", "a'b")).assertEqual(false);
    });
  });

  describe('PreviewTextStringMerge.previewIndexOfFirstConsecutiveImeApostropheRun', () => {
    it('returns index of first run', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewIndexOfFirstConsecutiveImeApostropheRun("ab''cd")).assertEqual(2);
    });

    it('returns -1 when no run', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewIndexOfFirstConsecutiveImeApostropheRun("a'b'c")).assertEqual(-1);
    });

    it('returns -1 for empty string', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.previewIndexOfFirstConsecutiveImeApostropheRun('')).assertEqual(-1);
    });

    it('returns 0 when run at start', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.previewIndexOfFirstConsecutiveImeApostropheRun("''abc")).assertEqual(0);
    });
  });

  describe('PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreview', () => {
    it('collapses double apostrophe to single', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreview("a''b")).assertEqual("a'b");
    });

    it('returns unchanged when no consecutive', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreview("a'b'c")).assertEqual("a'b'c");
    });

    it('collapses triple apostrophe to double then single', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreview("a'''b")).assertEqual("a'b");
    });

    it('returns empty for empty string', Level.LEVEL0, () => {
      expect(PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreview('')).assertEqual('');
    });

    it('collapses multiple separate runs', Level.LEVEL1, () => {
      expect(PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreview("a''b''c")).assertEqual("a'b'c");
    });
  });

  describe('PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreviewAndCaret', () => {
    it('collapses and keeps caret after collapse', Level.LEVEL0, () => {
      const result = PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreviewAndCaret("a''b", 4);
      expect(result.text).assertEqual("a'b");
      expect(result.rel).assertEqual(3);
    });

    it('keeps caret before collapse unaffected', Level.LEVEL1, () => {
      const result = PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreviewAndCaret("a''b", 1);
      expect(result.text).assertEqual("a'b");
      expect(result.rel).assertEqual(1);
    });

    it('clamps negative caret to 0', Level.LEVEL1, () => {
      const result = PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreviewAndCaret("a''b", -1);
      expect(result.rel).assertEqual(0);
    });

    it('clamps caret beyond length', Level.LEVEL1, () => {
      const result = PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreviewAndCaret("a''b", 100);
      expect(result.rel).assertEqual(3);
    });

    it('returns unchanged when no consecutive', Level.LEVEL0, () => {
      const result = PreviewTextStringMerge.collapseConsecutiveImeApostrophesInPreviewAndCaret("a'b", 3);
      expect(result.text).assertEqual("a'b");
      expect(result.rel).assertEqual(3);
    });
  });
}
