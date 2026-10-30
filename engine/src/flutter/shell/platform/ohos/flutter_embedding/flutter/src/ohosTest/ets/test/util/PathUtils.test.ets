/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import common from '@ohos.app.ability.common';
import PathUtils from '../../../../main/ets/util/PathUtils';

export default function pathUtilsTest() {
  describe('PathUtils.getFilesDir', () => {
    it('returns a non-empty path from getFilesDir', Level.LEVEL1, () => {
      const result: string = PathUtils.getFilesDir(getContext() as common.Context);
      expect(result.length).assertLarger(0);
    });

    it('matches context.filesDir exactly', Level.LEVEL1, () => {
      const ctx = getContext() as common.Context;
      expect(PathUtils.getFilesDir(ctx)).assertEqual(ctx.filesDir);
    });
  });

  describe('PathUtils.getCacheDirectory', () => {
    it('returns a non-empty path from getCacheDirectory', Level.LEVEL1, () => {
      const result: string = PathUtils.getCacheDirectory(getContext() as common.Context);
      expect(result.length).assertLarger(0);
    });

    it('matches context.cacheDir exactly', Level.LEVEL1, () => {
      const ctx = getContext() as common.Context;
      expect(PathUtils.getCacheDirectory(ctx)).assertEqual(ctx.cacheDir);
    });
  });

  describe('PathUtils.getDataDirectory', () => {
    it('returns a path ending with flutter', Level.LEVEL1, () => {
      const result: string | null = PathUtils.getDataDirectory(getContext() as common.Context);
      expect(result != null).assertEqual(true);
      expect(result as string).assertContain('flutter');
    });

    it('returns context.filesDir + /flutter', Level.LEVEL1, () => {
      const ctx = getContext() as common.Context;
      const result: string | null = PathUtils.getDataDirectory(ctx);
      expect(result as string).assertEqual(ctx.filesDir + '/flutter');
    });

    it('is idempotent (first call creates, second call reuses)', Level.LEVEL1, () => {
      const ctx = getContext() as common.Context;
      const first: string | null = PathUtils.getDataDirectory(ctx);
      const second: string | null = PathUtils.getDataDirectory(ctx);
      expect(first != null).assertEqual(true);
      expect(second != null).assertEqual(true);
      expect(first).assertEqual(second);
    });

    it('called multiple times always returns the same path', Level.LEVEL1, () => {
      const ctx = getContext() as common.Context;
      const r1: string | null = PathUtils.getDataDirectory(ctx);
      const r2: string | null = PathUtils.getDataDirectory(ctx);
      const r3: string | null = PathUtils.getDataDirectory(ctx);
      expect(r1).assertEqual(r2);
      expect(r2).assertEqual(r3);
    });
  });
}
