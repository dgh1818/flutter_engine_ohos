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

import LocalizationChannel, {
  LocalizationMessageHandler
} from '../../embedding/engine/systemchannels/LocalizationChannel'
import common from '@ohos.app.ability.common';
import intl from '@ohos.intl';
import Log from '../../util/Log';
import i18n from '@ohos.i18n';

const TAG = "LocalizationPlugin";

export default class LocalizationPlugin {
  private localizationChannel: LocalizationChannel;
  private context: common.Context;

  localeFromString(localeString: string): intl.Locale {
    localeString = localeString.replace('_', '-');
    let parts: string[] = localeString.split('-', -1);
    let languageCode = parts[0];
    let scriptCode = "";
    let countryCode = "";
    let index: number = 1;

    if (parts.length > index && parts[index].length == 4) {
      scriptCode = parts[index];
      index++;
    }

    if (parts.length > index && parts[index].length >= 2 && parts[index].length <= 3) {
      countryCode = parts[index];
      index++;
    }
    return new intl.Locale(languageCode + '-' + countryCode + '-' + scriptCode);
  }

  private localizationMessageHandler: LocalizationMessageHandler =
    new enterGetStringResource((key: string, localeString: string | null) => {

      Log.i(TAG, "getStringResource,key: " + key + ",localeString: " + localeString);
      let localContext: common.Context = this.context;
      let stringToReturn: string | null = null;
      // 获取资源管理器
      let resMgr = localContext.resourceManager;

      try {
        // 如果localeString不为空，则更新为指定地区的资源管理器
        if (localeString) {
          let overrideConfig = resMgr.getOverrideConfiguration();
          overrideConfig.locale = localeString;
          let overrideResMgr = resMgr.getOverrideResourceManager(overrideConfig);
          stringToReturn = overrideResMgr.getStringByNameSync(key);
        } else {
          stringToReturn = resMgr.getStringByNameSync(key);
        }
      } catch (e) {
        Log.e(TAG, e);
        return null;
      }

      return stringToReturn;
    })

  constructor(context: common.Context, localizationChannel: LocalizationChannel) {
    this.context = context;
    this.localizationChannel = localizationChannel;
    this.localizationChannel.setLocalizationMessageHandler(this.localizationMessageHandler);
  }

  sendLocaleToFlutter(): void {
    let systemLocale: string = i18n.System.getSystemLocale();
    let data: Array<string> = [];
    data.push(systemLocale);
    this.localizationChannel.sendLocales(data);
  }
}

class enterGetStringResource {
  getStringResource: (key: string, localeString: string | null) => string | null

  constructor(getStringResource: (key: string, localeString: string | null) => string | null) {
    this.getStringResource = getStringResource
  }
}