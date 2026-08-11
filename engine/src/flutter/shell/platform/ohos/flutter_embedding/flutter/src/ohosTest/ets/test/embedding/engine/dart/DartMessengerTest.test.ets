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

import { DartMessenger } from '../../../../../../main/ets/embedding/engine/dart/DartMessenger';
import { describe, expect, it } from '@ohos/hypium';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import { TaskQueue, } from '../../../../../../main/ets/plugin/common/BinaryMessenger';
export default function DartMessengerTest() {
  describe('FlutterDartMessengerTest', () => {
    it('TestMakeBackgroundTaskQueue', 0, async (done: Function) => {
      const flutterNapi: FlutterNapi = new FlutterNapi();
      const dartMessage = new DartMessenger(flutterNapi);
      let taskQueue: TaskQueue = dartMessage.makeBackgroundTaskQueue();
      expect(taskQueue).not().assertNull();
      done();
    })

    it('TestGetPendingChannelResponseCount', 0, async (done: Function) => {
      const flutterNapi: FlutterNapi = new FlutterNapi();
      const messenger = new DartMessenger(flutterNapi);
      let count: number = messenger.getPendingChannelResponseCount();
      expect(count).assertEqual(0);
      done();
    })

    it('TestAskQueueFactory', 0, async (done: Function) => {
      const flutterNapi: FlutterNapi = new FlutterNapi();
      const messenger = new DartMessenger(flutterNapi);
      expect(messenger.taskQueueFactory).not().assertNull();
      done();
    })
  })
}