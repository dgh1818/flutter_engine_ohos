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
import { lang } from '@kit.ArkTS';

import MethodCall from './MethodCall';
import { MethodResult } from './MethodChannel';

/** A handler of incoming method calls. */
type ISendable = lang.ISendable;
export default interface SendableMethodCallHandler extends ISendable {
  /**
   * Handles the specified method call received from Flutter.
   *
   * <p>Handler implementations must submit a result for all incoming calls, by making a single
   * call on the given {@link Result} callback. Failure to do so will result in lingering Flutter
   * result handlers. The result may be submitted asynchronously and on any thread. Calls to
   * unknown or unimplemented methods should be handled using {@link Result#notImplemented()}.
   *
   * <p>Any uncaught exception thrown by this method will be caught by the channel implementation
   * and logged, and an error result will be sent back to Flutter.
   *
   * <p>The handler is called on the platform thread (HarmonyOS main thread) by default, or
   * otherwise on the thread specified by the {@link BinaryMessenger.TaskQueue} provided to the
   * associated {@link MethodChannel} when it was created. See also <a
   * href="https://github.com/flutter/flutter/wiki/The-Engine-architecture#threading">Threading in
   * the Flutter Engine</a>.
   *
   * @param call A {@link MethodCall}.
   * @param result A {@link Result} used for submitting the result of the call.
   */
  onMethodCall(call: MethodCall, result: MethodResult, ...args: Object[]): void;
}