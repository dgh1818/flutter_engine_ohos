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
import FlutterEngineGroup, { Options } from '../../../../../main/ets/embedding/engine/FlutterEngineGroup';
import PlatformViewsController from '../../../../../main/ets/plugin/platform/PlatformViewsController';
import common from '@ohos.app.ability.common';
import FlutterEngine from '../../../../../main/ets/embedding/engine/FlutterEngine';

export default function FlutterEngineGroupTest() {
  const flutterEngineGroup = new FlutterEngineGroup();
  const platformViewsController = new PlatformViewsController();
  const context = getContext() as common.UIAbilityContext;
  const args: Array<string> = [];

  describe('FlutterEngineGroupTest', () => {

    it('checkLoader', 0, async (done: Function) => {
      console.info("uitest: checkLoader begin " + context);
      if (context) {
        flutterEngineGroup.checkLoader(context, args);
      }
      done();
    })

    it('createAndRunEngineByOptions', 0, async (done: Function) => {
      console.info("uitest: createAndRunEngineByOptions begin");
      if (context) {
        const options = new Options(context);
        const createAndRunEngineByOptionsCallBack = flutterEngineGroup.createAndRunEngineByOptions(options);
        expect(createAndRunEngineByOptionsCallBack).not().assertNull();
      }
      done();
    })

    it('createEngine', 0, async (done: Function) => {
      console.info("uitest: createEngine begin");
      if (context) {
        const fng: FlutterEngine = flutterEngineGroup.createEngine(context, platformViewsController);
        expect(fng).not().assertNull();
      }
      done();
    })
  })
}