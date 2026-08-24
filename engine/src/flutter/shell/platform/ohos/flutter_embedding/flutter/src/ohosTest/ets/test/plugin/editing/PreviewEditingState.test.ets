import { describe, it, expect, Level } from '@ohos/hypium';
import { PreviewEditingState } from '../../../../../main/ets/plugin/editing/PreviewEditingState';
import { ListenableEditingState } from '../../../../../main/ets/plugin/editing/ListenableEditingState';
interface Range {
  start: number,
  end: number
}

function makeRange(start: number, end: number): Range {
  return { start, end };
}

export default function previewEditingStateTest() {
  // ==================== initPreviewRangeSplice ====================
  describe('initPreviewRangeSplice', () => {
    it('skips when no active preview', Level.LEVEL0, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'ni';
      ctx.prevPreview = '';
      ctx.prevLen = 0;
      ctx.hadActivePreview = false;
      ctx.oldLeft = 0;
      ctx.textForMerge = 'ni';
      ctx.initPreviewRangeSplice(makeRange(1, 3));
      expect(ctx.textForMerge).assertEqual('ni');
    });

    it('skips when range is invalid {-1,-1}', Level.LEVEL0, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'n';
      ctx.prevPreview = 'nihao';
      ctx.prevLen = 5;
      ctx.hadActivePreview = true;
      ctx.oldLeft = 2;
      ctx.textForMerge = 'n';
      ctx.initPreviewRangeSplice(makeRange(-1, -1));
      expect(ctx.textForMerge).assertEqual('n');
    });

    it('splices correctly with leftIdx offset', Level.LEVEL0, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'n';
      ctx.prevPreview = 'nihao';
      ctx.prevLen = 5;
      ctx.hadActivePreview = true;
      ctx.oldLeft = 2;
      ctx.textForMerge = 'n';
      ctx.initPreviewRangeSplice(makeRange(3, 5));
      expect(ctx.textForMerge).assertEqual('nnao');
    });

    it('splices correctly without offset (leftIdx=0)', Level.LEVEL0, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'x';
      ctx.prevPreview = 'abc';
      ctx.prevLen = 3;
      ctx.hadActivePreview = true;
      ctx.oldLeft = 0;
      ctx.textForMerge = 'x';
      ctx.initPreviewRangeSplice(makeRange(1, 3));
      expect(ctx.textForMerge).assertEqual('ax');
    });

    it('clamps splice range to preview bounds', Level.LEVEL1, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'x';
      ctx.prevPreview = 'abc';
      ctx.prevLen = 3;
      ctx.hadActivePreview = true;
      ctx.oldLeft = 0;
      ctx.textForMerge = 'x';
      ctx.initPreviewRangeSplice(makeRange(1, 100));
      expect(ctx.textForMerge).assertEqual('ax');
    });

    it('swaps when start > end', Level.LEVEL1, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'x';
      ctx.prevPreview = 'abc';
      ctx.prevLen = 3;
      ctx.hadActivePreview = true;
      ctx.oldLeft = 0;
      ctx.textForMerge = 'x';
      ctx.initPreviewRangeSplice(makeRange(3, 1));
      expect(ctx.textForMerge).assertEqual('ax');
    });
  });

    // ==================== applyLocalFullBodyPreviewMerge ====================

  describe('applyLocalFullBodyPreviewMerge', () => {
    it('sets textForMerge to imeText in full-body mode', Level.LEVEL0, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'nihao';
      ctx.hadActivePreview = true;
      ctx.prevLen = 5;
      ctx.imeFullBodyMode = true;
      ctx.textForMerge = 'old';
      ctx.applyLocalFullBodyPreviewMerge();
      expect(ctx.textForMerge).assertEqual('nihao');
    });

    it('skips when not full-body mode', Level.LEVEL0, () => {
      const ctx = new PreviewEditingState();
      ctx.imeText = 'nihao';
      ctx.hadActivePreview = true;
      ctx.prevLen = 5;
      ctx.imeFullBodyMode = false;
      ctx.textForMerge = 'old';
      ctx.applyLocalFullBodyPreviewMerge();
      expect(ctx.textForMerge).assertEqual('old');
    });
  });

    // ==================== applyImePreviewStringCacheAndRange ====================

  describe('applyImePreviewStringCacheAndRange', () => {
    it('first preview with valid range inserts at range', Level.LEVEL0, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好');
      editable.setSelectionStart(2);
      editable.setSelectionEnd(2);

      const ctx = new PreviewEditingState();
      ctx.imeText = 'nihao';
      ctx.effectiveText = 'nihao';
      ctx.hadActivePreview = false;
      ctx.prevLen = 0;

      ctx.applyImePreviewStringCacheAndRange(makeRange(2, 2), editable);
      expect(editable.getStringCache()).assertEqual('你好nihao');
      expect(editable.getLeftIdxOfPreviewTextRange()).assertEqual(2);
      expect(editable.getRightIdxOfPreviewTextRange()).assertEqual(7);
    });

    it('first preview with invalid range uses cursor position', Level.LEVEL1, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好');
      editable.setSelectionStart(2);
      editable.setSelectionEnd(2);

      const ctx = new PreviewEditingState();
      ctx.imeText = 'nihao';
      ctx.effectiveText = 'nihao';
      ctx.hadActivePreview = false;
      ctx.prevLen = 0;

      ctx.applyImePreviewStringCacheAndRange(makeRange(-1, -1), editable);
      expect(editable.getStringCache()).assertEqual('你好nihao');
      expect(editable.getLeftIdxOfPreviewTextRange()).assertEqual(2);
    });

    it('first preview replaces selection range', Level.LEVEL1, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好world');
      editable.setSelectionStart(2);
      editable.setSelectionEnd(7);

      const ctx = new PreviewEditingState();
      ctx.imeText = 'nihao';
      ctx.effectiveText = 'nihao';
      ctx.hadActivePreview = false;
      ctx.prevLen = 0;

      ctx.applyImePreviewStringCacheAndRange(makeRange(-1, -1), editable);
      expect(editable.getStringCache()).assertEqual('你好nihao');
    });

    it('preview update replaces span content', Level.LEVEL0, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好nihao');
      editable.updatePreviewTextRange(2, 7);
      editable.setPreviewText('nihao');

      const ctx = new PreviewEditingState();
      ctx.imeText = 'nni';
      ctx.effectiveText = 'nni';
      ctx.hadActivePreview = true;
      ctx.prevPreview = 'nihao';
      ctx.prevLen = 5;

      ctx.applyImePreviewStringCacheAndRange(makeRange(3, 7), editable);
      expect(editable.getStringCache()).assertEqual('你好nni');
      expect(editable.getLeftIdxOfPreviewTextRange()).assertEqual(2);
      expect(editable.getRightIdxOfPreviewTextRange()).assertEqual(5);
    });

    it('preview update preserves suffix text after span', Level.LEVEL1, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好nihao def');
      editable.updatePreviewTextRange(2, 7);
      editable.setPreviewText('nihao');

      const ctx = new PreviewEditingState();
      ctx.imeText = 'nni';
      ctx.effectiveText = 'nni';
      ctx.hadActivePreview = true;
      ctx.prevPreview = 'nihao';
      ctx.prevLen = 5;

      ctx.applyImePreviewStringCacheAndRange(makeRange(3, 7), editable);
      expect(editable.getStringCache()).assertEqual('你好nni def');
    });
  });

    // ==================== applyImePreviewCaretAfterMerge ====================

  describe('applyImePreviewCaretAfterMerge', () => {
    it('first preview sets caret at end of preview', Level.LEVEL0, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好nihao');
      editable.updatePreviewTextRange(2, 7);

      const ctx = new PreviewEditingState();
      ctx.effectiveText = 'nihao';
      ctx.hadActivePreview = false;
      ctx.imeFullBodyMode = true;
      ctx.imeText = 'nihao';

      ctx.applyImePreviewCaretAfterMerge(makeRange(-1, -1), editable);
      expect(editable.getSelectionStart()).assertEqual(7);
      expect(editable.getSelectionEnd()).assertEqual(7);
    });

    it('full-body update sets caret at end of preview', Level.LEVEL0, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好nihao');
      editable.updatePreviewTextRange(2, 7);

      const ctx = new PreviewEditingState();
      ctx.effectiveText = 'nihao';
      ctx.hadActivePreview = true;
      ctx.imeFullBodyMode = true;
      ctx.imeText = 'nihao';

      ctx.applyImePreviewCaretAfterMerge(makeRange(-1, -1), editable);
      expect(editable.getSelectionStart()).assertEqual(7);
      expect(editable.getSelectionEnd()).assertEqual(7);
    });

    it('splice mode sets caret at range.start + imeText.length', Level.LEVEL0, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('你好nnao');
      editable.updatePreviewTextRange(2, 6);

      const ctx = new PreviewEditingState();
      ctx.effectiveText = 'nnao';
      ctx.hadActivePreview = true;
      ctx.imeFullBodyMode = false;
      ctx.imeText = 'n';

      ctx.applyImePreviewCaretAfterMerge(makeRange(3, 5), editable);
      expect(editable.getSelectionStart()).assertEqual(4);
      expect(editable.getSelectionEnd()).assertEqual(4);
    });
  });

    // ==================== composeDisplayState ====================

  describe('composeDisplayState', () => {
    it('returns composing=-1 when staging inactive', Level.LEVEL0, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('hello');
      editable.setSelectionStart(5);
      editable.setSelectionEnd(5);

      const state = PreviewEditingState.composeDisplayState(editable);
      expect(state.text).assertEqual('hello');
      expect(state.selectionStart).assertEqual(5);
      expect(state.selectionEnd).assertEqual(5);
      expect(state.composingStart).assertEqual(-1);
      expect(state.composingEnd).assertEqual(-1);
      expect(editable.getComposingStart()).assertEqual(-1);
      expect(editable.getComposingEnd()).assertEqual(-1);
    });

    it('clamps selection to text length when staging inactive', Level.LEVEL1, () => {
      const editable = new ListenableEditingState(null, 0, undefined);
      editable.setStringCache('abc');
      editable.setSelectionStart(100);
      editable.setSelectionEnd(100);

      const state = PreviewEditingState.composeDisplayState(editable);
      expect(state.selectionStart).assertEqual(3);
      expect(state.selectionEnd).assertEqual(3);
    });
  });
}