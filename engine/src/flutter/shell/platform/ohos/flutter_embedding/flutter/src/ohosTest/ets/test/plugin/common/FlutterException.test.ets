/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
import { describe, expect, it } from '@ohos/hypium';
import FlutterException from '../../../../../main/ets/plugin/common/FlutterException';

interface keyValue {
  key: string;
}

interface anotherKey {
  anotherKey: string;
}

interface defaultKey {
  defaultKey: string;
}

export default function FlutterExceptionTest() {
  describe('FlutterExceptionTest', () => {
    it('should initialize with correct values', 0, () => {
      const code = 'ERROR_CODE_123';
      const message = 'This is an error message';
      const details:keyValue = { key: 'value' };
      const exception = new FlutterException(code, message, details);
      expect(exception.message).assertEqual(message);
      expect(exception.code).assertEqual(code);
      expect(exception.name).assertEqual('');
    })
    it('should have a stack property if provided', 0, () => {
      const code = 'ERROR_CODE_456';
      const message = 'Another error message';
      const details: anotherKey = { anotherKey: 'anotherValue' };
      const stack = 'Error stack trace';
      const exception = new FlutterException(code, message, details);
      exception.stack = stack;
      expect(exception.stack).assertEqual(stack);
    })
    it('should default name to an empty string if not provided', 0, () => {
      const code = 'ERROR_CODE_789';
      const message = 'Default name test';
      const details: defaultKey = { defaultKey: 'defaultValue' };
      const exception = new FlutterException(code, message, details);
      expect(exception.name).assertEqual('');
    })
  })
}
