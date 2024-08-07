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

import HiLog from '@ohos.hilog';
import BuildProfile from '../../../../BuildProfile';

const DOMAIN: number = 0x00FF;
const TAG = "Flutter";
const SYMBOL = " --> ";

/**
 * Basic log class
 */
export default class Log {
  /**
   * Outputs debug-level logs.
   *
   * @param tag Identifies the log tag.
   * @param format Indicates the log format string.
   * @param args Indicates the log parameters.
   * @since 7
   */
  static d(tag: string, format: string, ...args: Object[]) {
    if (Log.isLoggable(HiLog.LogLevel.DEBUG)) {
      HiLog.debug(DOMAIN, TAG, tag + SYMBOL + format, args);
    }
  }

  /**
   * Outputs info-level logs.
   *
   * @param tag Identifies the log tag.
   * @param format Indicates the log format string.
   * @param args Indicates the log parameters.
   * @since 7
   */
  static i(tag: string, format: string, ...args: Object[]) {
    if (Log.isLoggable(HiLog.LogLevel.INFO)) {
      HiLog.info(DOMAIN, TAG, tag + SYMBOL + format, args);
    }
  }

  /**
   * Outputs warning-level logs.
   *
   * @param tag Identifies the log tag.
   * @param format Indicates the log format string.
   * @param args Indicates the log parameters.
   * @since 7
   */
  static w(tag: string, format: string, ...args: Object[]) {
    if (Log.isLoggable(HiLog.LogLevel.WARN)) {
      HiLog.warn(DOMAIN, TAG, tag + SYMBOL + format, args);
    }
  }

  /**
   * Outputs error-level logs.
   *
   * @param tag Identifies the log tag.
   * @param format Indicates the log format string.
   * @param args Indicates the log parameters.
   * @since 7
   */
  static e(tag: string, format: string, ...args: Object[]) {
    if (Log.isLoggable(HiLog.LogLevel.ERROR)) {
      args.forEach((item: Object, index: number) => {
        if (item instanceof Error) {
          args[index] = item.message + item.stack;
        }
        format += "%{public}s";
      })
      HiLog.error(DOMAIN, TAG, tag + SYMBOL + format, args);
    }
  }

  /**
   * Outputs fatal-level logs.
   *
   * @param tag Identifies the log tag.
   * @param format Indicates the log format string.
   * @param args Indicates the log parameters.
   * @since 7
   */
  static f(tag: string, format: string, ...args: Object[]) {
    if (Log.isLoggable(HiLog.LogLevel.FATAL)) {
      HiLog.fatal(DOMAIN, TAG, tag + SYMBOL + format, args);
    }
  }

  /**
   * Checks whether logs of the specified tag, and level can be printed.
   *
   * @param tag Identifies the log tag.
   * @param level log level
   * @since 7
   */
  private static isLoggable(level: HiLog.LogLevel): boolean {
    let buildModeName: string = BuildProfile.BUILD_MODE_NAME.toLowerCase();
    if (buildModeName == 'release' || buildModeName == 'profile') {
      return level >= HiLog.LogLevel.WARN && HiLog.isLoggable(DOMAIN, TAG, level);
    }
    return HiLog.isLoggable(DOMAIN, TAG, level);
  }
}
