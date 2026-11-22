// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:math';

import 'package:meta/meta.dart';
import 'package:process/process.dart';

import '../application_package.dart';
import '../base/common.dart';
import '../base/file_system.dart';
import '../base/io.dart';
import '../base/logger.dart';
import '../base/process.dart';
import '../build_info.dart';
import '../convert.dart';
import '../device.dart';
import '../device_port_forwarder.dart';
import '../globals.dart' as globals;
import '../project.dart';
import '../protocol_discovery.dart';
import '../vmservice.dart';
import 'application_package.dart';
import 'hdc_server.dart';
import 'ohos_builder.dart';
import 'ohos_sdk.dart';

class OhosDevice extends Device {
  OhosDevice(
    super.id, {
    this.deviceCodeName,
    required super.logger,
    required ProcessManager processManager,
    required HarmonySdk ohosSdk,
  }) : _logger = logger,
       _processManager = processManager,
       _ohosSdk = ohosSdk,
       _processUtils = ProcessUtils(logger: logger, processManager: processManager),
       super(category: Category.mobile, platformType: PlatformType.ohos, ephemeral: true);

  final Logger _logger;
  final ProcessManager _processManager;
  final HarmonySdk _ohosSdk;
  final ProcessUtils _processUtils;
  final String? deviceCodeName;

  @override
  void clearLogs() {
    // clear hilog history
    _processUtils.runSync(hdcCommandForDevice(<String>['shell', 'hilog', '-r']));
  }

  @override
  Future<void> dispose() async {
    _logReader?._stop();
    _pastLogReader?._stop();
  }

  @override
  Future<String?> get emulatorId async => id;

  HdcLogReader? _logReader;
  HdcLogReader? _pastLogReader;

  /// The bundle name of the most recently launched application, used by
  /// [queryMemoryInfo] to resolve the pid of the app DevTools is attached to.
  String? _lastLaunchedPackage;

  /// The device marketing name (`const.product.name`, falling back to
  /// `const.product.model`), cached when the properties are loaded; null
  /// when the properties could not be read, so `name` falls back to the
  /// serial number.
  String? _cachedMarketingName;

  @override
  FutureOr<DeviceLogReader> getLogReader({
    ApplicationPackage? app,
    bool includePastLogs = false,
  }) async {
    if (includePastLogs) {
      return _pastLogReader ??= await HdcLogReader.createLogReader(
        this,
        _processManager,
        includePastLogs: true,
      );
    } else {
      return _logReader ??= await HdcLogReader.createLogReader(this, _processManager);
    }
  }

  @override
  bool get supportsScreenshot => true;

  @override
  Future<void> takeScreenshot(File outputFile) async {
    const remotePath = '/data/local/tmp/flutter_screenshot.jpeg';
    await runHdcCheckedAsync(<String>['shell', 'snapshot_display', '-f', remotePath]);
    await _processUtils.run(
      hdcCommandForDevice(<String>['file recv', remotePath, outputFile.path]),
      throwOnError: true,
    );
    await runHdcCheckedAsync(<String>['shell', 'rm', remotePath]);
  }

  @override
  bool get supportsFlavors => true;

  Future<bool> _installApp(covariant ApplicationPackage app, {String? userIdentifier}) async {
    if (app is! OhosHap) {
      throwToolExit('this project or file is not contain(a) Hap file');
    }
    final OhosHap hap = app;
    final File file = globals.fs.file(hap.applicationPackage.path);
    if (!file.existsSync()) {
      throwToolExit('Failed to get the hap file: ${file.path}');
    }

    _logger.printStatus('installing hap. bundleName: ${app.id} ');
    const targetPath = 'data/local/tmp/flutterInstallTemp';
    final List<List<String>> hspCmds = app.ohosBuildData.hspModules
        .map(
          (OhosModule module) => OhosProject.getSignedFile(
            modulePath: module.srcPath,
            moduleName: module.name,
            flavor: module.flavor,
            type: OhosFileType.hsp,
          ),
        )
        .map((File file) => <String>['file', 'send', file.path, targetPath])
        .toList();
    final List<List<String>> cmds = <List<String>>[
      <String>['shell', 'rm', '-rf', targetPath],
      <String>['shell', 'mkdir', targetPath],
      ...hspCmds,
      <String>['file', 'send', hap.applicationPackage.path, targetPath],
      <String>['shell', 'bm', 'install', '-p', targetPath],
      <String>['shell', 'rm', '-rf', targetPath],
    ].map((List<String> cmd) => hdcCommandForDevice(cmd)).toList();

    RunResult? result;
    for (final cmd in cmds) {
      result = _processUtils.runSync(cmd, throwOnError: true);
      if (result.exitCode != 0 || result.stdout.contains('error')) {
        _logger.printError(
          '_installApp: cmd=$cmd\n  code=${result.exitCode}, stdout=${result.stdout}, stderr=${result.stderr}',
        );
        return false;
      }
    }
    return result != null && result.exitCode == 0;
  }

  @override
  Future<bool> isAppInstalled(covariant ApplicationPackage app, {String? userIdentifier}) async {
    final String bundleName = app.id;
    final List<String> propCommand = hdcCommandForDevice(<String>[
      'shell',
      '"bm dump -n $bundleName"',
    ]);
    _logger.printTrace(propCommand.join(' '));
    final RunResult result = _processUtils.runSync(propCommand);
    if (result.exitCode == 0) {
      final String cmdResult = result.stdout;
      if (cmdResult.contains(bundleName)) {
        return true;
      } else if (cmdResult.contains('error: failed to get information')) {
        return false;
      } else {
        throwToolExit('unknown result for bm dump.');
      }
    }
    return false;
  }

  @override
  Future<bool> isLatestBuildInstalled(covariant ApplicationPackage app) async {
    return false;
  }

  @override
  late final Future<bool> isLocalEmulator = () async {
    return false;
  }();

  @override
  Future<bool> isSupported() async {
    return true;
  }

  @override
  bool isSupportedForProject(FlutterProject flutterProject) {
    return flutterProject.ohos.existsSync();
  }

  @override
  String get name => _cachedMarketingName ?? deviceCodeName ?? 'unknown';

  @override
  late final DevicePortForwarder? portForwarder = () {
    final String? hdcPath = _ohosSdk.hdcPath;
    if (hdcPath == null) {
      return null;
    }
    return OhosDevicePortForwarder(
      processManager: _processManager,
      logger: _logger,
      deviceId: id,
      ohosDevice: this,
    );
  }();

  @override
  Future<String> get sdkNameAndVersion async => '${await _sdkVersion} (API ${await apiVersion})';

  /// `const.ohos.fullname` is e.g. `OpenHarmony-6.0.2.130`; reported
  /// verbatim (like Android reports `ro.build.version.release`).
  Future<String?> get _sdkVersion => _getProperty('const.ohos.fullname');

  Future<String?> get apiVersion => _getProperty('const.ohos.apiversion');

  @override
  Future<Map<String, Object>> toJson() async {
    // Warm the properties cache before `name` (a synchronous getter) is
    // read by super.toJson, so machine-readable output carries the
    // marketing name.
    await _properties;
    return super.toJson();
  }

  @override
  Future<LaunchResult> startApp(
    covariant ApplicationPackage? package, {
    String? mainPath,
    String? route,
    required DebuggingOptions debuggingOptions,
    Map<String, Object?> platformArgs = const <String, Object>{},
    bool prebuiltApplication = false,
    bool ipv6 = false,
    String? userIdentifier,
  }) async {
    ///
    /// 1. check build status
    /// 2. build or not
    /// 3. install or not
    /// 4. start app command
    ///

    final TargetPlatform devicePlatform = await targetPlatform;
    var builtPackage = package as OhosHap?;
    OhosArch ohosArch;
    switch (devicePlatform) {
      case TargetPlatform.ohos_arm64:
        ohosArch = OhosArch.arm64_v8a;
      case TargetPlatform.ohos_arm:
        ohosArch = OhosArch.armeabi_v7a;
      case TargetPlatform.ohos_x64:
        ohosArch = OhosArch.x86_64;
      case TargetPlatform.ohos:
      case TargetPlatform.android:
      case TargetPlatform.darwin:
      case TargetPlatform.fuchsia_arm64:
      case TargetPlatform.fuchsia_x64:
      case TargetPlatform.ios:
      case TargetPlatform.linux_arm64:
      case TargetPlatform.linux_riscv64:
      case TargetPlatform.linux_x64:
      case TargetPlatform.tester:
      case TargetPlatform.web_javascript:
      case TargetPlatform.windows_x64:
      case TargetPlatform.android_arm:
      case TargetPlatform.android_arm64:
      case TargetPlatform.android_x64:
      case TargetPlatform.windows_arm64:
      case TargetPlatform.unsupported:
        _logger.printError('Ohos platforms are only supported.');
        return LaunchResult.failed();
    }

    if (!prebuiltApplication) {
      _logger.printTrace('Building Hap');
      final FlutterProject project = FlutterProject.current();
      await ohosBuilder?.buildHap(
        project: project,
        ohosBuildInfo: OhosBuildInfo(
          debuggingOptions.buildInfo,
          targetArchs: <OhosArch>[ohosArch],
          enableImpellerFlag: debuggingOptions.enableImpeller.asBool,
          // enableHcpp is a non-nullable bool defaulting to false; map false
          // to null so a plain `flutter run` without --enable-hcpp leaves any
          // manually configured enable_ohos_hybrid_composition value in
          // buildinfo.json5 untouched instead of rewriting it to 'false'.
          enableHcppFlag: debuggingOptions.enableHcpp ? true : null,
          shouldCodesign: true,
        ),
        target: mainPath ?? 'lib/main.dart',
      );

      builtPackage =
          await ApplicationPackageFactory.instance!.getPackageForPlatform(
                devicePlatform,
                buildInfo: debuggingOptions.buildInfo,
              )
              as OhosHap?;
    }
    // There was a failure parsing the android project information.
    if (builtPackage == null) {
      throwToolExit('Problem building Ohos application: see above error(s).');
    }

    _logger.printTrace("Stopping app '${builtPackage.name}' on $name.");
    await stopApp(builtPackage, userIdentifier: userIdentifier);

    if (!await installApp(builtPackage, userIdentifier: userIdentifier)) {
      return LaunchResult.failed();
    }

    ProtocolDiscovery? observatoryDiscovery;

    if (debuggingOptions.debuggingEnabled) {
      // Clear hilog buffer before starting the app to ensure ProtocolDiscovery
      // picks up the NEW VM Service URI, not a stale one from a previous run.
      clearLogs();

      observatoryDiscovery = ProtocolDiscovery.vmService(
        // Avoid using getLogReader, which returns a singleton instance, because the
        // observatory discovery will dipose at the end. creating a new logger here allows
        // logs to be surfaced normally during `flutter drive`.
        await HdcLogReader.createLogReader(this, _processManager),
        portForwarder: portForwarder,
        hostPort: debuggingOptions.hostVmServicePort,
        devicePort: debuggingOptions.deviceVmServicePort,
        ipv6: ipv6,
        logger: _logger,
      );
    }
    final bool traceStartup = platformArgs['trace-startup'] as bool? ?? false;
    final String? traceSkiaAllowlist = debuggingOptions.traceSkiaAllowlist;
    // Dart VM flags start with `-` (e.g. `--random-seed=99`), but `aa start`
    // rejects `--ps` values beginning with `-` as unknown options. Prefix a
    // space so the value passes through; the embedding trims it back off in
    // FlutterShellArgs.fromWant when rebuilding `--dart-flags=`.
    final String dartFlags = debuggingOptions.dartFlags.startsWith('-')
        ? ' ${debuggingOptions.dartFlags}'
        : debuggingOptions.dartFlags;
    final cmd = <String>[
      'shell',
      'aa',
      'start',
      '-a',
      builtPackage.ohosBuildData.moduleInfo.mainElement!,
      '-b',
      builtPackage.ohosBuildData.appInfo!.bundleName,
      // Forward engine switches to the ability as Want parameters
      // (`--pb` boolean / `--ps` string), mirroring Android's
      // `am start --ez/--es` mechanism. Keys match what
      // FlutterShellArgs.ets parses on the embedding side.
      if (debuggingOptions.enableDartProfiling) ...<String>[
        '--pb',
        'enable-dart-profiling',
        'true',
      ],
      if (debuggingOptions.profileStartup) ...<String>['--pb', 'profile-startup', 'true'],
      if (traceStartup) ...<String>['--pb', 'trace-startup', 'true'],
      // Pass the initial route to the OHOS ability via Want parameters.
      // FlutterAbility.getInitialRoute() reads the "route" key from launchWant.parameters.
      if (route != null) ...<String>['--ps', 'route', route],
      if (debuggingOptions.enableSoftwareRendering) ...<String>[
        '--pb',
        'enable-software-rendering',
        'true',
      ],
      if (debuggingOptions.skiaDeterministicRendering) ...<String>[
        '--pb',
        'skia-deterministic-rendering',
        'true',
      ],
      if (debuggingOptions.traceSkia) ...<String>['--pb', 'trace-skia', 'true'],
      if (traceSkiaAllowlist != null) ...<String>[
        '--ps',
        'trace-skia-allowlist',
        traceSkiaAllowlist,
      ],
      if (debuggingOptions.traceSystrace) ...<String>['--pb', 'trace-systrace', 'true'],
      if (debuggingOptions.endlessTraceBuffer) ...<String>['--pb', 'endless-trace-buffer', 'true'],
      if (debuggingOptions.purgePersistentCache) ...<String>[
        '--pb',
        'purge-persistent-cache',
        'true',
      ],
      if (debuggingOptions.enableImpeller == ImpellerStatus.enabled) ...<String>[
        '--pb',
        'enable-impeller',
        'true',
      ],
      if (debuggingOptions.enableImpeller == ImpellerStatus.disabled) ...<String>[
        '--pb',
        'enable-impeller',
        'false',
      ],
      if (debuggingOptions.debuggingEnabled) ...<String>[
        if (debuggingOptions.startPaused) ...<String>['--pb', 'start-paused', 'true'],
        if (debuggingOptions.disableServiceAuthCodes) ...<String>[
          '--pb',
          'disable-service-auth-codes',
          'true',
        ],
        if (debuggingOptions.disableServiceOriginCheck) ...<String>[
          '--pb',
          'disable-service-origin-check',
          'true',
        ],
        if (debuggingOptions.dartFlags.isNotEmpty) ...<String>['--ps', 'dart-flags', dartFlags],
        if (debuggingOptions.useTestFonts) ...<String>['--pb', 'use-test-fonts', 'true'],
        if (debuggingOptions.verboseSystemLogs) ...<String>['--pb', 'verbose-logging', 'true'],
      ],
    ];
    final String result = (await runHdcCheckedAsync(cmd)).stdout;
    // This invocation returns 0 even when it fails.
    if (result.toLowerCase().contains('error')) {
      _logger.printError(result.trim(), wrap: false);
      return LaunchResult.failed();
    }

    _lastLaunchedPackage = builtPackage.ohosBuildData.appInfo!.bundleName;

    if (!debuggingOptions.debuggingEnabled) {
      return LaunchResult.succeeded();
    }

    // Wait for the service protocol port here. This will complete once the
    // device has printed "Observatory is listening on...".
    _logger.printTrace('Waiting for observatory port to be available...');
    try {
      Uri? observatoryUri;
      if (debuggingOptions.buildInfo.isDebug || debuggingOptions.buildInfo.isProfile) {
        observatoryUri = await observatoryDiscovery?.uri;
        _logger.printWarning('waiting for a debug connection: $observatoryUri');
        if (observatoryUri == null) {
          _logger.printError(
            'Error waiting for a debug connection: '
            'The log reader stopped unexpectedly',
          );
          return LaunchResult.failed();
        }
      }

      if (debuggingOptions.buildInfo.isDebug) {
        try {
          final attachCmd = <String>[
            'shell',
            'aa',
            'attach',
            '-b',
            builtPackage.ohosBuildData.appInfo!.bundleName,
          ];
          await runHdcCheckedAsync(attachCmd);
          _logger.printStatus(
            'Execute attach command for bundle: ${builtPackage.ohosBuildData.appInfo!.bundleName}',
          );
        } on Exception catch (e) {
          _logger.printWarning('Failed to execute attach command: $e');
        }
      }

      return LaunchResult.succeeded(vmServiceUri: observatoryUri);
    } on Exception catch (error) {
      _logger.printError('Error waiting for a debug connection: $error');
      return LaunchResult.failed();
    } finally {
      await observatoryDiscovery?.cancel();
    }
  }

  @override
  Future<bool> installApp(covariant ApplicationPackage app, {String? userIdentifier}) async {
    final bool wasInstalled = await isAppInstalled(app, userIdentifier: userIdentifier);
    _logger.printTrace('Installing Hap.');
    if (await _installApp(app, userIdentifier: userIdentifier)) {
      return true;
    }
    _logger.printTrace('Warning: Failed to install Hap.');
    if (!wasInstalled) {
      return false;
    }
    _logger.printStatus('Uninstalling old version...');
    if (!await uninstallApp(app, userIdentifier: userIdentifier)) {
      _logger.printError('Error: Uninstalling old version failed.');
      return false;
    }
    if (!await _installApp(app, userIdentifier: userIdentifier)) {
      _logger.printError('Error: Failed to install Hap again.');
      return false;
    }
    return true;
  }

  @override
  Future<bool> stopApp(covariant ApplicationPackage? app, {String? userIdentifier}) async {
    if (app == null) {
      return false;
    }

    await runHdcCheckedAsync(<String>['shell', 'aa', 'detach', '-b', app.id]);

    final RunResult result = _processUtils.runSync(
      hdcCommandForDevice(<String>['shell', 'aa', 'force-stop', app.id]),
    );
    return result.exitCode == 0;
  }

  @override
  Future<MemoryInfo> queryMemoryInfo() async {
    final String? packageName = _lastLaunchedPackage;
    if (packageName == null) {
      _logger.printTrace('No OHOS package was launched, skipping hidumper meminfo.');
      return const MemoryInfo.empty();
    }
    // `pidof` prints an empty stdout once the app has exited, so treat that
    // as "no memory info" instead of running hidumper against a stale pid.
    final RunResult pidResult = await _processUtils.run(
      hdcCommandForDevice(<String>['shell', 'pidof', packageName]),
    );
    final String pid = pidResult.stdout.trim().split(RegExp(r'\s+')).first;
    if (pidResult.exitCode != 0 || pid.isEmpty) {
      return const MemoryInfo.empty();
    }
    final RunResult memResult = await _processUtils.run(
      hdcCommandForDevice(<String>['shell', 'hidumper', '--mem', pid]),
    );
    if (memResult.exitCode != 0) {
      return const MemoryInfo.empty();
    }
    return parseHidumperMemDump(memResult.stdout) ?? const MemoryInfo.empty();
  }

  @override
  late final Future<TargetPlatform> targetPlatform = () async {
    // const.product.cpu.abilist = arm64-v8a
    final String? abilist = await _getProperty('const.product.cpu.abilist');
    if (abilist == null) {
      return TargetPlatform.ohos_arm64;
    } else if (abilist.contains('arm64-v8a')) {
      return TargetPlatform.ohos_arm64;
    } else if (abilist.contains('x64')) {
      return TargetPlatform.ohos_x64;
    } else if (abilist.contains('x86_64')) {
      return TargetPlatform.ohos_x64;
    } else {
      return TargetPlatform.ohos_arm64;
    }
  }();

  @override
  Future<bool> uninstallApp(covariant ApplicationPackage app, {String? userIdentifier}) async {
    final List<String> uninstallCommand = hdcCommandForDevice(<String>['uninstall', app.id]);
    final RunResult result = _processUtils.runSync(uninstallCommand, throwOnError: true);
    return result.exitCode == 0;
  }

  Future<String?> _getProperty(String name) async {
    return (await _properties)[name];
  }

  List<String> hdcCommandForDevice(List<String> args) {
    return getHdcCommandCompat(_ohosSdk, id, args);
  }

  Future<RunResult> runHdcCheckedAsync(
    List<String> params, {
    String? workingDirectory,
    bool allowReentrantFlutter = false,
  }) async {
    return _processUtils.run(
      hdcCommandForDevice(params),
      throwOnError: true,
      workingDirectory: workingDirectory,
      allowReentrantFlutter: allowReentrantFlutter,
    );
  }

  late final Future<Map<String, String>> _properties = () async {
    var properties = <String, String>{};

    final List<String> propCommand = hdcCommandForDevice(<String>['shell', 'param', 'get']);
    _logger.printTrace(propCommand.join(' '));
    try {
      final RunResult result = _processUtils.runSync(propCommand, throwOnError: true);

      if (result.exitCode == 0) {
        properties = parseHdcDeviceProperties(result.stdout);
      }
    } on Exception catch (err) {
      _logger.printTrace('reading device properties failed: $err');
    }
    final String? productName = properties['const.product.name'];
    final String? productModel = properties['const.product.model'];
    _cachedMarketingName = (productName != null && productName.isNotEmpty)
        ? productName
        : (productModel != null && productModel.isNotEmpty)
        ? productModel
        : null;
    return properties;
  }();

  Map<String, String> parseHdcDeviceProperties(String str) {
    final properties = <String, String>{};
    final List<String> split = str.split('\r\n');
    for (final line in split) {
      // some properties value may contain '=' char,but key not
      final int indexOf = line.indexOf('=');
      if (indexOf == -1) {
        continue;
      }
      final String key = line.substring(0, indexOf).trim();
      final String value = line.substring(indexOf + 1).trim();
      properties[key] = value;
    }
    return properties;
  }
}

/// A [DevicePortForwarder] implemented for Ohos devices that uses hdc.
class OhosDevicePortForwarder extends DevicePortForwarder {
  OhosDevicePortForwarder({
    required ProcessManager processManager,
    required Logger logger,
    required String deviceId,
    required OhosDevice ohosDevice,
  }) : _deviceId = deviceId,
       _logger = logger,
       _processUtils = ProcessUtils(logger: logger, processManager: processManager),
       _ohosDevice = ohosDevice;

  final String _deviceId;
  final Logger _logger;
  final ProcessUtils _processUtils;
  final OhosDevice _ohosDevice;

  static int? _extractPort(String portString) {
    return int.tryParse(portString.trim());
  }

  @override
  List<ForwardedPort> get forwardedPorts {
    final ports = <ForwardedPort>[];

    String stdout;
    try {
      stdout = _processUtils
          .runSync(_ohosDevice.hdcCommandForDevice(<String>['fport', 'ls']), throwOnError: true)
          .stdout
          .trim();
    } on ProcessException catch (error) {
      _logger.printError('Failed to list forwarded ports: $error.');
      return ports;
    }

    final List<String> lines = LineSplitter.split(stdout).toList();
    for (final line in lines) {
      if (!line.startsWith(_deviceId)) {
        continue;
      }
      final List<String> splitLine = line.split('tcp:');

      // Sanity check splitLine.
      if (splitLine.length != 3) {
        continue;
      }

      // Attempt to extract ports.
      final int? hostPort = _extractPort(splitLine[1]);
      final int? devicePort = _extractPort(splitLine[2]);

      // Failed, skip.
      if (hostPort == null || devicePort == null) {
        continue;
      }

      ports.add(ForwardedPort(hostPort, devicePort));
    }

    return ports;
  }

  /// return one random port num , between [50000 ~ 65535]
  int getOneRandomPort() {
    const min = 50000;
    const max = 65535;
    return min + Random().nextInt(max - min);
  }

  @override
  Future<int> forward(int devicePort, {int? hostPort}) async {
    /// if host port == 0 , hdc will tip '[Fail]Forward parament failed' , so give a random port
    hostPort ??= getOneRandomPort();

    final RunResult process = await _processUtils.run(
      _ohosDevice.hdcCommandForDevice(<String>['fport', 'tcp:$hostPort', 'tcp:$devicePort']),
      throwOnError: true,
    );

    if (process.stderr.isNotEmpty) {
      process.throwException('hdc returned error:\n${process.stderr}');
    }

    if (process.exitCode != 0) {
      if (process.stdout.isNotEmpty) {
        process.throwException('hdc returned error:\n${process.stdout}');
      }
      process.throwException('hdc failed without a message');
    }

    if (hostPort == 0) {
      if (process.stdout.isEmpty) {
        process.throwException('hdc did not report forwarded port');
      }
      hostPort = int.tryParse(process.stdout);
      if (hostPort == null) {
        process.throwException('hdc forward error:\n${process.stdout}');
      }
    } else {
      if (process.stdout.isNotEmpty && !process.stdout.trim().contains('OK')) {
        process.throwException('hdc returned error:\n${process.stdout}');
      }
    }

    return hostPort!;
  }

  @override
  Future<void> unforward(ForwardedPort forwardedPort) async {
    final RunResult runResult = await _processUtils.run(
      _ohosDevice.hdcCommandForDevice(<String>[
        'fport',
        'rm',
        'tcp:${forwardedPort.hostPort}',
        'tcp:${forwardedPort.devicePort}',
      ]),
    );
    if (runResult.exitCode == 0) {
      return;
    }
    _logger.printError('Failed to unforward port: $runResult');
  }

  @override
  Future<void> dispose() async {
    for (final ForwardedPort port in forwardedPorts) {
      await unforward(port);
    }
  }
}

/// A log reader that logs from `hdc hilog`.
class HdcLogReader extends DeviceLogReader {
  HdcLogReader._(this._hdcProcess, this.name);

  /// Create a new [HdcLogReader] from an [OhosDevice] instance.
  static Future<HdcLogReader> createLogReader(
    OhosDevice device,
    ProcessManager processManager, {
    bool includePastLogs = false,
  }) async {
    final args = <String>['shell', 'hilog'];

    // If past logs are included then filter for 'flutter' logs only.
    if (includePastLogs) {
      args.addAll(<String>['-e', 'flutter']);
    } else {
      // execute show local time log
      args.addAll(<String>['-v', 'time']);
    }
    final Process process = await processManager.start(device.hdcCommandForDevice(args));
    return HdcLogReader._(process, device.name);
  }

  final Process _hdcProcess;

  @override
  final String name;

  late final StreamController<String> _linesController = StreamController<String>.broadcast(
    onListen: _start,
    onCancel: _stop,
  );

  @override
  Stream<String> get logLines => _linesController.stream;

  void _start() {
    const decoder = Utf8Decoder(reportErrors: false);
    _hdcProcess.stdout
        .transform<String>(decoder)
        .transform<String>(const LineSplitter())
        .listen(_onLine);
    _hdcProcess.stderr
        .transform<String>(decoder)
        .transform<String>(const LineSplitter())
        .listen(_onLine);
    unawaited(
      _hdcProcess.exitCode.whenComplete(() {
        if (_linesController.hasListener) {
          _linesController.close();
        }
      }),
    );
  }

  // 10-27 19:57:53.779  1195  2885 I Thread:528202332952  [INFO:ohos_main.cpp(140)] flutter The Dart VM service is listening on http://0.0.0.0:34063/nBIFd7ZPwk0=/
  static final RegExp _logFormat = RegExp(r'^[\d-:. ]{30,40}[VDIWEF][^:]+:');

  static final List<RegExp> _allowedTags = <RegExp>[
    RegExp(r'^[\d-:. ]{30,40}[VDIWEF]\s[^:]+Flutter[^:]+:\sflutter\s'),
    RegExp(r'^[\d-:. ]{30,40}[IE].*Dart VM\s+'),
    RegExp(r'^[WEF]\/System\.err:\s+'),
    RegExp(r'^[F]\/[\S^:]+:\s+'),
  ];

  // The engine's log_message_callback emits Dart `print` output as
  // '<tag> settings log message: <message>' (engine ohos_main.cpp), so a raw
  // hilog line looks like
  // '... XComFlutterOHOS_Native: flutter settings log message: hello'.
  // Rewrite it to the standard 'flutter: <message>' prefix used on other
  // platforms so log filters (`grep "flutter:"`) and devicelab log matching
  // keep working.
  static final RegExp _settingsLogMessage = RegExp(r'flutter settings log message: (.*)$');

  // 'F/libc(pid): Fatal signal 11'

  // 'I/DEBUG(pid): ...'
  static final RegExp _tombstoneLine = RegExp(r'^[IF]\/DEBUG\s*\(\s*\d+\):\s(.+)$');

  // 'I/DEBUG(pid): Tombstone written to: '
  static final RegExp _tombstoneTerminator = RegExp(r'^Tombstone written to:\s');

  // we default to true in case none of the log lines match
  bool _acceptedLastLine = true;

  // Whether a fatal crash is happening or not.
  // During a fatal crash only lines from the crash are accepted, the rest are
  // dropped.
  bool _fatalCrash = false;

  // The format of the line is controlled by the '-v' parameter passed to
  // adb logcat. We are currently passing 'time', which has the format:
  // mm-dd hh:mm:ss.milliseconds Priority/Tag( PID): ....
  void _onLine(String line) {
    // This line might be processed after the subscription is closed but before
    // adb stops streaming logs.
    if (_linesController.isClosed) {
      return;
    }
    final Match? logMatch = _logFormat.firstMatch(line);
    if (logMatch != null) {
      var acceptLine = false;

      if (_fatalCrash) {
        // While a fatal crash is going on, only accept lines from the crash
        // Otherwise the crash log in the console may get interrupted
        final Match? fatalMatch = _tombstoneLine.firstMatch(line);

        if (fatalMatch != null) {
          acceptLine = true;

          line = fatalMatch[1]!;

          if (_tombstoneTerminator.hasMatch(line)) {
            // Hit crash terminator, stop logging the crash info
            _fatalCrash = false;
          }
        }
      } else {
        // Filter on approved names and levels.
        acceptLine = _allowedTags.any((RegExp re) => re.hasMatch(line));
      }

      if (acceptLine) {
        final Match? settingsLog = _settingsLogMessage.firstMatch(line);
        if (settingsLog != null) {
          line = 'flutter: ${settingsLog.group(1)}';
        }
        _acceptedLastLine = true;
        _linesController.add(line);
        return;
      }
      _acceptedLastLine = false;
    } else if (line == '--------- beginning of system' || line == '--------- beginning of main') {
      _acceptedLastLine = false;
    } else {
      if (_acceptedLastLine) {
        _linesController.add(line);
        return;
      }
    }
  }

  void _stop() {
    _linesController.close();
    _hdcProcess.kill();
  }

  @override
  void dispose() {
    _stop();
  }

  @override
  Future<void> provideVmService(FlutterVmService connectedVmService) async {}
}

/// Parse the output of `hidumper --mem <pid>` and extract the total PSS of
/// the process, in kB.
///
/// Supports the two dump formats seen across OHOS builds: HarmonyOS NEXT
/// prints a column table whose summary row starts with `Total <kB>`, while
/// older builds print a `Total PSS: <kB>` line.
///
/// Returns null when the dump contains no total PSS row.
@visibleForTesting
OhosMemoryInfo? parseHidumperMemDump(String input) {
  // Table summary row, e.g. `         Total         255902  ...`. The table
  // header row (` Total  Clean  Dirty ...`) never matches because a digit
  // must follow the row label.
  final Match? tableTotal = _kTotalRowPattern.firstMatch(input);
  if (tableTotal != null) {
    return OhosMemoryInfo(totalPss: int.parse(tableTotal.group(1)!));
  }
  final Match? totalPss = _kTotalPssLinePattern.firstMatch(input);
  if (totalPss == null) {
    return null;
  }
  return OhosMemoryInfo(totalPss: int.parse(totalPss.group(1)!));
}

final RegExp _kTotalRowPattern = RegExp(r'^\s*Total\s+(\d+)', multiLine: true);
final RegExp _kTotalPssLinePattern = RegExp(r'Total\s+PSS[^\d\n]*(\d+)', caseSensitive: false);

/// OHOS specific implementation of memory info.
class OhosMemoryInfo extends MemoryInfo {
  OhosMemoryInfo({required this.totalPss});

  static const String _kPlatformKey = 'platform';
  // Keys mirror AndroidMemoryInfo.toJson: DevTools' flutterMemoryInfo
  // parser reads the full set and throws when a key is missing, so every
  // key must always be present.
  static const String _kRealTimeKey = 'Realtime';
  static const String _kJavaHeapKey = 'Java Heap';
  static const String _kNativeHeapKey = 'Native Heap';
  static const String _kCodeKey = 'Code';
  static const String _kStackKey = 'Stack';
  static const String _kGraphicsKey = 'Graphics';
  static const String _kPrivateOtherKey = 'Private Other';
  static const String _kSystemKey = 'System';
  static const String _kTotalKey = 'Total';

  /// Total PSS of the process, in kB.
  final int totalPss;

  @override
  Map<String, Object> toJson() {
    // Only the total PSS is collected on OHOS today; the per-category
    // entries stay 0 until hidumper's rows are mapped to the Android
    // categories.
    return <String, Object>{
      _kPlatformKey: 'Ohos',
      _kRealTimeKey: 0,
      _kJavaHeapKey: 0,
      _kNativeHeapKey: 0,
      _kCodeKey: 0,
      _kStackKey: 0,
      _kGraphicsKey: 0,
      _kPrivateOtherKey: 0,
      _kSystemKey: 0,
      _kTotalKey: totalPss,
    };
  }
}
