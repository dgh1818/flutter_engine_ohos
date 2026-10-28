// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:file/memory.dart';
import 'package:flutter_tools/src/base/file_system.dart';
import 'package:flutter_tools/src/base/logger.dart';
import 'package:flutter_tools/src/base/platform.dart';
import 'package:flutter_tools/src/ohos/ohos_device_discovery.dart';
import 'package:flutter_tools/src/ohos/ohos_sdk.dart';
import 'package:flutter_tools/src/ohos/ohos_workflow.dart';
import 'package:test/fake.dart';

import '../../src/common.dart';
import '../../src/context.dart';

void main() {
  group('parseHdcDeviceOutput', () {
    test('parses a USB connect key as a device id', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '0005000E00AB1234',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['0005000E00AB1234']);
      expect(diagnostics, isEmpty);
    });

    test('parses a real USB connect key (with non-hex letters) as a device id', () {
      // Real `hdc list targets` output for a USB-connected phone: the connect
      // key contains non-hex letters, so a hex-only pattern would drop it.
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '63Q0226206003511',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['63Q0226206003511']);
      expect(diagnostics, isEmpty);
    });

    test('parses a wireless ip:port target as a device id', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '192.168.31.67:35095',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['192.168.31.67:35095']);
      expect(diagnostics, isEmpty);
    });

    test('[Empty] output produces no devices and one diagnostic', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput('[Empty]', deviceIds: deviceIds, diagnostics: diagnostics);

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['[Empty]']);
    });

    test('[Fail]Connect failed is routed to diagnostics, not devices', () {
      // Real hdc output observed with `hdc tconn 127.0.0.1:1` (issue #2991).
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '[Fail]Connect failed',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['[Fail]Connect failed']);
    });

    test('[Fail][E001005] device-not-found line is routed to diagnostics', () {
      // Real hdc output observed with `hdc -t 127.0.0.1:1 shell echo hi`
      // (issue #2991).
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '[Fail][E001005] Device not found or connected',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['[Fail][E001005] Device not found or connected']);
    });

    test('status words are routed to diagnostics', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        'Offline\nunauthorized\nUNKNOWN',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['Offline', 'unauthorized', 'UNKNOWN']);
    });

    test('routes an unauthorized device line to diagnostics', () {
      // Real `hdc list targets` output while the USB debugging authorization
      // dialog has not been accepted: the connect key is followed by a tab
      // and a status word, so the whole line must not become a device id.
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '63Q0226206003511\tUnauthorized',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['63Q0226206003511\tUnauthorized']);
    });

    test('mixed output lists only the real device and reports error lines', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '192.168.31.67:35095\n[Fail]Connect failed',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['192.168.31.67:35095']);
      expect(diagnostics, <String>['[Fail]Connect failed']);
    });

    test('handles CRLF line endings', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        'A1234567890B\r\n192.168.1.5:5555\r\n',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['A1234567890B', '192.168.1.5:5555']);
      expect(diagnostics, isEmpty);
    });
  });

  group('getDiagnostics', () {
    OhosDevices buildDevices(ProcessManager processManager, {HarmonySdk? ohosSdk}) {
      return OhosDevices(
        ohosWorkflow: _FakeOhosWorkflow(),
        processManager: processManager,
        logger: BufferLogger.test(),
        ohosSdk: ohosSdk,
      );
    }

    testUsingContext(
      'returns no diagnostics when hdc is unavailable, like Android',
      () async {
        final OhosDevices devices = buildDevices(FakeProcessManager.any());

        expect(await devices.getDiagnostics(), isEmpty);
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'hints how to connect a device when hdc reports [Empty]',
      () async {
        final OhosDevices devices = buildDevices(
          FakeProcessManager.list(<FakeCommand>[
            const FakeCommand(
              command: <String>['hdc', '-t', '', 'list', 'targets'],
              stdout: '[Empty]\n',
            ),
          ]),
          ohosSdk: _FakeHarmonySdk(),
        );

        final List<String> diagnostics = await devices.getDiagnostics();

        expect(diagnostics, hasLength(1));
        expect(diagnostics.single, contains('No HarmonyOS devices detected'));
        expect(diagnostics.single, contains('USB debugging'));
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'hints authorization when hdc fails to connect',
      () async {
        final OhosDevices devices = buildDevices(
          FakeProcessManager.list(<FakeCommand>[
            const FakeCommand(
              command: <String>['hdc', '-t', '', 'list', 'targets'],
              stdout: '[Fail]Connect failed\n',
            ),
          ]),
          ohosSdk: _FakeHarmonySdk(),
        );

        final List<String> diagnostics = await devices.getDiagnostics();

        expect(diagnostics, hasLength(1));
        expect(diagnostics.single, contains('Failed to connect to a HarmonyOS device'));
        expect(diagnostics.single, contains('authorization'));
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'hints authorization for an unauthorized device line',
      () async {
        // Real `hdc list targets` output while the authorization dialog has not
        // been accepted (see the parse tests above).
        final OhosDevices devices = buildDevices(
          FakeProcessManager.list(<FakeCommand>[
            const FakeCommand(
              command: <String>['hdc', '-t', '', 'list', 'targets'],
              stdout: '63Q0226206003511\tUnauthorized\n',
            ),
          ]),
          ohosSdk: _FakeHarmonySdk(),
        );

        final List<String> diagnostics = await devices.getDiagnostics();

        expect(diagnostics, hasLength(1));
        expect(diagnostics.single, contains('is not authorized'));
        expect(diagnostics.single, contains('authorization dialog'));
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'hints restart for an offline device line',
      () async {
        final OhosDevices devices = buildDevices(
          FakeProcessManager.list(<FakeCommand>[
            const FakeCommand(
              command: <String>['hdc', '-t', '', 'list', 'targets'],
              stdout: 'Offline\n',
            ),
          ]),
          ohosSdk: _FakeHarmonySdk(),
        );

        final List<String> diagnostics = await devices.getDiagnostics();

        expect(diagnostics, hasLength(1));
        expect(diagnostics.single, contains('is offline'));
        expect(diagnostics.single, contains('hdc kill'));
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'surfaces stderr when hdc list targets fails',
      () async {
        final OhosDevices devices = buildDevices(
          FakeProcessManager.list(<FakeCommand>[
            const FakeCommand(
              command: <String>['hdc', '-t', '', 'list', 'targets'],
              exitCode: 1,
              stderr: 'hdc server not running\n',
            ),
          ]),
          ohosSdk: _FakeHarmonySdk(),
        );

        final List<String> diagnostics = await devices.getDiagnostics();

        expect(diagnostics, hasLength(1));
        expect(diagnostics.single, contains('`hdc list targets` failed'));
        expect(diagnostics.single, contains('hdc server not running'));
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );

    testUsingContext(
      'mixed output yields a hint for the error line only',
      () async {
        final OhosDevices devices = buildDevices(
          FakeProcessManager.list(<FakeCommand>[
            const FakeCommand(
              command: <String>['hdc', '-t', '', 'list', 'targets'],
              stdout: '63Q0226206003511\n[Fail]Connect failed\n',
            ),
          ]),
          ohosSdk: _FakeHarmonySdk(),
        );

        final List<String> diagnostics = await devices.getDiagnostics();

        expect(diagnostics, hasLength(1));
        expect(diagnostics.single, contains('Failed to connect to a HarmonyOS device'));
      },
      overrides: <Type, Generator>{
        FileSystem: () => MemoryFileSystem.test(),
        ProcessManager: () => FakeProcessManager.any(),
        Platform: () => FakePlatform(),
      },
    );
  });
}

class _FakeOhosWorkflow extends Fake implements OhosWorkflow {}

class _FakeHarmonySdk extends Fake implements HarmonySdk {
  @override
  String get name => 'FakeHarmonySDK';

  @override
  String get sdkPath => '/fake/sdk';

  @override
  String? get hdcPath => 'hdc';
}
