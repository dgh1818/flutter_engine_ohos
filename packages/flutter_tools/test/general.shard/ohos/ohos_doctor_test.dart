// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:file/memory.dart';
import 'package:flutter_tools/src/base/file_system.dart';
import 'package:flutter_tools/src/base/platform.dart';
import 'package:flutter_tools/src/base/user_messages.dart';
import 'package:flutter_tools/src/doctor_validator.dart';
import 'package:flutter_tools/src/ohos/ohos_doctor.dart';

import '../../src/common.dart';
import '../../src/context.dart';

void main() {
  OhosValidator buildValidator() {
    return OhosValidator(
      ohosSdk: null,
      platform: FakePlatform(),
      processManager: FakeProcessManager.any(),
      userMessages: UserMessages(),
    );
  }

  group('OhosValidator', () {
    // Regression test for issue #2992: calling validateImpl() directly used
    // to bounce through super.validate() back into the override and stack
    // overflow instead of returning a result.
    testUsingContext(
      'validateImpl returns a result without recursing',
      () async {
        final ValidationResult result = await buildValidator().validateImpl();

        // No SDK exists on the hermetic test file system, so the validator
        // must report the toolchain as missing.
        expect(result.type, ValidationType.missing);
        expect(result.messages, isNotEmpty);
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    // Regression test for issue #2992: OhosValidator must rely on the base
    // DoctorValidator.validate() timing wrapper so that `flutter doctor -v`
    // reports the section's execution time like every other validator.
    testUsingContext(
      'validate records the execution time',
      () async {
        final ValidationResult result = await buildValidator().validate();

        expect(result.executionTime, isNotNull);
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );
  });
}
