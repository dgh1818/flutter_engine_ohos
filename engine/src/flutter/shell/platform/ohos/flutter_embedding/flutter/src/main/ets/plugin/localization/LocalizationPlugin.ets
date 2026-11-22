/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*
* Based on LocalizationPlugin.java originally written by
* Copyright (C) 2013 The Flutter Authors.
*
*/

import LocalizationChannel, {
  LocalizationMessageHandler
} from '../../embedding/engine/systemchannels/LocalizationChannel'
import common from '@ohos.app.ability.common';
import intl from '@ohos.intl';
import Log from '../../util/Log';
import i18n from '@ohos.i18n';

const TAG = "LocalizationPlugin";

/**
 * Plugin for handling localization in Flutter applications.
 * This class manages the interaction between Flutter's localization system
 * and the OpenHarmony resource management system.
 */
export default class LocalizationPlugin {
  private localizationChannel: LocalizationChannel;
  private context: common.Context;

  /**
   * Converts a locale string to an intl.Locale object.
   * @param localeString - The locale string (e.g., "en_US" or "zh-CN")
   * @returns The corresponding intl.Locale object
   */
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
      // Get the resource manager
      let resMgr = localContext.resourceManager;

      try {
        // If localeString is not null, update to the resource manager for the specified locale
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

  /**
   * Constructs a new LocalizationPlugin instance.
   * @param context - The application context
   * @param localizationChannel - The LocalizationChannel for communication with Flutter
   */
  constructor(context: common.Context, localizationChannel: LocalizationChannel) {
    this.context = context;
    this.localizationChannel = localizationChannel;
    this.localizationChannel.setLocalizationMessageHandler(this.localizationMessageHandler);
  }

  /**
   * Sends the preferred language list to Flutter.
   * A language that already has a region keeps it. A language without a region
   * gets the current system region, matching the list iOS puts in preferredLanguages.
   */
  sendLocaleToFlutter(): void {
    let languages: Array<string> = i18n.System.getPreferredLanguageList();
    let systemRegion: string = i18n.System.getSystemRegion();
    let data: Array<string> = [];
    for (let i = 0; i < languages.length; i++) {
      data.push(this.languageIdWithRegion(languages[i], systemRegion));
    }
    this.localizationChannel.sendLocales(data);
  }

  /**
   * Returns the language id unchanged when it already has a region.
   * Otherwise appends systemRegion, keeping the script when one is present.
   * Not private so unit tests can call each synthesis branch directly.
   *
   * The `locale.language.length === 0` check is purely defensive: a
   * preferred-language entry that intl.Locale cannot resolve to a language
   * keeps its original value instead of getting a region appended. The same
   * case is treated as reachable on iOS, which skips such entries
   * (FlutterEngine.mm: `if (!languageCode) { continue; }`); OHOS returns
   * them unchanged. It has no deterministic unit-test coverage because the
   * test HAP lacks full i18n capability: intl.Locale either throws on
   * ''/'-'/'--' or falls back to the system locale, so an empty language
   * cannot be constructed reliably.
   */
  languageIdWithRegion(languageId: string, systemRegion: string): string {
    let locale: intl.Locale = new intl.Locale(languageId);
    if (systemRegion.length === 0 || locale.language.length === 0 || locale.region.length > 0) {
      return languageId;
    }
    if (locale.script.length > 0) {
      return locale.language + '-' + locale.script + '-' + systemRegion;
    }
    return locale.language + '-' + systemRegion;
  }

  /**
   * Called when the application locale is changed via dart:ui setApplicationLocale.
   * @param locale - The application locale in BCP 47 format
   */
  onApplicationLocaleChanged(locale: string): void {
    Log.i(TAG, "onApplicationLocaleChanged: " + locale);
  }
}

/**
 * Implementation of LocalizationMessageHandler for getting string resources.
 */
class enterGetStringResource {
  /**
   * The function to get string resources.
   * @param key - The resource key
   * @param localeString - The locale string, or null to use the default locale
   * @returns The localized string, or null if not found
   */
  getStringResource: (key: string, localeString: string | null) => string | null

  /**
   * Constructs a new enterGetStringResource instance.
   * @param getStringResource - The function to get string resources
   */
  constructor(getStringResource: (key: string, localeString: string | null) => string | null) {
    this.getStringResource = getStringResource
  }
}
