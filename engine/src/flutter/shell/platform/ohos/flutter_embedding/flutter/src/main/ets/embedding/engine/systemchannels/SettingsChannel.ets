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

import BasicMessageChannel from '../../../plugin/common/BasicMessageChannel';
import JSONMessageCodec from '../../../plugin/common/JSONMessageCodec';
import Log from '../../../util/Log';
import DartExecutor from '../dart/DartExecutor';

export enum PlatformBrightness {
  LIGHT = "light",
  DARK = "dark"
}

const TAG = "SettingsChannel";
const TEXT_SCALE_FACTOR = "textScaleFactor";
const NATIVE_SPELL_CHECK_SERVICE_DEFINED = "nativeSpellCheckServiceDefined";
const BRIEFLY_SHOW_PASSWORD = "brieflyShowPassword";
const ALWAYS_USE_24_HOUR_FORMAT = "alwaysUse24HourFormat";
const PLATFORM_BRIGHTNESS = "platformBrightness";

export default class SettingsChannel {
  private static CHANNEL_NAME = "flutter/settings";
  private channel: BasicMessageChannel<Object>;
  
  constructor(dartExecutor: DartExecutor) {
    this.channel =
      new BasicMessageChannel<Object>(dartExecutor, SettingsChannel.CHANNEL_NAME, JSONMessageCodec.INSTANCE);
  }

  startMessage(): MessageBuilder {
    return new MessageBuilder(this.channel);
  }
}

class MessageBuilder {
  private channel: BasicMessageChannel<Object>;
  private settingsMessage: Map<String, Object> = new Map<String, Object>([
    [TEXT_SCALE_FACTOR, 1.0],
    [NATIVE_SPELL_CHECK_SERVICE_DEFINED, false],
    [BRIEFLY_SHOW_PASSWORD, false],
    [ALWAYS_USE_24_HOUR_FORMAT, false],
    [PLATFORM_BRIGHTNESS, PlatformBrightness.LIGHT]
  ]);

  constructor(channel: BasicMessageChannel<Object>) {
    this.channel = channel;
  }

  setTextScaleFactor(textScaleFactor: Number): MessageBuilder {
    this.settingsMessage.set(TEXT_SCALE_FACTOR, textScaleFactor);
    return this;
  }

  setNativeSpellCheckServiceDefined(nativeSpellCheckServiceDefined: boolean): MessageBuilder {
    this.settingsMessage.set(NATIVE_SPELL_CHECK_SERVICE_DEFINED, nativeSpellCheckServiceDefined);
    return this;
  }

  setBrieflyShowPassword(brieflyShowPassword: boolean): MessageBuilder {
    this.settingsMessage.set(BRIEFLY_SHOW_PASSWORD, brieflyShowPassword);
    return this;
  }

  setAlwaysUse24HourFormat(alwaysUse24HourFormat: boolean): MessageBuilder {
    this.settingsMessage.set(ALWAYS_USE_24_HOUR_FORMAT, alwaysUse24HourFormat);
    return this;
  }

  setPlatformBrightness(platformBrightness: PlatformBrightness): MessageBuilder {
    this.settingsMessage.set(PLATFORM_BRIGHTNESS, platformBrightness);
    return this;
  }

  send(): void {
    Log.i(TAG, "Sending message: "
      + TEXT_SCALE_FACTOR + " : "
      + this.settingsMessage.get(TEXT_SCALE_FACTOR)
      + ", " + NATIVE_SPELL_CHECK_SERVICE_DEFINED + " : "
      + this.settingsMessage.get(NATIVE_SPELL_CHECK_SERVICE_DEFINED)
      + ", " + BRIEFLY_SHOW_PASSWORD + " : "
      + this.settingsMessage.get(BRIEFLY_SHOW_PASSWORD)
      + ", " + ALWAYS_USE_24_HOUR_FORMAT + " : "
      + this.settingsMessage.get(ALWAYS_USE_24_HOUR_FORMAT)
      + ", " + PLATFORM_BRIGHTNESS + " : "
      + this.settingsMessage.get(PLATFORM_BRIGHTNESS));
    this.channel.send(this.settingsMessage)
  }
}