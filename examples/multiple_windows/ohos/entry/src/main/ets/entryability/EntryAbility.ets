/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */


import { window } from '@kit.ArkUI';
import { FlutterAbility, FlutterEngine, FlutterEngineCache, FlutterAbilityLaunchConfigs } from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';

export default class EntryAbility extends FlutterAbility {
  // Window titles are not configurable via module.json5 (label is only the
  // desktop icon label); setWindowTitle() (API 15+) must run after
  // loadContent() settles and only works in free-window mode — hence the
  // delayed re-assert. Idempotent.
  private applyMainWindowTitle(windowStage: window.WindowStage): void {
    try {
      windowStage.getMainWindowSync().setWindowTitle('Multi-Window Reference Application')
        .catch((e: Object) => {
          console.warn(`EntryAbility setWindowTitle rejected: ${JSON.stringify(e)}`);
        });
    } catch (e) {
      console.warn(`EntryAbility setWindowTitle failed: ${JSON.stringify(e)}`);
    }
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    super.onWindowStageCreate(windowStage);
    this.applyMainWindowTitle(windowStage);
    setTimeout(() => this.applyMainWindowTitle(windowStage), 500);
  }

  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine)
    GeneratedPluginRegistrant.registerWith(flutterEngine)
    // Single-engine multi-view: cache the engine under a stable id so
    // RegularWindowAbility instances reuse it instead of spawning their own.
    FlutterEngineCache.getInstance().put(
      FlutterAbilityLaunchConfigs.MULTI_WINDOW_ENGINE_CACHE_ID,
      flutterEngine);
  }
}
