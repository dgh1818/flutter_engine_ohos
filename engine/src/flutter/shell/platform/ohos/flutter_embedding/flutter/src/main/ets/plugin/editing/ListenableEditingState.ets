/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*
* Based on ListenableEditingState.java originally written by
* Copyright (C) 2013 The Flutter Authors.
*
*/

import { TextEditState } from '../../embedding/engine/systemchannels/TextInputChannel';
import Log from '../../util/Log';
import inputMethod from '@ohos.inputMethod';
import ArrayList from '@ohos.util.ArrayList';
import { TextEditingDelta } from './TextEditingDelta';
import TextInputChannel from '../../embedding/engine/systemchannels/TextInputChannel';
import { FlutterTextUtils } from './TextUtils';
import { KeyCode } from '@kit.InputKit';
import KeyEventChannel, { SimulateKeyEvent } from '../../embedding/engine/systemchannels/KeyEventChannel';
import { PasteboardUtils } from '../../util/PasteboardUtils';

const TAG = "ListenableEditingState";

/**
 * Enumeration of delete operation states.
 */
enum DeleteStates {
  /** Delete operation has started */
  START,
  /** Delete operation is in progress */
  MOVING,
  /** Delete operation has ended */
  END
}

/**
 * Manages the editable text state and notifies listeners of changes.
 * This class tracks text content, selection, composing regions, and preview text.
 */
export class ListenableEditingState {
  private TextInputChannel: TextInputChannel | null = null;
  private keyEventChannel: KeyEventChannel | undefined;
  private client: number = 0
  private leftDeleteState: DeleteStates = DeleteStates.END
  private rightDeleteState: DeleteStates = DeleteStates.END
  //Cache used to storage software keyboard input action
  private mStringCache: string;
  private mSelectionStartCache: number = 0;
  private mSelectionEndCache: number = 0;
  private mComposingStartCache: number = 0;
  private mComposingEndCache: number = 0;
  //used to compare with Cache

  private mListeners: ArrayList<EditingStateWatcher> = new ArrayList<EditingStateWatcher>();
  private mPendingListeners: ArrayList<EditingStateWatcher> = new ArrayList<EditingStateWatcher>();
  private mBatchTextEditingDeltas: ArrayList<TextEditingDelta> = new ArrayList<TextEditingDelta>();
  private mChangeNotificationDepth: number = 0;
  private mBatchEditNestDepth: number = 0;
  private mTextWhenBeginBatchEdit: string;
  private mSelectionStartWhenBeginBatchEdit: number = 0;
  private mSelectionEndWhenBeginBatchEdit: number = 0;
  private mComposingStartWhenBeginBatchEdit: number = 0;
  private mComposingEndWhenBeginBatchEdit: number = 0;

  // preview text
  private mPreviewText: string = "";
  private mPreviewTextStart: number = 0;
  private mPreviewTextEnd: number = 0;
  private mLeftIdxOfPreviewTextRange: number = 0;
  private mRightIdxOfPreviewTextRange: number = 0;
  private mIsCursorIdxOutOfPreviewTextRange: boolean = false;
  private mIsEnglishPreviewMode: boolean = false;

  /**
   * Constructs a new ListenableEditingState instance.
   * @param TextInputChannel - The TextInputChannel for communication, possibly null
   * @param client - The client ID
   * @param keyEventChannel - Optional KeyEventChannel for key event handling
   */
  constructor(TextInputChannel:TextInputChannel | null,client:number, keyEventChannel?: KeyEventChannel) {
    this.TextInputChannel = TextInputChannel;
    this.keyEventChannel = keyEventChannel;
    this.client = client
    this.mStringCache = "";
    this.mTextWhenBeginBatchEdit = "";
    this.mSelectionStartCache = 0;
    this.mSelectionEndCache = 0;
    this.mComposingStartCache = -1;
    this.mComposingEndCache = -1;
    this.mPreviewText = "";
  }

  /**
   * Extracts and clears the batch of text editing deltas.
   * @returns A list of TextEditingDelta objects representing the changes
   */
  extractBatchTextEditingDeltas(): ArrayList<TextEditingDelta> {
    let currentBatchDeltas = new ArrayList<TextEditingDelta>();
    this.mBatchTextEditingDeltas.forEach((data) => {
      currentBatchDeltas.add(data);
    })
    this.mBatchTextEditingDeltas.clear();
    return currentBatchDeltas;
  }

  /**
   * Clears all batched text editing deltas.
   */
  clearBatchDeltas(): void {
    this.mBatchTextEditingDeltas.clear();
  }

  /**
   * Replaces a range of text with new text.
   * @param start - The start position of the range to replace
   * @param end - The end position of the range to replace
   * @param tb - The replacement text
   * @param tbStart - The start position within the replacement text
   * @param tbEnd - The end position within the replacement text
   */
  replace(start: number, end: number, tb: String, tbStart: number, tbEnd: number): void {
    const placeIndex =
      this.mSelectionStartCache < this.mSelectionEndCache ? this.mSelectionStartCache : this.mSelectionEndCache;

    this.mBatchTextEditingDeltas.add(
      new TextEditingDelta(
        this.mStringCache.toString(),
        placeIndex + tbEnd,
        placeIndex + tbEnd,
        this.getComposingStart(),
        this.getComposingEnd(),
        start,
        end + tbStart,
        tb.toString()
      ));
  }

  /**
   * Gets the current selection start position.
   * @returns The selection start position
   */
  getSelectionStart(): number {
    return this.mSelectionStartCache;
  }

  /**
   * Gets the current selection end position.
   * @returns The selection end position
   */
  getSelectionEnd(): number {
    return this.mSelectionEndCache;
  }

  /**
   * Gets the current composing region start position.
   * @returns The composing start position, or -1 if no composing region
   */
  getComposingStart(): number {
    return this.mComposingStartCache;
  }

  /**
   * Gets the current composing region end position.
   * @returns The composing end position, or -1 if no composing region
   */
  getComposingEnd(): number {
    return this.mComposingEndCache;
  }

  /**
   * Gets the current text content.
   * @returns The text string
   */
  getStringCache(): string {
    return this.mStringCache;
  }

  /**
   * Gets the currently selected text.
   * @returns The selected text, or empty string if selection is invalid
   */
  getSelectionString(): string {
    if (this.mSelectionStartCache < 0 || this.mSelectionEndCache > this.mStringCache.length) {
      return "";
    }
    return this.mStringCache.substring(this.mSelectionStartCache, this.mSelectionEndCache);
  }

  /**
   * Gets the current preview text from the input method.
   * @returns The preview text
   */
  getPreviewText(): string {
    return this.mPreviewText;
  }

  /**
   * Gets the start position of the preview text in the original text.
   * @returns The preview text start position
   */
  getPreviewTextStart(): number {
    return this.mPreviewTextStart;
  }

  /**
   * Gets the end position of the preview text in the original text.
   * @returns The preview text end position
   */
  getPreviewTextEnd(): number {
    return this.mPreviewTextEnd;
  }

  /**
   * Gets the right index of the preview text range in the string cache.
   * @returns The right index of the preview text range
   */
  getRightIdxOfPreviewTextRange() : number {
    return this.mRightIdxOfPreviewTextRange;
  }

  /**
   * Gets the left index of the preview text range in the string cache.
   * @returns The left index of the preview text range
   */
  getLeftIdxOfPreviewTextRange() : number {
    return this.mLeftIdxOfPreviewTextRange;
  }

  /**
   * Sets the preview text.
   * @param previewText - The preview text to set
   */
  setPreviewText(previewText: string): void {
    this.mPreviewText = previewText;
  }

  /**
   * Sets the start position of the preview text.
   * @param previewTextStart - The start position
   */
  setPreviewTextStart(previewTextStart: number): void {
    this.mPreviewTextStart = previewTextStart;
  }

  /**
   * Sets the end position of the preview text.
   * @param previewTextEnd - The end position
   */
  setPreviewTextEnd(previewTextEnd: number): void {
    this.mPreviewTextEnd = previewTextEnd;
  }

  /**
   * Updates the preview text range indices.
   * @param leftIdx - The left index in the string cache
   * @param rightIdx - The right index in the string cache
   */
  updatePreviewTextRange(leftIdx: number, rightIdx: number): void {
    this.mLeftIdxOfPreviewTextRange = leftIdx;
    this.mRightIdxOfPreviewTextRange = rightIdx;
  }

  /**
   * Clears all preview text contents and resets related indices.
   */
  clearPreviewTextContents() : void {
    this.mPreviewText = "";
    this.mPreviewTextStart = 0;
    this.mPreviewTextEnd = 0;
    this.mLeftIdxOfPreviewTextRange = 0;
    this.mRightIdxOfPreviewTextRange = 0;
  }

  /**
   * Sets whether the cursor index is out of the preview text range.
   * @param hasChanged - True if the cursor is out of range, false otherwise
   */
  setIsCursorIdxOutOfPreviewTextRange(hasChanged: boolean): void {
    this.mIsCursorIdxOutOfPreviewTextRange = hasChanged;
  }

  /**
   * Gets whether the cursor index is out of the preview text range.
   * @returns True if the cursor is out of range, false otherwise
   */
  getIsCursorIdxOutOfPreviewTextRange(): boolean {
    return this.mIsCursorIdxOutOfPreviewTextRange;
  }

  /**
   * Sets the selection start position.
   * @param newSelectionStart - The new selection start position
   */
  setSelectionStart(newSelectionStart: number): void {
    this.mSelectionStartCache = newSelectionStart;
  }

  /**
   * Sets the selection end position.
   * @param newSelectionEnd - The new selection end position
   */
  setSelectionEnd(newSelectionEnd: number): void {
    this.mSelectionEndCache = newSelectionEnd;
  }

  /**
   * Sets the composing region start position.
   * @param newComposingStart - The new composing start position, or -1 to clear
   */
  setComposingStart(newComposingStart: number): void {
    this.mComposingStartCache = newComposingStart;
  }

  /**
   * Sets the composing region end position.
   * @param newComposingEnd - The new composing end position, or -1 to clear
   */
  setComposingEnd(newComposingEnd: number): void {
    this.mComposingEndCache = newComposingEnd;
  }

  /**
   * Sets the text content.
   * @param newStringCache - The new text content
   */
  setStringCache(newStringCache: string): void {
    this.mStringCache = newStringCache;
  }

  /**
   * Notifies a single listener of editing state changes.
   * @param listener - The listener to notify
   * @param textChanged - Whether the text has changed
   * @param selectionChanged - Whether the selection has changed
   * @param composingChanged - Whether the composing region has changed
   */
  notifyListener(listener: EditingStateWatcher,
    textChanged: boolean,
    selectionChanged: boolean,
    composingChanged: boolean): void {
    this.mChangeNotificationDepth++;
    listener.didChangeEditingState(textChanged, selectionChanged, composingChanged);
    this.mChangeNotificationDepth--;
  }

  /**
   * Notifies all listeners if any changes have occurred.
   * @param textChanged - Whether the text has changed
   * @param selectionChanged - Whether the selection has changed
   * @param composingChanged - Whether the composing region has changed
   */
  notifyListenersIfNeeded(textChanged: boolean, selectionChanged: boolean, composingChanged: boolean) {
    if (textChanged || selectionChanged || composingChanged) {
      for (const listener of this.mListeners) {
        this.notifyListener(listener, textChanged, selectionChanged, composingChanged);
      }
    }
  }

  /**
   * Handles insertion of preview text from the input method.
   * @param text - The preview text to insert
   * @param range - The range in the original text where preview text applies
   */
  handleInsertPreviewTextEvent(text: string, range: inputMethod.Range): void {
    // Determine whether it is in English preview input mode
    if (range.start != -1 && range.end != -1 && text.indexOf("'") == -1) {
      this.mIsEnglishPreviewMode = true;
    }
    // preview text callback with complete text contents，like a, a'b, a'b'c
    // its text char do not appear one by one
    this.setPreviewText(text);
    this.setPreviewTextStart(range.start);
    this.setPreviewTextEnd(range.end);

    // update preview text start idx and end idx
    let leftIdxOfPreviewText: number = this.getSelectionStart();
    let rightIdxOfPreviewText: number = this.getSelectionStart() + text.length;
    this.updatePreviewTextRange(leftIdxOfPreviewText, rightIdxOfPreviewText)

    if (this.mListeners == null) {
      Log.e(TAG, "handleInsertPreviewTextEvent, mListeners is null");
      return;
    }
    this.notifyListenersIfNeeded(true, true, false);
  }

  /**
   * Handles insertion of final text from the input method.
   * @param text - The text to insert
   */
  handleInsertTextEvent(text: string): void {
    // clear preview text cache before insert the final texts
    if (this.getPreviewText().length != 0) {
      this.setPreviewText("");
    }
    // When previewTextChangeSelection async callback has been invoked, covering the previous preview text.
    // But if that callback did not work, then manually insert the candidate word, covering the previous preview text.
    if (this.getLeftIdxOfPreviewTextRange() < this.mSelectionStartCache &&
      (this.getLeftIdxOfPreviewTextRange() != this.getRightIdxOfPreviewTextRange())) {
      this.mSelectionStartCache = this.getLeftIdxOfPreviewTextRange();
      this.mSelectionEndCache = this.getRightIdxOfPreviewTextRange();
    } else if (this.mIsEnglishPreviewMode) {
      // To comply with the specifications of Xiaoyi-InputMethod, change the
      // range of inserted English chars and add a space at the backend
      this.mSelectionStartCache = this.getPreviewTextStart();
      this.mSelectionEndCache = this.getPreviewTextEnd();
      text += " ";
    }

    let start =
      this.mSelectionStartCache < this.mSelectionEndCache ? this.mSelectionStartCache : this.mSelectionEndCache;
    let end = this.mSelectionStartCache > this.mSelectionEndCache ? this.mSelectionStartCache : this.mSelectionEndCache;
    const length = text.length;
    this.replace(start, end, text, 0, length);

    if (this.mStringCache.length == this.mSelectionStartCache) {
      //Insert text one by one
      let tempStr: string = this.mStringCache.substring(0, start) + text + this.mStringCache.substring(end);
      this.mStringCache = tempStr;
      this.setSelectionStart(this.mStringCache.length);
      this.setSelectionEnd(this.mStringCache.length);
    } else if (this.mStringCache.length > this.mSelectionStartCache) {
      //Insert text in the middle of string
      let tempStr: string = this.mStringCache.substring(0, start) + text + this.mStringCache.substring(end);
      this.mStringCache = tempStr;
      this.mSelectionStartCache = start + text.length;
      this.mSelectionEndCache = this.mSelectionStartCache;
    }
    if (this.mListeners == null) {
      Log.e(TAG, "mListeners is null");
      return;
    }
    this.notifyListenersIfNeeded(true, true, false);
    // when preview text did insert, reset the params
    this.updatePreviewTextRange(0, 0);
    this.setPreviewTextStart(-1);
    this.setPreviewTextEnd(-1);
    this.mIsEnglishPreviewMode = false;
  }

  /**
   * Updates the text input state from Flutter.
   * @param state - The new text edit state
   */
  updateTextInputState(state: TextEditState): void {
    if (this.leftDeleteState === DeleteStates.START) {
      this.leftDeleteState = DeleteStates.MOVING;
    }
    if (this.rightDeleteState === DeleteStates.START) {
      this.rightDeleteState = DeleteStates.MOVING;
    }
    this.beginBatchEdit();
    this.setStringCache(state.text);
    if (state.hasSelection()) {
      this.setSelectionStart(state.selectionStart);
      this.setSelectionEnd(state.selectionEnd);
    } else {
      this.setSelectionStart(0);
      this.setSelectionEnd(0);
    }
    this.endBatchEdit();
  }

  /**
   * Begins a batch edit operation.
   * Multiple edits can be batched together to reduce notifications.
   */
  beginBatchEdit(): void {
    this.mBatchEditNestDepth++;
    if (this.mChangeNotificationDepth > 0) {
      Log.e(TAG, "editing state should not be changed in a listener callback");
    }
    if (this.mBatchEditNestDepth == 1 && !this.mListeners.isEmpty()) {
      this.mTextWhenBeginBatchEdit = this.getStringCache();
      this.mSelectionStartWhenBeginBatchEdit = this.getSelectionStart();
      this.mSelectionEndWhenBeginBatchEdit = this.getSelectionEnd();
      this.mComposingStartWhenBeginBatchEdit = this.getComposingStart();
      this.mComposingEndWhenBeginBatchEdit = this.getComposingEnd();
    }
  }

  /**
   * Ends a batch edit operation and notifies listeners of all changes.
   */
  endBatchEdit(): void {
    if (this.mBatchEditNestDepth == 0) {
      Log.e(TAG, "endBatchEdit called without a matching beginBatchEdit");
      return;
    }
    if (this.mBatchEditNestDepth == 1) {
      Log.d(TAG, "mBatchEditNestDepth == 1");
      for (const listener of this.mPendingListeners) {
        this.notifyListener(listener, true, true, true);
      }

      if (!this.mListeners.isEmpty()) {
        Log.d(TAG, "didFinishBatchEdit with " + this.mListeners.length + " listener(s)");
        const textChanged = !(this.mStringCache == this.mTextWhenBeginBatchEdit);
        const selectionChanged = this.mSelectionStartWhenBeginBatchEdit != this.getSelectionStart()
          || this.mSelectionEndWhenBeginBatchEdit != this.getSelectionEnd();
        const composingRegionChanged = this.mComposingStartWhenBeginBatchEdit != this.getComposingStart()
          || this.mComposingEndWhenBeginBatchEdit != this.getComposingEnd();
        Log.d(TAG, "textChanged: " + textChanged + " selectionChanged: " + selectionChanged +
          " composingRegionChanged: " + composingRegionChanged);
        this.notifyListenersIfNeeded(textChanged, selectionChanged, composingRegionChanged);
      }
    }
    for (const listener of this.mPendingListeners) {
      this.mListeners.add(listener);
    }
    this.mPendingListeners.clear();
    this.mBatchEditNestDepth--;
  }

  /**
   * Adds a listener to be notified of editing state changes.
   * @param listener - The listener to add
   */
  addEditingStateListener(listener: EditingStateWatcher): void {
    if (this.mChangeNotificationDepth > 0) {
      Log.e(TAG, "adding a listener " + JSON.stringify(listener) + " in a listener callback");
    }
    if (this.mBatchEditNestDepth > 0) {
      Log.d(TAG, "a listener was added to EditingState while a batch edit was in progress");
      this.mPendingListeners.add(listener);
    } else {
      this.mListeners.add(listener);
    }
  }

  /**
   * Removes an editing state listener.
   * @param listener - The listener to remove
   */
  removeEditingStateListener(listener: EditingStateWatcher): void {
    if (this.mChangeNotificationDepth > 0) {
      Log.e(TAG, "removing a listener " + JSON.stringify(listener) + " in a listener callback");
    }
    this.mListeners.remove(listener);
    if (this.mBatchEditNestDepth > 0) {
      this.mPendingListeners.remove(listener);
    }
  }

  /**
   * Marks the start of a deletion operation.
   * @param code - The key code that triggered the deletion
   */
  startDeleting(code: number) {
    if (code === KeyCode.KEYCODE_FORWARD_DEL) {
      this.rightDeleteState = DeleteStates.START
    } else {
      this.leftDeleteState = DeleteStates.START
    }
  }

  /**
   * Marks the end of a deletion operation.
   * @param code - The key code that triggered the deletion
   */
  endDeletion(code: number) {
    if (code === KeyCode.KEYCODE_FORWARD_DEL) {
      this.rightDeleteState = DeleteStates.END
    } else {
      this.leftDeleteState = DeleteStates.END
    }
  }

  /**
   * Handles a delete event from the input method.
   * @param leftOrRight - True for delete right (forward delete), false for delete left (backspace)
   * @param length - The number of characters to delete
   * @param enableDeltaModel - Whether delta model is enabled
   */
  handleDeleteEvent(leftOrRight: boolean, length: number, enableDeltaModel: boolean | undefined): void {
    if (length === 0) {
      return;
    }
    // clear preview text cache
    if (this.getPreviewText().length != 0) {
      this.setPreviewText("");
    }

    if (enableDeltaModel) {
      let selectionStart = this.getSelectionStart();
      let selectionEnd = this.getSelectionEnd();
      if(selectionStart === 0 && selectionEnd === 0){
        // [selectionStart]和[selectionEnd]都为0时，删除文本范围为空
        return;
      }
      if (selectionStart === selectionEnd) {
        // [selectionStart]和[selectionEnd]一致时为普通删除操作，需要修改偏移量
        this.setSelectionStart(this.mSelectionStartCache - length);
      }
    }

    let start =
      this.mSelectionStartCache < this.mSelectionEndCache ? this.mSelectionStartCache : this.mSelectionEndCache;
    let end = this.mSelectionStartCache > this.mSelectionEndCache ? this.mSelectionStartCache : this.mSelectionEndCache;

    if (leftOrRight == false && this.leftDeleteState !== DeleteStates.MOVING) {
      //delete left
      if (start == 0 && end == 0) {
        return;
      }

      let unicodeStart = start;
      if (start == end) {
        for (let i = 0; i < length; i++) {
          unicodeStart = FlutterTextUtils.getOffsetBefore(this.mStringCache, unicodeStart);
          if (unicodeStart === 0) {
            break;
          }
        }
      }
      this.replace(unicodeStart, end, "", 0, 0);
      this.mSelectionStartCache = unicodeStart;
      let tempStr: string = this.mStringCache.slice(0, unicodeStart) + this.mStringCache.slice(end);
      this.mStringCache = tempStr;
      this.mSelectionEndCache = this.mSelectionStartCache;
    } else if (leftOrRight == true && this.rightDeleteState !== DeleteStates.MOVING) {
      //delete right
      if (start == this.mStringCache.length) {
        return;
      }
      let unicodeEnd = end;
      if (start == end) {
        for (let i = 0; i < length; i++) {
          unicodeEnd = FlutterTextUtils.getOffsetAfter(this.mStringCache, unicodeEnd);
          if (unicodeEnd === this.mStringCache.length) {
            break;
          }
        }
      }
      this.replace(start, unicodeEnd, "", 0, 0);
      this.mSelectionEndCache = start;
      let tempStr: string = this.mStringCache.slice(0, start) +
        (unicodeEnd >= this.mStringCache.length ? "" : this.mStringCache.slice(unicodeEnd));
      this.mStringCache = tempStr;
      this.mSelectionStartCache = this.mSelectionEndCache;
    }
    this.notifyListenersIfNeeded(true, true, false);
  }

  /**
   * Handles a newline event from the input method.
   */
  handleNewlineEvent(): void {
    // 获取光标所在位置;
    // 当光标移动前位置小于移动后的位置时，获取光标移动前位置;反之获取移动后位置
    let start =
      this.mSelectionStartCache < this.mSelectionEndCache ? this.mSelectionStartCache : this.mSelectionEndCache;
    // 当光标移动前位置大于移动后的位置时，获取光标移动前位置;反之获取移动后位置
    let end = this.mSelectionStartCache > this.mSelectionEndCache ? this.mSelectionStartCache : this.mSelectionEndCache;

    this.replace(start, end, '\n', 0, 1);
    // 对光标位置和字符串长度进行对比，决定光标位置的计算方法
    if (this.mStringCache.length == this.mSelectionStartCache) {
      //Insert newline one by one
      let tempStr: string = this.mStringCache.substring(0, start) + '\n' + this.mStringCache.substring(end);
      this.mStringCache = tempStr;
      this.setSelectionStart(this.mStringCache.length);
      this.setSelectionEnd(this.mStringCache.length);
    } else if (this.mStringCache.length > this.mSelectionStartCache) {
      //Insert newline in the middle of string
      let tempStr: string = this.mStringCache.substring(0, start) + '\n' + this.mStringCache.substring(end);
      this.mStringCache = tempStr;
      this.mSelectionStartCache = start + 1;
      this.mSelectionEndCache = this.mSelectionStartCache;
    }
    if (this.mListeners == null) {
      Log.e(TAG, "mListeners is null");
      return;
    }
    this.notifyListenersIfNeeded(true, true, false);
  }

  /**
   * Handles a function key event from the input method.
   * @param functionKey - The function key that was pressed
   */
  handleFunctionKey(functionKey: inputMethod.FunctionKey): void {
    if (!this.TextInputChannel) {
      return
    }
    switch (functionKey.enterKeyType) {
      case inputMethod.EnterKeyType.PREVIOUS:
        this.TextInputChannel.previous(this.client);
        break;
      case inputMethod.EnterKeyType.UNSPECIFIED:
        this.TextInputChannel.unspecifiedAction(this.client);
        break;
      case inputMethod.EnterKeyType.NEWLINE:
        this.TextInputChannel.newline(this.client);
        break;
      case inputMethod.EnterKeyType.GO:
        this.TextInputChannel.go(this.client);
        break;
      case inputMethod.EnterKeyType.SEARCH:
        this.TextInputChannel.search(this.client);
        break;
      case inputMethod.EnterKeyType.SEND:
        this.TextInputChannel.send(this.client);
        break;
      case inputMethod.EnterKeyType.NEXT:
        this.TextInputChannel.next(this.client);
        break;
      case inputMethod.EnterKeyType.DONE:
        this.TextInputChannel.done(this.client);
        break;
    }
  }

  /**
   * Handles a text selection by range event.
   * @param range - The range to select
   */
  handleSelectByRangeEvent(range: inputMethod.Range): void {
    if (range.start === 0) { // cursor index updated at the start pos of text
      this.setSelectionStart(0);
      this.setSelectionEnd(0);
    } else { // cursor index updated at the end pos of text
      this.setSelectionStart(this.getStringCache().length);
      this.setSelectionEnd(this.getStringCache().length);
    }
    this.notifyListenersIfNeeded(false, true, false);
  }

  /**
   * cursor moved at 2D-text with 4 directions
   *  aaaaaa                   aaaaaa|
   *  bbbbbbb|b -(cursor_up)-> bbbbbbbb
   *  cccc                     cccc
   *
   *  aaaaaa                     aaaaaa
   *  bbbbbbb|b -(cursor_down)-> bbbbbbbb
   *  cccc                       cccc|
   *
   *  aaaaaa                      aaaaaa
   *  bbbbbbb|b -(cursor_right)-> bbbbbbbb|
   *  cccc                        cccc
   *
   *  aaaaaa                     aaaaaa
   *  bbbbbbb|b -(cursor_left)-> bbbbbb|bb
   *  cccc                       cccc
   */
  /**
   * Handles cursor movement events in 2D text.
   * @param direction - The direction to move the cursor
   */
  handleMoveCursorEvent(direction: inputMethod.Direction): void {
    switch (direction) {
      case inputMethod.Direction.CURSOR_LEFT:
      case inputMethod.Direction.CURSOR_RIGHT:
      case inputMethod.Direction.CURSOR_UP:
      case inputMethod.Direction.CURSOR_DOWN:
        // simulate the hardware-keyboard arrow-key pressed with any directions,
        // and the cursor index in text cache will be moved
        this.simulateHardwareCursorMovement(direction, false);
        break;
      default:
        Log.w(TAG, `"Unknown cursor movement direction: ${direction}"`);
        break;
    }
  }

  /**
   * text-selection changed with cursor moving at 2D-text
   * (...) -> denotes the selection range of text
   *  aaaaaa                   aaaa(aa
   *  bbbb|bbbb -(cursor_up)-> bbbb)bbbb
   *  cccc                     cccc
   *
   *  aaaaaa                     aaaaaa
   *  bbbbbbb|b -(cursor_down)-> bbbbbbb(b
   *  cccc                       cccc)
   *
   *  aaaaaa                      aaaaaa
   *  bb|bbbbbb -(cursor_right)-> bb(b)bbbbb
   *  cccc                        cccc
   *
   *  aaaaaa                     aaaaaa
   *  bbbbbbb|b -(cursor_left)-> bbbbbb(b)b
   *  cccc                       cccc
   */
  /**
   * Handles text selection changes with cursor movement.
   * @param movement - The movement that triggers the selection change
   */
  handleSelectByMovementEvent(movement: inputMethod.Movement) : void {
    switch (movement.direction) {
      case inputMethod.Direction.CURSOR_LEFT:
      case inputMethod.Direction.CURSOR_RIGHT:
      case inputMethod.Direction.CURSOR_UP:
      case inputMethod.Direction.CURSOR_DOWN:
        // simulate the hardware-keyboard shift-key combined with arrow-key simultaneously pressed,
        // and the text selection changed with cursor movement
        this.simulateHardwareCursorMovement(movement.direction, true);
        break;
      default:
        Log.w(TAG, `"Unknown cursor movement direction: ${movement.direction}"`);
        break;
    }
  }

  /**
   * Gets the text to the left of the cursor.
   * @param length - The maximum number of characters to return
   * @returns The text to the left of the cursor
   */
  getLeftTextOfCursor(length: number) : string {
    if (length <= 0) {
      return "";
    }
    const strCacheLen = this.getStringCache().length;
    if (!strCacheLen) {
      return "";
    }
    const cursorIdx = this.getSelectionStart();
    let startIdx = cursorIdx - length;
    if (startIdx < 0) {
      startIdx = 0;
    }
    return this.getStringCache().substring(startIdx, cursorIdx);
  }

  /**
   * Gets the text to the right of the cursor.
   * @param length - The maximum number of characters to return
   * @returns The text to the right of the cursor
   */
  getRightTextOfCursor(length: number) : string {
    if (length <= 0) {
      return "";
    }
    const strCacheLen = this.getStringCache().length;
    if (!strCacheLen) {
      return "";
    }
    const cursorIdx = this.getSelectionEnd();
    let endIdx = cursorIdx + length;
    if (endIdx >= strCacheLen) {
      endIdx = strCacheLen;
    }
    return this.getStringCache().substring(cursorIdx, endIdx);
  }

  /**
   * Handles extended actions like select all, cut, copy, and paste.
   * @param action - The action to perform
   */
  handleExtendActionEvent(action: inputMethod.ExtendAction) : void {
    switch (action) {
      case inputMethod.ExtendAction.SELECT_ALL:
        // select all text from the start to end, and notify cursor state updating
        this.setSelectionStart(0);
        this.setSelectionEnd(this.getStringCache().length);
        this.notifyListenersIfNeeded(false, true, false);
        break;
      case inputMethod.ExtendAction.CUT:
        // based on the text selection length (endIdx - startIdx) for cutting
        this.handleDeleteEvent(false, this.getSelectionEnd() - this.getSelectionStart(), false);
        break;
      case inputMethod.ExtendAction.COPY:
        // obtain the current selection range of text cache as the copy text
        const copyText = this.getStringCache().substring(this.getSelectionStart(), this.getSelectionEnd());
        PasteboardUtils.setCopyData(copyText);
        break;
      case inputMethod.ExtendAction.PASTE:
        // PasteboardUtils.getPasteDataAsync() is a async function, so must await the async paste operatopn 
        // in sync func, otherwise there can be a problem of timing inconsistency in the insertion of pasted text here.
        const handlePasteDataAsync = async () => {
          try {
            const pasteText = await PasteboardUtils.getPasteDataAsync(); 
            if (pasteText) { // insert the paste text into the editing box
              this.handleInsertTextEvent(pasteText);
            }
          } catch (err) {
            Log.e(TAG, "get PasteData error: " + err);
          }
        };
        handlePasteDataAsync();
        break;
      default:
        Log.w(TAG, `"Unknown ExtendAction: ${action}"`);
        break;
    }
  }

  /**
   * This method is used to simulate the hard-keyboard key event in soft-keyboard mode,
   * Case 1: cursor moving is simulated by sending arrow-key event to flutter SDK (Dart-side),
   * hence the cursor will move up/down/left/right in editing box.
   * Case 2: text-selection changed with cursor moving is simulated by sending shift-key combined with arrow-key events
   * to flutter SDK, hence the text-selection range will be updated by moving cursor up/down/left/right in editing box.
   * @param direction: cursor move direction from IMC callback
   * @param isShiftPressed: determine whether shift-key is pressed
   */
  /**
   * Simulates hardware keyboard cursor movement by sending key events to Flutter.
   * @param direction - The direction to move the cursor
   * @param isShiftPressed - Whether shift key is pressed (for text selection)
   * @private
   */
  private simulateHardwareCursorMovement(direction: inputMethod.Direction, isShiftPressed: boolean) : void {
    // Here, '选择' button in '文本编辑' function of soft-keyboard inputMethod apps is a simulation for hardware shift-key.
    // Therefore, when '选择' button is pressed in soft-keyboard equals to shift-key pressed in hard-keyboard
    enum SimulatedKeyText {
      KEY_SHIFT = 'KEYCODE_SHIFT_LEFT',
      KEY_ARROW_UP = 'KEYCODE_DPAD_UP',
      KEY_ARROW_DOWN = 'KEYCODE_DPAD_DOWN',
      KEY_ARROW_LEFT = 'KEYCODE_DPAD_LEFT',
      KEY_ARROW_RIGHT = 'KEYCODE_DPAD_RIGHT',
      KEY_UNKNOWN = 'KEYCODE_UNKNOWN'
    }
    if (isShiftPressed) { // simulate shift-key down
      this.sendSimulatedKeyEvent(new SimulateKeyEvent(KeyCode.KEYCODE_SHIFT_LEFT, SimulatedKeyText.KEY_SHIFT), false);
    }
    let arrowEvent: SimulateKeyEvent;
    switch (direction) {
      case inputMethod.Direction.CURSOR_UP: // simulate hardware arrow-up key event
        arrowEvent = new SimulateKeyEvent(KeyCode.KEYCODE_DPAD_UP, SimulatedKeyText.KEY_ARROW_UP);
        break;
      case inputMethod.Direction.CURSOR_DOWN: // simulate hardware arrow-down key event
        arrowEvent = new SimulateKeyEvent(KeyCode.KEYCODE_DPAD_DOWN, SimulatedKeyText.KEY_ARROW_DOWN);
        break;
      case inputMethod.Direction.CURSOR_LEFT: // simulate hardware arrow-left key event
        arrowEvent = new SimulateKeyEvent(KeyCode.KEYCODE_DPAD_LEFT, SimulatedKeyText.KEY_ARROW_LEFT);
        break;
      case inputMethod.Direction.CURSOR_RIGHT: // simulate hardware arrow-right key event
        arrowEvent = new SimulateKeyEvent(KeyCode.KEYCODE_DPAD_RIGHT, SimulatedKeyText.KEY_ARROW_RIGHT);
        break;
      default:
        arrowEvent = new SimulateKeyEvent(KeyCode.KEYCODE_UNKNOWN, SimulatedKeyText.KEY_UNKNOWN);
        break;
    }
    this.sendSimulatedKeyEvent(arrowEvent, false); // simulate arrow-key down
    this.sendSimulatedKeyEvent(arrowEvent, true);  // simulate arrow-key up
    if (isShiftPressed) { // simulate shift-key up
      this.sendSimulatedKeyEvent(new SimulateKeyEvent(KeyCode.KEYCODE_SHIFT_LEFT, SimulatedKeyText.KEY_SHIFT), true);
    }
  }

  /**
   * Sends a simulated key event to Flutter.
   * @param event - The key event to simulate
   * @param isKeyUp - True for key up event, false for key down event
   * @private
   */
  private sendSimulatedKeyEvent(event: SimulateKeyEvent, isKeyUp: boolean) : void {
    this.keyEventChannel?.simulateSendFlutterKeyEvent(
      event, isKeyUp, {
      onFrameworkResponse: (isEventHandled: boolean): void => {}
    });
  }
}

/**
 * Interface for objects that watch for editing state changes.
 * Changing the editing state in a didChangeEditingState callback may cause unexpected behavior.
 */
export interface EditingStateWatcher {
  /**
   * Called when the editing state changes.
   * @param textChanged - Whether the text has changed
   * @param selectionChanged - Whether the selection has changed
   * @param composingRegionChanged - Whether the composing region has changed
   */
  didChangeEditingState(textChanged: boolean, selectionChanged: boolean, composingRegionChanged: boolean): void;
}