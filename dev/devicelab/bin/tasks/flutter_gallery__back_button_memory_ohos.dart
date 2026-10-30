// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter_devicelab/framework/devices.dart';
import 'package:flutter_devicelab/framework/framework.dart';
import 'package:flutter_devicelab/framework/task_result.dart';
import 'package:flutter_devicelab/framework/utils.dart';
import 'package:flutter_devicelab/tasks/perf_tests.dart';

const String testFile = 'test_memory/back_button.dart';

/// Measure application memory usage after pausing and resuming the app
/// with the OHos back button.
///
/// On OHOS, Dart print output is forwarded to hilog by the engine embedder.
/// However, the VM service connection used by `flutter run` can drop after
/// multiple app pause/resume cycles, causing `flutter run` to stop forwarding
/// device logs to stdout. This test uses both `flutter run` stdout and a
/// separate `hdc shell hilog` stream to capture lifecycle messages reliably.
Future<TaskResult> runBackButtonMemoryTestOhos() async {
  deviceOperatingSystem = DeviceOperatingSystem.ohos;
  final device = (await devices.workingDevice) as OhosDevice;
  await device.unlock();

  final projectPath = '${flutterDirectory.path}/dev/integration_tests/flutter_gallery';
  final String bundleName = ohosBundleName(projectPath);

  final Directory appDir = dir(projectPath);
  return inDirectory<TaskResult>(appDir, () async {
    await flutter('packages', options: <String>['get']);

    final startMemory = <int>[];
    final endMemory = <int>[];
    final diffMemory = <int>[];
    const iterationCount = 3;

    for (var iteration = 0; iteration < iterationCount; iteration += 1) {
      print('running memory test iteration $iteration...');

      final ready = Completer<void>();
      final messages = StreamController<String>.broadcast();

      // Start a separate hilog stream as a secondary source for lifecycle
      // events. On OHOS, the VM service connection used by `flutter run`
      // can drop after multiple app pause/resume cycles, causing Dart
      // print output to stop being forwarded to stdout. The hilog stream
      // provides a more reliable channel for capturing lifecycle messages.
      final StreamSubscription<String> hilogSub = device.logcat.listen(
        (String line) {
          if (line.contains('==== MEMORY BENCHMARK ==== ')) {
            final int idx = line.indexOf('==== MEMORY BENCHMARK ==== ');
            messages.add(
              line.substring(idx + '==== MEMORY BENCHMARK ==== '.length).replaceAll(' ====', ''),
            );
          }
        },
        onError: (Object error) {
          print('hilog stream error: $error');
        },
        onDone: () {
          print('hilog stream done');
        },
      );

      // Launch app with flutter run (debug, persistent connection).
      // Wait for VM Service connection rather than the READY message, because
      // the app may print READY before the VM Service connection is established,
      // causing the message to be missed in the stdout stream.
      print('launching $projectPath/$testFile on device...');
      final Process run = await startFlutter(
        'run',
        options: <String>['--verbose', '--debug', '-d', device.deviceId, testFile],
      );

      run.stdout.transform<String>(utf8.decoder).transform<String>(const LineSplitter()).listen((
        String line,
      ) {
        print('run:stdout: $line');
        if (line.contains('Successfully connected to service protocol') && !ready.isCompleted) {
          ready.complete();
        }
        if (line.contains('==== MEMORY BENCHMARK ==== ')) {
          final int idx = line.indexOf('==== MEMORY BENCHMARK ==== ');
          messages.add(
            line.substring(idx + '==== MEMORY BENCHMARK ==== '.length).replaceAll(' ====', ''),
          );
        }
      });
      run.stderr.transform<String>(utf8.decoder).transform<String>(const LineSplitter()).listen((
        String line,
      ) {
        stderr.writeln('run:stderr: $line');
      });

      await ready.future.timeout(const Duration(seconds: 120));
      // Give the app time to fully initialize after VM Service connection.
      await Future<void>.delayed(const Duration(seconds: 10));
      print('app is ready');

      // Record start memory.
      print('snapshotting memory usage (start)...');
      final Map<String, dynamic> startMem = await device.getMemoryStats(bundleName);
      final startKb = startMem['total_kb'] as int?;

      // Perform back button cycles.
      for (var cycle = 0; cycle < 8; cycle += 1) {
        print('back/forward iteration $cycle');

        // Wait for "AppLifecycleState.paused" message.
        final paused = Completer<void>();
        final StreamSubscription<String> sub = messages.stream.listen((String msg) {
          if (msg.contains('AppLifecycleState.paused') && !paused.isCompleted) {
            paused.complete();
          }
        });

        // Push back button.
        await device.hdcShellExec('uitest', <String>['uiInput', 'keyEvent', 'Back']);
        await paused.future.timeout(const Duration(seconds: 120));
        await sub.cancel();

        await Future<void>.delayed(const Duration(milliseconds: 100));

        // Wait for "AppLifecycleState.resumed" message after relaunch.
        final relaunched = Completer<void>();
        final StreamSubscription<String> sub2 = messages.stream.listen((String msg) {
          if (msg.contains('AppLifecycleState.resumed') && !relaunched.isCompleted) {
            relaunched.complete();
          }
        });

        // Relaunch the app.
        final String output = await device.hdcShellEval('aa', <String>[
          'start',
          '-a',
          'EntryAbility',
          '-b',
          bundleName,
        ]);
        print('hdc shell aa start: $output');

        await relaunched.future.timeout(const Duration(seconds: 120));
        await sub2.cancel();

        await Future<void>.delayed(const Duration(milliseconds: 100));
      }

      // Record end memory.
      print('snapshotting memory usage (end)...');
      final Map<String, dynamic> endMem = await device.getMemoryStats(bundleName);
      final endKb = endMem['total_kb'] as int?;

      if (startKb != null && endKb != null) {
        startMemory.add(startKb);
        endMemory.add(endKb);
        diffMemory.add(endKb - startKb);
      }

      // Quit the app.
      run.stdin.write('q');
      await run.exitCode;
      await Future<void>.delayed(const Duration(seconds: 5));

      print('terminating...');
      await device.stop(bundleName);
      await Future<void>.delayed(const Duration(seconds: 3));

      await hilogSub.cancel();
    }

    await device.uninstallApp();

    final memoryUsage = <String, dynamic>{};
    if (startMemory.isNotEmpty) {
      final double startAvg = startMemory.reduce((int a, int b) => a + b) / startMemory.length;
      final double endAvg = endMemory.reduce((int a, int b) => a + b) / endMemory.length;
      final double diffAvg = diffMemory.reduce((int a, int b) => a + b) / diffMemory.length;
      memoryUsage['start_avg_kb'] = startAvg.round();
      memoryUsage['end_avg_kb'] = endAvg.round();
      memoryUsage['diff_avg_kb'] = diffAvg.round();
    }

    return TaskResult.success(memoryUsage, benchmarkScoreKeys: memoryUsage.keys.toList());
  });
}

Future<void> main() async {
  await task(runBackButtonMemoryTestOhos);
}
