// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:meta/meta.dart';
import 'package:process/process.dart';

import '../base/common.dart';
import '../base/io.dart';
import '../base/logger.dart';
import '../base/process.dart';
import '../device.dart';
import 'hdc_server.dart';
import 'ohos_device.dart';
import 'ohos_sdk.dart';
import 'ohos_workflow.dart';

class OhosDevices extends PollingDeviceDiscovery {
  OhosDevices({
    required OhosWorkflow ohosWorkflow,
    required ProcessManager processManager,
    required Logger logger,
    HarmonySdk? ohosSdk,
  }) : _ohosWorkflow = ohosWorkflow,
       _processUtils = ProcessUtils(logger: logger, processManager: processManager),
       _ohosSdk = ohosSdk,
       _processManager = processManager,
       _logger = logger,
       super('HarmonyOS devices');

  final OhosWorkflow _ohosWorkflow;
  final ProcessUtils _processUtils;
  final ProcessManager _processManager;
  final Logger _logger;
  final HarmonySdk? _ohosSdk;

  bool _doesNotHaveHdc() {
    return _ohosSdk == null ||
        _ohosSdk.hdcPath == null ||
        !_processManager.canRun(_ohosSdk.hdcPath);
  }

  @override
  Future<List<Device>> pollingGetDevices({
    Duration? timeout,
    bool forWirelessDiscovery = false,
  }) async {
    if (_doesNotHaveHdc()) {
      return <OhosDevice>[];
    }
    String text;

    final List<String> cmd = getHdcCommandCompat(_ohosSdk!, '', <String>['list', 'targets']);

    try {
      text = (await _processUtils.run(cmd, throwOnError: true)).stdout.trim();
      // _logger.printStatus('hdc list result:\n$text');
    } on ProcessException catch (exception) {
      throwToolExit(
        'Unable to run "hdc", check your Ohos SDK installation and '
        '$kOhosSdkRoot environment variable: ${exception.executable}',
      );
    }
    final devices = <OhosDevice>[];
    _parseHdcDeviceOutput(text, devices: devices);
    return devices;
  }

  @override
  bool get supportsPlatform => _ohosWorkflow.appliesToHostPlatform;

  @override
  bool get canListAnything => _ohosWorkflow.canListDevices;

  @override
  Future<List<String>> getDiagnostics() async {
    if (_doesNotHaveHdc()) {
      return const <String>[];
    }

    final RunResult result = await _processUtils.run(
      getHdcCommandCompat(_ohosSdk!, '', <String>['list', 'targets']),
    );
    if (result.exitCode != 0) {
      return <String>['`hdc list targets` failed: ${result.stderr.trim()}'];
    }
    final lines = <String>[];
    parseHdcDeviceOutput(result.stdout, diagnostics: lines);
    // Translate raw hdc status/error lines into actionable hints, mirroring
    // how AndroidDevices.getDiagnostics reports adb device states.
    return <String>[for (final String line in lines) _hdcLineHint(line)];
  }

  // Maps a raw non-device line from `hdc list targets` to a hint with
  // troubleshooting steps, in the spirit of the adb diagnostics.
  static String _hdcLineHint(String line) {
    final String lowerCaseLine = line.toLowerCase();
    if (line.contains('[Empty]')) {
      return 'No HarmonyOS devices detected (`hdc list targets` returned empty).\n'
          '1. Connect the phone over USB and enable Developer options > USB debugging.\n'
          '2. On first connection, accept the debugging authorization dialog on the device.\n'
          '3. For wireless debugging, run `hdc tconn <ip>:<port>`.\n'
          '4. If no device shows up, run `hdc kill && hdc start`, then `hdc list targets` again.';
    }
    if (lowerCaseLine.contains('connect failed')) {
      return 'Failed to connect to a HarmonyOS device: $line\n'
          'This usually means the debugging authorization was not granted. Revoke USB debugging '
          'authorization in Developer options, reconnect the device, and accept the authorization '
          'dialog; or run `hdc kill && hdc start` and retry.';
    }
    if (lowerCaseLine.contains('unauthorized')) {
      return 'A HarmonyOS device is not authorized: $line\n'
          'You might need to check your device for an authorization dialog.';
    }
    if (lowerCaseLine.contains('offline')) {
      return 'A HarmonyOS device is offline: $line\n'
          'Run `hdc kill && hdc start`, then reconnect the device.';
    }
    return line;
  }

  static final RegExp _kTargetLinePattern = RegExp(
    r'^(?:\d{1,3}(?:\.\d{1,3}){3}:\d+|[0-9A-Za-z:_-]{4,})$',
  );

  // hdc status words that would otherwise look like connect keys.
  static const Set<String> _kStatusLines = <String>{'offline', 'unauthorized', 'unknown'};

  /// Parses the output of `hdc list targets` into [deviceIds] (valid target
  /// lines) and [diagnostics] (error/status lines), never treating a
  /// non-device line as a device.
  @visibleForTesting
  static void parseHdcDeviceOutput(
    String text, {
    List<String>? deviceIds,
    List<String>? diagnostics,
  }) {
    for (final String rawLine in text.trim().split('\n')) {
      final String line = rawLine.trim();
      if (line.isEmpty) {
        continue;
      }
      final bool isTargetLine =
          _kTargetLinePattern.hasMatch(line) && !_kStatusLines.contains(line.toLowerCase());
      if (isTargetLine) {
        deviceIds?.add(line);
      } else {
        diagnostics?.add(line);
      }
    }
  }

  void _parseHdcDeviceOutput(String text, {List<OhosDevice>? devices, List<String>? diagnostics}) {
    final deviceIds = <String>[];
    parseHdcDeviceOutput(text, deviceIds: deviceIds, diagnostics: diagnostics);
    for (final deviceId in deviceIds) {
      devices?.add(
        OhosDevice(
          deviceId,
          deviceCodeName: deviceId,
          ohosSdk: _ohosSdk!,
          logger: _logger,
          processManager: _processManager,
        ),
      );
    }
  }

  @override
  List<String> get wellKnownIds => const <String>[];
}
