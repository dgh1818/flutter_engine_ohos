// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:convert';

import 'package:file/memory.dart';
import 'package:flutter_tools/src/base/config.dart';
import 'package:flutter_tools/src/base/file_system.dart';
import 'package:flutter_tools/src/base/io.dart' show ProcessException;
import 'package:flutter_tools/src/base/logger.dart';
import 'package:flutter_tools/src/base/platform.dart';
import 'package:flutter_tools/src/cache.dart';
import 'package:flutter_tools/src/commands/emulators.dart';
import 'package:flutter_tools/src/doctor.dart';
import 'package:flutter_tools/src/doctor_validator.dart';
import 'package:flutter_tools/src/globals.dart' as globals;
import 'package:flutter_tools/src/ohos/ohos_emulators.dart';
import 'package:test/fake.dart';

import '../src/common.dart';
import '../src/context.dart';
import '../src/fake_process_manager.dart';
import '../src/test_flutter_command_runner.dart';

const String _kEmulatorName = 'Pura X View';

/// Sample '-list -details' output with string-typed fields, as produced by
/// the emulator tool shipped with DevEco Studio.
final String _kEmulatorListJson = jsonEncode(<Object>[
  <String, String>{
    'name': _kEmulatorName,
    'deviceType': 'phone',
    'os.osVersion': 'HarmonyOS 6.1.1(24)',
    'isRunning': 'false',
    'productModel': _kEmulatorName,
    'instancePath': 'D:/emulator/Pura X View',
    'imageRoot': 'D:/emulator',
    'hw.cpu.arch': 'x86_64',
    'hw.ramSize': '4096',
  },
]);

/// Sample '-imageList -deviceType phone' output with a downloaded image, as
/// produced by the emulator tool shipped with DevEco Studio.
final String _kImageListPhoneJson = jsonEncode(<Object>[
  <String, String>{
    'deviceType': 'phone',
    'downloaded': 'true',
    'osVersion': 'HarmonyOS 6.1.1(24)',
  },
]);

/// A doctor whose registered workflows cannot list any emulators, as on a
/// machine with only an Ohos SDK installed.
class _NoEmulatorSourcesDoctor extends Fake implements Doctor {
  @override
  List<Workflow> get workflows => <Workflow>[];
}

void main() {
  setUpAll(() {
    Cache.disableLocking();
  });

  late MemoryFileSystem fs;
  late String ohosHomePath;
  late String emptyHomePath;
  late BufferLogger logger;
  late FakeProcessManager processManager;

  setUp(() {
    fs = MemoryFileSystem.test();
    ohosHomePath = '/ohos-home';
    fs.directory('$ohosHomePath/emulator').createSync(recursive: true);
    // The command resolves Emulator.exe on Windows and Emulator elsewhere.
    fs.file('$ohosHomePath/emulator/Emulator.exe').createSync();
    fs.file('$ohosHomePath/emulator/Emulator').createSync();
    emptyHomePath = '/empty-home';
    fs.directory(emptyHomePath).createSync();
    logger = BufferLogger.test();
  });

  // Builds the executable path the command under test resolves for the given
  // home directory.
  String emulatorExecutable(String home) => globals.fs.path.join(
    home,
    'emulator',
    globals.platform.isWindows ? 'Emulator.exe' : 'Emulator',
  );

  group('list ohos emulators', () {
    testUsingContext(
      'prints the emulators returned by the emulator tool',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains('1 available Ohos emulator'));
        expect(logger.statusText, contains('Name'));
        expect(logger.statusText, contains('Device Type'));
        expect(logger.statusText, contains('OS Version'));
        expect(logger.statusText, contains(_kEmulatorName));
        expect(logger.statusText, contains('phone'));
        expect(logger.statusText, contains('HarmonyOS 6.1.1(24)'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'falls back to plain parsing when the output is not JSON',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: '$_kEmulatorName\n-skipped-option\n',
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains(_kEmulatorName));
        expect(logger.statusText, isNot(contains('skipped-option')));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'treats the [Empty] marker as no emulators',
      () async {
        // The tool prints '[Empty]' (not valid JSON) when no instance exists.
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: '[Empty]',
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains('No Ohos emulators available.'));
        expect(logger.statusText, isNot(contains('[Empty]')));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'tolerates JSON fields with unexpected types',
      () async {
        final String jsonWithBool = jsonEncode(<Object>[
          <String, Object>{'name': _kEmulatorName, 'isRunning': true},
        ]);
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: jsonWithBool,
          ),
        ]);

        // 'isRunning': true must not crash the parse nor leak the raw JSON
        // text into the list; the entry is kept with the default status.
        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains(_kEmulatorName));
        expect(logger.statusText, isNot(contains('isRunning')));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'prints no emulators when the output is not a JSON list',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: '{"emulators": []}',
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains('No Ohos emulators available.'));
        expect(logger.statusText, isNot(contains('{"')));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'exits when the executable is missing',
      () async {
        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--list-ohos-emulator']),
          throwsToolExit(exitCode: 1, message: 'Ohos emulator executable not found'),
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': emptyHomePath}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );

    testUsingContext(
      'exits when the emulator directory is missing',
      () async {
        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--list-ohos-emulator']),
          throwsToolExit(exitCode: 1, message: 'Ohos emulator directory not found'),
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': '/no-such-home'}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );

    testUsingContext(
      'exits with setup guidance when the variable is not set and no SDK is discovered',
      () async {
        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--list-ohos-emulator']),
          throwsToolExit(exitCode: 1, message: 'No Ohos emulator found'),
        );
      },
      overrides: <Type, Generator>{
        Config: () => Config.test(),
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () => FakePlatform(environment: <String, String>{}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );

    testUsingContext(
      'surfaces the tool error when listing fails',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            exitCode: 1,
            stderr: 'disk corrupted',
          ),
        ]);

        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--list-ohos-emulator']),
          throwsToolExit(exitCode: 1, message: 'disk corrupted'),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'accepts a plain JSON array of names',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: jsonEncode(<String>['Alpha', 'Beta']),
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains('Alpha'));
        expect(logger.statusText, contains('Beta'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'exits when the emulator tool cannot be run',
      () async {
        final String exe = emulatorExecutable(ohosHomePath);
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[exe, '-list', '-details'],
            exception: ProcessException(exe, <String>[]),
          ),
        ]);

        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--list-ohos-emulator']),
          throwsToolExit(exitCode: 1, message: 'Failed to run the Ohos emulator tool'),
        );

        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'resolves Emulator.exe on Windows',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[
              fs.path.join(ohosHomePath, 'emulator', 'Emulator.exe'),
              '-list',
              '-details',
            ],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () => FakePlatform(
          environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath},
          operatingSystem: 'windows',
        ),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'discovers the emulator next to the SDK when the variable is not set',
      () async {
        // A DevEco Studio style layout: '<home>/sdk' (a valid HarmonyOS SDK)
        // next to '<home>/tools/emulator/Emulator(.exe)'.
        fs.directory('/deveco-home/sdk/hmscore').createSync(recursive: true);
        fs.directory('/deveco-home/sdk/openharmony').createSync(recursive: true);
        fs.directory('/deveco-home/tools/emulator').createSync(recursive: true);
        fs.file(emulatorExecutable('/deveco-home/tools')).createSync();
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable('/deveco-home/tools'), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains(_kEmulatorName));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        Config: () => Config.test(),
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () => FakePlatform(
          environment: <String, String>{'DEVECO_SDK_HOME': '/deveco-home/sdk'},
        ),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'prefers the variable over the discovered SDK location',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains(_kEmulatorName));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        Config: () => Config.test(),
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () => FakePlatform(environment: <String, String>{
          'OHOS_EMULATOR_HOME': ohosHomePath,
          'DEVECO_SDK_HOME': '/deveco-home/sdk',
        }),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'discovers the emulator of a command-line-tools layout',
      () async {
        // command-line-tools layout: '<TOOL_HOME>/sdk' next to
        // '<TOOL_HOME>/emulator/Emulator(.exe)'; there is no 'tools' directory,
        // and the SDK components live under 'sdk/default/openharmony', which
        // HmosSdk.localHmosSdk() rejects as a build toolchain SDK.
        fs.directory('/clt-home/sdk/default/openharmony').createSync(recursive: true);
        fs.directory('/clt-home/emulator').createSync(recursive: true);
        fs.file(emulatorExecutable('/clt-home')).createSync();
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable('/clt-home'), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains(_kEmulatorName));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        Config: () => Config.test(),
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () => FakePlatform(
          environment: <String, String>{'DEVECO_SDK_HOME': '/clt-home/sdk'},
        ),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'exits with the derived candidates when neither layout matches',
      () async {
        // An SDK anchor with neither '<anchor>/../tools/emulator' nor
        // '<anchor>/../emulator' present; no SDK internals are required.
        fs.directory('/clt-home/sdk').createSync(recursive: true);

        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--list-ohos-emulator']),
          throwsToolExit(exitCode: 1, message: 'Ohos emulator executable not found'),
        );
      },
      overrides: <Type, Generator>{
        Config: () => Config.test(),
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () => FakePlatform(
          environment: <String, String>{'DEVECO_SDK_HOME': '/clt-home/sdk'},
        ),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );
  });

  group('print ohos emulators', () {
    testUsingContext(
      'prints the details of each emulator',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--print-ohos-emulator']);

        expect(logger.statusText, contains('Name: $_kEmulatorName'));
        expect(logger.statusText, contains('Device Type: phone'));
        expect(logger.statusText, contains('OS Version: HarmonyOS 6.1.1(24)'));
        expect(logger.statusText, contains('Status: false'));
        expect(logger.statusText, contains('RAM Size: 4096 MB'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

  });

  group('launch ohos emulator', () {
    testUsingContext(
      'errors and lists emulators when the name is unknown',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--launch-ohos-emulator', 'nope']),
          throwsToolExit(exitCode: 1, message: 'Ohos emulator "nope" not found.'),
        );

        expect(logger.statusText, contains('Available Ohos emulators:'));
        expect(logger.statusText, contains('- $_kEmulatorName'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'reports no emulators when the list is empty',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: '[]',
          ),
        ]);

        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--launch-ohos-emulator', 'nope']),
          throwsToolExit(exitCode: 1, message: 'Ohos emulator "nope" not found.'),
        );

        expect(logger.statusText, contains('No Ohos emulators available.'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'starts the emulator process for a known name',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
          FakeCommand(command: <String>[emulatorExecutable(ohosHomePath), '-hvd', _kEmulatorName]),
        ]);

        await OhosEmulators().launchEmulator(_kEmulatorName, startupDuration: Duration.zero);

        expect(logger.statusText, contains('Launching Ohos emulator "$_kEmulatorName"...'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'reports the exit code and stderr when the emulator exits non-zero',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-hvd', _kEmulatorName],
            exitCode: 1,
            stdout: 'boot log',
            stderr: 'boom',
          ),
        ]);

        await OhosEmulators().launchEmulator(
          _kEmulatorName,
          startupDuration: const Duration(milliseconds: 100),
        );

        expect(logger.errorText, contains('The Ohos emulator exited with code 1'));
        expect(logger.errorText, contains('Ohos emulator stderr:'));
        expect(logger.errorText, contains('boom'));
        expect(logger.errorText, contains('Address these issues and try again.'));
        expect(logger.traceText, contains('boot log'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'exits when starting the emulator fails',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-hvd', _kEmulatorName],
            exception: ProcessException(emulatorExecutable(ohosHomePath), <String>[
              '-hvd',
              _kEmulatorName,
            ]),
          ),
        ]);

        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--launch-ohos-emulator', _kEmulatorName]),
          throwsToolExit(exitCode: 1, message: 'Failed to launch Ohos emulator'),
        );
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'requires a non-empty emulator name',
      () async {
        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--launch-ohos-emulator=']),
          throwsToolExit(exitCode: 1, message: '--launch-ohos-emulator requires an emulator name.'),
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );

    testUsingContext(
      'warns when the emulator name may contain spaces',
      () async {
        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--launch-ohos-emulator', 'Pura', 'X', 'View']),
          throwsToolExit(exitCode: 1, message: 'value may contain spaces'),
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );
  });

  group('create ohos emulator', () {
    testUsingContext(
      'requires a non-empty emulator name',
      () async {
        await expectLater(
          createTestCommandRunner(
            EmulatorsCommand(),
          ).run(<String>['emulators', '--create-ohos-emulator=']),
          throwsToolExit(exitCode: 1, message: '--create-ohos-emulator requires an emulator name.'),
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () => FakePlatform(environment: <String, String>{}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );

    testUsingContext(
      'defaults the device type to phone',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-imageList', '-deviceType', 'phone'],
            stdout: _kImageListPhoneJson,
          ),
          FakeCommand(
            command: <String>[
              emulatorExecutable(ohosHomePath),
              '-create',
              'demo',
              '-deviceType',
              'phone',
              '-osVersion',
              'HarmonyOS 6.1.1(24)',
            ],
          ),
        ]);

        await createTestCommandRunner(EmulatorsCommand()).run(<String>[
          'emulators',
          '--create-ohos-emulator',
          'demo',
          '--osversion',
          'HarmonyOS 6.1.1(24)',
        ]);

        expect(logger.statusText, contains('Created Ohos emulator "demo".'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'detects the os version of the downloaded image',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[
              emulatorExecutable(ohosHomePath),
              '-imageList',
              '-deviceType',
              'phone',
            ],
            stdout: jsonEncode(<Object>[
              <String, String>{
                'deviceType': 'phone',
                'downloaded': 'false',
                'osVersion': 'HarmonyOS 6.0.0(20)',
              },
              <String, String>{
                'deviceType': 'phone',
                'downloaded': 'true',
                'osVersion': 'HarmonyOS 6.1.1(24)',
              },
            ]),
          ),
          FakeCommand(
            command: <String>[
              emulatorExecutable(ohosHomePath),
              '-create',
              'demo',
              '-deviceType',
              'phone',
              '-osVersion',
              'HarmonyOS 6.1.1(24)',
            ],
          ),
        ]);

        await createTestCommandRunner(EmulatorsCommand()).run(<String>[
          'emulators',
          '--create-ohos-emulator',
          'demo',
          '--devicetype',
          'phone',
        ]);

        expect(
          logger.statusText,
          contains('No --osversion specified; using "HarmonyOS 6.1.1(24)"'),
        );
        expect(logger.statusText, contains('Created Ohos emulator "demo".'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'errors when no downloaded image matches the device type',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-imageList', '-deviceType', 'tv'],
            stdout: jsonEncode(<Object>[
              <String, String>{
                'deviceType': 'tv',
                'downloaded': 'false',
                'osVersion': 'HarmonyOS 6.1.1(24)',
              },
            ]),
          ),
        ]);

        await expectLater(
          createTestCommandRunner(EmulatorsCommand()).run(<String>[
            'emulators',
            '--create-ohos-emulator',
            'demo',
            '--devicetype',
            'tv',
          ]),
          throwsToolExit(
            exitCode: 1,
            message:
                'No downloaded Ohos image found for device type "tv". '
                'Available for "tv": HarmonyOS 6.1.1(24).',
          ),
        );

        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'errors when the requested os version is not downloaded',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-imageList', '-deviceType', 'tv'],
            stdout: jsonEncode(<Object>[
              <String, String>{
                'deviceType': 'tv',
                'downloaded': 'false',
                'osVersion': 'HarmonyOS 5.1.1(19)',
              },
            ]),
          ),
        ]);

        await expectLater(
          createTestCommandRunner(EmulatorsCommand()).run(<String>[
            'emulators',
            '--create-ohos-emulator',
            'demo',
            '--devicetype',
            'tv',
            '--osversion',
            'HarmonyOS 5.1.1(19)',
          ]),
          throwsToolExit(
            exitCode: 1,
            message:
                'Ohos image "HarmonyOS 5.1.1(19)" is not downloaded for device type "tv".',
          ),
        );

        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'errors when the image list output cannot be parsed',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-imageList', '-deviceType', 'phone'],
            stdout: 'not json',
          ),
        ]);

        await expectLater(
          createTestCommandRunner(EmulatorsCommand()).run(<String>[
            'emulators',
            '--create-ohos-emulator',
            'demo',
          ]),
          throwsToolExit(
            exitCode: 1,
            message: 'Failed to parse the "-imageList" output for device type "phone".',
          ),
        );

        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'warns when an option value may contain spaces',
      () async {
        await expectLater(
          createTestCommandRunner(EmulatorsCommand()).run(<String>[
            'emulators',
            '--create-ohos-emulator',
            'demo',
            '--devicetype',
            'phone',
            '--osversion',
            'HarmonyOS',
            '6.1.1(24)',
          ]),
          throwsToolExit(exitCode: 1, message: 'may contain spaces'),
        );
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );

    testUsingContext(
      'exits non-zero when the emulator tool fails to create',
      () async {
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-imageList', '-deviceType', 'phone'],
            stdout: _kImageListPhoneJson,
          ),
          FakeCommand(
            command: <String>[
              emulatorExecutable(ohosHomePath),
              '-create',
              'demo',
              '-deviceType',
              'phone',
              '-osVersion',
              'HarmonyOS 6.1.1(24)',
            ],
            exitCode: 1,
            stderr: 'disk full',
          ),
        ]);

        await expectLater(
          createTestCommandRunner(EmulatorsCommand()).run(<String>[
            'emulators',
            '--create-ohos-emulator',
            'demo',
            '--devicetype',
            'phone',
            '--osversion',
            'HarmonyOS 6.1.1(24)',
          ]),
          throwsToolExit(exitCode: 1, message: 'Failed to create Ohos emulator "demo".'),
        );

        expect(logger.errorText, contains('disk full'));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );
  });

  group('emulator sources gate', () {
    testUsingContext(
      'ohos commands run without any emulator sources',
      () async {
        // A machine with only an Ohos SDK has no workflow that can list
        // emulators; the ohos commands must not be blocked by that check.
        processManager = FakeProcessManager.list(<FakeCommand>[
          FakeCommand(
            command: <String>[emulatorExecutable(ohosHomePath), '-list', '-details'],
            stdout: _kEmulatorListJson,
          ),
        ]);

        await createTestCommandRunner(
          EmulatorsCommand(),
        ).run(<String>['emulators', '--list-ohos-emulator']);

        expect(logger.statusText, contains(_kEmulatorName));
        expect(processManager, hasNoRemainingExpectations);
      },
      overrides: <Type, Generator>{
        Doctor: () => _NoEmulatorSourcesDoctor(),
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => processManager,
      },
    );

    testUsingContext(
      'listing android emulators still requires an emulator source',
      () async {
        await expectLater(
          createTestCommandRunner(EmulatorsCommand()).run(<String>['emulators']),
          throwsToolExit(message: 'Unable to find any emulator sources'),
        );
      },
      overrides: <Type, Generator>{
        Doctor: () => _NoEmulatorSourcesDoctor(),
        FileSystem: () => fs,
        Logger: () => logger,
        Platform: () =>
            FakePlatform(environment: <String, String>{'OHOS_EMULATOR_HOME': ohosHomePath}),
        ProcessManager: () => FakeProcessManager.empty(),
      },
    );
  });
}
