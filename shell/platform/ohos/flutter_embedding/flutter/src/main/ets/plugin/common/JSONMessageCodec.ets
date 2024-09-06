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
import StringUtils from '../../util/StringUtils';

import MessageCodec from './MessageCodec';
import StringCodec from './StringCodec';
import TreeMap from '@ohos.util.TreeMap';
import HashMap from '@ohos.util.HashMap';
import LightWeightMap from '@ohos.util.LightWeightMap';
import PlainArray from '@ohos.util.PlainArray';
import List from '@ohos.util.List';
import LinkedList from '@ohos.util.LinkedList';
import Any from './Any';

/**
 * A {@link MessageCodec} using UTF-8 encoded JSON method calls and result envelopes.
 *
 * <p>This codec is guaranteed to be compatible with the corresponding <a
 * href="https://api.flutter.dev/flutter/services/JSONMethodCodec-class.html">JSONMethodCodec</a> on
 * the Dart side. These parts of the Flutter SDK are evolved synchronously.
 *
 * <p>On the Dart side, JSON messages are handled by the JSON facilities of the <a
 * href="https://api.dartlang.org/stable/dart-convert/JSON-constant.html">dart:convert</a> package.
 */
export default class JSONMessageCodec implements MessageCodec<Object> {
  static INSTANCE = new JSONMessageCodec();

  encodeMessage(message: Any): ArrayBuffer {
    if (message == null) {
      return StringUtils.stringToArrayBuffer("");
    }
    return StringCodec.INSTANCE.encodeMessage(JSON.stringify(this.toBaseData(message)));
  }

  decodeMessage(message: ArrayBuffer | null): Any {
    if (message == null) {
      return StringUtils.stringToArrayBuffer("");
    }
    try {
      const jsonStr = StringCodec.INSTANCE.decodeMessage(message);
      let jsonObj: Record<string, Any> = JSON.parse(jsonStr);
      if (jsonObj instanceof Object) {
        const list = Object.keys(jsonObj);
        if (list.includes('args')) {
          let args: Any = jsonObj['args'];
          if (args instanceof Object && !(args instanceof Array)) {
            let argsMap: Map<string, Any> = new Map();
            Object.keys(args).forEach(key => {
              argsMap.set(key, args[key]);
            })
            jsonObj['args'] = argsMap;
          }
        }
      }
      return jsonObj;
    } catch (e) {
      throw new Error("Invalid JSON");
    }
  }

  toBaseData(message: Any): Any {
    if (message == null || message == undefined) {
      return null;
    } else if (message instanceof List || message instanceof LinkedList) {
      return this.toBaseData(message.convertToArray());
    } else if (message instanceof Map || message instanceof HashMap || message instanceof TreeMap
      || message instanceof LightWeightMap || message instanceof PlainArray) {
      let messageObj: Any = {};
      message.forEach((value: Any, key: Any) => {
        messageObj[this.toBaseData(key)] = this.toBaseData(value);
      });
      return messageObj;
    } else if (message instanceof Array) {
      let messageArr: Array<Any> = [];
      message.forEach((value: Any) => {
        messageArr.push(this.toBaseData(value));
      })
      return messageArr;
    } else if (message instanceof Object) {
      let messageObj: Any = {};
      Object.keys(message).forEach((key: Any) => {
        messageObj[this.toBaseData(key)] = this.toBaseData(message[key]);
      })
      return messageObj;
    } else {
      return message;
    }
  }
}