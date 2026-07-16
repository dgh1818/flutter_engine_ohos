import { describe, it, expect, Level } from '@ohos/hypium';
import { PreviewTextCoordinator, DisplayState } from '../../../../../main/ets/plugin/editing/PreviewTextCoordinator';
import { TextEditState } from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';

export default function previewTextCoordinatorTest() {
  describe('PreviewTextCoordinator.safeTextEditStateForNativeDocument', () => {
    it('returns state with clamped selection', Level.LEVEL0, () => {
      const state = PreviewTextCoordinator.safeTextEditStateForNativeDocument('hello', 2, 4, -1, -1);
      expect(state.text).assertEqual('hello');
      expect(state.selectionStart).assertEqual(2);
      expect(state.selectionEnd).assertEqual(4);
      expect(state.composingStart).assertEqual(-1);
      expect(state.composingEnd).assertEqual(-1);
    });

    it('clamps selection to text length', Level.LEVEL1, () => {
      const state = PreviewTextCoordinator.safeTextEditStateForNativeDocument('abc', 10, 20, -1, -1);
      expect(state.selectionStart).assertEqual(3);
      expect(state.selectionEnd).assertEqual(3);
    });

    it('returns -1 selection when both are -1', Level.LEVEL1, () => {
      const state = PreviewTextCoordinator.safeTextEditStateForNativeDocument('abc', -1, -1, -1, -1);
      expect(state.selectionStart).assertEqual(-1);
      expect(state.selectionEnd).assertEqual(-1);
    });

    it('clamps composing to text length', Level.LEVEL1, () => {
      const state = PreviewTextCoordinator.safeTextEditStateForNativeDocument('abc', 0, 3, 0, 10);
      expect(state.composingStart).assertEqual(0);
      expect(state.composingEnd).assertEqual(3);
    });

    it('resets composing when start >= end after clamp', Level.LEVEL1, () => {
      const state = PreviewTextCoordinator.safeTextEditStateForNativeDocument('ab', 0, 2, 2, 10);
      expect(state.composingStart).assertEqual(-1);
      expect(state.composingEnd).assertEqual(-1);
    });

    it('resets composing when start < 0', Level.LEVEL1, () => {
      const state = PreviewTextCoordinator.safeTextEditStateForNativeDocument('abc', 0, 1, -1, 2);
      expect(state.composingStart).assertEqual(-1);
      expect(state.composingEnd).assertEqual(-1);
    });
  });

  describe('PreviewTextCoordinator.displayStateToTextEditState', () => {
    it('converts DisplayState to TextEditState with clamping', Level.LEVEL0, () => {
      const display: DisplayState = {
        text: 'hello',
        selectionStart: 2,
        selectionEnd: 4,
        composingStart: -1,
        composingEnd: -1
      };
      const state = PreviewTextCoordinator.displayStateToTextEditState(display);
      expect(state.text).assertEqual('hello');
      expect(state.selectionStart).assertEqual(2);
      expect(state.selectionEnd).assertEqual(4);
    });

    it('clamps out-of-range selection from DisplayState', Level.LEVEL1, () => {
      const display: DisplayState = {
        text: 'abc',
        selectionStart: 10,
        selectionEnd: 20,
        composingStart: -1,
        composingEnd: -1
      };
      const state = PreviewTextCoordinator.displayStateToTextEditState(display);
      expect(state.selectionStart).assertEqual(3);
      expect(state.selectionEnd).assertEqual(3);
    });
  });

  describe('PreviewTextCoordinator.editingStatesEqualForChannel', () => {
    it('returns true for identical states', Level.LEVEL0, () => {
      const a = new TextEditState('hello', 0, 5, -1, -1);
      const b = new TextEditState('hello', 0, 5, -1, -1);
      expect(PreviewTextCoordinator.editingStatesEqualForChannel(a, b)).assertEqual(true);
    });

    it('returns false when text differs', Level.LEVEL1, () => {
      const a = new TextEditState('hello', 0, 5, -1, -1);
      const b = new TextEditState('world', 0, 5, -1, -1);
      expect(PreviewTextCoordinator.editingStatesEqualForChannel(a, b)).assertEqual(false);
    });

    it('returns false when selection differs', Level.LEVEL1, () => {
      const a = new TextEditState('hello', 0, 3, -1, -1);
      const b = new TextEditState('hello', 0, 5, -1, -1);
      expect(PreviewTextCoordinator.editingStatesEqualForChannel(a, b)).assertEqual(false);
    });

    it('returns false when composing differs', Level.LEVEL1, () => {
      const a = new TextEditState('hello', 0, 5, 0, 3);
      const b = new TextEditState('hello', 0, 5, -1, -1);
      expect(PreviewTextCoordinator.editingStatesEqualForChannel(a, b)).assertEqual(false);
    });
  });
}
