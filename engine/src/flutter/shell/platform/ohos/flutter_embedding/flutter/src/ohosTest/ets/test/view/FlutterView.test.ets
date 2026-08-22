/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { FlutterView } from '../../../../main/ets/view/FlutterView';

export default function flutterViewTest() {
  describe('FlutterView.getFlutterEngine', () => {
    it('returns null when no engine is attached', Level.LEVEL0, () => {
      const flutterView: FlutterView = new FlutterView('oh_flutter_test_view', getContext());
      expect(flutterView.getFlutterEngine()).assertNull();
    });
  });
}
