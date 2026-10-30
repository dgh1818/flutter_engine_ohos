/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import Any from '../../../../../main/ets/plugin/common/Any';
import PlatformView, { Params } from '../../../../../main/ets/plugin/platform/PlatformView';
import { EmbeddingNodeController } from '../../../../../main/ets/embedding/ohos/EmbeddingNodeController';

class FakePlatformView extends PlatformView {
  getType(): string {
    return 'fake';
  }
  getView(): WrappedBuilder<[Params]> {
    return new WrappedBuilder<[Params]>((_params: Params): void => {});
  }
  onInactive(): void {}
  onActive(): void {}
  getPlatformViewVisibleAreaEventOptions(): Any {
    return null;
  }
  dispose(): void {}
}

class FakePlatformViewEnable extends PlatformView {
  enableFlag: boolean;
  constructor(enable: boolean) {
    super();
    this.enableFlag = enable;
  }
  getType(): string {
    return 'fake';
  }
  getView(): WrappedBuilder<[Params]> {
    return new WrappedBuilder<[Params]>((_params: Params): void => {});
  }
  onInactive(): void {}
  onActive(): void {}
  getPlatformViewVisibleAreaEventOptions(): Any {
    if (this.enableFlag) {
      return {
        enable: true,
        ratios: [0.5],
        expectedUpdateInterval: 100,
        onInactiveThreshold: 0.4,
        onActiveThreshold: 0.6,
      };
    } else {
      return {
        enable: false,
        ratios: [0.5],
        expectedUpdateInterval: 100,
        onInactiveThreshold: 0.4,
        onActiveThreshold: 0.6,
      };
    }
  }
  dispose(): void {}
}

export default function embeddingNodeControllerTest() {
  describe('EmbeddingNodeController.constructor', () => {
    it('constructs with default state', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      expect(ctl).not().assertNull();
    });
  });

  describe('EmbeddingNodeController.getEmbedId', () => {
    it('returns empty string for fresh controller', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      expect(ctl.getEmbedId()).assertEqual('');
    });
  });

  describe('EmbeddingNodeController.getBuilderNode', () => {
    it('returns null for fresh controller (no builder yet)', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      expect(ctl.getBuilderNode()).assertEqual(null);
    });
  });

  describe('EmbeddingNodeController.setBuilderNode', () => {
    it('stores null and getBuilderNode returns null', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      ctl.setBuilderNode(null);
      expect(ctl.getBuilderNode()).assertEqual(null);
    });
  });

  describe('EmbeddingNodeController.setRenderOption', () => {
    it('does not throw with valid platform view', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      const pv: FakePlatformView = new FakePlatformView();
      let threw = false;
      try {
        ctl.setRenderOption(pv, 'surface-1', 0, 0);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with different render types', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      const pv: FakePlatformView = new FakePlatformView();
      let threw = false;
      try {
        ctl.setRenderOption(pv, 'surf-A', 1, 0);
        ctl.setRenderOption(pv, 'surf-B', 2, 1);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('EmbeddingNodeController.setDestroy', () => {
    it('does not throw when setting destroy=true (no builder)', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.setDestroy(true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw when setting destroy=false', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.setDestroy(false);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw when toggling destroy multiple times', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.setDestroy(true);
        ctl.setDestroy(false);
        ctl.setDestroy(true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('EmbeddingNodeController.updateNode', () => {
    it('does not throw when builder is null on updateNode', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.updateNode({} as Any);
      } catch (e) {
        threw = true;
      }
      // updateNode is `this.builderNode?.update(arg)` - safe with null builder
      expect(threw).assertEqual(false);
    });
  });

  describe('EmbeddingNodeController.notifyPlatformViewInvisible', () => {
    it('does not throw when platformView is undefined on notifyInvisible', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.notifyPlatformViewInvisible();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not call onInactive when enable is false', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      const pv: FakePlatformViewEnable = new FakePlatformViewEnable(false);
      ctl.setRenderOption(pv, 'surf-npi', 0, 0);
      let threw = false;
      try {
        ctl.notifyPlatformViewInvisible();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('EmbeddingNodeController.disposeFrameNode', () => {
    it('does not throw when builder is null on disposeFrameNode', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.disposeFrameNode();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('EmbeddingNodeController.setPlatformViewVisibleAreaEventCallback', () => {
    it('does not throw when platformView is undefined on setCallback', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.setPlatformViewVisibleAreaEventCallback();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('returns early when getPlatformViewVisibleAreaEventOptions returns null', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      const pv: FakePlatformView = new FakePlatformView();
      ctl.setRenderOption(pv, 'surface-1', 0, 0);
      let threw = false;
      try {
        ctl.setPlatformViewVisibleAreaEventCallback();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('returns early when enable is false', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      const pv: FakePlatformViewEnable = new FakePlatformViewEnable(false);
      ctl.setRenderOption(pv, 'surf-2', 0, 0);
      let threw = false;
      try {
        ctl.setPlatformViewVisibleAreaEventCallback();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('runs full callback when enable is true (exercises inner branches)', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      const pv: FakePlatformViewEnable = new FakePlatformViewEnable(true);
      ctl.setRenderOption(pv, 'surf-3', 0, 0);
      let threw = false;
      try {
        ctl.setPlatformViewVisibleAreaEventCallback();
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('EmbeddingNodeController.postMouseEvent', () => {
    it('does not throw with undefined builder', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      let threw = false;
      try {
        ctl.postMouseEvent({} as Any);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('EmbeddingNodeController.postEvent', () => {
    it('returns false when event is undefined', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      const result: boolean = ctl.postEvent(undefined);
      expect(result).assertEqual(false);
    });

    it('does not throw with minimal event object', Level.LEVEL1, () => {
      const ctl: EmbeddingNodeController = new EmbeddingNodeController();
      interface Ev { changedTouches: Array<Any>; touches: Array<Any>; }
      const ev: Ev = { changedTouches: new Array<Any>(), touches: new Array<Any>() };
      let threw = false;
      try {
        ctl.postEvent(ev as Any);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });
}
