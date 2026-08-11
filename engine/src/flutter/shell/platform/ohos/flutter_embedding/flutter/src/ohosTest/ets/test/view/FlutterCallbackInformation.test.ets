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
import { FlutterCallbackInformation } from '../../../../main/ets/view/FlutterCallbackInformation';

export default function FlutterCallbackInformationTest() {
  describe('FlutterCallbackInformationTest', () => {
    it('constructor_with_all_params', 0, () => {
      const callbackInfo = new FlutterCallbackInformation(
        'myCallback',
        'MyClass',
        'package:my_app/main.dart'
      );
      expect(callbackInfo.callbackName).assertEqual('myCallback');
      expect(callbackInfo.callbackClassName).assertEqual('MyClass');
      expect(callbackInfo.callbackLibraryPath).assertEqual('package:my_app/main.dart');
    })

    it('constructor_with_no_params', 0, () => {
      const callbackInfo = new FlutterCallbackInformation();
      expect(callbackInfo.callbackName).assertUndefined();
      expect(callbackInfo.callbackClassName).assertUndefined();
      expect(callbackInfo.callbackLibraryPath).assertUndefined();
    })

    it('constructor_with_callback_name_only', 0, () => {
      const callbackInfo = new FlutterCallbackInformation('onlyCallback');
      expect(callbackInfo.callbackName).assertEqual('onlyCallback');
      expect(callbackInfo.callbackClassName).assertUndefined();
      expect(callbackInfo.callbackLibraryPath).assertUndefined();
    })

    it('constructor_with_callback_name_and_class', 0, () => {
      const callbackInfo = new FlutterCallbackInformation('myCallback', 'MyClass');
      expect(callbackInfo.callbackName).assertEqual('myCallback');
      expect(callbackInfo.callbackClassName).assertEqual('MyClass');
      expect(callbackInfo.callbackLibraryPath).assertUndefined();
    })

    it('init_method', 0, () => {
      const callbackInfo = new FlutterCallbackInformation();
      callbackInfo.init('initCallback', 'InitClass', 'package:init/path.dart');
      expect(callbackInfo.callbackName).assertEqual('initCallback');
      expect(callbackInfo.callbackClassName).assertEqual('InitClass');
      expect(callbackInfo.callbackLibraryPath).assertEqual('package:init/path.dart');
    })

    it('init_method_overwrites_existing', 0, () => {
      const callbackInfo = new FlutterCallbackInformation('oldCallback', 'OldClass', 'package:old/path.dart');
      callbackInfo.init('newCallback', 'NewClass', 'package:new/path.dart');
      expect(callbackInfo.callbackName).assertEqual('newCallback');
      expect(callbackInfo.callbackClassName).assertEqual('NewClass');
      expect(callbackInfo.callbackLibraryPath).assertEqual('package:new/path.dart');
    })

    it('callback_name_can_be_empty_string', 0, () => {
      const callbackInfo = new FlutterCallbackInformation('', 'MyClass', 'package:app/main.dart');
      expect(callbackInfo.callbackName).assertEqual('');
    })

    it('callback_class_name_can_be_empty_string', 0, () => {
      const callbackInfo = new FlutterCallbackInformation('myCallback', '', 'package:app/main.dart');
      expect(callbackInfo.callbackClassName).assertEqual('');
    })

    it('callback_library_path_can_be_empty_string', 0, () => {
      const callbackInfo = new FlutterCallbackInformation('myCallback', 'MyClass', '');
      expect(callbackInfo.callbackLibraryPath).assertEqual('');
    })

    it('lookupCallbackInformation_returns_null_for_invalid_handle', 0, () => {
      const result = FlutterCallbackInformation.lookupCallbackInformation(0);
      expect(result).assertNull();
    })

    it('lookupCallbackInformation_returns_null_for_negative_handle', 0, () => {
      const result = FlutterCallbackInformation.lookupCallbackInformation(-1);
      expect(result).assertNull();
    })

    it('lookupCallbackInformationBigInt_returns_null_for_invalid_handle', 0, () => {
      const result = FlutterCallbackInformation.lookupCallbackInformationBigInt(BigInt(0));
      expect(result).assertNull();
    })

    it('lookupCallbackInformationBigInt_returns_null_for_negative_handle', 0, () => {
      const result = FlutterCallbackInformation.lookupCallbackInformationBigInt(BigInt(-1));
      expect(result).assertNull();
    })

    it('multiple_instances_are_independent', 0, () => {
      const callbackInfo1 = new FlutterCallbackInformation('callback1', 'Class1', 'package:app1/main.dart');
      const callbackInfo2 = new FlutterCallbackInformation('callback2', 'Class2', 'package:app2/main.dart');
      expect(callbackInfo1.callbackName).assertEqual('callback1');
      expect(callbackInfo2.callbackName).assertEqual('callback2');
    })
  })
}