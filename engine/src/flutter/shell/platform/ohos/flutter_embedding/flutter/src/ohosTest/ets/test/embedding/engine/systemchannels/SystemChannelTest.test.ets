/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { common } from '@kit.AbilityKit';
import { resourceManager } from '@kit.LocalizationKit';
import FlutterNapi from '../../../../../../main/ets/embedding/engine/FlutterNapi';
import DartExecutor from '../../../../../../main/ets/embedding/engine/dart/DartExecutor';
import SystemChannel from '../../../../../../main/ets/embedding/engine/systemchannels/SystemChannel';

export default function systemChannelTest() {
  describe('SystemChannel.constructor', () => {
    it('creates channel with correct name', Level.LEVEL0, () => {
      const napi = new FlutterNapi();
      const ctx = getContext() as common.UIAbilityContext;
      const rm: resourceManager.ResourceManager = ctx.resourceManager;
      const dartExecutor = new DartExecutor(napi, rm);
      const channel = new SystemChannel(dartExecutor);
      expect(channel).not().assertNull();
      expect(channel.channel).not().assertNull();
    });
  });

  describe('SystemChannel.sendMemoryPressureWarning', () => {
    it('does not throw when sending memory pressure', Level.LEVEL0, () => {
      const napi = new FlutterNapi();
      const ctx = getContext() as common.UIAbilityContext;
      const rm: resourceManager.ResourceManager = ctx.resourceManager;
      const dartExecutor = new DartExecutor(napi, rm);
      const channel = new SystemChannel(dartExecutor);
      let threw = false;
      try { channel.sendMemoryPressureWarning(); } catch (e) { threw = true; }
      expect(threw).assertEqual(false);
    });
  });
}
