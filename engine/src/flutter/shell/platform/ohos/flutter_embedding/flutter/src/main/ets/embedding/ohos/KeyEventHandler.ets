/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

import { HashMap } from '@kit.ArkTS';
import deviceInfo from '@ohos.deviceInfo';
import TextInputPlugin from '../../plugin/editing/TextInputPlugin';
import Log from '../../util/Log';
import { KeyCode } from '@kit.InputKit';

const TAG = "KeyEventHandler";

export class KeyEventHandler {
  private textInputPlugin?: TextInputPlugin;
  private charMap : HashMap<number, string> = new HashMap();
  private shiftMap : HashMap<number, string> = new HashMap();
  private isShiftMode: boolean = false;

  constructor(textInputPlugin?: TextInputPlugin) {
    this.textInputPlugin = textInputPlugin;
    this.initCharMap();
    this.initShiftMap();
  }

  private initCharMap() {
    this.charMap.set(KeyCode.KEYCODE_0, '0')
    this.charMap.set(KeyCode.KEYCODE_1, '1')
    this.charMap.set(KeyCode.KEYCODE_2, '2')
    this.charMap.set(KeyCode.KEYCODE_3, '3')
    this.charMap.set(KeyCode.KEYCODE_4, '4')
    this.charMap.set(KeyCode.KEYCODE_5, '5')
    this.charMap.set(KeyCode.KEYCODE_6, '6')
    this.charMap.set(KeyCode.KEYCODE_7, '7')
    this.charMap.set(KeyCode.KEYCODE_8, '8')
    this.charMap.set(KeyCode.KEYCODE_9, '9')
    this.charMap.set(KeyCode.KEYCODE_A, 'a')
    this.charMap.set(KeyCode.KEYCODE_B, 'b')
    this.charMap.set(KeyCode.KEYCODE_C, 'c')
    this.charMap.set(KeyCode.KEYCODE_D, 'd')
    this.charMap.set(KeyCode.KEYCODE_E, 'e')
    this.charMap.set(KeyCode.KEYCODE_F, 'f')
    this.charMap.set(KeyCode.KEYCODE_G, 'g')
    this.charMap.set(KeyCode.KEYCODE_H, 'h')
    this.charMap.set(KeyCode.KEYCODE_I, 'i')
    this.charMap.set(KeyCode.KEYCODE_J, 'j')
    this.charMap.set(KeyCode.KEYCODE_K, 'k')
    this.charMap.set(KeyCode.KEYCODE_L, 'l')
    this.charMap.set(KeyCode.KEYCODE_M, 'm')
    this.charMap.set(KeyCode.KEYCODE_N, 'n')
    this.charMap.set(KeyCode.KEYCODE_O, 'o')
    this.charMap.set(KeyCode.KEYCODE_P, 'p')
    this.charMap.set(KeyCode.KEYCODE_Q, 'q')
    this.charMap.set(KeyCode.KEYCODE_R, 'r')
    this.charMap.set(KeyCode.KEYCODE_S, 's')
    this.charMap.set(KeyCode.KEYCODE_T, 't')
    this.charMap.set(KeyCode.KEYCODE_U, 'u')
    this.charMap.set(KeyCode.KEYCODE_V, 'v')
    this.charMap.set(KeyCode.KEYCODE_W, 'w')
    this.charMap.set(KeyCode.KEYCODE_X, 'x')
    this.charMap.set(KeyCode.KEYCODE_Y, 'y')
    this.charMap.set(KeyCode.KEYCODE_Z, 'z')
    this.charMap.set(KeyCode.KEYCODE_GRAVE, '`')
    this.charMap.set(KeyCode.KEYCODE_MINUS, '-')
    this.charMap.set(KeyCode.KEYCODE_EQUALS, '=')
    this.charMap.set(KeyCode.KEYCODE_LEFT_BRACKET, '[')
    this.charMap.set(KeyCode.KEYCODE_RIGHT_BRACKET, ']')
    this.charMap.set(KeyCode.KEYCODE_BACKSLASH, '\\')
    this.charMap.set(KeyCode.KEYCODE_SEMICOLON, ';')
    this.charMap.set(KeyCode.KEYCODE_APOSTROPHE, '\'')
    this.charMap.set(KeyCode.KEYCODE_COMMA, ',')
    this.charMap.set(KeyCode.KEYCODE_PERIOD, '.')
    this.charMap.set(KeyCode.KEYCODE_SLASH, '/')
    this.charMap.set(KeyCode.KEYCODE_SPACE, ' ')
  }

  private initShiftMap() {
    this.shiftMap.set(KeyCode.KEYCODE_0, ')')
    this.shiftMap.set(KeyCode.KEYCODE_1, '!')
    this.shiftMap.set(KeyCode.KEYCODE_2, '@')
    this.shiftMap.set(KeyCode.KEYCODE_3, '#')
    this.shiftMap.set(KeyCode.KEYCODE_4, '$')
    this.shiftMap.set(KeyCode.KEYCODE_5, '%')
    this.shiftMap.set(KeyCode.KEYCODE_6, '^')
    this.shiftMap.set(KeyCode.KEYCODE_7, '&')
    this.shiftMap.set(KeyCode.KEYCODE_8, '*')
    this.shiftMap.set(KeyCode.KEYCODE_9, '(')
    this.shiftMap.set(KeyCode.KEYCODE_A, 'A')
    this.shiftMap.set(KeyCode.KEYCODE_B, 'B')
    this.shiftMap.set(KeyCode.KEYCODE_C, 'C')
    this.shiftMap.set(KeyCode.KEYCODE_D, 'D')
    this.shiftMap.set(KeyCode.KEYCODE_E, 'E')
    this.shiftMap.set(KeyCode.KEYCODE_F, 'F')
    this.shiftMap.set(KeyCode.KEYCODE_G, 'G')
    this.shiftMap.set(KeyCode.KEYCODE_H, 'H')
    this.shiftMap.set(KeyCode.KEYCODE_I, 'I')
    this.shiftMap.set(KeyCode.KEYCODE_J, 'J')
    this.shiftMap.set(KeyCode.KEYCODE_K, 'K')
    this.shiftMap.set(KeyCode.KEYCODE_L, 'L')
    this.shiftMap.set(KeyCode.KEYCODE_M, 'M')
    this.shiftMap.set(KeyCode.KEYCODE_N, 'N')
    this.shiftMap.set(KeyCode.KEYCODE_O, 'O')
    this.shiftMap.set(KeyCode.KEYCODE_P, 'P')
    this.shiftMap.set(KeyCode.KEYCODE_Q, 'Q')
    this.shiftMap.set(KeyCode.KEYCODE_R, 'R')
    this.shiftMap.set(KeyCode.KEYCODE_S, 'S')
    this.shiftMap.set(KeyCode.KEYCODE_T, 'T')
    this.shiftMap.set(KeyCode.KEYCODE_U, 'U')
    this.shiftMap.set(KeyCode.KEYCODE_V, 'V')
    this.shiftMap.set(KeyCode.KEYCODE_W, 'W')
    this.shiftMap.set(KeyCode.KEYCODE_X, 'X')
    this.shiftMap.set(KeyCode.KEYCODE_Y, 'Y')
    this.shiftMap.set(KeyCode.KEYCODE_Z, 'Z')
    this.shiftMap.set(KeyCode.KEYCODE_GRAVE, '~')
    this.shiftMap.set(KeyCode.KEYCODE_MINUS, '_')
    this.shiftMap.set(KeyCode.KEYCODE_EQUALS, '+')
    this.shiftMap.set(KeyCode.KEYCODE_LEFT_BRACKET, '{')
    this.shiftMap.set(KeyCode.KEYCODE_RIGHT_BRACKET, '}')
    this.shiftMap.set(KeyCode.KEYCODE_BACKSLASH, '|')
    this.shiftMap.set(KeyCode.KEYCODE_SEMICOLON, ':')
    this.shiftMap.set(KeyCode.KEYCODE_APOSTROPHE, '"')
    this.shiftMap.set(KeyCode.KEYCODE_COMMA, '<')
    this.shiftMap.set(KeyCode.KEYCODE_PERIOD, '>')
    this.shiftMap.set(KeyCode.KEYCODE_SLASH, '?')
    this.shiftMap.set(KeyCode.KEYCODE_SPACE, ' ')
  }

  getCharByEvent(event: KeyEvent) : string {
    let key = event.keyCode;
    if (this.isShiftMode) {
      return this.shiftMap.hasKey(key) ? this.shiftMap.get(key) : ''
    } else {
      return this.charMap.hasKey(key) ? this.charMap.get(key) : ''
    }
  }

  handleKeyEvent(event: KeyEvent) {
    Log.i(TAG, JSON.stringify({
      "name": "handleKeyEvent",
      "event": event
    }));
    if (event.type == KeyType.Up) {
      // 处理字符按键相关逻辑
      if (this.charMap.hasKey(event.keyCode)) {
        this.textInputPlugin?.getEditingState().handleInsertTextEvent(this.getCharByEvent(event))
      }
      // 处理非字符按键
      if (event.keyCode == KeyCode.KEYCODE_DEL) {
        this.textInputPlugin?.getEditingState().handleDeleteEvent(false, 0)
      } else if (event.keyCode == KeyCode.KEYCODE_FORWARD_DEL) {
        this.textInputPlugin?.getEditingState().handleDeleteEvent(true, 0)
      }
    }
    this.isShiftMode = (event.keyCode == KeyCode.KEYCODE_SHIFT_LEFT || event.keyCode == KeyCode.KEYCODE_SHIFT_RIGHT)
  }
}
