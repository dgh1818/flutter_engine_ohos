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
import { MediaQuery } from '@kit.ArkUI';
import { PlatformBrightness } from '../../../../../main/ets/embedding/engine/systemchannels/SettingsChannel';
import Settings from '../../../../../main/ets/embedding/ohos/Settings';

const settings: Settings = new Settings(null);

export default function SettingTest() {
  describe('SettingTest', () => {
    it('TestGetTextScaleFactor', 0, async (done: Function) => {
      const result: number = settings.getTextScaleFactor();
      expect(result).not().assertNull();
      done();
    })

    it('TestGetThemeMode', 0, async (done: Function) => {
      const mediaQuery: MediaQuery = new MediaQuery();
      settings.sendSettings(mediaQuery);
      const result: PlatformBrightness = settings.getThemeMode(mediaQuery);
      expect(result).not().assertNull();
      done();
    })
  })
}