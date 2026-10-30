/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { describe, it, expect, Level } from '@ohos/hypium';
import HiLog from '@ohos.hilog';
import Log from '../../../../main/ets/util/Log';

export default function logTest() {
  // Ensure all log levels are allowed in this test session so the body of
  // each d/i/w/e/f is entered (instead of being short-circuited by isLoggable).
  Log.setLogLevel(HiLog.LogLevel.DEBUG);

  describe('Log.setLogLevel', () => {
    it('does not throw', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.setLogLevel(HiLog.LogLevel.DEBUG);
        Log.setLogLevel(HiLog.LogLevel.INFO);
        Log.setLogLevel(HiLog.LogLevel.WARN);
        Log.setLogLevel(HiLog.LogLevel.ERROR);
        Log.setLogLevel(HiLog.LogLevel.FATAL);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('Log.d', () => {
    it('Log.d does not throw with simple format', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.d('TestTag', 'debug message');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('Log.d does not throw with format and args', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.d('TestTag', 'value is %d', 42);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('Log.d does not throw with empty tag and format', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.d('', '');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('Log.i', () => {
    it('Log.i does not throw with simple format', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.i('TestTag', 'info message');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('Log.i does not throw with format and args', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.i('TestTag', 'value is %s', 'hello');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('Log.w', () => {
    it('Log.w does not throw with simple format', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.w('TestTag', 'warn message');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('Log.w does not throw with format and args', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.w('TestTag', 'count is %d', 0);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('Log.e', () => {
    it('does not throw with simple format', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.e('TestTag', 'error message');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with no args (empty forEach)', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.e('TestTag', 'error without args');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with non-Error args (forEach but no Error branch)', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.e('TestTag', 'values: %s %s', 'a', 123);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with Error instance (exercises Error branch in forEach)', Level.LEVEL1, () => {
      let threw = false;
      try {
        const err = new Error('boom');
        Log.e('TestTag', 'failed: %s', err);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('does not throw with mixed Error and non-Error args', Level.LEVEL1, () => {
      let threw = false;
      try {
        const err = new Error('boom');
        Log.e('TestTag', 'failed: %s code=%d', err, 500);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });

  describe('Log.f', () => {
    it('Log.f does not throw with simple format', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.f('TestTag', 'fatal message');
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('Log.f does not throw with format and args', Level.LEVEL1, () => {
      let threw = false;
      try {
        Log.f('TestTag', 'fatal value %d', 99);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });
  });
}
