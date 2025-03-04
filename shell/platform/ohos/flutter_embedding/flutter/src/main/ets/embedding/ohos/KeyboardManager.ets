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
import TextInputPlugin from '../../plugin/editing/TextInputPlugin';
import FlutterEngine from '../engine/FlutterEngine';
import KeyEventChannel, { FlutterKeyEvent } from '../engine/systemchannels/KeyEventChannel';
import KeyboardChannel from '../engine/systemchannels/KeyboardChannel';
import KeyEmbedderResponder from './KeyEmbedderResponder';
import { BinaryMessenger } from '../../plugin/common/BinaryMessenger';
import { KeyEventHandler } from './KeyEventHandler';
import HashSet from '@ohos.util.HashSet';

export default class KeyboardManager {
  private keyEventChannel: KeyEventChannel | null = null;
  private keyboardChannel: KeyboardChannel | null = null;
  protected keyEmbedderResponder: KeyEmbedderResponder;
  private keyEventHandler: KeyEventHandler;

  constructor(engine: FlutterEngine, textInputPlugin: TextInputPlugin) {
    this.keyEventChannel = new KeyEventChannel(engine.dartExecutor);
    this.keyboardChannel = new KeyboardChannel(engine.dartExecutor);
    this.keyboardChannel.setKeyboardMethodHandler(this);
    this.keyEmbedderResponder = new KeyEmbedderResponder(engine.dartExecutor);
    this.keyEventHandler = new KeyEventHandler(textInputPlugin);
  }

  onKeyPreIme(event: KeyEvent) : boolean {
    this.keyEmbedderResponder.handleKeyEvent(event);

    this.keyEventChannel?.sendFlutterKeyEvent(new FlutterKeyEvent(event), event.type == KeyType.Up, {
      onFrameworkResponse: (isEventHandled: boolean): void => {

      }
    })
    return false;
  }

  onKeyEvent(event: KeyEvent) : boolean {
    this.keyEventHandler.handleKeyEvent(event);
    return false;
  }

  public getKeyboardState(): Map<number, number> {
    return this.keyEmbedderResponder.getPressedKeys();
  }
}

export interface Responder {
  handleKeyEvent(keyEvent: KeyEvent): void;
}