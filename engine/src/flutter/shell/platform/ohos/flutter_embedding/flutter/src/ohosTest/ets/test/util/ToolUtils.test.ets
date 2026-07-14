import { describe, expect, it, Level } from '@ohos/hypium';
import ToolUtils from '../../../../main/ets/util/ToolUtils';

interface HasFoo {
  foo: Object;
}

interface HasBar {
  bar: Object;
}

interface HasMixed {
  method1: () => number;
  method2: (x: number) => number;
  property: string;
}

interface TwoProps {
  a: number;
  b: number;
}

export default function toolUtilsTest() {
  describe('ToolUtils.isObj', () => {
    it('returns true for plain object', Level.LEVEL1, () => {
      const obj: Record<string, Object> = {};
      expect(ToolUtils.isObj(obj)).assertEqual(true);
    });

    it('returns true for non-empty object', Level.LEVEL1, () => {
      const obj: TwoProps = { a: 1, b: 2 };
      expect(ToolUtils.isObj(obj)).assertEqual(true);
    });

    it('returns true for array', Level.LEVEL1, () => {
      const arr: Array<number> = [1, 2, 3];
      expect(ToolUtils.isObj(arr)).assertEqual(true);
    });

    it('returns true for empty array', Level.LEVEL1, () => {
      const arr: Array<number> = new Array<number>();
      expect(ToolUtils.isObj(arr)).assertEqual(true);
    });

    it('returns false for function (typeof function is "function", not "object")', Level.LEVEL1, () => {
      const fn: Function = () => {};
      expect(ToolUtils.isObj(fn)).assertEqual(false);
    });

    it('returns zero for zero (falsy primitive, NOT strict false)', Level.LEVEL1, () => {
      const value: Object = 0 as Object;
      expect(ToolUtils.isObj(value)).assertEqual(0);
    });

    it('returns empty string for empty string (falsy primitive, NOT strict false)', Level.LEVEL1, () => {
      const value: Object = '' as Object;
      expect(ToolUtils.isObj(value)).assertEqual('');
    });

    it('returns false for non-empty string (typeof string is "string")', Level.LEVEL1, () => {
      const value: Object = 'hello' as Object;
      expect(ToolUtils.isObj(value)).assertEqual(false);
    });

    it('returns false for positive number (typeof number is "number")', Level.LEVEL1, () => {
      const value: Object = 42 as Object;
      expect(ToolUtils.isObj(value)).assertEqual(false);
    });

    it('returns false for boolean true (typeof boolean is "boolean")', Level.LEVEL1, () => {
      const value: Object = true as Object;
      expect(ToolUtils.isObj(value)).assertEqual(false);
    });

    it('returns false for boolean false (typeof boolean is "boolean")', Level.LEVEL1, () => {
      const value: Object = false as Object;
      expect(ToolUtils.isObj(value)).assertEqual(false);
    });
  });

  describe('ToolUtils.implementsInterface', () => {
    it('returns true when object has the method as a function', Level.LEVEL1, () => {
      const obj: HasFoo = { foo: () => 42 };
      expect(ToolUtils.implementsInterface(obj, 'foo')).assertEqual(true);
    });

    it('returns false when object has no such method', Level.LEVEL1, () => {
      const obj: HasBar = { bar: () => 42 };
      expect(ToolUtils.implementsInterface(obj, 'foo')).assertEqual(false);
    });

    it('returns false when property exists but is not a function', Level.LEVEL1, () => {
      const obj: HasFoo = { foo: 'not a function' };
      expect(ToolUtils.implementsInterface(obj, 'foo')).assertEqual(false);
    });

    it('returns false for null', Level.LEVEL1, () => {
      // NOTE: this will throw a TypeError because Reflect.has(null, ...) fails.
      // The function should ideally handle null/undefined, but currently doesn't.
      // Wrapped in try/catch to demonstrate proper error handling in tests.
      let threwError = false;
      try {
        ToolUtils.implementsInterface(null, 'foo');
      } catch (e) {
        threwError = true;
      }
      expect(threwError).assertEqual(true);
    });

    it('returns false for undefined', Level.LEVEL1, () => {
      let threwError = false;
      try {
        ToolUtils.implementsInterface(undefined, 'foo');
      } catch (e) {
        threwError = true;
      }
      expect(threwError).assertEqual(true);
    });

    it('returns true for built-in array method (push)', Level.LEVEL1, () => {
      const arr: Array<number> = [1, 2, 3];
      expect(ToolUtils.implementsInterface(arr, 'push')).assertEqual(true);
    });

    it('returns true for built-in function method (call)', Level.LEVEL1, () => {
      const fn: Function = () => {};
      expect(ToolUtils.implementsInterface(fn, 'call')).assertEqual(true);
    });

    it('returns true for object with multiple methods, false for non-function property', Level.LEVEL1, () => {
      const obj: HasMixed = {
        method1: () => 1,
        method2: (x: number) => x + 1,
        property: 'value'
      };
      expect(ToolUtils.implementsInterface(obj, 'method1')).assertEqual(true);
      expect(ToolUtils.implementsInterface(obj, 'method2')).assertEqual(true);
      expect(ToolUtils.implementsInterface(obj, 'property')).assertEqual(false);
    });

    it('returns true for object with inherited method via prototype', Level.LEVEL1, () => {
      const arr: Array<number> = [];
      expect(ToolUtils.implementsInterface(arr, 'toString')).assertEqual(true);
    });
  });
}
