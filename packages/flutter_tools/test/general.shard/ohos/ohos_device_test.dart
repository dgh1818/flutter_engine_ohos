// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:file/memory.dart';
import 'package:flutter_tools/src/base/file_system.dart';
import 'package:flutter_tools/src/base/logger.dart';
import 'package:flutter_tools/src/build_info.dart';
import 'package:flutter_tools/src/device.dart';
import 'package:flutter_tools/src/globals.dart' as globals;
import 'package:flutter_tools/src/ohos/application_package.dart';
import 'package:flutter_tools/src/ohos/ohos_device.dart';
import 'package:flutter_tools/src/ohos/ohos_sdk.dart';
import 'package:test/fake.dart';

import '../../src/common.dart';
import '../../src/context.dart';
import '../../src/fake_process_manager.dart';

void main() {
  testUsingContext(
    'startApp using route',
    () async {
      final processManager = FakeProcessManager.list(
        _launchSequence(<String>[
          '--pb',
          'enable-dart-profiling',
          'true',
          '--ps',
          'route',
          '/animation',
        ]),
      );

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final OhosHap hap = _createTestHap();

      final LaunchResult launchResult = await device.startApp(
        hap,
        prebuiltApplication: true,
        debuggingOptions: DebuggingOptions.disabled(BuildInfo.release),
        platformArgs: <String, dynamic>{},
        route: '/animation',
      );

      expect(launchResult.started, true);
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'startApp forwards all supported engine switches',
    () async {
      final processManager = FakeProcessManager.list(
        _launchSequence(<String>[
          '--pb', 'enable-dart-profiling', 'true',
          '--pb', 'profile-startup', 'true',
          '--pb', 'trace-startup', 'true',
          '--ps', 'route', '/smuggle-it',
          '--pb', 'enable-software-rendering', 'true',
          '--pb', 'skia-deterministic-rendering', 'true',
          '--pb', 'trace-skia', 'true',
          '--ps', 'trace-skia-allowlist', 'skia.a,skia.b',
          '--pb', 'trace-systrace', 'true',
          '--pb', 'endless-trace-buffer', 'true',
          '--pb', 'purge-persistent-cache', 'true',
          '--pb', 'enable-impeller', 'true',
          '--pb', 'start-paused', 'true',
          '--pb', 'disable-service-auth-codes', 'true',
          // dart-flags values start with `-`, which `aa start` rejects as
          // an unknown option; startApp prefixes a space (trimmed by the
          // embedding's FlutterShellArgs.fromWant).
          '--ps', 'dart-flags', ' --enable-asserts',
          '--pb', 'use-test-fonts', 'true',
          '--pb', 'verbose-logging', 'true',
        ], debugLaunch: true),
      );

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final OhosHap hap = _createTestHap();

      final LaunchResult launchResult = await device.startApp(
        hap,
        prebuiltApplication: true,
        debuggingOptions: DebuggingOptions.enabled(
          BuildInfo.debug,
          startPaused: true,
          disableServiceAuthCodes: true,
          dartFlags: '--enable-asserts',
          enableSoftwareRendering: true,
          skiaDeterministicRendering: true,
          traceSkia: true,
          traceSkiaAllowlist: 'skia.a,skia.b',
          traceSystrace: true,
          endlessTraceBuffer: true,
          purgePersistentCache: true,
          useTestFonts: true,
          verboseSystemLogs: true,
          enableImpeller: ImpellerStatus.enabled,
          profileStartup: true,
        ),
        platformArgs: <String, dynamic>{'trace-startup': true},
        route: '/smuggle-it',
      );

      // The launch fails because the fake hilog reader emits no VM service
      // URI, but the aa start command above has already been asserted by
      // matching the exact expected command sequence.
      expect(launchResult.started, false);
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'startApp forwards enable-impeller false when impeller is disabled',
    () async {
      final processManager = FakeProcessManager.list(
        _launchSequence(<String>[
          '--pb',
          'enable-dart-profiling',
          'true',
          '--pb',
          'enable-impeller',
          'false',
        ]),
      );

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final OhosHap hap = _createTestHap();

      final LaunchResult launchResult = await device.startApp(
        hap,
        prebuiltApplication: true,
        debuggingOptions: DebuggingOptions.disabled(
          BuildInfo.release,
          enableImpeller: ImpellerStatus.disabled,
        ),
        platformArgs: <String, dynamic>{},
      );

      expect(launchResult.started, true);
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'startApp without engine switches only forwards the defaults',
    () async {
      final processManager = FakeProcessManager.list(
        _launchSequence(<String>['--pb', 'enable-dart-profiling', 'true']),
      );

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final OhosHap hap = _createTestHap();

      final LaunchResult launchResult = await device.startApp(
        hap,
        prebuiltApplication: true,
        debuggingOptions: DebuggingOptions.disabled(BuildInfo.release),
        platformArgs: <String, dynamic>{},
      );

      expect(launchResult.started, true);
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'sdkNameAndVersion reports the system fullname verbatim',
    () async {
      final processManager = FakeProcessManager.list(<FakeCommand>[
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'param', 'get'],
          stdout:
              'const.product.name=HUAWEI Mate 70 Pro \r\n'
              'const.product.model=PLR-AL00\r\n'
              'const.ohos.fullname=OpenHarmony-6.0.2.130\r\n'
              'const.ohos.apiversion=22\r\n'
              'const.product.cpu.abilist=arm64-v8a\r\n',
        ),
      ]);

      final device = OhosDevice(
        '123',
        deviceCodeName: '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      expect(await device.sdkNameAndVersion, 'OpenHarmony-6.0.2.130 (API 22)');
      // Loading the properties cached the marketing name for `name`.
      expect(device.name, 'HUAWEI Mate 70 Pro');
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'name falls back to the model code without a product name',
    () async {
      final processManager = FakeProcessManager.list(<FakeCommand>[
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'param', 'get'],
          stdout: 'const.product.model=PLR-AL00\r\nconst.product.cpu.abilist=arm64-v8a\r\n',
        ),
      ]);

      final device = OhosDevice(
        '123',
        deviceCodeName: '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      expect(await device.sdkNameAndVersion, 'null (API null)');
      expect(device.name, 'PLR-AL00');
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'name falls back to the serial when the properties fail',
    () async {
      final processManager = FakeProcessManager.list(<FakeCommand>[
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'param', 'get'],
          exitCode: 255,
        ),
      ]);

      final device = OhosDevice(
        '123',
        deviceCodeName: '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      // The failed property read must not throw; the display falls back to
      // the serial number.
      expect(await device.sdkNameAndVersion, 'null (API null)');
      expect(device.name, '123');
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'toJson reports the marketing name and stripped sdk version',
    () async {
      final processManager = FakeProcessManager.list(<FakeCommand>[
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'param', 'get'],
          stdout:
              'const.product.name=HUAWEI Mate 70 Pro\r\n'
              'const.product.model=PLR-AL00\r\n'
              'const.ohos.fullname=OpenHarmony-6.0.2.130\r\n'
              'const.ohos.apiversion=22\r\n'
              'const.product.cpu.abilist=arm64-v8a\r\n',
        ),
      ]);

      final device = OhosDevice(
        '123',
        deviceCodeName: '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final Map<String, Object> json = await device.toJson();
      expect(json['name'], 'HUAWEI Mate 70 Pro');
      expect(json['sdk'], 'OpenHarmony-6.0.2.130 (API 22)');
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'HdcLogReader rewrites Dart print lines to the flutter: prefix',
    () async {
      const rawLine =
          '09-14 20:15:33.123  4321  8765 W A01d0101/XComFlutterOHOS_Native: '
          'flutter settings log message: hello world';
      final processManager = FakeProcessManager.list(<FakeCommand>[
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'hilog', '-v', 'time'],
          stdout: '$rawLine\n',
        ),
      ]);

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final HdcLogReader logReader = await HdcLogReader.createLogReader(device, processManager);
      await expectLater(logReader.logLines, emits('flutter: hello world'));
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'HdcLogReader filters hilog noise and passes non-print logs through unchanged',
    () async {
      const noiseLine = '09-14 20:15:33.123  4321  8765 I C01500/binder: some system noise';
      const dartPrintLine =
          '09-14 20:15:33.900  4321  8765 W A01d0101/XComFlutterOHOS_Native: '
          'flutter settings log message: hello world';
      const vmServiceLine =
          '09-14 20:15:34.001  4321  8765 I A01d0101/XComFlutterOHOS_Native: '
          'flutter The Dart VM service is listening on http://0.0.0.0:12345/abc=/';
      final processManager = FakeProcessManager.list(<FakeCommand>[
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'hilog', '-v', 'time'],
          stdout: '$noiseLine\n$dartPrintLine\n$vmServiceLine\n',
        ),
      ]);

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final HdcLogReader logReader = await HdcLogReader.createLogReader(device, processManager);
      await expectLater(
        logReader.logLines,
        emitsInOrder(<String>['flutter: hello world', vmServiceLine]),
      );
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'parseHidumperMemDump extracts the total PSS from a HarmonyOS NEXT table dump',
    () {
      final OhosMemoryInfo? info = parseHidumperMemDump(hidumperMemDumpFixture);
      expect(info, isNotNull);
      expect(info!.totalPss, 255902);
      expect(info.toJson(), <String, Object>{
        'platform': 'Ohos',
        'Realtime': 0,
        'Java Heap': 0,
        'Native Heap': 0,
        'Code': 0,
        'Stack': 0,
        'Graphics': 0,
        'Private Other': 0,
        'System': 0,
        'Total': 255902,
      });
    },
  );

  testUsingContext('parseHidumperMemDump extracts the total PSS from a legacy Total PSS line', () {
    final OhosMemoryInfo? info = parseHidumperMemDump(legacyHidumperMemDumpFixture);
    expect(info, isNotNull);
    expect(info!.totalPss, 401840);
    expect(info.toJson(), <String, Object>{
      'platform': 'Ohos',
      'Realtime': 0,
      'Java Heap': 0,
      'Native Heap': 0,
      'Code': 0,
      'Stack': 0,
      'Graphics': 0,
      'Private Other': 0,
      'System': 0,
      'Total': 401840,
    });
  });

  testUsingContext('parseHidumperMemDump returns null when the dump has no total row', () {
    expect(parseHidumperMemDump('hidumper: no such process'), isNull);
  });

  testUsingContext('parseHidumperMemDump returns null for unparseable output', () {
    expect(parseHidumperMemDump('no total pss in here'), isNull);
  });

  testUsingContext(
    'queryMemoryInfo returns parsed hidumper memory info for the launched package',
    () async {
      final processManager = FakeProcessManager.list(<FakeCommand>[
        ..._launchSequence(<String>['--pb', 'enable-dart-profiling', 'true']),
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'pidof', 'com.example.test'],
          stdout: '4321\n',
        ),
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'hidumper', '--mem', '4321'],
          stdout: hidumperMemDumpFixture,
        ),
      ]);

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final LaunchResult launchResult = await device.startApp(
        _createTestHap(),
        prebuiltApplication: true,
        debuggingOptions: DebuggingOptions.disabled(BuildInfo.release),
        platformArgs: <String, dynamic>{},
      );
      expect(launchResult.started, true);

      final MemoryInfo info = await device.queryMemoryInfo();
      expect(info, isA<OhosMemoryInfo>());
      expect((info as OhosMemoryInfo).totalPss, 255902);
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'queryMemoryInfo returns empty MemoryInfo when the app has exited',
    () async {
      final processManager = FakeProcessManager.list(<FakeCommand>[
        ..._launchSequence(<String>['--pb', 'enable-dart-profiling', 'true']),
        const FakeCommand(
          command: <String>['hdc', '-t', '123', 'shell', 'pidof', 'com.example.test'],
        ),
      ]);

      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final LaunchResult launchResult = await device.startApp(
        _createTestHap(),
        prebuiltApplication: true,
        debuggingOptions: DebuggingOptions.disabled(BuildInfo.release),
        platformArgs: <String, dynamic>{},
      );
      expect(launchResult.started, true);

      final MemoryInfo info = await device.queryMemoryInfo();
      expect(info.toJson(), isEmpty);
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );

  testUsingContext(
    'queryMemoryInfo returns empty MemoryInfo when no app was launched',
    () async {
      final processManager = FakeProcessManager.empty();
      final device = OhosDevice(
        '123',
        logger: BufferLogger.test(),
        processManager: processManager,
        ohosSdk: _FakeHarmonySdk(),
      );

      final MemoryInfo info = await device.queryMemoryInfo();
      expect(info.toJson(), isEmpty);
      expect(processManager, hasNoRemainingExpectations);
    },
    overrides: <Type, Generator>{
      FileSystem: () => MemoryFileSystem.test(),
      ProcessManager: () => FakeProcessManager.any(),
    },
  );
}

/// Sample `hidumper --mem <pid>` output modeled on the real-device capture
/// (HUAWEI Mate 60, OpenHarmony 7.0.0.102, `Total 401840`).
/// Real `hidumper --mem <pid>` output captured on a HarmonyOS NEXT device
/// (HUAWEI Mate 70 Pro): a column table whose last row is the `Total`
/// summary. The header rows deliberately stay in the fixture so a parser
/// regression that matches the header `Total` is caught.
const String hidumperMemDumpFixture = '''
-------------------------------[memory]-------------------------------

                          Pss         Shared         Shared        Private        Private           Swap        SwapPss           Heap           Heap           Heap
                        Total          Clean          Dirty          Clean          Dirty          Total          Total           Size           Alloc           Free
                       ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )
              ------------------------------------------------------------------------------------------------------------------------------------------------------
            GL          15184              0              0              0          15184              0              0              0              0              0
         Graph          15004              0              0              0          15004              0              0              0              0              0
   ark ts heap           7229           6016              0           6980              0              0              0              0              0              0
   native heap          54192          32328              0          52844              0           1916           1905          82156          77052          10209
          .hap          37556              0              0          37556              0              0              0              0              0              0
AnonPage other          95135           5980              0          94944              0             12             12              0              0              0
         stack           1160              0              0           1160              0              4              4              0              0              0
--------------------------------------------------------------------------------------------------------------------------------------------------------------------
         Total         255902         113484          34876         211488          31884           1940           1929          82156          77052          10209
''';

/// Older OHOS builds print `Total PSS:` lines instead of the column table.
const String legacyHidumperMemDumpFixture = '''
----------------------------[Memory Usage Information]----------------------------
Total PSS:               401840 kB
Total Kernel PSS:          1024 kB
Total SWAP PSS:               0 kB

                          Pss  Shared_Clean  Shared_Dirty  Private_Clean  Private_Dirty
Native Heap             231408              0          4096              0          219136
ArkTS Heap              81920              0             0              0           81920
File Backend Pages      65536          20480             0          16384           28672
''';

/// The full hdc command sequence for a startApp call: query the device
/// properties, stop and (re)install the app, then start it with the given
/// extra `aa start` arguments (the [extra] entries of the last command).
///
/// When [debugLaunch] is true, the hilog buffer is cleared and a log reader
/// is attached before `aa start`, matching the debuggingEnabled code path.
List<FakeCommand> _launchSequence(List<String> extra, {bool debugLaunch = false}) {
  return <FakeCommand>[
    // targetPlatform: param get (sync)
    const FakeCommand(
      command: <String>['hdc', '-t', '123', 'shell', 'param', 'get'],
      stdout: 'const.product.cpu.abilist=arm64-v8a',
    ),
    // stopApp: aa detach (async)
    const FakeCommand(
      command: <String>['hdc', '-t', '123', 'shell', 'aa', 'detach', '-b', 'com.example.test'],
    ),
    // stopApp: aa force-stop (sync)
    const FakeCommand(
      command: <String>['hdc', '-t', '123', 'shell', 'aa', 'force-stop', 'com.example.test'],
    ),
    // isAppInstalled: bm dump (sync)
    const FakeCommand(
      command: <String>['hdc', '-t', '123', 'shell', '"bm dump -n com.example.test"'],
      stdout: '/bin/sh: bm dump -n com.example.test: inaccessible or not found',
    ),
    // installApp: rm -rf
    const FakeCommand(
      command: <String>[
        'hdc',
        '-t',
        '123',
        'shell',
        'rm',
        '-rf',
        'data/local/tmp/flutterInstallTemp',
      ],
    ),
    // installApp: mkdir
    const FakeCommand(
      command: <String>['hdc', '-t', '123', 'shell', 'mkdir', 'data/local/tmp/flutterInstallTemp'],
    ),
    // installApp: file send
    const FakeCommand(
      command: <String>[
        'hdc',
        '-t',
        '123',
        'file',
        'send',
        '/test.hap',
        'data/local/tmp/flutterInstallTemp',
      ],
    ),
    // installApp: bm install
    const FakeCommand(
      command: <String>[
        'hdc',
        '-t',
        '123',
        'shell',
        'bm',
        'install',
        '-p',
        'data/local/tmp/flutterInstallTemp',
      ],
      stdout: 'install bundle successfully.',
    ),
    // installApp: rm -rf cleanup
    const FakeCommand(
      command: <String>[
        'hdc',
        '-t',
        '123',
        'shell',
        'rm',
        '-rf',
        'data/local/tmp/flutterInstallTemp',
      ],
    ),
    if (debugLaunch) ...<FakeCommand>[
      // clearLogs before starting the app.
      const FakeCommand(command: <String>['hdc', '-t', '123', 'shell', 'hilog', '-r']),
      // HdcLogReader stream; emits nothing so VM service discovery fails.
      const FakeCommand(command: <String>['hdc', '-t', '123', 'shell', 'hilog', '-v', 'time']),
    ],
    // aa start with the forwarded engine switches.
    FakeCommand(
      command: <String>[
        'hdc',
        '-t',
        '123',
        'shell',
        'aa',
        'start',
        '-a',
        'EntryAbility',
        '-b',
        'com.example.test',
        ...extra,
      ],
      stdout: 'start ability successfully.',
    ),
  ];
}

OhosHap _createTestHap() {
  // Use globals.fs so the file exists in the same FileSystem the device uses.
  final File hapFile = globals.fs.file('/test.hap')..createSync();

  final appInfo = AppInfo('com.example.test', 1, '1.0.0');
  final module = OhosModule(
    name: 'entry',
    srcPath: './entry',
    isEntry: true,
    mainElement: 'EntryAbility',
    type: OhosModuleType.entry,
    flavor: '',
  );
  final moduleInfo = ModuleInfo(<OhosModule>[module]);
  final buildData = OhosBuildData(appInfo, moduleInfo, 12, null);

  return OhosHap(id: 'com.example.test', applicationPackage: hapFile, ohosBuildData: buildData);
}

class _FakeHarmonySdk extends Fake implements HarmonySdk {
  @override
  String get name => 'FakeHarmonySDK';

  @override
  String get sdkPath => '/fake/sdk';

  @override
  String? get hdcPath => 'hdc';

  @override
  String? get npmPath => null;

  @override
  List<String> get apiAvailable => const <String>['12'];

  @override
  bool get isValidDirectory => true;
}
