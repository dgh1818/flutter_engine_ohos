/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import hilog from '@ohos.hilog';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium'

export default function abilityTest() {
  describe('PlatformInteractionTest', function () {
    beforeAll(function () {
      hilog.info(0x0000, 'testTag', '%{public}s', 'PlatformInteraction beforeAll called');
    })
    beforeEach(function () {
      hilog.info(0x0000, 'testTag', '%{public}s', 'PlatformInteraction beforeEach called');
    })
    afterEach(function () {
      hilog.info(0x0000, 'testTag', '%{public}s', 'PlatformInteraction afterEach called');
    })
    afterAll(function () {
      hilog.info(0x0000, 'testTag', '%{public}s', 'PlatformInteraction afterAll called');
    })
    it('assertContain', 0, function () {
      hilog.info(0x0000, 'testTag', '%{public}s', 'it begin');
      let a = 'abc'
      let b = 'b'
      expect(a).assertContain(b)
      expect(a).assertEqual(a)
    })
  })
}
