// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

/// Reads through the print commands from [process] waiting for the magic phase
/// that contains microbenchmarks results as defined in
/// `dev/benchmarks/microbenchmarks/lib/common.dart`.
///
/// If you are using this outside of microbenchmarks, ensure you print a single
/// line with `╡ ••• Done ••• ╞` to signal the end of collection.
///
/// When [deviceLogStream] is provided (e.g. `hdc hilog` on OHOS), JSON results
/// are also collected from it. This is necessary on OHOS where the VM service
/// connection can drop mid-run, causing `flutter run` to stop forwarding device
/// logs before all benchmark results have been delivered.
Future<Map<String, double>> readJsonResults(Process process, {Stream<String>? deviceLogStream}) {
  // IMPORTANT: keep these values in sync with dev/benchmarks/microbenchmarks/lib/common.dart
  const jsonStart = '================ RESULTS ================';
  const jsonEnd = '================ FORMATTED ==============';
  const jsonPrefix = ':::JSON:::';
  const testComplete = '╡ ••• Done ••• ╞';

  var jsonStarted = false;
  final jsonBuf = StringBuffer();
  final completer = Completer<Map<String, double>>();

  final StreamSubscription<String> stderrSub = process.stderr
      .transform<String>(const Utf8Decoder())
      .transform<String>(const LineSplitter())
      .listen((String line) {
        stderr.writeln('[STDERR] $line');
      });

  final collectedJson = <String>[];

  var processWasKilledIntentionally = false;
  final stdoutDone = Completer<void>();

  void handleDoneMarker() {
    if (processWasKilledIntentionally || completer.isCompleted) {
      return;
    }
    processWasKilledIntentionally = true;

    // Complete with results immediately — all JSON blocks are printed before
    // the Done marker, so collectedJson should have everything.
    try {
      final results = Map<String, double>.from(<String, dynamic>{
        for (final String data in collectedJson) ...json.decode(data) as Map<String, dynamic>,
      });
      completer.complete(results);
    } catch (ex) {
      completer.completeError('Decoding JSON failed ($ex). JSON strings where: $collectedJson');
    }

    // Try to gracefully quit the process. It may have already exited (e.g.
    // when the Done marker arrived via the device log stream after the VM
    // service connection dropped).
    try {
      process.stdin.write('q');
      process.stdin
          .flush()
          .then((_) {
            return Future<void>.delayed(const Duration(seconds: 2));
          })
          .then((_) {
            process.kill(ProcessSignal.sigint);
          })
          .catchError((_) {});
    } catch (_) {}
  }

  final StreamSubscription<String> stdoutSub = process.stdout
      .transform<String>(const Utf8Decoder())
      .transform<String>(const LineSplitter())
      .listen(
        (String line) async {
          print('[STDOUT] $line');

          if (line.contains(jsonStart)) {
            jsonStarted = true;
            return;
          }

          if (line.contains(testComplete)) {
            handleDoneMarker();
            return;
          }

          if (jsonStarted && line.contains(jsonEnd)) {
            collectedJson.add(jsonBuf.toString().trim());
            jsonBuf.clear();
            jsonStarted = false;
          }

          if (jsonStarted && line.contains(jsonPrefix)) {
            jsonBuf.writeln(line.substring(line.indexOf(jsonPrefix) + jsonPrefix.length));
          }
        },
        onDone: () {
          if (!stdoutDone.isCompleted) {
            stdoutDone.complete();
          }
        },
      );

  // Listen to the device log stream (e.g. hilog on OHOS) as a secondary
  // source of benchmark results. This is necessary because `flutter run` may
  // stop forwarding device logs when the VM service connection drops, even
  // though the app is still running and producing output.
  StreamSubscription<String>? deviceLogSub;
  final deviceLogDone = Completer<void>();
  if (deviceLogStream != null) {
    var deviceJsonStarted = false;
    final deviceJsonBuf = StringBuffer();
    deviceLogSub = deviceLogStream.listen(
      (String line) {
        if (line.contains(jsonStart)) {
          deviceJsonStarted = true;
          return;
        }

        if (line.contains(testComplete)) {
          if (!deviceLogDone.isCompleted) {
            deviceLogDone.complete();
          }
          handleDoneMarker();
          return;
        }

        if (deviceJsonStarted && line.contains(jsonEnd)) {
          collectedJson.add(deviceJsonBuf.toString().trim());
          deviceJsonBuf.clear();
          deviceJsonStarted = false;
        }

        if (deviceJsonStarted && line.contains(jsonPrefix)) {
          deviceJsonBuf.writeln(line.substring(line.indexOf(jsonPrefix) + jsonPrefix.length));
        }
      },
      onDone: () {
        if (!deviceLogDone.isCompleted) {
          deviceLogDone.complete();
        }
      },
    );
  }

  process.exitCode.then<void>((int code) async {
    // Wait for the stdout stream to drain before canceling the subscription.
    // On OHOS, device logs (hilog) are forwarded asynchronously by `flutter run`.
    // When the app exits, there may still be unprocessed benchmark results in
    // the stdout pipe. Canceling immediately would discard them.
    if (!stdoutDone.isCompleted) {
      await stdoutDone.future.timeout(const Duration(seconds: 10));
    }
    await Future.wait<void>(<Future<void>>[stdoutSub.cancel(), stderrSub.cancel()]);

    if (!processWasKilledIntentionally && !completer.isCompleted) {
      if (code != 0) {
        await deviceLogSub?.cancel();
        completer.completeError('flutter run failed: exit code=$code');
      } else {
        // Process exited cleanly but without the Done marker (e.g. lost
        // connection to device on OHOS). If we have a device log stream,
        // wait for the Done marker from it — the app may still be running
        // on the device even though `flutter run` has lost its connection.
        if (deviceLogSub != null && !deviceLogDone.isCompleted) {
          try {
            await deviceLogDone.future.timeout(const Duration(seconds: 120));
          } catch (_) {
            // Timeout — no Done marker in device log stream.
          }
        }
        await deviceLogSub?.cancel();
        if (!completer.isCompleted) {
          // Return whatever was collected from both sources.
          if (collectedJson.isNotEmpty) {
            try {
              final results = Map<String, double>.from(<String, dynamic>{
                for (final String data in collectedJson)
                  ...json.decode(data) as Map<String, dynamic>,
              });
              completer.complete(results);
            } catch (ex) {
              completer.completeError(
                'Decoding JSON failed ($ex). JSON strings where: $collectedJson',
              );
            }
          } else {
            completer.completeError('flutter run exited without producing results');
          }
        }
      }
    } else {
      await deviceLogSub?.cancel();
    }
  });

  return completer.future;
}
