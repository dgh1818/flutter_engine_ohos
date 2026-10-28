// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:file/memory.dart';
import 'package:flutter_tools/src/base/file_system.dart';
import 'package:flutter_tools/src/base/platform.dart';
import 'package:flutter_tools/src/base/user_messages.dart';
import 'package:flutter_tools/src/doctor_validator.dart';
import 'package:flutter_tools/src/ohos/ohos_doctor.dart';
import 'package:flutter_tools/src/ohos/ohos_sdk.dart';
import 'package:test/fake.dart';

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

    // Issue #2987: the toolchain check must cover the hdc device connector,
    // mirroring how the Android toolchain relies on the SDK's adb binary.
    testUsingContext(
      'reports the hdc connector when the SDK provides it',
      () async {
        final validator = OhosValidator(
          ohosSdk: _FakeHarmonySdkWithHdc(),
          platform: FakePlatform(),
          processManager: FakeProcessManager.any(),
          userMessages: UserMessages(),
        );

        final ValidationResult result = await validator.validateImpl();

        expect(
          result.messages.any(
            (ValidationMessage message) =>
                message.message.contains('hdc - HarmonyOS device connector at hdc'),
          ),
          isTrue,
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'reports missing hdc when the SDK does not provide it',
      () async {
        final validator = OhosValidator(
          ohosSdk: _FakeHarmonySdkWithoutHdc(),
          platform: FakePlatform(),
          processManager: FakeProcessManager.any(),
          userMessages: UserMessages(),
        );

        final ValidationResult result = await validator.validateImpl();

        expect(
          result.messages.any(
            (ValidationMessage message) =>
                message.message.contains('hdc not found or not executable'),
          ),
          isTrue,
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );
  });
}

class _FakeHarmonySdkWithHdc extends Fake implements HarmonySdk {
  @override
  String get sdkPath => '/fake/sdk';

  @override
  String? get hdcPath => 'hdc';
}

class _FakeHarmonySdkWithoutHdc extends Fake implements HarmonySdk {
  @override
  String? get hdcPath => null;

  @override
  String get sdkPath => '/fake/sdk';
}
