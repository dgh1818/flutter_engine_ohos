// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:meta/meta.dart';

import '../base/common.dart';
import '../base/utils.dart';
import '../globals.dart' as globals;
import 'ohos_sdk.dart';

const String kOhosSdkEmulatorPath = 'OHOS_EMULATOR_HOME';

/// The device type used by '--create-ohos-emulator' when '--devicetype' is
/// not specified.
const String kOhosDefaultDeviceType = 'phone';

/// Manages the emulator instances of an OHOS SDK installation.
///
/// The emulator executable shipped with the SDK, resolved from the
/// [kOhosSdkEmulatorPath] environment variable, is driven through its
/// command line interface to list, print, create, and launch emulator
/// instances for the ohos options of 'flutter emulators'.
class OhosEmulators {
  /// Lists the available emulators ('--list-ohos-emulator').
  Future<void> listEmulators() async {
    final List<OhosEmulatorInfo> emulators = await getEmulators();
    if (emulators.isEmpty) {
      globals.printStatus('No Ohos emulators available.');
      _printAdditionalInfo();
      return;
    }
    globals.printStatus(
      '${emulators.length} available Ohos ${pluralize('emulator', emulators.length)}:\n',
    );
    globals.printStatus('${'Name'.padRight(18)} • ${'Device Type'.padRight(11)} • OS Version');
    for (final emulator in emulators) {
      globals.printStatus(
        '${emulator.name.padRight(18)} • ${emulator.deviceType.padRight(11)} • ${emulator.osVersion}',
      );
    }
    globals.printStatus('');
    _printAdditionalInfo();
  }

  /// Prints detailed information about each emulator
  /// ('--print-ohos-emulator').
  Future<void> printEmulatorDetails() async {
    final List<OhosEmulatorInfo> emulators = await getEmulators();
    if (emulators.isEmpty) {
      globals.printStatus('No Ohos emulators available.');
      _printAdditionalInfo();
      return;
    }
    globals.printStatus('Ohos Emulator Details:\n');
    for (final emulator in emulators) {
      globals.printStatus('Name: ${emulator.name}');
      globals.printStatus('  Device Type: ${emulator.deviceType}');
      _printDetail('Product Model', emulator.productModel);
      globals.printStatus('  OS Version: ${emulator.osVersion}');
      globals.printStatus('  Status: ${emulator.status}');
      _printDetail('Instance Path', emulator.instancePath);
      _printDetail('Image Root', emulator.imageRoot);
      _printDetail('CPU Arch', emulator.hwCpuArch);
      _printDetail('RAM Size', emulator.hwRamSize, ' MB');
      globals.printStatus('');
    }
  }

  /// Launches the emulator instance with the given name
  /// ('--launch-ohos-emulator').
  Future<void> launchEmulator(String name, {@visibleForTesting Duration? startupDuration}) async {
    final String emulatorDirectory = _existingEmulatorDirectory();

    final String emulatorExecutable = _emulatorExecutable(emulatorDirectory);

    // Check if the emulator exists in the list
    final List<OhosEmulatorInfo> ohosEmulators = await getEmulators();
    final bool emulatorExists = ohosEmulators.any(
      (OhosEmulatorInfo emulator) => emulator.name == name,
    );
    if (!emulatorExists) {
      if (ohosEmulators.isNotEmpty) {
        globals.printStatus('Available Ohos emulators:');
        for (final emulator in ohosEmulators) {
          globals.printStatus('  - ${emulator.name}');
        }
      } else {
        globals.printStatus('No Ohos emulators available.');
      }
      throwToolExit('Ohos emulator "$name" not found.', exitCode: 1);
    }

    final cmd = <String>[emulatorExecutable, '-hvd', name];

    globals.printStatus('Launching Ohos emulator "$name"...');
    try {
      // Use start() instead of run() so we do not wait for the emulator process to exit.
      final Process process = await globals.processManager.start(
        cmd,
        workingDirectory: emulatorDirectory,
      );

      // Record output from the emulator process.
      final stdoutList = <String>[];
      final stderrList = <String>[];
      final StreamSubscription<String> stdoutSubscription = process.stdout
          .transform(utf8LineDecoder)
          .listen(stdoutList.add);
      final StreamSubscription<String> stderrSubscription = process.stderr
          .transform(utf8LineDecoder)
          .listen(stderrList.add);
      final Future<void> stdioFuture = Future.wait<void>(<Future<void>>[
        stdoutSubscription.asFuture<void>(),
        stderrSubscription.asFuture<void>(),
      ]);

      // The emulator continues running on success, so we don't wait for the
      // process to complete before continuing. However, if the process fails
      // after the startup phase (3 seconds), then we only echo its output if
      // its error code is non-zero and its stderr is non-empty.
      var earlyFailure = true;
      unawaited(
        process.exitCode.then((int exitCode) async {
          if (exitCode == 0) {
            globals.printTrace('The Ohos emulator exited successfully');
            return;
          }
          // Make sure the process' stdout and stderr are drained before
          // reporting the failure; the exit notification does not wait for
          // buffered output.
          await stdioFuture;
          unawaited(stdoutSubscription.cancel());
          unawaited(stderrSubscription.cancel());
          if (stdoutList.isNotEmpty) {
            globals.printTrace('Ohos emulator stdout:');
            stdoutList.forEach(globals.printTrace);
          }
          if (!earlyFailure && stderrList.isEmpty) {
            globals.printStatus('The Ohos emulator exited with code $exitCode');
            return;
          }
          final when = earlyFailure ? 'during startup' : 'after startup';
          globals.printError('The Ohos emulator exited with code $exitCode $when');
          globals.printError('Ohos emulator stderr:');
          stderrList.forEach(globals.printError);
          globals.printError('Address these issues and try again.');
        }),
      );

      // Wait a few seconds for the emulator to start so that an early failure
      // is reported before this command returns.
      await Future<void>.delayed(startupDuration ?? const Duration(seconds: 3));
      earlyFailure = false;
    } on Exception catch (error) {
      throwToolExit('Failed to launch Ohos emulator "$name": $error', exitCode: 1);
    }
  }

  /// Creates a new emulator instance ('--create-ohos-emulator').
  ///
  /// The emulator tool requires a device type and the OS version of a
  /// downloaded image, and silently creates nothing when either is missing or
  /// wrong, so the pair is validated against '-imageList' and defaulted here.
  Future<void> createEmulator({
    required String name,
    String? deviceType,
    String? osVersion,
  }) async {
    final String emulatorDirectory = _existingEmulatorDirectory();

    final String emulatorExecutable = _emulatorExecutable(emulatorDirectory);

    final String resolvedDeviceType = deviceType == null || deviceType.isEmpty
        ? kOhosDefaultDeviceType
        : deviceType;
    final String resolvedOsVersion = await _resolveOsVersion(
      emulatorExecutable,
      emulatorDirectory,
      resolvedDeviceType,
      osVersion,
    );

    final cmd = <String>[
      emulatorExecutable,
      '-create',
      name,
      '-deviceType',
      resolvedDeviceType,
      '-osVersion',
      resolvedOsVersion,
    ];

    final ProcessResult result = await _runEmulatorTool(cmd, emulatorDirectory);
    final stderr = result.stderr.toString();
    final stdout = result.stdout.toString();
    if (result.exitCode != 0) {
      if (stderr.isNotEmpty) {
        globals.printError(stderr.trim());
      }
      if (stdout.isNotEmpty) {
        globals.printStatus(stdout.trim());
      }
      throwToolExit('Failed to create Ohos emulator "$name".', exitCode: 1);
    }
    globals.printStatus('Created Ohos emulator "$name".');
  }

  /// Returns the OS version to pass to '-create' for [deviceType], as
  /// validated against '-imageList'.
  ///
  /// The emulator tool needs the exact version of a downloaded image and
  /// silently creates nothing otherwise, so an explicitly passed [osVersion]
  /// is rejected unless its image is downloaded, and a missing one is
  /// detected from the downloaded images.
  Future<String> _resolveOsVersion(
    String emulatorExecutable,
    String emulatorDirectory,
    String deviceType,
    String? osVersion,
  ) async {
    final ProcessResult result = await _runEmulatorTool(
      <String>[emulatorExecutable, '-imageList', '-deviceType', deviceType],
      emulatorDirectory,
    );
    if (result.exitCode != 0) {
      final String error = result.stderr.toString().trim();
      throwToolExit(
        error.isNotEmpty ? error : 'Ohos emulator tool failed with exit code ${result.exitCode}.',
        exitCode: 1,
      );
    }

    var parseFailed = false;
    final availableVersions = <String>{};
    final downloadedVersions = <String>{};
    try {
      final Object? decoded = json.decode(result.stdout.toString());
      if (decoded is List<dynamic>) {
        for (final Object? item in decoded) {
          if (item is! Map<String, dynamic>) {
            continue;
          }
          final String? version = _stringField(item, 'osVersion');
          if (version == null || version.isEmpty) {
            continue;
          }
          availableVersions.add(version);
          // The tool reports "downloaded" as a "true"/"false" string; a JSON
          // bool is accepted as well in case that ever changes.
          final Object? downloaded = item['downloaded'];
          if (downloaded == 'true' || downloaded == true) {
            downloadedVersions.add(version);
          }
        }
      } else {
        parseFailed = true;
      }
    } on FormatException {
      parseFailed = true;
    }
    if (parseFailed) {
      // Distinguish an output that cannot be parsed from a missing image,
      // which the errors below report.
      throwToolExit(
        'Failed to parse the "-imageList" output for device type "$deviceType".',
        exitCode: 1,
      );
    }

    if (osVersion != null && osVersion.isNotEmpty) {
      if (downloadedVersions.contains(osVersion)) {
        return osVersion;
      }
      final downloadedHint = downloadedVersions.isEmpty
          ? ''
          : ' Downloaded for "$deviceType": ${downloadedVersions.join(', ')}.';
      throwToolExit(
        'Ohos image "$osVersion" is not downloaded for device type "$deviceType".'
        '$downloadedHint Download an image in DevEco Studio, or pass one of the '
        'downloaded versions as --osversion.',
        exitCode: 1,
      );
    }

    if (downloadedVersions.isEmpty) {
      final imageHint = availableVersions.isEmpty
          ? ''
          : ' Available for "$deviceType": ${availableVersions.join(', ')}.';
      throwToolExit(
        'No downloaded Ohos image found for device type "$deviceType".$imageHint '
        'Download an image in DevEco Studio, or pass --osversion explicitly.',
        exitCode: 1,
      );
    }

    final String downloadedVersion = downloadedVersions.first;
    globals.printStatus(
      'No --osversion specified; using "$downloadedVersion" for device type "$deviceType".',
    );
    return downloadedVersion;
  }

  /// Returns the emulator instances reported by the emulator executable.
  ///
  /// Exits with an actionable message when [kOhosSdkEmulatorPath] is not
  /// set, does not point at an installation with the emulator executable,
  /// or the emulator tool itself fails.
  Future<List<OhosEmulatorInfo>> getEmulators() async {
    final String emulatorDirectory = _existingEmulatorDirectory();

    final String emulatorExecutable = _emulatorExecutable(emulatorDirectory);

    final cmd = <String>[emulatorExecutable, '-list', '-details'];

    final ProcessResult result = await _runEmulatorTool(cmd, emulatorDirectory);
    if (result.exitCode != 0) {
      final String error = result.stderr.toString().trim();
      throwToolExit(
        error.isNotEmpty ? error : 'Ohos emulator tool failed with exit code ${result.exitCode}.',
        exitCode: 1,
      );
    }

    // Parse JSON output to extract emulator information
    final emulators = <OhosEmulatorInfo>[];
    final output = result.stdout.toString();
    try {
      final jsonList = json.decode(output) as List<dynamic>;
      for (final item in jsonList) {
        if (item is String) {
          // A plain JSON array of names, as accepted by the plain-list
          // fallback below.
          emulators.add(_plainEmulatorInfo(item));
          continue;
        }
        final map = item as Map<String, dynamic>;
        emulators.add(
          OhosEmulatorInfo(
            name: _stringField(map, 'name') ?? '',
            deviceType: _stringField(map, 'deviceType') ?? 'phone',
            osVersion: _stringField(map, 'os.osVersion') ?? '',
            status: _stringField(map, 'isRunning') ?? 'false',
            productModel: _stringField(map, 'productModel'),
            instancePath: _stringField(map, 'instancePath'),
            imageRoot: _stringField(map, 'imageRoot'),
            hwCpuArch: _stringField(map, 'hw.cpu.arch'),
            hwRamSize: _stringField(map, 'hw.ramSize'),
          ),
        );
      }
    } on FormatException {
      _parseEmulatorsPlainList(output, emulators);
    } on TypeError {
      // The output is valid JSON but not the expected structure (for example
      // a top-level object instead of a list); parsing it as plain names
      // would print raw JSON text, so degrade to an empty list instead.
      emulators.clear();
    }

    return emulators;
  }

  /// Returns the emulator home directory, from [kOhosSdkEmulatorPath] when
  /// set, or from the SDK-based discovery otherwise, exiting with an
  /// actionable message when neither points at an installation with the
  /// emulator executable.
  String _existingEmulatorDirectory() {
    // The environment variable remains the explicit override, validated
    // strictly so a misconfigured value reports its own path.
    final String? emulatorDirectory = globals.platform.environment[kOhosSdkEmulatorPath];
    if (emulatorDirectory != null) {
      return _validatedEmulatorDirectory(emulatorDirectory);
    }

    // Collect SDK anchors without validating them as a build toolchain SDK:
    // HmosSdk.localHmosSdk() rejects command-line-tools layouts (their
    // components live under 'sdk/default/openharmony' instead of directly
    // under 'sdk'), but the emulator only needs the location as an anchor.
    final List<String> sdkAnchors = <String>[
      if (globals.platform.environment.containsKey(kDevecoSdk))
        globals.platform.environment[kDevecoSdk]!,
      if (globals.platform.environment.containsKey(kHmosHome))
        globals.platform.environment[kHmosHome]!,
      if (globals.config.containsKey('ohos-sdk'))
        globals.config.getValue('ohos-sdk') as String? ?? '',
    ].where((dir) => dir.isNotEmpty).toList();

    // Two installation layouts place the emulator executable next to the
    // 'sdk' directory, so each candidate directory next to an anchor is
    // tried in turn:
    // - DevEco Studio: '<DevEcoStudio>/tools/emulator', next to
    //   '<DevEcoStudio>/sdk'.
    // - Command-line tools: '<TOOL_HOME>/emulator', next to '<TOOL_HOME>/sdk'
    //   (there is no 'tools' directory, so the SDK parent itself is the home).
    final triedPaths = <String>[];
    for (final anchor in sdkAnchors) {
      for (final home in <String>[
        globals.fs.path.normalize(globals.fs.path.join(anchor, '..', 'tools')),
        globals.fs.path.normalize(globals.fs.path.join(anchor, '..')),
      ]) {
        if (triedPaths.contains(home)) {
          continue;
        }
        triedPaths.add(home);
        if (_hasEmulatorExecutable(home)) {
          return home;
        }
      }
    }
    if (triedPaths.isNotEmpty) {
      throwToolExit(
        'Ohos emulator executable not found. Tried: ${triedPaths.join(', ')}. '
        'Set $kOhosSdkEmulatorPath to the directory containing the emulator, '
        'or install the emulator with DevEco Studio or the command-line tools.',
        exitCode: 1,
      );
    }

    throwToolExit(
      'No Ohos emulator found: neither $kOhosSdkEmulatorPath nor a discoverable '
      'HarmonyOS SDK points at the emulator. Set $kOhosSdkEmulatorPath to the '
      'directory containing the emulator, or install DevEco Studio.',
      exitCode: 1,
    );
  }

  /// Validates that [emulatorDirectory] exists and contains the emulator
  /// executable, exiting with an actionable message otherwise.
  String _validatedEmulatorDirectory(String emulatorDirectory) {
    if (!globals.fs.directory(emulatorDirectory).existsSync()) {
      throwToolExit(
        'Ohos emulator directory not found: "$emulatorDirectory" (check OHOS_EMULATOR_HOME).',
        exitCode: 1,
      );
    }

    final String emulatorExecutable = _emulatorExecutable(emulatorDirectory);
    if (!globals.fs.file(emulatorExecutable).existsSync()) {
      throwToolExit('Ohos emulator executable not found: "$emulatorExecutable".', exitCode: 1);
    }

    return emulatorDirectory;
  }

  /// Returns whether [emulatorDirectory] exists and contains the emulator
  /// executable.
  bool _hasEmulatorExecutable(String emulatorDirectory) {
    return globals.fs.directory(emulatorDirectory).existsSync() &&
        globals.fs.file(_emulatorExecutable(emulatorDirectory)).existsSync();
  }

  /// Runs the emulator executable, exiting with an actionable message when
  /// the tool cannot be started at all; failing to start does not surface as
  /// a non-zero exit code.
  Future<ProcessResult> _runEmulatorTool(List<String> cmd, String emulatorDirectory) async {
    try {
      return await globals.processManager.run(cmd, workingDirectory: emulatorDirectory);
    } on Exception catch (error) {
      throwToolExit('Failed to run the Ohos emulator tool: $error', exitCode: 1);
    }
  }

  String _emulatorExecutable(String emulatorDirectory) => globals.platform.isWindows
      ? globals.fs.path.join(emulatorDirectory, 'emulator', 'Emulator.exe')
      : globals.fs.path.join(emulatorDirectory, 'emulator', 'Emulator');

  /// Returns the string value of [key], or null when it is absent or has an
  /// unexpected type (for example a bool for "isRunning").
  static String? _stringField(Map<String, dynamic> map, String key) {
    final Object? value = map[key];
    return value is String ? value : null;
  }

  /// Falls back to plain line parsing when the tool output is not valid JSON.
  void _parseEmulatorsPlainList(String output, List<OhosEmulatorInfo> emulators) {
    emulators.clear();
    final List<String> lines = output.split('\n');
    for (final line in lines) {
      final String trimmed = line.trim();
      // The tool prints '[Empty]' when no instance exists (the RNOH CLI
      // treats the marker the same way); it must not become an instance.
      if (trimmed.isEmpty || trimmed == '[Empty]' || trimmed.startsWith('-')) {
        continue;
      }
      emulators.add(_plainEmulatorInfo(trimmed));
    }
  }

  /// Returns the entry used when only the name is known: plain-list output
  /// and JSON string arrays carry no details.
  OhosEmulatorInfo _plainEmulatorInfo(String name) {
    return OhosEmulatorInfo(
      name: name,
      deviceType: 'phone',
      osVersion: '',
      status: 'available',
    );
  }

  /// Prints '  `<label>`: `<value><suffix>`' when [value] is non-empty.
  void _printDetail(String label, String? value, [String suffix = '']) {
    if (value != null && value.isNotEmpty) {
      globals.printStatus('  $label: $value$suffix');
    }
  }

  void _printAdditionalInfo() {
    globals.printStatus('');
    globals.printStatus('Ohos emulator commands:');
    globals.printStatus(
      '  flutter emulators --create-ohos-emulator <name> [--devicetype <type>] [--osversion <version>]',
    );
    globals.printStatus('  flutter emulators --list-ohos-emulator');
    globals.printStatus('  flutter emulators --print-ohos-emulator');
    globals.printStatus('  flutter emulators --launch-ohos-emulator <name>');
    globals.printStatus('');
  }
}

/// Information about an Ohos emulator.
class OhosEmulatorInfo {
  OhosEmulatorInfo({
    required this.name,
    required this.deviceType,
    required this.osVersion,
    required this.status,
    this.productModel,
    this.instancePath,
    this.imageRoot,
    this.hwCpuArch,
    this.hwRamSize,
  });

  final String name;
  final String deviceType;
  final String osVersion;
  final String status;
  final String? productModel;
  final String? instancePath;
  final String? imageRoot;
  final String? hwCpuArch;
  final String? hwRamSize;
}
