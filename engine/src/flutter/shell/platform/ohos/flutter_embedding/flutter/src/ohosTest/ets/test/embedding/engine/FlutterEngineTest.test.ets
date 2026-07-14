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
import { beforeAll, describe, expect, it } from '@ohos/hypium';
import DartExecutor, { DartEntrypoint } from '../../../../../main/ets/embedding/engine/dart/DartExecutor';
import { Options } from '../../../../../main/ets/embedding/engine/FlutterEngineGroup';
import ActivityControlSurface from '../../../../../main/ets/embedding/engine/plugins/ability/AbilityControlSurface';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import common from '@ohos.app.ability.common';
import LocalizationPlugin from '../../../../../main/ets/plugin/localization/LocalizationPlugin';
import AccessibilityChannel from '../../../../../main/ets/embedding/engine/systemchannels/AccessibilityChannel';
import RestorationChannel from '../../../../../main/ets/embedding/engine/systemchannels/RestorationChannel';
import FlutterLoader from '../../../../../main/ets/embedding/engine/loader/FlutterLoader';
import SettingsChannel from '../../../../../main/ets/embedding/engine/systemchannels/SettingsChannel';
import PluginRegistry from '../../../../../main/ets/embedding/engine/plugins/PluginRegistry';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import { FlutterRenderer } from '../../../../../main/ets/embedding/engine/renderer/FlutterRenderer';
import LocalizationChannel from '../../../../../main/ets/embedding/engine/systemchannels/LocalizationChannel';
import MouseCursorChannel from '../../../../../main/ets/embedding/engine/systemchannels/MouseCursorChannel';
import SystemChannel from '../../../../../main/ets/embedding/engine/systemchannels/SystemChannel';
import PlatformChannel from '../../../../../main/ets/embedding/engine/systemchannels/PlatformChannel';
import TextInputChannel from '../../../../../main/ets/embedding/engine/systemchannels/TextInputChannel';
import NavigationChannel from '../../../../../main/ets/embedding/engine/systemchannels/NavigationChannel';
import LifecycleChannel from '../../../../../main/ets/embedding/engine/systemchannels/LifecycleChannel';

export default function FlutterEngineTest() {
  const platformViewsController = new PlatformViewsController();
  let context: common.UIAbilityContext;
  let flutterEngine: FlutterEngine;

  describe('FlutterEngineTest', () => {

    beforeAll(() => {
      const context = getContext() as common.UIAbilityContext;
      flutterEngine = new FlutterEngine(context, null, null, platformViewsController);
    })

    it('spawn', 0, async (done: Function) => {
      console.info("uitest: spawn begin");
      const options = new Options(context);
      let dartEntrypoint: DartEntrypoint | null = options.getDartEntrypoint();
      let initialRoute: string = options.getInitialRoute();
      let dartEntrypointArgs: Array<string> = options.getDartEntrypointArgs();
      let eg: FlutterEngine | null = null;
      if (dartEntrypoint) {
        eg = await flutterEngine.spawn(context,
          dartEntrypoint,
          initialRoute,
          dartEntrypointArgs,
          platformViewsController,
          false);
        if (eg) {
          expect(eg).not().assertNull();
        }
      }
      done();
    })

    it('getLifecycleChannel', 0, async (done: Function) => {
      console.info("uitest: getLifecycleChannel begin");
      if (flutterEngine) {
        const lifecycleChannel: LifecycleChannel | null = flutterEngine.getLifecycleChannel();
        if (lifecycleChannel) {
          expect(lifecycleChannel).not().assertNull();
        }
      }
      done();
    })

    it('getNavigationChannel', 0, async (done: Function) => {
      console.info("uitest: getNavigationChannel begin");
      if (flutterEngine) {
        const navigationChannel: NavigationChannel | null = flutterEngine.getNavigationChannel();
        if (navigationChannel) {
          expect(navigationChannel).not().assertNull();
        }
      }
      done();
    })

    it('getTextInputChannel', 0, async (done: Function) => {
      console.info("uitest: getTextInputChannel begin");
      if (flutterEngine) {
        const textInputChannel: TextInputChannel | null = flutterEngine.getTextInputChannel();
        if (textInputChannel) {
          expect(textInputChannel).not().assertNull();
        }
      }
      done();
    })

    it('getPlatformChannel', 0, async (done: Function) => {
      console.info("uitest: getPlatformChannel begin");
      if (flutterEngine) {
        const platformChannel: PlatformChannel | null = flutterEngine.getPlatformChannel();
        if (platformChannel) {
          expect(platformChannel).not().assertNull();
        }
      }
      done();
    })

    it('getSystemChannel', 0, async (done: Function) => {
      console.info("uitest: getSystemChannel begin");
      if (flutterEngine) {
        const systemChannel: SystemChannel | null = flutterEngine.getSystemChannel();
        if (systemChannel) {
          expect(systemChannel).not().assertNull();
        }
      }
      done();
    })

    it('getLocaleChannel', 0, async (done: Function) => {
      console.info("uitest: getLocaleChannel begin");
      if (flutterEngine) {
        const localizationChannel: LocalizationChannel | null = flutterEngine.getLocaleChannel();
        if (localizationChannel) {
          expect(localizationChannel).not().assertNull();
        }
      }
      done();
    })

    it('getMouseCursorChannel', 0, async (done: Function) => {
      console.info("uitest: getMouseCursorChannel begin");
      if (flutterEngine) {
        const mouseCursorChannel: MouseCursorChannel | null = flutterEngine.getMouseCursorChannel();
        if (mouseCursorChannel) {
          expect(mouseCursorChannel).not().assertNull();
        }
      }
      done();
    })

    it('getFlutterNapi', 0, async (done: Function) => {
      console.info("uitest: getFlutterNapi begin");
      if (flutterEngine) {
        const flutterNapi: FlutterNapi = flutterEngine.getFlutterNapi();
        expect(flutterNapi).not().assertNull();
      }
      done();
    })

    it('getFlutterRenderer', 0, async (done: Function) => {
      console.info("uitest: getFlutterRenderer begin");
      if (flutterEngine) {
        const flutterRenderer: FlutterRenderer = flutterEngine.getFlutterRenderer();
        expect(flutterRenderer).not().assertNull();
      }
      done();
    })

    it('getDartExecutor', 0, async (done: Function) => {
      console.info("uitest: getDartExecutor begin");
      if (flutterEngine) {
        const dartExecutor: DartExecutor = flutterEngine.getDartExecutor();
        expect(dartExecutor).not().assertNull();
      }
      done();
    })

    it('getPlugins', 0, async (done: Function) => {
      console.info("uitest: getPlugins begin");
      if (flutterEngine) {
        const pluginRegistry: PluginRegistry | null = flutterEngine.getPlugins();
        if (pluginRegistry) {
          expect(pluginRegistry).not().assertNull();
        }
      }
      done();
    })

    it('getAbilityControlSurface', 0, async (done: Function) => {
      console.info("uitest: getAbilityControlSurface begin");
      if (flutterEngine) {
        const activityControlSurface: ActivityControlSurface | null = flutterEngine.getAbilityControlSurface();
        if (activityControlSurface) {
          expect(activityControlSurface).not().assertNull();
        }
      }
      done();
    })

    it('getSettingsChannel', 0, async (done: Function) => {
      console.info("uitest: getSettingsChannel begin");
      if (flutterEngine) {
        const settingsChannel: SettingsChannel | null = flutterEngine.getSettingsChannel();
        if (settingsChannel) {
          expect(settingsChannel).not().assertNull();
        }
      }
      done();
    })

    it('getFlutterLoader', 0, async (done: Function) => {
      console.info("uitest: getFlutterLoader begin");
      if (flutterEngine) {
        const flutterLoader: FlutterLoader = flutterEngine.getFlutterLoader();
        expect(flutterLoader).not().assertNull();
      }
      done();
    })

    it('getRestorationChannel', 0, async (done: Function) => {
      console.info("uitest: getRestorationChannel begin");
      if (flutterEngine) {
        const restorationChannel: RestorationChannel | null = flutterEngine.getRestorationChannel();
        if (restorationChannel) {
          expect(restorationChannel).not().assertNull();
        }
      }
      done();
    })

    it('getAccessibilityChannel', 0, async (done: Function) => {
      console.info("uitest: getAccessibilityChannel begin");
      if (flutterEngine) {
        const accessibilityChannel: AccessibilityChannel | null = flutterEngine.getAccessibilityChannel();
        if (accessibilityChannel) {
          expect(accessibilityChannel).not().assertNull();
        }
      }
      done();
    })

    it('getLocalizationPlugin', 0, async (done: Function) => {
      console.info("uitest: getLocalizationPlugin begin");
      if (flutterEngine) {
        const localizationChannel: LocalizationPlugin | null = flutterEngine.getLocalizationPlugin();
        if (localizationChannel) {
          expect(localizationChannel).not().assertNull();
        }
      }
      done();
    })

    it('getPlatformViewsController', 0, async (done: Function) => {
      console.info("uitest: getPlatformViewsController begin");
      if (flutterEngine) {
        const platformViewsController: PlatformViewsController | null = flutterEngine.getPlatformViewsController();
        if (platformViewsController) {
          expect(platformViewsController).not().assertNull();
        }
      }
      done();
    })
  })
}