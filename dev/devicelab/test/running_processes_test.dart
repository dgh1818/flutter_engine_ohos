// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter_devicelab/framework/running_processes.dart';
import 'package:process/process.dart';

import 'common.dart';

void main() {
  test('Parse PowerShell result', () {
    final t1 = DateTime(2019, 7, 3, 17, 0, 27);
    final t2 = DateTime(2019, 7, 3, 22, 0, 27);
    final t3 = DateTime(2019, 7, 3, 11, 0, 27);
    final String powershellOutput = jsonEncode(<Object>[
      <String, Object>{
        'ProcessId': 6552,
        'CreationDate': '/Date(${t1.millisecondsSinceEpoch})/',
        'CommandLine': r'"C:\tools\dart-sdk\bin\dart.exe" .\bin\agent.dart ci',
      },
      <String, Object>{
        'ProcessId': 6553,
        'CreationDate': '/Date(${t2.millisecondsSinceEpoch})/',
        'CommandLine': r'"C:\tools\dart-sdk1\bin\dart.exe" .\bin\agent.dart ci',
      },
      <String, Object>{
        'ProcessId': 6554,
        'CreationDate': '/Date(${t3.millisecondsSinceEpoch})/',
        'CommandLine': r'"C:\tools\dart-sdk2\bin\dart.exe" .\bin\agent.dart ci',
      },
    ]);
    final List<RunningProcessInfo> results = parseWindowsProcessJson(powershellOutput).toList();
    expect(results.length, 3);
    expect(
      results,
      equals(<RunningProcessInfo>[
        RunningProcessInfo(6552, r'"C:\tools\dart-sdk\bin\dart.exe" .\bin\agent.dart ci', t1),
        RunningProcessInfo(6553, r'"C:\tools\dart-sdk1\bin\dart.exe" .\bin\agent.dart ci', t2),
        RunningProcessInfo(6554, r'"C:\tools\dart-sdk2\bin\dart.exe" .\bin\agent.dart ci', t3),
      ]),
    );
  });

  test('Parse PowerShell result (single object)', () {
    final t1 = DateTime(2019, 7, 3, 17, 0, 27);
    final String powershellOutput = jsonEncode(<String, Object>{
      'ProcessId': 6552,
      'CreationDate': '/Date(${t1.millisecondsSinceEpoch})/',
      'CommandLine': r'"C:\tools\dart-sdk\bin\dart.exe" .\bin\agent.dart ci',
    });
    final List<RunningProcessInfo> results = parseWindowsProcessJson(powershellOutput).toList();
    expect(results.length, 1);
    expect(
      results,
      equals(<RunningProcessInfo>[
        RunningProcessInfo(6552, r'"C:\tools\dart-sdk\bin\dart.exe" .\bin\agent.dart ci', t1),
      ]),
    );
  });

  test('Parse PowerShell result (empty)', () {
    expect(parseWindowsProcessJson('').toList(), isEmpty);
    expect(parseWindowsProcessJson('   ').toList(), isEmpty);
  });

  test('Parse Posix output', () {
    const psOutput = r'''
STARTED                        PID COMMAND
Sat Mar  9 20:12:47 2019         1 /sbin/launchd
Sat Mar  9 20:13:00 2019        49 /usr/sbin/syslogd
''';

    final List<RunningProcessInfo> results = processPsOutput(psOutput, null).toList();
    expect(results.length, 2);
    expect(
      results,
      equals(<RunningProcessInfo>[
        RunningProcessInfo(1, '/sbin/launchd', DateTime(2019, 3, 9, 20, 12, 47)),
        RunningProcessInfo(49, '/usr/sbin/syslogd', DateTime(2019, 3, 9, 20, 13)),
      ]),
    );
  });

  test('RunningProcessInfo.terminate', () {
    final process = RunningProcessInfo(123, 'test', DateTime(456));
    final fakeProcessManager = FakeProcessManager();
    process.terminate(processManager: fakeProcessManager);
    if (Platform.isWindows) {
      expect(fakeProcessManager.log, <String>[
        'run([taskkill.exe, /pid, 123, /f], null, null, null, null, null, null)',
      ]);
    } else {
      expect(fakeProcessManager.log, <String>['killPid(123, SIGKILL)']);
    }
  });
}

class FakeProcessManager implements ProcessManager {
  final List<String> log = <String>[];

  @override
  bool canRun(Object? a, {String? workingDirectory}) {
    log.add('canRun($a, $workingDirectory)');
    return true;
  }

  @override
  bool killPid(int a, [ProcessSignal? b]) {
    log.add('killPid($a, $b)');
    return true;
  }

  @override
  Future<ProcessResult> run(
    List<Object> a, {
    Map<String, String>? environment,
    bool? includeParentEnvironment,
    bool? runInShell,
    Encoding? stderrEncoding,
    Encoding? stdoutEncoding,
    String? workingDirectory,
  }) async {
    log.add(
      'run($a, $environment, $includeParentEnvironment, $runInShell, $stderrEncoding, $stdoutEncoding, $workingDirectory)',
    );
    return ProcessResult(1, 0, 'stdout', 'stderr');
  }

  @override
  ProcessResult runSync(
    List<Object> a, {
    Map<String, String>? environment,
    bool? includeParentEnvironment,
    bool? runInShell,
    Encoding? stderrEncoding,
    Encoding? stdoutEncoding,
    String? workingDirectory,
  }) {
    log.add(
      'runSync($a, $environment, $includeParentEnvironment, $runInShell, $stderrEncoding, $stdoutEncoding, $workingDirectory)',
    );
    return ProcessResult(1, 0, 'stdout', 'stderr');
  }

  @override
  Future<Process> start(
    List<Object> a, {
    Map<String, String>? environment,
    bool? includeParentEnvironment,
    ProcessStartMode? mode,
    bool? runInShell,
    String? workingDirectory,
  }) {
    log.add(
      'start($a, $environment, $includeParentEnvironment, $mode, $runInShell, $workingDirectory)',
    );
    return Completer<Process>().future;
  }
}
