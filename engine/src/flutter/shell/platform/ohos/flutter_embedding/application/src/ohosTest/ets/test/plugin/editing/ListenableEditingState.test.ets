import { describe, it, expect } from '@ohos/hypium';
import inputMethod from '@ohos.inputMethod';
// import { inputMethod } from '@kit.IMEKit';
import {
  EditingStateWatcher,
  ListenableEditingState
} from '@ohos/flutter_ohos/src/main/ets/plugin/editing/ListenableEditingState';

export default function ListenableEditingStateTest() {
  describe('ListenableEditingStateTest', () => {
    let editingState: ListenableEditingState = new ListenableEditingState(null, 0, null);
    it('should initialize with default values', 0, () => {
      expect(editingState.getSelectionStart()).assertEqual(0);
      expect(editingState.getSelectionEnd()).assertEqual(0);
      expect(editingState.getComposingStart()).assertEqual(-1);
      expect(editingState.getComposingEnd()).assertEqual(-1);
      expect(editingState.getStringCache()).assertEqual('');
    })
    it('should handle insert text event correctly', 0, () => {
      editingState.handleInsertTextEvent('hello');
      expect(editingState.getStringCache()).assertEqual('hello');
      expect(editingState.getSelectionStart()).assertEqual(5);
      expect(editingState.getSelectionEnd()).assertEqual(5);
    })

    it('should handle delete event correctly (right)', 0, () => {
      editingState.handleDeleteEvent(true, 1);
      expect(editingState.getStringCache()).assertEqual('hello');
      expect(editingState.getSelectionStart()).assertEqual(5);
      expect(editingState.getSelectionEnd()).assertEqual(5);
    })
    it('should handle newline event correctly', 0, () => {
      editingState.handleNewlineEvent();
      expect(editingState.getStringCache()).assertEqual('hello\n');
      expect(editingState.getSelectionStart()).assertEqual(6);
      expect(editingState.getSelectionEnd()).assertEqual(6);
    })

    // test handleSelectByRangeEvent()
    it('handleSelectByRangeEvent', 0, (range: inputMethod.Range) => {
      const testStr: string = "handleSelectByRangeEvent";
      editingState.setStringCache(testStr);
      editingState.setSelectionStart(testStr.length);
      editingState.setSelectionEnd(testStr.length);
      // cursor index is moved to start
      range.start = 0;
      range.end = 0;
      editingState.handleSelectByRangeEvent(range);
      expect(editingState.getSelectionStart()).assertEqual(0);
      expect(editingState.getSelectionEnd()).assertEqual(0);
      // cursor index is moved to end
      range.start = -1;
      range.end = -1;
      editingState.handleSelectByRangeEvent(range);
      expect(editingState.getSelectionStart()).assertEqual(testStr.length);
      expect(editingState.getSelectionEnd()).assertEqual(testStr.length);
    })

    // test handleMoveCursorEvent()
    it('handleMoveCursorEvent', 0, () => {
      const testStr: string = "handleMoveCursorEvent";
      let cursorIdx: number = 3;
      editingState.setStringCache(testStr);
      editingState.setSelectionStart(cursorIdx);
      editingState.setSelectionEnd(cursorIdx);
      // 1. move cursor by left direction
      editingState.handleMoveCursorEvent(inputMethod.Direction.CURSOR_LEFT);
      expect(editingState.getSelectionStart()).assertEqual(cursorIdx - 1);
      expect(editingState.getSelectionEnd()).assertEqual(cursorIdx - 1);
      // 2. move cursor by right direction
      editingState.handleMoveCursorEvent(inputMethod.Direction.CURSOR_RIGHT);
      expect(editingState.getSelectionStart()).assertEqual(cursorIdx + 1);
      expect(editingState.getSelectionEnd()).assertEqual(cursorIdx + 1);
      // 3. move cursor by up direction
      editingState.handleMoveCursorEvent(inputMethod.Direction.CURSOR_UP);
      expect(editingState.getSelectionStart()).assertEqual(0);
      expect(editingState.getSelectionEnd()).assertEqual(0);
      // 4. move cursor by down direction
      editingState.handleMoveCursorEvent(inputMethod.Direction.CURSOR_DOWN);
      expect(editingState.getSelectionStart()).assertEqual(testStr.length);
      expect(editingState.getSelectionEnd()).assertEqual(testStr.length);
    })

    // test handleSelectByMovementEvent()
    it('handleSelectByMovementEvent', 0, (movement: inputMethod.Movement) => {
      const testStr: string = "handleSelectByMovementEvent";
      let selectStartIdx: number = 3;
      let selectEndIdx: number = 5;
      editingState.setStringCache(testStr);
      editingState.setSelectionStart(selectStartIdx);
      editingState.setSelectionEnd(selectEndIdx);
      // 1. move cursor by left direction
      movement.direction = inputMethod.Direction.CURSOR_LEFT;
      editingState.handleSelectByMovementEvent(movement);
      expect(editingState.getSelectionStart()).assertEqual(selectStartIdx);
      expect(editingState.getSelectionEnd()).assertEqual(selectEndIdx - 1);
      // 2. move cursor by right direction
      movement.direction = inputMethod.Direction.CURSOR_LEFT;
      editingState.handleSelectByMovementEvent(movement);
      expect(editingState.getSelectionStart()).assertEqual(selectStartIdx);
      expect(editingState.getSelectionEnd()).assertEqual(selectEndIdx + 1);
      // 3. move cursor by up direction
      movement.direction = inputMethod.Direction.CURSOR_UP;
      editingState.handleSelectByMovementEvent(movement);
      expect(editingState.getSelectionStart()).assertEqual(0);
      expect(editingState.getSelectionEnd()).assertEqual(selectStartIdx);
      // 4. move cursor by down direction
      movement.direction = inputMethod.Direction.CURSOR_DOWN;
      editingState.handleSelectByMovementEvent(movement);
      expect(editingState.getSelectionStart()).assertEqual(selectStartIdx);
      expect(editingState.getSelectionEnd()).assertEqual(testStr.length);
    })

    // test getLeftTextOfCursor()
    it('getLeftTextOfCursor', 0, (length: number) => {
      const testStr: string = "getLeftTextOfCursor";
      let cursorIdx = 3;
      editingState.setStringCache(testStr);
      editingState.setSelectionStart(cursorIdx);
      editingState.setSelectionEnd(cursorIdx);

      const leftText = editingState.getLeftTextOfCursor(length);
      expect(leftText).assertEqual("get");
    })

    // test getRightTextOfCursor()
    it('getRightTextOfCursor', 0, (length: number) => {
      const testStr: string = "getRightTextOfCursor";
      let cursorIdx = 3;
      editingState.setStringCache(testStr);
      editingState.setSelectionStart(cursorIdx);
      editingState.setSelectionEnd(cursorIdx);

      const rightText = editingState.getRightTextOfCursor(length);
      expect(rightText).assertEqual("RightTextOfCursor");
    })
  })
}

class Range {
  start: number;
  end: number;
  constructor(start: number, end: number) {
    this.start = start;
    this.end = end;
  }
};
