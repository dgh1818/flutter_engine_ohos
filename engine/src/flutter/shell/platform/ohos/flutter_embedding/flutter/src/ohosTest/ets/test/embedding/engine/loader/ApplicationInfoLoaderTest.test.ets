/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, expect, it } from '@ohos/hypium';
import ApplicationInfoLoader from '../../../../../../main/ets/embedding/engine/loader/ApplicationInfoLoader';

export default function ApplicationInfoLoaderTest() {
  describe('ApplicationInfoLoaderTest', () => {

    // This test bundle's module.json5 declares no metadata, so these tests
    // pin the null-metadata path against the real bundleManager API. The
    // metadata-consuming path (metadata -> engine args) is covered in
    // FlutterLoaderTest through a patched getModuleMetadata seam; the
    // bundleManager namespace itself cannot be mocked in ArkTS.
    it('getModuleMetadata_returnsEmptyMapWhenNoMetadataDeclared', 0, () => {
      const metadata = ApplicationInfoLoader.getModuleMetadata();
      expect(metadata).not().assertNull();
      expect(metadata.size).assertEqual(0);
    })

    it('getModuleMetadata_freshMapOnEachCall', 0, () => {
      const first = ApplicationInfoLoader.getModuleMetadata();
      const second = ApplicationInfoLoader.getModuleMetadata();
      // Metadata is re-read on each call; the map must not be cached.
      expect(first === second).assertFalse();
    })
  })
}
