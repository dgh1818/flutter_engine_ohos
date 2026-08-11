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
import { common } from '@kit.AbilityKit';
import { Options } from '../../../../../../main/ets/embedding/engine/FlutterEngineGroup';
import DartExecutor, { DartEntrypoint } from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import { resourceManager } from '@kit.LocalizationKit';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';

export default function DartExecutorTest() {
  const flutterNapi = new FlutterNapi();
  let context: common.UIAbilityContext;
  let dartExecutor: DartExecutor;
  let options: Options;
  let assetManager: resourceManager.ResourceManager;

  describe('DartExecutorTest', () => {

    beforeAll(() => {
      context = getContext() as common.UIAbilityContext;
      assetManager = context.resourceManager;
      dartExecutor = new DartExecutor(flutterNapi, assetManager);
      options = new Options(context);
    })

    it('isExecutingDart', 0, async (done: Function) => {
      console.info("uitest: isExecutingDart begin");
      if (dartExecutor) {
        const isExecutingDart = dartExecutor.isExecutingDart();
        expect(isExecutingDart).not().assertNull();
      }
      done();
    })

    it('getBinaryMessenger', 0, async (done: Function) => {
      console.info("uitest: getBinaryMessenger begin");
      if (dartExecutor) {
        const binaryMessenger = dartExecutor.getBinaryMessenger();
        expect(binaryMessenger).not().assertNull();
      }
      done();
    })

    it('makeBackgroundTaskQueue', 0, async (done: Function) => {
      console.info("uitest: makeBackgroundTaskQueue begin");
      if (dartExecutor) {
        const taskQueue = dartExecutor.makeBackgroundTaskQueue();
        expect(taskQueue).not().assertNull();
      }
      done();
    })
  })
}