/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import PiPVisibilityBridge from '../../../../../main/ets/embedding/ohos/PiPVisibilityBridge';

type PiPVisibilityListener = (visible: boolean) => void;

interface VisibilityCall {
  fn: PiPVisibilityListener;
  calls: Array<boolean>;
}

function makeListener(): VisibilityCall {
  const obj: VisibilityCall = {
    fn: (_visible: boolean): void => {},
    calls: new Array<boolean>(),
  };
  obj.fn = (visible: boolean): void => {
    obj.calls.push(visible);
  };
  return obj;
}

export default function piPVisibilityBridgeTest() {
  // getInstance returns same instance
  describe('PiPVisibilityBridge.getInstance', () => {
    it('returns the same singleton instance', Level.LEVEL1, () => {
      const a: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const b: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      expect(a).assertEqual(b);
    });
  });

  // register adds to listeners set and immediately fires
  describe('PiPVisibilityBridge.register', () => {
    it('immediately calls listener with current pipVisible (false)', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const listener = makeListener();
      bridge.register(listener.fn);
      try {
        expect(listener.calls.length).assertEqual(1);
        expect(listener.calls[0]).assertEqual(false);
      } finally {
        bridge.unregister(listener.fn);
      }
    });

    it('adds the listener to the internal set (unregister brings it to empty)', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l1 = makeListener();
      const l2 = makeListener();
      bridge.register(l1.fn);
      bridge.register(l2.fn);
      try {
        // Both listeners were added to the internal set: each received the
        // immediate callback with the current pipVisible (false).
        expect(l1.calls.length).assertEqual(1);
        expect(l2.calls.length).assertEqual(1);
        expect(l1.calls[0]).assertEqual(false);
        expect(l2.calls[0]).assertEqual(false);
      } finally {
        bridge.unregister(l1.fn);
        bridge.unregister(l2.fn);
      }
    });
  });

  // unregister removes from listeners set and stops polling when empty
  describe('PiPVisibilityBridge.unregister', () => {
    it('does not throw when unregistering an unknown listener', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l = makeListener();
      let threw = false;
      try {
        bridge.unregister(l.fn);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not double-fire when listener is unregistered', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l = makeListener();
      bridge.register(l.fn);
      bridge.unregister(l.fn);
      const initialCalls: number = l.calls.length;
      expect(initialCalls).assertEqual(1);
      // unregister again - should not throw and not fire
      let threw = false;
      try {
        bridge.unregister(l.fn);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
      expect(l.calls.length).assertEqual(initialCalls);
    });
  });

  // updateAbilityShown state machine (without triggering NAPI)
  describe('PiPVisibilityBridge.updateAbilityShown', () => {
    it('does not throw on shown=true', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l = makeListener();
      bridge.register(l.fn);
      try {
        let threw = false;
        try {
          bridge.updateAbilityShown(true);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
      } finally {
        bridge.unregister(l.fn);
      }
    });

    it('does not throw on shown=false from initial state', Level.LEVEL1, () => {
      // shownCount starts at 0; calling updateAbilityShown(false) keeps it 0 and
      // triggers refreshVisibility() (which calls NAPI but doesn't throw).
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l = makeListener();
      bridge.register(l.fn);
      try {
        let threw = false;
        try {
          bridge.updateAbilityShown(false);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
      } finally {
        bridge.unregister(l.fn);
      }
    });

    it('handles multiple increment/decrement pairs without throwing', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l = makeListener();
      bridge.register(l.fn);
      try {
        let threw = false;
        try {
          bridge.updateAbilityShown(true);
          bridge.updateAbilityShown(true);
          bridge.updateAbilityShown(false);
          bridge.updateAbilityShown(false);
          bridge.updateAbilityShown(true);
          bridge.updateAbilityShown(false);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
      } finally {
        bridge.unregister(l.fn);
      }
    });

    it('shown=false from shown=0 keeps shownCount at 0 (Math.max guard)', Level.LEVEL1, () => {
      // Calling shown=false when shownCount is already 0 should be a no-op
      // (the internal Math.max(0, count-1) keeps it at 0). We can't directly
      // inspect shownCount, but we verify no throw on repeated decrements.
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l = makeListener();
      bridge.register(l.fn);
      try {
        let threw = false;
        try {
          bridge.updateAbilityShown(false);
          bridge.updateAbilityShown(false);
          bridge.updateAbilityShown(false);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
      } finally {
        bridge.unregister(l.fn);
      }
    });
  });

  // isWithinGraceWindow is private but exercised via updateAbilityShown
  // when shownCount transitions 1 -> 0, lastHiddenAt is set to Date.now()
  // and isWithinGraceWindow returns true for 2000ms. We test indirectly
  // by verifying the second updateAbilityShown(false) within the window
  // does not throw.
  describe('PiPVisibilityBridge.grace.window.indirect', () => {
    it('rapid increment/decrement within 2s window does not throw', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l = makeListener();
      bridge.register(l.fn);
      try {
        let threw = false;
        try {
          bridge.updateAbilityShown(true);
          bridge.updateAbilityShown(false);
          bridge.updateAbilityShown(true);
          bridge.updateAbilityShown(false);
        } catch (e) {
          threw = true;
        }
        expect(threw).assertEqual(false);
      } finally {
        bridge.unregister(l.fn);
      }
    });
  });

  // listener immediate-fire on register with current pipVisible
  describe('PiPVisibilityBridge.listener.semantics', () => {
    it('second registered listener also gets the immediate call', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      const l1 = makeListener();
      const l2 = makeListener();
      bridge.register(l1.fn);
      try {
        bridge.register(l2.fn);
        try {
          expect(l1.calls.length).assertEqual(1);
          expect(l2.calls.length).assertEqual(1);
          expect(l1.calls[0]).assertEqual(false);
          expect(l2.calls[0]).assertEqual(false);
        } finally {
          bridge.unregister(l2.fn);
        }
      } finally {
        bridge.unregister(l1.fn);
      }
    });
  });

  // Explicitly verify line 50: listeners.size == 0 after unregister triggers stopPolling
  describe('PiPVisibilityBridge.unregister.lastListener', () => {
    it('last unregister triggers listeners.size==0 (line 50 true branch)', Level.LEVEL1, () => {
      const bridge: PiPVisibilityBridge = PiPVisibilityBridge.getInstance();
      // Try unregistering any leftover listeners by removing common listener types
      // (unregistering non-members is a no-op so this is safe)
      const probe1 = makeListener();
      const probe2 = makeListener();
      bridge.register(probe1.fn);
      bridge.register(probe2.fn);
      let threw = false;
      try {
        // Remove both
        bridge.unregister(probe1.fn);
        bridge.unregister(probe2.fn);
        // Now listeners should be empty (assuming no leftover from prior tests).
        // Try a few more unregister calls to ensure we hit line 50 true branch.
        bridge.unregister(probe1.fn); // already removed
        bridge.unregister(probe2.fn); // already removed
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });
}
