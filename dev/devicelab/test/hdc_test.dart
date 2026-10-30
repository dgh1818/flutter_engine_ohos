// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:async';

import 'package:collection/collection.dart' show ListEquality, MapEquality;
import 'package:flutter_devicelab/framework/devices.dart';
import 'package:meta/meta.dart';

import 'common.dart';

void main() {
  group('OhosDevice', () {
    late FakeOhosDevice device;

    setUp(() {
      FakeOhosDevice.resetLog();
      device = FakeOhosDevice(deviceId: 'test-device');
    });

    group('isAwake/isAsleep', () {
      test('isAwake returns true when AWAKE', () async {
        FakeOhosDevice.output = 'Current State: AWAKE  Reason: 34  Time: 12345';
        expect(await device.isAwake(), isTrue);
        expect(await device.isAsleep(), isFalse);
      });

      test('isAwake returns false when SLEEP', () async {
        FakeOhosDevice.output = 'Current State: SLEEP  Reason: 1  Time: 12345';
        expect(await device.isAwake(), isFalse);
        expect(await device.isAsleep(), isTrue);
      });
    });

    group('wakeUp', () {
      test('sends power-shell wakeup', () async {
        await device.wakeUp();
        expectLog(<CommandArgs>[
          cmd(command: 'power-shell', arguments: <String>['wakeup']),
        ]);
      });
    });

    group('sendToSleep', () {
      test('sends power-shell suspend', () async {
        await device.sendToSleep();
        expectLog(<CommandArgs>[
          cmd(command: 'power-shell', arguments: <String>['suspend']),
        ]);
      });
    });

    group('home', () {
      test('sends home key event', () async {
        await device.home();
        expectLog(<CommandArgs>[
          cmd(command: 'uitest', arguments: <String>['uiInput', 'keyEvent', 'Home']),
        ]);
      });
    });

    group('togglePower', () {
      test('sends power key event', () async {
        await device.togglePower();
        expectLog(<CommandArgs>[
          cmd(command: 'uitest', arguments: <String>['uiInput', 'keyEvent', 'Power']),
        ]);
      });
    });

    group('unlock', () {
      test('wakes up then swipes up', () async {
        await device.unlock();
        expectLog(<CommandArgs>[
          cmd(command: 'power-shell', arguments: <String>['wakeup']),
          cmd(command: 'uitest', arguments: <String>['uiInput', 'dircFling', '2', '40000', '10']),
        ]);
      });
    });

    group('tap', () {
      test('sends click command', () async {
        await device.tap(100, 200);
        expectLog(<CommandArgs>[
          cmd(command: 'uitest', arguments: <String>['uiInput', 'click', '100', '200']),
        ]);
      });
    });

    group('reboot', () {
      test('sends reboot command via hdc', () async {
        await device.reboot();
        expectLog(<CommandArgs>[
          cmd(
            command: 'hdc',
            arguments: <String>['-t', device.deviceId, 'shell', 'reboot'],
            environment: <String, String>{FakeOhosDevice.canFailKey: 'false'},
          ),
        ]);
      });
    });

    group('stop', () {
      test('sends force-stop command', () async {
        await device.stop('com.example.app');
        expectLog(<CommandArgs>[
          cmd(command: 'aa', arguments: <String>['force-stop', 'com.example.app']),
        ]);
      });
    });

    group('getMemoryStats', () {
      test('parses memory stats from hidumper output', () async {
        FakeOhosDevice.outputMap = <String, String>{
          'pidof com.example.flutter_dfx_sample': '10937',
          'hidumper --mem 10937': '''
-------------------------------[memory]-------------------------------
                             Pss         Shared         Shared        Private        Private           Swap        SwapPss           Heap           Heap           Heap
                           Total          Clean          Dirty          Clean          Dirty          Total          Total           Size          Alloc           Free
                          ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )         ( kB )
                 ------------------------------------------------------------------------------------------------------------------------------------------------------
               GL           8716              0              0              0           8716              0              0              0              0              0
            Graph          74508              0              0              0          74508              0              0              0              0              0
      ark ts heap           4245           5796              0           3964              0           4080           4080              0              0              0
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
            Total         169407         109676          40092          42544          85376          27200          27200          68768          66983           3537
''',
        };
        final Map<String, dynamic> stats = await device.getMemoryStats(
          'com.example.flutter_dfx_sample',
        );
        expect(stats, <String, dynamic>{'total_kb': 169407});
      });

      test('returns empty when pid not found', () async {
        FakeOhosDevice.outputMap = <String, String>{'pidof com.example.app': ''};
        final Map<String, dynamic> stats = await device.getMemoryStats('com.example.app');
        expect(stats, <String, dynamic>{});
      });

      test('returns empty when Total not found in output', () async {
        FakeOhosDevice.outputMap = <String, String>{
          'pidof com.example.app': '1234',
          'hidumper --mem 1234': 'no data',
        };
        final Map<String, dynamic> stats = await device.getMemoryStats('com.example.app');
        expect(stats, <String, dynamic>{});
      });
    });

    group('clearLogs', () {
      test('sends hilog clear command', () async {
        await device.clearLogs();
        expectLog(<CommandArgs>[
          cmd(
            command: 'hdc',
            arguments: <String>['-t', device.deviceId, 'shell', 'hilog', '-r'],
            environment: <String, String>{FakeOhosDevice.canFailKey: 'true'},
          ),
        ]);
      });
    });

    group('toggleFixedPerformanceMode', () {
      test('enables performance mode', () async {
        await device.toggleFixedPerformanceMode(true);
        expectLog(<CommandArgs>[
          cmd(command: 'power-shell', arguments: <String>['setmode', '602']),
        ]);
      });

      test('disables performance mode', () async {
        await device.toggleFixedPerformanceMode(false);
        expectLog(<CommandArgs>[
          cmd(command: 'power-shell', arguments: <String>['setmode', '600']),
        ]);
      });
    });

    group('awaitDevice', () {
      test('is a no-op', () async {
        await device.awaitDevice();
        expectLog(<CommandArgs>[]);
      });
    });

    group('toString', () {
      test('returns correct string', () {
        expect(device.toString(), 'OhosDevice(test-device)');
      });
    });
  });
}

void expectLog(List<CommandArgs> log) {
  expect(FakeOhosDevice.commandLog, log);
}

CommandArgs cmd({
  required String command,
  List<String>? arguments,
  Map<String, String>? environment,
}) {
  return CommandArgs(command: command, arguments: arguments, environment: environment);
}

@immutable
class CommandArgs {
  const CommandArgs({required this.command, this.arguments, this.environment});

  final String command;
  final List<String>? arguments;
  final Map<String, String>? environment;

  @override
  String toString() =>
      'CommandArgs(command: $command, arguments: $arguments, environment: $environment)';

  @override
  bool operator ==(Object other) {
    if (other.runtimeType != runtimeType) {
      return false;
    }
    return other is CommandArgs &&
        other.command == command &&
        const ListEquality<String>().equals(other.arguments, arguments) &&
        const MapEquality<String, String>().equals(other.environment, environment);
  }

  @override
  int get hashCode {
    return Object.hash(
      command,
      Object.hashAll(arguments ?? const <String>[]),
      Object.hashAllUnordered(environment?.keys ?? const <String>[]),
      Object.hashAllUnordered(environment?.values ?? const <String>[]),
    );
  }
}

class FakeOhosDevice extends OhosDevice {
  FakeOhosDevice({required super.deviceId});

  static const String canFailKey = 'canFail';

  static String output = '';

  static Map<String, String> outputMap = <String, String>{};

  static List<CommandArgs> commandLog = <CommandArgs>[];

  static void resetLog() {
    commandLog.clear();
    output = '';
    outputMap.clear();
  }

  @override
  Future<void> hdcShellExec(
    String command,
    List<String> arguments, {
    Map<String, String>? environment,
    bool silent = false,
  }) async {
    commandLog.add(CommandArgs(command: command, arguments: arguments, environment: environment));
  }

  @override
  Future<String> hdcShellEval(
    String command,
    List<String> arguments, {
    Map<String, String>? environment,
    bool silent = false,
  }) async {
    final key = '$command ${arguments.join(' ')}';
    commandLog.add(CommandArgs(command: command, arguments: arguments, environment: environment));
    return outputMap[key] ?? output;
  }

  @override
  Future<String> hdc(
    List<String> arguments, {
    Map<String, String>? environment,
    bool silent = false,
    bool canFail = false,
  }) async {
    environment ??= <String, String>{};
    commandLog.add(
      CommandArgs(
        command: 'hdc',
        arguments: <String>['-t', deviceId, ...arguments],
        environment: environment..putIfAbsent(canFailKey, () => '$canFail'),
      ),
    );
    return output;
  }
}
