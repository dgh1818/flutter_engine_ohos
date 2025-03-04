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

import flutter from 'libflutter.so'

/**
 * 默认字符串工具
 */
export default class StringUtils {

  static stringToArrayBuffer(str: string): ArrayBuffer {
    if (str.length == 0) {
      return new ArrayBuffer(0);
    }
    return flutter.nativeEncodeUtf8(str).buffer;
  }

  static arrayBufferToString(buffer: ArrayBuffer): string {
    if (buffer.byteLength <= 0) {
      return "";
    }
    return flutter.nativeDecodeUtf8(new Uint8Array(buffer));
  }

  static uint8ArrayToString(buffer: Uint8Array): string {
    if (buffer.length <= 0) {
      return "";
    }
    return flutter.nativeDecodeUtf8(buffer);
  }

  static isNotEmpty(str: string): boolean {
    return str != null && str.length > 0;
  }

  static isEmpty(str: string): boolean {
    return (!str) || str.length == 0;
  }
}