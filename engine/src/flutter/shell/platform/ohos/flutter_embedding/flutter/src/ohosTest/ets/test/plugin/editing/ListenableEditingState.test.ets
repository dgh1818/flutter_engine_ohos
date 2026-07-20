/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import { describe, expect, it } from '@ohos/hypium';
import inputMethod from '@ohos.inputMethod';
// import { inputMethod } from '@kit.IMEKit';
import {
  EditingStateWatcher,
  ListenableEditingState
} from '../../../../../main/ets/plugin/editing/ListenableEditingState';

export default function ListenableEditingStateTest() {
  describe('ListenableEditingStateTest', () => {
    let editingState: ListenableEditingState = new ListenableEditingState(null, 0, undefined);
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
      editingState.handleDeleteEvent(true, 1, undefined);
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
   })
}