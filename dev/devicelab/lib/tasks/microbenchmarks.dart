// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:io';

import 'package:path/path.dart' as path;

import '../framework/devices.dart';
import '../framework/framework.dart';
import '../framework/task_result.dart';
import '../framework/utils.dart';
import '../microbenchmarks.dart';

/// Creates a device lab task that runs benchmarks in
/// `dev/benchmarks/microbenchmarks` reports results to the dashboard.
TaskFunction createMicrobenchmarkTask({
  bool? enableImpeller,
  Map<String, String> environment = const <String, String>{},
}) {
  // Generate a seed for this test stable around the date.
  final DateTime seedDate = DateTime.now().toUtc().subtract(const Duration(hours: 7));
  final seed = DateTime(seedDate.year, seedDate.month, seedDate.day).hashCode;

  return () async {
    final Device device = await devices.workingDevice;
    await device.unlock();
    await device.clearLogs();

    final Directory appDir = dir(
      path.join(flutterDirectory.path, 'dev/benchmarks/microbenchmarks'),
    );

    // Hard-uninstall any prior apps.
    await inDirectory(appDir, () async {
      section('Uninstall previous microbenchmarks app');
      await flutter('install', options: <String>['-v', '--uninstall-only', '-d', device.deviceId]);
    });

    Future<Map<String, double>> runMicrobench(String benchmarkPath) async {
      Future<Map<String, double>> run() async {
        print('Running $benchmarkPath with seed $seed');

        // On OHOS, use the device's logcat stream (hdc hilog) as a secondary
        // source of benchmark results. The VM service connection can drop
        // mid-run, causing `flutter run` to stop forwarding device logs.
        // Start listening before `flutter run` so the logcat stream's
        // internal clearLogs() doesn't erase early benchmark output.
        StreamController<String>? deviceLogController;
        StreamSubscription<String>? deviceLogSub;
        if (deviceOperatingSystem == DeviceOperatingSystem.ohos) {
          deviceLogController = StreamController<String>();
          deviceLogSub = device.logcat.listen(
            deviceLogController.add,
            onError: (_) {},
            onDone: deviceLogController.close,
          );
        }

        final Process flutterProcess;
        try {
          flutterProcess = await inDirectory(appDir, () async {
            final options = <String>[
              // On OHOS, -v produces excessive output that can block the stdout
              // pipe between flutter run and readJsonResults, causing benchmark
              // results to be lost when the app exits. Skip -v on OHOS.
              if (deviceOperatingSystem != DeviceOperatingSystem.ohos) '-v',
              // --release doesn't work on iOS due to code signing issues
              '--profile',
              '--no-publish-port',
              if (enableImpeller != null && enableImpeller) '--enable-impeller',
              if (enableImpeller != null && !enableImpeller) '--no-enable-impeller',
              '-d',
              device.deviceId,
              '--dart-define=seed=$seed',
              benchmarkPath,
            ];
            return startFlutter('run', options: options, environment: environment);
          });
          return await readJsonResults(
            flutterProcess,
            deviceLogStream: deviceLogController?.stream,
          );
        } finally {
          await deviceLogSub?.cancel();
          await deviceLogController?.close();
        }
      }

      return run();
    }

    final allResults = <String, double>{...await runMicrobench('lib/benchmark_collection.dart')};

    return TaskResult.success(allResults, benchmarkScoreKeys: allResults.keys.toList());
  };
}
