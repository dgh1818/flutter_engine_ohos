// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:io' show Directory, Process;

import 'package:path/path.dart' as path;

import '../framework/devices.dart' as adb;
import '../framework/framework.dart' show TaskFunction;
import '../framework/task_result.dart' show TaskResult;
import '../framework/utils.dart' as utils;
import '../microbenchmarks.dart' as microbenchmarks;

TaskFunction runTask(adb.DeviceOperatingSystem operatingSystem) {
  return () async {
    adb.deviceOperatingSystem = operatingSystem;
    final adb.Device device = await adb.devices.workingDevice;
    await device.unlock();
    await device.clearLogs();

    final Directory appDir = utils.dir(
      path.join(utils.flutterDirectory.path, 'dev/benchmarks/platform_channels_benchmarks'),
    );

    // On OHOS, use the device's logcat stream (hdc hilog) as a secondary
    // source of benchmark results. The VM service connection can drop
    // mid-run, causing `flutter run` to stop forwarding device logs.
    // Start listening before `flutter run` so the logcat stream's
    // internal clearLogs() doesn't erase early benchmark output.
    StreamController<String>? deviceLogController;
    StreamSubscription<String>? deviceLogSub;
    if (operatingSystem == adb.DeviceOperatingSystem.ohos) {
      deviceLogController = StreamController<String>();
      deviceLogSub = device.logcat.listen(
        deviceLogController.add,
        onError: (_) {},
        onDone: deviceLogController.close,
      );
    }

    final Process flutterProcess;
    try {
      flutterProcess = await utils.inDirectory(appDir, () async {
        final createArgs = <String>['--platforms', 'ios,android', '--no-overwrite', '-v', '.'];
        print('\nExecuting: flutter create $createArgs $appDir');
        await utils.flutter('create', options: createArgs);

        final options = <String>[
          // On OHOS, -v produces excessive output that can block the stdout
          // pipe between flutter run and readJsonResults, causing benchmark
          // results to be lost when the app exits. Skip -v on OHOS.
          if (operatingSystem != adb.DeviceOperatingSystem.ohos) '-v',
          // --release doesn't work on iOS due to code signing issues
          '--profile',
          '--no-publish-port',
          '-d',
          device.deviceId,
        ];
        return utils.startFlutter('run', options: options);
      });

      final Map<String, double> results = await microbenchmarks.readJsonResults(
        flutterProcess,
        deviceLogStream: deviceLogController?.stream,
      );
      return TaskResult.success(results, benchmarkScoreKeys: results.keys.toList());
    } finally {
      await deviceLogSub?.cancel();
      await deviceLogController?.close();
    }
  };
}
