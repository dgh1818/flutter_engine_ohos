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
import { beforeEach, describe, expect, it } from '@ohos/hypium';
import LocalizationPlugin from '../../../../../main/ets/plugin/localization/LocalizationPlugin';
import common from '@ohos.app.ability.common';
import LocalizationChannel from '../../../../../main/ets/embedding/engine/systemchannels/LocalizationChannel';
import intl from '@ohos.intl';
import DartExecutor from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import FlutterInjector from '../../../../../main/ets/FlutterInjector';
import Any from '../../../../../main/ets/plugin/common/Any';

export default function LocalizationPluginTest() {
  describe('LocalizationPluginTest', () => {
    let localizationChannel: LocalizationChannel;
    let localizationPlugin: LocalizationPlugin;
    let dartExecutor: DartExecutor;

    beforeEach(() => {
      let context: common.Context = getContext();
      const assetManager = context.resourceManager;
      const flutterNapi = FlutterInjector.getInstance().getFlutterNapi();
      dartExecutor = new DartExecutor(flutterNapi, assetManager);
      localizationChannel = new LocalizationChannel(dartExecutor); // 初始化LocalizationChannel
      localizationPlugin = new LocalizationPlugin(context, localizationChannel);
    });
    it('test_localeFromString', 0, () => {
      let localeString = "zh_CN";
      let expectedLocale = new intl.Locale("zh_CN");
      let actualLocale = localizationPlugin.localeFromString(localeString);

      expect(actualLocale.language).assertEqual(expectedLocale.language);
      expect(actualLocale.region).assertEqual(expectedLocale.region);
      expect(actualLocale.script).assertEqual(expectedLocale.script);
    })
    it('testSendLocaleToFlutter', 0, () => {
      let mockSystemLocale = 'zh-Hans-CN';
      (localizationChannel as Any).sendLocales = (locales:Any) => {
        expect(locales).assertContain(mockSystemLocale);
      };
      localizationPlugin.sendLocaleToFlutter();
    })
  })
}