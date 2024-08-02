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

import ToolUtils from '../../util/ToolUtils';
import TreeMap from '@ohos.util.TreeMap';
import HashMap from '@ohos.util.HashMap';
import LightWeightMap from '@ohos.util.LightWeightMap';
import Any from './Any';

/** Command object representing a method call on a {@link MethodChannel}. */
export default class MethodCall {
  /** The name of the called method. */
  method: string;
  /**
   * Arguments for the call.
   *
   * <p>Consider using {@link #arguments()} for cases where a particular run-time type is expected.
   * Consider using {@link #argument(String)} when that run-time type is {@link Map} or {@link
   * JSONObject}.
   */
  args: Any;

  constructor(method: string, args: Any) {
    this.method = method;
    this.args = args;
  }

  argument(key: string): Any {
    if (this.args == null) {
      return null;
    } else if (this.args instanceof Map) {
      return (this.args as Map<Any, Any>).get(key);
    } else if (ToolUtils.isObj(this.args)) {
      return this.args[key];
    } else {
      throw new Error("ClassCastException");
    }
  }

  hasArgument(key: string): boolean {
    if (this.args == null) {
      return false;
    } else if (this.args instanceof Map) {
      return (this.args as Map<Any, Any>).has(key);
    } else if (ToolUtils.isObj(this.args)) {
      return this.args.hasOwnProperty(key);
    } else {
      throw new Error("ClassCastException");
    }
  }
}