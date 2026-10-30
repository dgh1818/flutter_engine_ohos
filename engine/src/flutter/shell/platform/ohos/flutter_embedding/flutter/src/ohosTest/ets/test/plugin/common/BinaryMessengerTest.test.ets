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
  TaskPriority,
  TaskQueue,
  TaskQueueOptions
} from '../../../../../main/ets/plugin/common/BinaryMessenger';

export default function BinaryMessengerTest() {
  let binaryMessenger: CurBinaryMessenger = new CurBinaryMessenger();

  describe('BinaryMessengerTest', () => {

    it('makeBackgroundTaskQueue', 0, async (done: Function) => {
      console.info("uitest: makeBackgroundTaskQueue begin");
      // Crash-cover: BinaryMessenger is an interface exercised via the local fake;
      // only the no-crash path is asserted (hcpp style).
      binaryMessenger.makeBackgroundTaskQueue(new TaskQueueOptions());
      done();
    })

    it('TaskQueueOptions defaults and setters', 0, () => {
      const options = new TaskQueueOptions();
      expect(options.getIsSerial()).assertEqual(true);
      expect(options.getPriority()).assertEqual(TaskPriority.MEDIUM);
      expect(options.isSingleThreadMode()).assertEqual(false);
      options.setIsSerial(false).setPriority(TaskPriority.HIGH).setSingleThreadMode(true);
      expect(options.getIsSerial()).assertEqual(false);
      expect(options.getPriority()).assertEqual(TaskPriority.HIGH);
      expect(options.isSingleThreadMode()).assertEqual(true);
    })

    it('TaskPriority enum values', 0, () => {
      expect(TaskPriority.HIGH).assertEqual(0);
      expect(TaskPriority.MEDIUM).assertEqual(1);
      expect(TaskPriority.LOW).assertEqual(2);
      expect(TaskPriority.IDLE).assertEqual(3);
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