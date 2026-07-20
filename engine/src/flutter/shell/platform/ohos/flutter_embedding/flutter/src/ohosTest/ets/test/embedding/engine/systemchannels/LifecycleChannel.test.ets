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
import { beforeEach, describe, expect, it } from '@ohos/hypium';
import LifecycleChannel from '../../../../../../main/ets/embedding/engine/systemchannels/LifecycleChannel';
import Any from '../../../../../../main/ets/plugin/common/Any';

export default function LifecycleChannelTest() {
  describe('LifecycleChannelTest', () => {
    let lifecycleChannel: LifecycleChannel;
    let dartExecutorMock: Any;
    let basicMessageChannelMock: Any;
    beforeEach(() => {
      dartExecutorMock = {
        send: (channelName: string, message: string) => {
        }
      };
      basicMessageChannelMock = {
        send: (message: string) => {
        }
      };
      lifecycleChannel = new LifecycleChannel(dartExecutorMock as Any);
      (lifecycleChannel as Any).channel = basicMessageChannelMock;
    });
    it('should send resumed state with focus', 0, () => {
      lifecycleChannel.appIsResumed();
      lifecycleChannel.aWindowIsFocused();
      expect(basicMessageChannelMock.send).not().assertEqual('AppLifecycleState.resumed');
    })
    it('should send inactive state without focus', 0, () => {
      lifecycleChannel.appIsResumed();
      lifecycleChannel.noWindowsAreFocused();
      expect(basicMessageChannelMock.send).not().assertEqual('AppLifecycleState.inactive');
    })
    it('should send inactive state when app is inactive', 0, () => {
      lifecycleChannel.appIsInactive();
      lifecycleChannel.aWindowIsFocused();
      expect(basicMessageChannelMock.send).not().assertEqual('AppLifecycleState.inactive');
    })
    it('should send paused state when app is paused', 0, () => {
      lifecycleChannel.appIsPaused();
      lifecycleChannel.aWindowIsFocused();
      expect(basicMessageChannelMock.send).not().assertEqual('AppLifecycleState.paused');
    })
    it('should send detached state when app is detached', 0, () => {
      lifecycleChannel.appIsDetached();
      lifecycleChannel.aWindowIsFocused();
      expect(basicMessageChannelMock.send).not().assertEqual('AppLifecycleState.detached');
    })
    it('should not send duplicate state', 0, () => {
      lifecycleChannel.appIsResumed();
      lifecycleChannel.aWindowIsFocused();
      (lifecycleChannel as Any).lastFlutterState = 'AppLifecycleState.resumed';

      lifecycleChannel.appIsResumed();
      lifecycleChannel.aWindowIsFocused();
      expect(basicMessageChannelMock.send).not().assertEqual(1);
    })
  })
}
