/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
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

import TextInputChannel, {
  Configuration,
  TextEditState,
  TextInputMethodHandler
} from '../../embedding/engine/systemchannels/TextInputChannel';
import inputMethod from '@ohos.inputMethod';
import Log from '../../util/Log';
import { EditingStateWatcher, ListenableEditingState } from './ListenableEditingState';
import Any from '../common/Any';
import { inputDevice } from '@kit.InputKit';

/// 临时规避缺少newline对应枚举问题
const NEWLINE_KEY_TYPE: number = 8;

export default class TextInputPlugin implements EditingStateWatcher {
  private static TAG = "TextInputPlugin";
  private textInputChannel: TextInputChannel;
  private mTextInputHandler: TextInputMethodHandlerImpl;

  constructor(textInputChannel: TextInputChannel, viewId: string) {
    this.textInputChannel = textInputChannel;
    // viewId is used for requestFocus
    this.mTextInputHandler = new TextInputMethodHandlerImpl(this, viewId);
    this.textInputChannel.setTextInputMethodHandler(this.mTextInputHandler);
  }

  public clearTextInputClient() {
    this.textInputChannel.textInputMethodHandler?.clearClient();
  }

  setTextInputEditingState(state: TextEditState) {

  }

  getEditingState() {
    return this.mTextInputHandler.mEditable;
  }

  didChangeEditingState(textChanged: boolean, selectionChanged: boolean, composingRegionChanged: boolean): void {
    let editable = this.mTextInputHandler.mEditable;
    let inputTarget = this.mTextInputHandler.inputTarget;
    let configuration = this.mTextInputHandler.configuration;
    if (configuration != null && configuration.enableDeltaModel) {
      this.textInputChannel.updateEditingStateWithDeltas(inputTarget.id, editable.extractBatchTextEditingDeltas());
      editable.clearBatchDeltas();
    } else {
      this.textInputChannel.updateEditingState(inputTarget.id, editable.getStringCache(),
        editable.getSelectionStart(), editable.getSelectionEnd(),
        editable.getComposingStart(), editable.getComposingEnd())
    }
  }

  detach(): void {
    this.mTextInputHandler.inputMethodController.detach((err) => {
      if (err) {
        Log.e(TextInputPlugin.TAG, "Failed to detach: " + JSON.stringify(err));
      }
    })
  }

  destroy() {
    // Since the Dart side no longer listens to the lifecycle, 
    // the keyboard must be explicitly hidden before the engine is destroyed.
    this.mTextInputHandler.hide();
    this.textInputChannel.setTextInputMethodHandler(null);
  }
}

class TextInputMethodHandlerImpl implements TextInputMethodHandler {
  private static TAG = "TextInputMethodHandlerImpl";
  private textConfig: inputMethod.TextConfig;
  inputMethodController: inputMethod.InputMethodController;
  inputTarget: InputTarget;
  public configuration: Configuration | null = null;
  mEditable: ListenableEditingState;
  private mRestartInputPending: boolean = false;
  private plugin: EditingStateWatcher | Any;
  private imcFlag: boolean = false;
  private keyboardStatus: inputMethod.KeyboardStatus = inputMethod.KeyboardStatus.HIDE;
  private inputAttribute: inputMethod.InputAttribute =
    { textInputType: inputMethod.TextInputType.TEXT, enterKeyType: inputMethod.EnterKeyType.NONE };
  private keyboardFocusState: boolean = false;
  private focusViewId: string = "";

  constructor(plugin: TextInputPlugin | Any, viewId: string) {
    this.textConfig = {
      inputAttribute: this.inputAttribute
    };
    this.plugin = plugin;
    this.mEditable = new ListenableEditingState(null, 0);
    this.inputMethodController = inputMethod.getController();
    this.inputTarget = new InputTarget(Type.NO_TARGET, 0);
    this.focusViewId = viewId;
  }

  /// 通过判断是否是TextInputType.none来决定是否弹出键盘
  show(): void {
    if (this.canShowTextInput()) {
      // Ensure the Xcomponent gains focus before the soft keyboard is displayed.
      focusControl.requestFocus(this.focusViewId);
      this.keyboardFocusState = true;
      this.showTextInput();
    } else {
      this.hide();
    }
  }

  hide(): void {
    // Ensure the Xcomponent loses focus before the soft keyboard is hided.
    focusControl.requestFocus("unfocus-xcomponent-node");
    this.keyboardFocusState = false;
    this.hideTextInput();
  }

  requestAutofill(): void {

  }

  finishAutofillContext(shouldSave: boolean): void {

  }

  setClient(textInputClientId: number, configuration: Configuration | null): void {
    Log.d(TextInputMethodHandlerImpl.TAG, "textInputClientId: " + textInputClientId);
    this.setTextInputClient(textInputClientId, configuration);
  }

  setPlatformViewClient(id: number, usesVirtualDisplay: boolean): void {

  }

  setEditableSizeAndTransform(width: number, height: number, transform: number[]): void {

  }

  setCursorSizeAndPosition(cursorInfo: inputMethod.CursorInfo) {
    this.inputMethodController.updateCursor(cursorInfo)
  }

  setEditingState(editingState: TextEditState): void {
    Log.d(TextInputMethodHandlerImpl.TAG,
      "text:" + editingState.text + " selectionStart:" + editingState.selectionStart + " selectionEnd:"
        + editingState.selectionEnd + " composingStart:" + editingState.composingStart + " composingEnd" +
      editingState.composingEnd);
    this.mEditable.updateTextInputState(editingState);
  }

  clearClient(): void {
    this.clearTextInputClient();
  }

  private async showTextInput(): Promise<void> {
    if (this.keyboardStatus == inputMethod.KeyboardStatus.SHOW) {
      return;
    }
    await this.attach(true);
    if (!this.imcFlag) {
      this.listenKeyBoardEvent();
    }
  }

  private async hideTextInput(): Promise<void> {
    this.inputMethodController.detach();
    this.keyboardStatus = inputMethod.KeyboardStatus.NONE;
    this.cancelListenKeyBoardEvent();
  }

  async attach(showKeyboard: boolean): Promise<void> {
    try {
      await this.inputMethodController.attach(showKeyboard, this.textConfig);
      this.keyboardStatus = inputMethod.KeyboardStatus.SHOW;
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to attach:" + JSON.stringify(err));
      this.keyboardStatus = inputMethod.KeyboardStatus.NONE;
    }
  }

  handleChangeFocus(focusState: boolean) {
    if (focusState && this.keyboardFocusState) {
      // When the app loses focus, the system automatically detaches the input method.
      // Upon regaining focus, if the input method should be displayed, it must be reattached.
      this.show();
    }
    try {
      inputDevice.getDeviceList((Error: Error, ids: Array<number>) => {
        let isPhysicalKeyboard = false;
        for (let i = 0; i < ids.length; i++) {
          const type = inputDevice.getKeyboardTypeSync(ids[i]);
          if (type == inputDevice.KeyboardType.ALPHABETIC_KEYBOARD || type == inputDevice.KeyboardType.DIGITAL_KEYBOARD) {
            isPhysicalKeyboard = true;
            break;
          }
        }

        if(focusState && isPhysicalKeyboard && this.keyboardFocusState) {
          this.cancelListenKeyBoardEvent();
          this.inputMethodController.detach().then(async () =>{
            await this.attach(true);
            this.listenKeyBoardEvent();
          })
        }
      })
    } catch (error) {
      Log.e(TextInputMethodHandlerImpl.TAG, `Failed to query device. Code is ${error.code}, message is ${error.message}`)
    }
  }

  async updateAttribute(): Promise<void> {
    if (this.keyboardStatus != inputMethod.KeyboardStatus.SHOW) {
      return;
    }
    try {
      await this.inputMethodController.updateAttribute(this.inputAttribute);
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to updateAttribute:" + JSON.stringify(err));
    }
  }

  setTextInputClient(client: number, configuration: Configuration | null): void {
    if (configuration) {
      this.configuration = configuration;
      if (configuration.inputType) {
        this.textConfig.inputAttribute.textInputType = configuration.inputType.type;
        this.textConfig.inputAttribute.enterKeyType = configuration.inputAction as Any;
      }
    }
    if (this.canShowTextInput()) {
      this.inputTarget = new InputTarget(Type.FRAMEWORK_CLIENT, client);
    } else {
      this.inputTarget = new InputTarget(Type.NO_TARGET, client);
    }
    this.mEditable.removeEditingStateListener(this.plugin);

    this.mEditable = new ListenableEditingState(this.plugin.textInputChannel, this.inputTarget.id);

    this.mRestartInputPending = true;
    this.mEditable.addEditingStateListener(this.plugin);

    this.inputAttribute = this.textConfig.inputAttribute;

    this.updateAttribute();
  }

  canShowTextInput(): boolean {
    if (this.configuration == null || this.configuration.inputType == null) {
      return true;
    }
    return this.configuration.inputType.type != inputMethod.TextInputType.NONE;
  }

  listenKeyBoardEvent(): void {
    try {
      this.inputMethodController.on('insertText', this.insertTextCallback);
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe insertText:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('deleteLeft', this.deleteLeftCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe deleteLeft:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('deleteRight', this.deleteRightCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe deleteRight:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('sendFunctionKey', this.sendFunctionKeyCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe sendFunctionKey:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('sendKeyboardStatus', this.sendKeyboardStatusCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe sendKeyboardStatus:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }

    try {
      this.inputMethodController.on('selectByRange', this.selectByRangeCallback)
    } catch (err) {
      Log.e(TextInputMethodHandlerImpl.TAG, "Failed to subscribe selectByRange:" + JSON.stringify(err));
      this.cancelListenKeyBoardEvent();
      return;
    }
    Log.d(TextInputMethodHandlerImpl.TAG, "listenKeyBoardEvent success");
    this.imcFlag = true;
  }

  private insertTextCallback = (text: string) => {
    Log.d(TextInputMethodHandlerImpl.TAG, "insertText: " + text);
    this.mEditable.handleInsertTextEvent(text);
  }

  private deleteLeftCallback = (length: number) => {
    this.mEditable.handleDeleteEvent(false, length);
  }

  private deleteRightCallback = (length: number) => {
    this.mEditable.handleDeleteEvent(true, length);
  }

  private sendFunctionKeyCallback = (functionKey: inputMethod.FunctionKey) => {
    /// 临时规避缺少newline对应枚举类型问题
    if (functionKey.enterKeyType == NEWLINE_KEY_TYPE) {
      this.mEditable.handleNewlineEvent();
    }
    this.mEditable.handleFunctionKey(functionKey);
  }

  private sendKeyboardStatusCallback = (state: inputMethod.KeyboardStatus) => {
    this.keyboardStatus = state;
    if (state == inputMethod.KeyboardStatus.HIDE) {
      this.plugin.textInputChannel.onConnectionClosed(this.inputTarget.id);
    }
  }

  private selectByRangeCallback = (range: inputMethod.Range) => {
    this.mEditable.handleSelectByRange(range);
  }

  cancelListenKeyBoardEvent(): void {
    this.inputMethodController?.off('insertText', this.insertTextCallback);
    this.inputMethodController?.off('deleteLeft', this.deleteLeftCallback);
    this.inputMethodController?.off('deleteRight', this.deleteRightCallback);
    this.inputMethodController?.off('sendFunctionKey', this.sendFunctionKeyCallback);
    this.inputMethodController?.off('sendKeyboardStatus', this.sendKeyboardStatusCallback);
    this.inputMethodController?.off('selectByRange', this.selectByRangeCallback);
    this.imcFlag = false;
  }

  public clearTextInputClient(): void {
    if (this.inputTarget.type == Type.VIRTUAL_DISPLAY_PLATFORM_VIEW) {
      return;
    }
    this.mEditable.removeEditingStateListener(this.plugin);
    this.configuration = null;
    this.inputTarget = new InputTarget(Type.NO_TARGET, 0);
  }
}

enum Type {
  NO_TARGET,
  // InputConnection is managed by the TextInputPlugin, and events are forwarded to the Flutter
  // framework.
  FRAMEWORK_CLIENT,
  // InputConnection is managed by a platform view that is presented on a virtual display.
  VIRTUAL_DISPLAY_PLATFORM_VIEW,
  PHYSICAL_DISPLAY_PLATFORM_VIEW,
}

export class InputTarget {
  type: Type;
  id: number;

  constructor(type: Type, id: number) {
    this.type = type;
    this.id = id;
  }
}