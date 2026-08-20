/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import Any from '../../../../../main/ets/plugin/common/Any';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';
import FlutterNapi from '../../../../../main/ets/embedding/engine/FlutterNapi';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';

function makeEngine(): FlutterEngine {
  const context: common.UIAbilityContext = getContext() as common.UIAbilityContext;
  return new FlutterEngine(context, null, new FlutterNapi(), new PlatformViewsController());
}

export default function FlutterEngineHybridTest() {
  describe('FlutterEngineHybridTest', () => {
    it('constructor_wires_hybrid_controller_and_getter', 0, () => {
      const engine: FlutterEngine = makeEngine();
      expect(engine.getPlatformViewsControllerHybrid()).not().assertNull();
      expect(engine.getPlatformViewsController()).not().assertNull();
    })

    it('onPreEngineRestart_safe_with_and_without_hybrid_controller', 0, () => {
      const engine: FlutterEngine = makeEngine();
      engine.onPreEngineRestart();
      expect(engine.getPlatformViewsControllerHybrid()).not().assertNull();
      const anyEngine: Any = engine as Any;
      anyEngine.platformViewsControllerHybrid = null;
      engine.onPreEngineRestart();
      expect(engine.getPlatformViewsControllerHybrid()).assertNull();
    })
  })
}
