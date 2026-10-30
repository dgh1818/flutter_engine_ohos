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

const String testFile = 'test_memory/image_cache_memory.dart';

/// Measures image cache memory usage on OHos.
///
/// On OHos, Dart print output is forwarded to hilog by the engine embedder
/// (log_message_callback is set in ohos_main.cpp). However, this test does
/// not involve app pause/resume cycles, so the VM service connection used by
/// `flutter run` stays stable for the whole iteration and stdout alone is
/// sufficient to capture the READY and DONE benchmark messages. No separate
/// hilog stream is needed, unlike flutter_gallery__back_button_memory_ohos,
/// where the VM service connection can drop after multiple app pause/resume
/// cycles.
Future<TaskResult> runImageCacheMemoryTestOhos() async {
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
    const iterationCount = 10;

    for (var iteration = 0; iteration < iterationCount; iteration += 1) {
      print('running memory test iteration $iteration...');

      final ready = Completer<void>();
      final done = Completer<void>();

      // Clear hilog buffer to prevent stale messages from the previous app
      // instance from being forwarded by `flutter run` and prematurely
      // completing the completers below.
      await device.clearLogs();

      print('launching $projectPath/$testFile on device...');
      // Skip --verbose on OHOS: verbose output can block the stdout pipe and
      // cause stale hilog messages from previous app instances to be
      // forwarded, leading to premature completer completion.
      final Process run = await startFlutter(
        'run',
        options: <String>['--debug', '-d', device.deviceId, testFile],
      );

      run.stdout.transform<String>(utf8.decoder).transform<String>(const LineSplitter()).listen((
        String line,
      ) {
        print('run:stdout: $line');
        if (line.contains('==== MEMORY BENCHMARK ==== READY ====') && !ready.isCompleted) {
          ready.complete();
        }
        if (line.contains('==== MEMORY BENCHMARK ==== DONE ====') && !done.isCompleted) {
          done.complete();
        }
      });
      run.stderr.transform<String>(utf8.decoder).transform<String>(const LineSplitter()).listen((
        String line,
      ) {
        stderr.writeln('run:stderr: $line');
      });

      await ready.future.timeout(const Duration(seconds: 120));
      print('app is ready');

      // Record start memory.
      print('snapshotting memory usage (start)...');
      final Map<String, dynamic> startMem = await device.getMemoryStats(bundleName);
      final startKb = startMem['total_kb'] as int?;

      // Wait for the test to finish scrolling and print DONE.
      print('awaiting "done" message...');
      await done.future.timeout(const Duration(seconds: 300));

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
  await task(runImageCacheMemoryTestOhos);
}
