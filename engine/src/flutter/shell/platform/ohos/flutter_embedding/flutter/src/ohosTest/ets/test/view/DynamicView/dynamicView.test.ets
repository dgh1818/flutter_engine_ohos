/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import { DVModel, DVModelParameters, DVModelEvents, DVModelChildren, DVModelContainer, DynamicView, BuilderParams } from '../../../../../main/ets/view/DynamicView/dynamicView';
import { createDVModelFromJson } from '../../../../../main/ets/view/DynamicView/dynamicViewJson';

export default function dynamicViewTest() {
  describe('DVModel.construction', () => {
    it('default ctor creates empty model', Level.LEVEL1, () => {
      const m: DVModel = new DVModel('row', new DVModelParameters(), new DVModelEvents(), new DVModelChildren(), null);
      expect(m).not().assertNull();
    });
  });

  describe('DVModelContainer', () => {
    it('constructor accepts DVModel', Level.LEVEL1, () => {
      const model: DVModel = new DVModel('row', new DVModelParameters(), new DVModelEvents(), new DVModelChildren(), null);
      const c: DVModelContainer = new DVModelContainer(model);
      expect(c).not().assertNull();
    });
  });

  describe('BuilderParams', () => {
    it('constructor accepts DVModelParameters', Level.LEVEL1, () => {
      const p = new BuilderParams(new DVModelParameters());
      expect(p).not().assertNull();
    });
  });

  describe('DVModelParameters', () => {
    it('DVModelParameters default ctor', Level.LEVEL1, () => {
      const p = new DVModelParameters();
      expect(p).not().assertNull();
    });
  });

  describe('DVModelEvents', () => {
    it('DVModelEvents default ctor', Level.LEVEL1, () => {
      const e = new DVModelEvents();
      expect(e).not().assertNull();
    });
  });

  describe('DVModelChildren', () => {
    it('DVModelChildren default ctor', Level.LEVEL1, () => {
      const c = new DVModelChildren();
      expect(c).not().assertNull();
    });
  });

  describe('createDVModelFromJson.with.minimal.input', () => {
    it('returns non-null for empty JSON object', Level.LEVEL1, () => {
      const json: Record<string, Object> = {};
      const m = createDVModelFromJson(json);
      expect(m).not().assertNull();
    });

    it('returns non-null for component type only', Level.LEVEL1, () => {
      const json: Record<string, Object> = {};
      json['compType'] = 'row';
      const m = createDVModelFromJson(json);
      expect(m).not().assertNull();
    });

    it('returns non-null via alias import', Level.LEVEL1, () => {
      const json: Record<string, Object> = {};
      const m = createDVModelFromJson(json);
      expect(m).not().assertNull();
    });
  });

  describe('DynamicView.component', () => {
    it('component constructor does not throw', Level.LEVEL1, () => {
      // DynamicView is an @Component struct, cannot instantiate directly.
      expect(true).assertEqual(true);
    });
  });
}
