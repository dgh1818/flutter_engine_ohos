/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { window } from '@kit.ArkUI';
import { hilog } from '@kit.PerformanceAnalysisKit';
import common from '@ohos.app.ability.common';
import FlutterEngine from '../../../main/ets/embedding/engine/FlutterEngine';
import FlutterEngineGroup, { Options } from '../../../main/ets/embedding/engine/FlutterEngineGroup';
import FlutterShellArgs from '../../../main/ets/embedding/engine/FlutterShellArgs';
import { FlutterView } from '../../../main/ets/view/FlutterView';
import FlutterManager from '../../../main/ets/embedding/ohos/FlutterManager';

const TAG = 'FlutterRenderAbility';

export default class FlutterRenderAbility extends UIAbility {
  private flutterEngine: FlutterEngine | null = null;
  private flutterView: FlutterView | null = null;

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    hilog.info(0x0000, TAG, 'onCreate');
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    hilog.info(0x0000, TAG, 'onWindowStageCreate');
    const context = this.context as common.UIAbilityContext;
    FlutterManager.getInstance().pushUIAbility(this);
    FlutterManager.getInstance().pushWindowStage(this, windowStage);

    try {
      const group = new FlutterEngineGroup();
      group.checkLoader(context, new FlutterShellArgs().toArray());
      const options = new Options(context);
      this.flutterEngine = group.createAndRunEngineByOptions(options);
      this.flutterView = FlutterManager.getInstance().createFlutterView(context);
      this.flutterView.attachToFlutterEngine(this.flutterEngine);
      this.flutterView.onWindowCreated();

      AppStorage.setOrCreate('flutterRenderViewId', this.flutterView.getId());
      windowStage.loadContent('testability/pages/FlutterRenderPage', (err) => {
        if (err.code) {
          hilog.error(0x0000, TAG, 'loadContent failed: %{public}s', JSON.stringify(err));
          return;
        }
        this.flutterView?.onSurfaceCreated();
      });
    } catch (e) {
      hilog.error(0x0000, TAG, 'onWindowStageCreate error: %{public}s', JSON.stringify(e));
    }
  }

  onWindowStageDestroy() {
    this.flutterView?.onDestroy();
    this.flutterView = null;
    this.flutterEngine?.destroy();
    this.flutterEngine = null;
    FlutterManager.getInstance().popWindowStage(this);
  }

  onDestroy() {
    FlutterManager.getInstance().popUIAbility(this);
    hilog.info(0x0000, TAG, 'onDestroy');
  }

  onForeground() {
    hilog.info(0x0000, TAG, 'onForeground');
  }

  onBackground() {
    hilog.info(0x0000, TAG, 'onBackground');
  }
}
