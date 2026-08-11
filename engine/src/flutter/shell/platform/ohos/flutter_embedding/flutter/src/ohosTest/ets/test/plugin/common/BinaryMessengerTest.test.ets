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
import {
  BinaryMessageHandler,
  BinaryMessenger,
  BinaryReply,
  TaskQueue,
  TaskQueueOptions
} from '../../../../../main/ets/plugin/common/BinaryMessenger';

export default function BinaryMessengerTest() {
  let binaryMessenger: CurBinaryMessenger = new CurBinaryMessenger();

  describe('BinaryMessengerTest', () => {

    it('makeBackgroundTaskQueue', 0, async (done: Function) => {
      console.info("uitest: makeBackgroundTaskQueue begin");
      const options = new TaskQueueOptions();
      const taskQueue: TaskQueue = binaryMessenger.makeBackgroundTaskQueue(options);
      expect(taskQueue).not().assertNull();
      done();
    })
  })
}

class CurBinaryMessenger implements BinaryMessenger {
  makeBackgroundTaskQueue(options?: TaskQueueOptions | undefined): TaskQueue {
    return {};
  }

  send(channel: String, message: ArrayBuffer, callback?: BinaryReply | null | undefined) {

  }

  setMessageHandler(channel: String, handler: BinaryMessageHandler | BinaryMessageHandler | null, taskQueue?: TaskQueue,
    ...args: Object[]) {
  }
}