// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:meta/meta.dart';
import 'package:vm_service/vm_service.dart';
import 'package:vm_service/vm_service_io.dart';

import 'devices.dart';
import 'metrics_result_writer.dart';
import 'task_result.dart';
import 'utils.dart';

/// Run a list of tasks.
///
/// For each task, an auto rerun will be triggered when task fails.
///
/// If the task succeeds the first time, it will be recorded as successful.
///
/// If the task fails first, but gets passed in the end, the
/// test will be recorded as successful but with a flake flag.
///
/// If the task fails all reruns, it will be recorded as failed.
Future<void> runTasks(
  List<String> taskNames, {
  bool exitOnFirstTestFailure = false,
  // terminateStrayDartProcesses defaults to false so that tests don't have to specify it.
  // It is set based on the --terminate-stray-dart-processes command line argument in
  // normal execution, and that flag defaults to true.
  bool terminateStrayDartProcesses = false,
  bool silent = false,
  String? deviceId,
  String? gitBranch,
  String? localEngine,
  String? localEngineHost,
  String? localEngineSrcPath,
  String? localWebSdk,
  String? luciBuilder,
  String? resultsPath,
  List<String>? taskArgs,
  bool useEmulator = false,
  String buildMode = 'profile',
  @visibleForTesting Map<String, String>? isolateParams,
  @visibleForTesting void Function(String) print = print,
  @visibleForTesting List<String>? logs,
}) async {
  for (final taskName in taskNames) {
    var result = TaskResult.success(null);
    var failureCount = 0;
    while (failureCount <= MetricsResultWriter.retryNumber) {
      result = await rerunTask(
        taskName,
        deviceId: deviceId,
        localEngine: localEngine,
        localEngineHost: localEngineHost,
        localEngineSrcPath: localEngineSrcPath,
        localWebSdk: localWebSdk,
        terminateStrayDartProcesses: terminateStrayDartProcesses,
        silent: silent,
        taskArgs: taskArgs,
        resultsPath: resultsPath,
        gitBranch: gitBranch,
        luciBuilder: luciBuilder,
        isolateParams: isolateParams,
        useEmulator: useEmulator,
        buildMode: buildMode,
      );

      if (!result.succeeded) {
        failureCount += 1;
        if (exitOnFirstTestFailure) {
          break;
        }
      } else {
        section('Flaky status for "$taskName"');
        if (failureCount > 0) {
          print(
            'Total ${failureCount + 1} executions: $failureCount failures and 1 false positive.',
          );
          print('flaky: true');
          // TODO(ianh): stop ignoring this failure. We should set exitCode=1, and quit
          // if exitOnFirstTestFailure is true.
        } else {
          print('Test passed on first attempt.');
          print('flaky: false');
        }
        break;
      }
    }

    if (!result.succeeded) {
      section('Flaky status for "$taskName"');
      print('Consistently failed across all $failureCount executions.');
      print('flaky: false');
      exitCode = 1;
      if (exitOnFirstTestFailure) {
        return;
      }
    }
  }
}

/// A rerun wrapper for `runTask`.
///
/// This separates reruns in separate sections.
Future<TaskResult> rerunTask(
  String taskName, {
  String? deviceId,
  String? localEngine,
  String? localEngineHost,
  String? localEngineSrcPath,
  String? localWebSdk,
  bool terminateStrayDartProcesses = false,
  bool silent = false,
  List<String>? taskArgs,
  String? resultsPath,
  String? gitBranch,
  String? luciBuilder,
  bool useEmulator = false,
  String buildMode = 'profile',
  @visibleForTesting Map<String, String>? isolateParams,
}) async {
  section('Running task "$taskName"');
  final TaskResult result = await runTask(
    taskName,
    deviceId: deviceId,
    localEngine: localEngine,
    localEngineHost: localEngineHost,
    localEngineSrcPath: localEngineSrcPath,
    localWebSdk: localWebSdk,
    terminateStrayDartProcesses: terminateStrayDartProcesses,
    silent: silent,
    taskArgs: taskArgs,
    isolateParams: isolateParams,
    useEmulator: useEmulator,
    buildMode: buildMode,
  );

  print('Task result:');
  print(const JsonEncoder.withIndent('  ').convert(result));
  section('Finished task "$taskName"');

  if (resultsPath != null) {
    final cocoon = MetricsResultWriter();
    await cocoon.writeTaskResultToFile(
      builderName: luciBuilder,
      gitBranch: gitBranch,
      result: result,
      resultsPath: resultsPath,
    );
  }
  return result;
}

/// Runs a task in a separate Dart VM and collects the result.
///
/// Uses stdin/stdout IPC protocol when Dart 3.9+ does not support
/// --enable-vm-service. Falls back to VM service protocol when available.
///
/// [taskName] is the name of the task. The corresponding task executable is
/// expected to be found under `bin/tasks`.
///
/// Running the task in [silent] mode will suppress standard output from task
/// processes and only print standard errors.
///
/// [taskArgs] are passed to the task executable for additional configuration.
Future<TaskResult> runTask(
  String taskName, {
  bool terminateStrayDartProcesses = false,
  bool silent = false,
  String? localEngine,
  String? localEngineHost,
  String? localWebSdk,
  String? localEngineSrcPath,
  String? deviceId,
  List<String>? taskArgs,
  bool useEmulator = false,
  String buildMode = 'profile',
  @visibleForTesting Map<String, String>? isolateParams,
}) async {
  final taskExecutable = 'bin/tasks/$taskName.dart';

  if (!file(taskExecutable).existsSync()) {
    print('Executable Dart file not found: $taskExecutable');
    exit(1);
  }

  if (useEmulator) {
    taskArgs ??= <String>[];
    taskArgs
      ..add('--android-emulator')
      ..add('--browser-name=android-chrome');
  }

  stdout.writeln('Starting process for task: [$taskName]');

  final bool useIpc = _shouldUseIpc();
  final Process runner = await startProcess(
    dartBin,
    <String>[
      if (!useIpc) ...<String>['--enable-vm-service=0', '--no-pause-isolates-on-exit'],
      '-DbuildMode=$buildMode',
      if (localEngine != null) '-DlocalEngine=$localEngine',
      if (localEngineHost != null) '-DlocalEngineHost=$localEngineHost',
      if (localWebSdk != null) '-DlocalWebSdk=$localWebSdk',
      if (localEngineSrcPath != null) '-DlocalEngineSrcPath=$localEngineSrcPath',
      taskExecutable,
      ...?taskArgs,
    ],
    environment: <String, String>{
      DeviceIdEnvName: ?deviceId,
      if (useIpc) 'FLUTTER_DEVICELAB_IPC': '1',
    },
  );

  var runnerFinished = false;

  unawaited(
    runner.exitCode.whenComplete(() {
      runnerFinished = true;
    }),
  );

  final uri = Completer<Uri>();
  final ipcMessages = StreamController<Map<String, dynamic>>.broadcast();

  final StreamSubscription<String> stdoutSub = runner.stdout
      .transform<String>(const Utf8Decoder())
      .transform<String>(const LineSplitter())
      .listen((String line) {
        if (useIpc && line.startsWith(kIpcPrefix)) {
          final String jsonStr = line.substring(kIpcPrefix.length);
          try {
            final message = json.decode(jsonStr) as Map<String, dynamic>;
            ipcMessages.add(message);
          } catch (e) {
            stderr.writeln('Failed to parse IPC message: $jsonStr');
          }
          return;
        }
        if (!useIpc && !uri.isCompleted) {
          final Uri? serviceUri = parseServiceUri(
            line,
            prefix: RegExp('The Dart VM service is listening on '),
          );
          if (serviceUri != null) {
            uri.complete(serviceUri);
          }
        }
        if (!silent) {
          stdout.writeln('[${DateTime.now()}] [STDOUT] $line');
        }
      });

  final StreamSubscription<String> stderrSub = runner.stderr
      .transform<String>(const Utf8Decoder())
      .transform<String>(const LineSplitter())
      .listen((String line) {
        stderr.writeln('[${DateTime.now()}] [STDERR] $line');
      });

  try {
    final TaskResult taskResult;
    if (useIpc) {
      taskResult = await _runTaskViaIpc(
        taskName: taskName,
        runner: runner,
        ipcMessages: ipcMessages.stream,
        isolateParams: isolateParams,
        terminateStrayDartProcesses: terminateStrayDartProcesses,
      );
    } else {
      taskResult = await _runTaskViaVmService(
        taskName: taskName,
        vmServiceUri: await uri.future,
        isolateParams: isolateParams,
        terminateStrayDartProcesses: terminateStrayDartProcesses,
      );
    }
    final int exitCode = await runner.exitCode;
    print('[$taskName] Process terminated with exit code $exitCode.');
    return taskResult;
  } catch (error, stack) {
    print('[$taskName] Task runner system failed with exception!\n$error\n$stack');
    rethrow;
  } finally {
    if (!runnerFinished) {
      print('[$taskName] Terminating process...');
      // On Windows, Process.kill() without a signal argument performs a
      // forceful termination equivalent to SIGKILL, since Windows does not
      // support POSIX signals. On other platforms, use SIGKILL explicitly.
      if (Platform.isWindows) {
        runner.kill();
      } else {
        runner.kill(ProcessSignal.sigkill);
      }
    }
    await stdoutSub.cancel();
    await stderrSub.cancel();
    await ipcMessages.close();
  }
}

bool? _shouldUseIpcCache;

bool _shouldUseIpc() {
  return _shouldUseIpcCache ??= () {
    try {
      final String dartBinPath = dartBin;
      final ProcessResult result = Process.runSync(dartBinPath, <String>['run', '--help']);
      final helpText = result.stdout as String;
      return !helpText.contains('--enable-vm-service');
    } catch (_) {
      // If dart binary is unavailable or fails, fall back to VM service mode.
      return false;
    }
  }();
}

Future<TaskResult> _runTaskViaIpc({
  required String taskName,
  required Process runner,
  required Stream<Map<String, dynamic>> ipcMessages,
  required Map<String, String>? isolateParams,
  required bool terminateStrayDartProcesses,
}) async {
  await ipcMessages
      .firstWhere((Map<String, dynamic> m) => m['type'] == 'ready')
      .timeout(const Duration(seconds: 120));
  print('[$taskName] Received ready signal via IPC.');

  isolateParams = isolateParams == null
      ? <String, String>{}
      : Map<String, String>.of(isolateParams);
  isolateParams['runProcessCleanup'] = terminateStrayDartProcesses.toString();
  // IPC mode runs the task in a separate process; the child skips the
  // flutter config step (the flag is a no-op in this branch, and the parent
  // does not run it either).
  isolateParams['runFlutterConfig'] = 'false';

  final String runJson = json.encode(<String, dynamic>{'type': 'run', 'args': isolateParams});
  try {
    runner.stdin.writeln('$kIpcPrefix$runJson');
    await runner.stdin.flush();
  } on SocketException catch (e) {
    print('[$taskName] Failed to send IPC run message: $e (process may have exited)');
    rethrow;
  }

  // Align the result wait timeout with the child's task timeout plus a buffer
  // for result transmission and ack confirmation, so the parent does not time
  // out before the child's own task timeout expires.
  final resultTimeout = isolateParams.containsKey('timeoutInMinutes')
      ? Duration(minutes: int.parse(isolateParams['timeoutInMinutes']!) + 5)
      : const Duration(minutes: 60);
  // The child exits without sending a 'result' when run() throws a
  // non-timeout exception (for example a DeviceException from
  // device.home(), which framework.dart's run() does not catch). Race the
  // result wait against process exit so we fail fast instead of idling
  // until resultTimeout.
  final Map<String, dynamic> resultMessage =
      await Future.any<Map<String, dynamic>>(<Future<Map<String, dynamic>>>[
        ipcMessages
            .firstWhere((Map<String, dynamic> m) => m['type'] == 'result')
            .timeout(resultTimeout),
        runner.exitCode.then<Map<String, dynamic>>((int exitCode) {
          throw Exception(
            '[$taskName] Task runner exited before reporting a result (exit code $exitCode).',
          );
        }),
      ]);
  print('[$taskName] Received result via IPC.');

  final taskResult = TaskResult.fromJson(resultMessage['data'] as Map<String, dynamic>);

  final String ackJson = json.encode(<String, dynamic>{'type': 'ack'});
  try {
    runner.stdin.writeln('$kIpcPrefix$ackJson');
    await runner.stdin.flush();
  } on SocketException catch (e) {
    print('[$taskName] Failed to send IPC ack message: $e (process may have exited)');
    rethrow;
  }

  return taskResult;
}

Future<TaskResult> _runTaskViaVmService({
  required String taskName,
  required Uri vmServiceUri,
  required Map<String, String>? isolateParams,
  required bool terminateStrayDartProcesses,
}) async {
  final ConnectionResult result = await _connectToRunnerIsolate(vmServiceUri);
  print('[$taskName] Connected to VM server.');
  isolateParams = isolateParams == null
      ? <String, String>{}
      : Map<String, String>.of(isolateParams);
  isolateParams['runProcessCleanup'] = terminateStrayDartProcesses.toString();
  final VmService service = result.vmService;
  final String isolateId = result.isolate.id!;
  final Map<String, dynamic> taskResultJson = (await service.callServiceExtension(
    'ext.cocoonRunTask',
    args: isolateParams,
    isolateId: isolateId,
  )).json!;
  await _acknowledgeTaskResultReceived(service: service, isolateId: isolateId);
  final taskResult = TaskResult.fromJson(taskResultJson);
  return taskResult;
}

Future<ConnectionResult> _connectToRunnerIsolate(Uri vmServiceUri) async {
  final pathSegments = <String>[
    // Add authentication code.
    if (vmServiceUri.pathSegments.isNotEmpty) vmServiceUri.pathSegments[0],
    'ws',
  ];
  final url = vmServiceUri.replace(scheme: 'ws', pathSegments: pathSegments).toString();
  final stopwatch = Stopwatch()..start();

  while (true) {
    try {
      // Look up the isolate.
      final VmService client = await vmServiceConnectUri(url);
      VM vm = await client.getVM();
      while (vm.isolates!.isEmpty) {
        await Future<void>.delayed(const Duration(seconds: 1));
        vm = await client.getVM();
      }
      final IsolateRef isolate = vm.isolates!.first;
      // Sanity check to ensure we're talking with the main isolate.
      final Response response = await client.callServiceExtension(
        'ext.cocoonRunnerReady',
        isolateId: isolate.id,
      );
      if (response.json!['result'] != 'success') {
        throw 'not ready yet';
      }
      return ConnectionResult(client, isolate);
    } catch (error) {
      if (stopwatch.elapsed > const Duration(seconds: 10)) {
        print(
          'VM service still not ready. It is possible the target has failed.\n'
          'Latest connection error:\n'
          '  $error\n'
          'Continuing to retry...\n',
        );
        stopwatch.reset();
      }
      await Future<void>.delayed(const Duration(milliseconds: 50));
    }
  }
}

Future<void> _acknowledgeTaskResultReceived({
  required VmService service,
  required String isolateId,
}) async {
  try {
    await service.callServiceExtension('ext.cocoonTaskResultReceived', isolateId: isolateId);
  } on RPCError {
    // The target VM may shutdown before the response is received.
  }
}

class ConnectionResult {
  ConnectionResult(this.vmService, this.isolate);

  final VmService vmService;
  final IsolateRef isolate;
}
