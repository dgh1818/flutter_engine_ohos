// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert';
import 'dart:io';

import 'package:meta/meta.dart';
import 'package:process/process.dart';

@immutable
class RunningProcessInfo {
  const RunningProcessInfo(this.pid, this.commandLine, this.creationDate);

  final int pid;
  final String commandLine;
  final DateTime creationDate;

  @override
  bool operator ==(Object other) {
    return other is RunningProcessInfo &&
        other.pid == pid &&
        other.commandLine == commandLine &&
        other.creationDate == creationDate;
  }

  Future<bool> terminate({required ProcessManager processManager}) async {
    // This returns true when the signal is sent, not when the process goes away.
    // See also https://github.com/dart-lang/sdk/issues/40759 (killPid should wait for process to be terminated).
    if (Platform.isWindows) {
      // TODO(ianh): Move Windows to killPid once we can.
      //  - killPid on Windows has not-useful return code: https://github.com/dart-lang/sdk/issues/47675
      final ProcessResult result = await processManager.run(<String>[
        'taskkill.exe',
        '/pid',
        '$pid',
        '/f',
      ]);
      return result.exitCode == 0;
    }
    return processManager.killPid(pid, ProcessSignal.sigkill);
  }

  @override
  int get hashCode => Object.hash(pid, commandLine, creationDate);

  @override
  String toString() {
    return 'RunningProcesses(pid: $pid, commandLine: $commandLine, creationDate: $creationDate)';
  }
}

Future<Set<RunningProcessInfo>> getRunningProcesses({
  String? processName,
  required ProcessManager processManager,
}) {
  if (Platform.isWindows) {
    return windowsRunningProcesses(processName, processManager);
  }
  return posixRunningProcesses(processName, processManager);
}

@visibleForTesting
Future<Set<RunningProcessInfo>> windowsRunningProcesses(
  String? processName,
  ProcessManager processManager,
) async {
  // PowerShell script to get the command line arguments and create time of a process.
  // See: https://docs.microsoft.com/en-us/windows/desktop/cimwin32prov/win32-process
  //
  // The output is serialized with `ConvertTo-Json` rather than formatted as a table so
  // that the `CreationDate` is emitted as a locale-independent .NET JSON date (e.g.
  // `/Date(1580000000000)/`) and the `CommandLine` is properly escaped. Parsing a
  // `Format-Table` layout relied on a US-locale date format and broke on machines using
  // other regional formats (e.g. `yyyy/M/d H:mm:ss`).
  final script = processName != null
      ? '"Get-CimInstance Win32_Process -Filter \\"name=\'$processName\'\\" | Select-Object ProcessId,CreationDate,CommandLine | ConvertTo-Json -Depth 2"'
      : '"Get-CimInstance Win32_Process | Select-Object ProcessId,CreationDate,CommandLine | ConvertTo-Json -Depth 2"';
  // TODO(ianh): Unfortunately, there doesn't seem to be a good way to get
  // ProcessManager to run this.
  final ProcessResult result = await Process.run('powershell -command $script', <String>[]);
  if (result.exitCode != 0) {
    print('Could not list processes!');
    print(result.stderr);
    print(result.stdout);
    return <RunningProcessInfo>{};
  }
  return parseWindowsProcessJson(result.stdout as String).toSet();
}

/// Parses the JSON output of the PowerShell script from [windowsRunningProcesses].
///
/// `ConvertTo-Json` serializes each process as an object with `ProcessId`,
/// `CreationDate` and `CommandLine` fields. The `CreationDate` is a .NET JSON date in
/// the form `/Date(<milliseconds since epoch>[+<offset>])/`. When there is exactly one
/// process the output is a single object instead of an array.
@visibleForTesting
Iterable<RunningProcessInfo> parseWindowsProcessJson(String output) sync* {
  final String trimmed = output.trim();
  if (trimmed.isEmpty) {
    return;
  }
  final Object? decoded = jsonDecode(trimmed);
  final List<Object?> entries;
  if (decoded is List<Object?>) {
    entries = decoded;
  } else if (decoded is Map<Object?, Object?>) {
    entries = <Object?>[decoded];
  } else {
    return;
  }
  for (final entry in entries) {
    if (entry is! Map<Object?, Object?>) {
      continue;
    }
    final Object? pidValue = entry['ProcessId'];
    final Object? dateValue = entry['CreationDate'];
    final Object? cmdValue = entry['CommandLine'];
    if (pidValue == null || dateValue == null || cmdValue == null) {
      continue;
    }
    final int pid = pidValue is int ? pidValue : int.parse(pidValue.toString());
    final Match? match = _jsonDateRegExp.firstMatch(dateValue.toString());
    if (match == null) {
      continue;
    }
    final int millisSinceEpoch = int.parse(match.group(1)!);
    final creationDate = DateTime.fromMillisecondsSinceEpoch(millisSinceEpoch);
    yield RunningProcessInfo(pid, cmdValue.toString(), creationDate);
  }
}

final RegExp _jsonDateRegExp = RegExp(r'/Date\((-?\d+)(?:[+-]\d{4})?\)/');

@visibleForTesting
Future<Set<RunningProcessInfo>> posixRunningProcesses(
  String? processName,
  ProcessManager processManager,
) async {
  final ProcessResult result = await processManager.run(<String>[
    'ps',
    '-eo',
    'lstart,pid,command',
  ]);
  if (result.exitCode != 0) {
    print('Could not list processes!');
    print(result.stderr);
    print(result.stdout);
    return <RunningProcessInfo>{};
  }
  return processPsOutput(result.stdout as String, processName).toSet();
}

/// Parses the output of the command in [posixRunningProcesses].
///
/// E.g.:
///
/// STARTED                        PID COMMAND
/// Sat Mar  9 20:12:47 2019         1 /sbin/launchd
/// Sat Mar  9 20:13:00 2019        49 /usr/sbin/syslogd
@visibleForTesting
Iterable<RunningProcessInfo> processPsOutput(String output, String? processName) sync* {
  var inTableBody = false;
  for (String line in output.split('\n')) {
    if (line.trim().startsWith('STARTED')) {
      inTableBody = true;
      continue;
    }
    if (!inTableBody || line.isEmpty) {
      continue;
    }

    if (processName != null && !line.contains(processName)) {
      continue;
    }
    if (line.length < 25) {
      continue;
    }

    // 'Sat Feb 16 02:29:55 2019'
    // 'Sat Mar  9 20:12:47 2019'
    const months = <String, String>{
      'Jan': '01',
      'Feb': '02',
      'Mar': '03',
      'Apr': '04',
      'May': '05',
      'Jun': '06',
      'Jul': '07',
      'Aug': '08',
      'Sep': '09',
      'Oct': '10',
      'Nov': '11',
      'Dec': '12',
    };
    final String rawTime = line.substring(0, 24);

    final String year = rawTime.substring(20, 24);
    final String month = months[rawTime.substring(4, 7)]!;
    final String day = rawTime.substring(8, 10).replaceFirst(' ', '0');
    final String time = rawTime.substring(11, 19);

    final DateTime creationDate = DateTime.parse('$year-$month-${day}T$time');
    line = line.substring(24).trim();
    final int nextSpace = line.indexOf(' ');
    final int pid = int.parse(line.substring(0, nextSpace));
    final String commandLine = line.substring(nextSpace + 1);
    yield RunningProcessInfo(pid, commandLine, creationDate);
  }
}
