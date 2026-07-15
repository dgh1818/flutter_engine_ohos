// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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

  void _parseHdcDeviceOutput(String text, {List<OhosDevice>? devices, List<String>? diagnostics}) {
    // return empty if do not discovery any devices
    if (text.contains('[Empty]') || text.contains('connect failed')) {
      diagnostics?.add(text);
      return;
    }

    for (final String line in text.trim().split('\n')) {
      final String deviceId = line.trim();
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
