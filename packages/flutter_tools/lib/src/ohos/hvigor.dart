/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*/

import 'dart:convert';
import 'dart:io';

import 'package:json5/json5.dart';
import 'package:process/process.dart';

import '../artifacts.dart';
import '../base/common.dart';
import '../base/file_system.dart';
import '../base/logger.dart';
import '../base/os.dart';
import '../base/platform.dart' as base_platform;
import '../base/process.dart';
import '../base/terminal.dart';
import '../base/utils.dart';
import '../build_info.dart';
import '../globals.dart' as globals;
import '../project.dart';
import '../reporting/reporting.dart';
import 'application_package.dart';
import 'hvigor_utils.dart';
import 'ohos_builder.dart';

const String HAR_FILE_NAME = 'flutter.har';

const String BUILD_INFO_JSON_DES_PATH =
    'src/main/resources/rawfile/buildinfo.json5';

final bool isWindows = globals.platform.isWindows;

String getHvigorwFile() => isWindows ? 'hvigorw.bat' : 'hvigorw';

void checkPlatformEnvironment(String environment, Logger? logger) {
  final String? environmentConfig = Platform.environment[environment];
  if (environmentConfig == null) {
    throwToolExit(
        'error:current platform environment $environment have not set');
  } else {
    logger?.printStatus(
        'current platform environment $environment = $environmentConfig');
  }
}

Future<void> setImpellerEnableFlag(
    OhosProject ohosProject, OhosBuildInfo ohosBuildInfo) async {
  final String buildinfoFilePath = globals.fs.path
      .join(ohosProject.flutterModuleDirectory.path, BUILD_INFO_JSON_DES_PATH);

  final File file = globals.localFileSystem.file(buildinfoFilePath);

  if (!await file.exists()) {
    throw Exception('Failed to find buildinfo.json5 file: $buildinfoFilePath');
  }

  final String content = await file.readAsString();

  final Map<String, dynamic> json = jsonDecode(content) as Map<String, dynamic>;

  // find "enable_impeller" in json file
  final List<dynamic> stringList = json['string'] as List<dynamic>;
  final Map<String, dynamic>? enableImpellerItem = stringList.firstWhere(
    (dynamic item) =>
        (item as Map<String, dynamic>)['name'] == 'enable_impeller',
    orElse: () => null,
  ) as Map<String, dynamic>?;

  if (enableImpellerItem != null) {
    enableImpellerItem['value'] = ohosBuildInfo.enableImpellerFlag?.toString();
  }

  final String updatedContent =
      const JsonEncoder.withIndent('  ').convert(json);

  // save setting
  await file.writeAsString(updatedContent);
}

String getHvigorwPath(String ohosRootPath, {bool checkMod = false}) {
  final String hvigorwPath =
      globals.fs.path.join(ohosRootPath, getHvigorwFile());
  if (globals.fs.file(hvigorwPath).existsSync()) {
    if (checkMod) {
      final OperatingSystemUtils operatingSystemUtils = globals.os;
      final File file = globals.localFileSystem.file(hvigorwPath);
      operatingSystemUtils.chmod(file, '755');
    }
    return hvigorwPath;
  } else {
    return 'hvigorw';
  }
}

String getAbsolutePath(FlutterProject flutterProject, String path) {
  if (globals.fs.path.isRelative(path)) {
    return globals.fs.path.join(flutterProject.directory.path, path);
  }
  return path;
}

///hvigorw任务
Future<int> hvigorwTask(List<String> taskCommand,
    {required ProcessUtils processUtils,
    required String workPath,
    required String hvigorwPath,
    Logger? logger}) async {
  final RunResult result = processUtils.runSync(taskCommand,
      workingDirectory: workPath, throwOnError: true);
  return result.exitCode;
}

Future<int> assembleHap(
    {required ProcessUtils processUtils,
    required String ohosRootPath,
    required String hvigorwPath,
    required String flavor,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
    Logger? logger}) async {
  final List<String> command = <String>[
    hvigorwPath,
    // 'clean',
    'assembleHap',
    '-p',
    'product=$flavor',
    '-p',
    'buildMode=${ohosBuildInfo.buildInfo.modeName}',
    '--no-daemon',
  ];
  _appendCommands(command, ohosBuildInfo, target: target, logger: logger);
  return hvigorwTask(command,
      processUtils: processUtils,
      workPath: ohosRootPath,
      hvigorwPath: hvigorwPath,
      logger: logger);
}

Future<int> assembleApp(
    {required ProcessUtils processUtils,
    required String ohosRootPath,
    required String hvigorwPath,
    required String flavor,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
    Logger? logger}) async {
  final List<String> command = <String>[
    hvigorwPath,
    // 'clean',
    'assembleApp',
    '-p',
    'product=$flavor',
    '-p',
    'buildMode=${ohosBuildInfo.buildInfo.modeName}',
    '--no-daemon',
  ];
  _appendCommands(command, ohosBuildInfo, target: target, logger: logger);
  return hvigorwTask(command,
      processUtils: processUtils,
      workPath: ohosRootPath,
      hvigorwPath: hvigorwPath,
      logger: logger);
}

Future<int> assembleHar(
    {required ProcessUtils processUtils,
    required String workPath,
    required String hvigorwPath,
    required String moduleName,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
    String product = 'default',
    Logger? logger}) async {
  final List<String> command = <String>[
    hvigorwPath,
    // 'clean',
    '--mode',
    'module',
    '-p',
    'module=$moduleName',
    '-p',
    'product=$product',
    '-p',
    'buildMode=${ohosBuildInfo.buildInfo.modeName}',
    'assembleHar',
    '--no-daemon',
  ];
  _appendCommands(command, ohosBuildInfo, target: target, logger: logger);
  return hvigorwTask(command,
      processUtils: processUtils,
      workPath: workPath,
      hvigorwPath: hvigorwPath,
      logger: logger);
}

Future<int> assembleHsp(
    {required ProcessUtils processUtils,
    required String workPath,
    required String hvigorwPath,
    required String moduleName,
    required String flavor,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
    Logger? logger}) async {
  final List<String> command = <String>[
    hvigorwPath,
    // 'clean',
    '--mode',
    'module',
    '-p',
    'module=$moduleName',
    '-p',
    'product=$flavor',
    '-p',
    'buildMode=${ohosBuildInfo.buildInfo.modeName}',
    'assembleHsp',
    '--no-daemon',
  ];
  _appendCommands(command, ohosBuildInfo, target: target, logger: logger);
  return hvigorwTask(command,
      processUtils: processUtils,
      workPath: workPath,
      hvigorwPath: hvigorwPath,
      logger: logger);
}

void copyFlutterRuntime(
  OhosProject ohosProject,
  OhosBuildInfo ohosBuildInfo,
  Logger? logger,
  OhosBuildData ohosBuildData,
) {
  logger?.printTrace('copy flutter runtime to project start');
  final BuildMode buildMode = ohosBuildInfo.buildInfo.mode;
  final String localEngineHarPath = globals.artifacts!.getArtifactPath(
    Artifact.flutterEngineHar,
    platform: getTargetPlatformForName(
        getPlatformNameForOhosArch(ohosBuildInfo.targetArchs.first)),
    mode: buildMode,
  );
  final String desHarPath = globals.fs.path.join(
    ohosProject.parent.buildDirectory.path,
    'ohos',
    'har',
    buildMode.name,
    HAR_FILE_NAME,
  );
  ensureParentExists(desHarPath);
  final File originHarFile = globals.localFileSystem.file(localEngineHarPath);
  originHarFile.copySync(desHarPath);
  logger?.printTrace('copy from "$localEngineHarPath" to "$desHarPath"');
  logger?.printTrace('copy flutter runtime to project end');
}

void ensureParentExists(String path) {
  final Directory directory = globals.localFileSystem.file(path).parent;
  if (!directory.existsSync()) {
    directory.createSync(recursive: true);
  }
}

class OhosHvigorBuilder implements OhosBuilder {
  OhosHvigorBuilder({
    required Logger logger,
    required ProcessManager processManager,
    required FileSystem fileSystem,
    required Artifacts artifacts,
    required Usage usage,
    required HvigorUtils hvigorUtils,
    required base_platform.Platform platform,
  })  : _logger = logger,
        _fileSystem = fileSystem,
        _artifacts = artifacts,
        _usage = usage,
        _hvigorUtils = hvigorUtils,
        _fileSystemUtils =
            FileSystemUtils(fileSystem: fileSystem, platform: platform),
        _processUtils =
            ProcessUtils(logger: logger, processManager: processManager);

  final Logger _logger;
  final ProcessUtils _processUtils;
  final FileSystem _fileSystem;
  final Artifacts _artifacts;
  final Usage _usage;
  final HvigorUtils _hvigorUtils;
  final FileSystemUtils _fileSystemUtils;

  late OhosProject ohosProject;
  late String ohosRootPath;
  late OhosBuildData ohosBuildData;

  void parseData(FlutterProject flutterProject, Logger? logger) {
    ohosProject = flutterProject.ohos;
    ohosRootPath = ohosProject.ohosRoot.path;
    ohosBuildData = OhosBuildData.parseOhosBuildData(ohosProject, logger);
  }

  /// build hap
  @override
  Future<void> buildHap({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  }) async {
    _logger.printStatus('start hap build...');

    if (!project.ohos.ohosBuildData.moduleInfo.hasEntryModule) {
      throwToolExit(
          "this ohos project don't have a entry module, can't build to a hap file.");
    }
    final Status status = _logger.startProgress(
      'Running Hvigor task assembleHap...',
    );
    updateLocalProperties(project: project, buildInfo: ohosBuildInfo.buildInfo);

    parseData(project, _logger);

    if (ohosBuildInfo.enableImpellerFlag != null) {
      await setImpellerEnableFlag(ohosProject, ohosBuildInfo);
    }

    final String hvigorwPath = getHvigorwPath(ohosRootPath, checkMod: true);

    /// invoke hvigow task generate hap file.
    final int errorCode = await assembleHap(
        processUtils: _processUtils,
        ohosRootPath: ohosRootPath,
        hvigorwPath: hvigorwPath,
        flavor: getFlavor(
            ohosProject.getBuildProfileFile(), ohosBuildInfo.buildInfo.flavor),
        ohosBuildInfo: ohosBuildInfo,
        target: target,
        logger: _logger);
    status.stop();
    if (errorCode != 0) {
      throwToolExit('assembleHap error! please check log.');
    }

    final File buildProfile = project.ohos.getBuildProfileFile();
    final String buildProfileConfig = buildProfile.readAsStringSync();
    final dynamic obj = JSON5.parse(buildProfileConfig);
    final dynamic signingConfigs = obj['app']?['signingConfigs'];
    if (signingConfigs is List && signingConfigs.isEmpty) {
      _logger.printError(
          '请通过DevEco Studio打开ohos工程后配置调试签名(File -> Project Structure -> Signing Configs 勾选Automatically generate signature)');
    } else {
      final BuildInfo buildInfo = ohosBuildInfo.buildInfo;
      File bundleFile = OhosProject.getSignedFile(
        modulePath: ohosProject.mainModuleDirectory.path,
        moduleName: ohosProject.mainModuleName,
        flavor: getFlavor(ohosProject.getBuildProfileFile(), buildInfo.flavor),
        throwOnMissing: true,
      );
      if (bundleFile.existsSync()) {
        final String outputPath = globals.fs.path.join(
          ohosProject.parent.buildDirectory.path,
          'ohos',
          'hap',
          bundleFile.basename,
        );
        bundleFile = bundleFile.copySync(outputPath);
      }
      final String appSize = (buildInfo.mode == BuildMode.debug)
          ? '' // Don't display the size when building a debug variant.
          : ' (${getSizeAsPlatformMB(bundleFile.lengthSync())})';
      _logger.printStatus(
        '${_logger.terminal.successMark} Built ${_fileSystem.path.relative(bundleFile.path)}$appSize.',
        color: TerminalColor.green,
      );
    }
  }

  @override
  Future<void> buildHar({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  }) async {
    if (!project.isModule ||
        !project.ohos.flutterModuleDirectory.existsSync()) {
      throwToolExit('current project is not module or has not pub get');
    }

    final Status status = _logger.startProgress(
      'Running Hvigor task assembleHar...',
    );

    parseData(project, _logger);

    if (ohosBuildInfo.enableImpellerFlag != null) {
      await setImpellerEnableFlag(ohosProject, ohosBuildInfo);
    }

    // 删除 build/ohos/har 目录
    final String harOutput = globals.fs.path.join(
      ohosProject.parent.buildDirectory.path,
      'ohos',
      'har',
      ohosBuildInfo.buildInfo.mode.name,
    );
    if (globals.fs.directory(harOutput).existsSync()) {
      globals.fs.directory(harOutput).deleteSync(recursive: true);
    }

    // 复制 flutter.har
    copyFlutterRuntime(ohosProject, ohosBuildInfo, _logger, ohosBuildData);

    // 生成 module 和所有 plugin 的 har
    await assembleHars(_processUtils, project, ohosBuildInfo, _logger, target);
    await assembleHsps(_processUtils, project, ohosBuildInfo, _logger, target);

    status.stop();
    printHowToConsumeHar(logger: _logger);
  }

  /// Prints how to consume the har from a host app.
  void printHowToConsumeHar({
    Logger? logger,
  }) {
    logger?.printStatus('\nConsuming the Module', emphasis: true);
    logger?.printStatus('''
    1. Open ${globals.fs.path.join('<host project>', 'oh-package.json5')}
    2. Override flutter, flutter_module and plugins dependencies:

      "overrides" {
        "@ohos/flutter_ohos": "file:path/to/flutter.har",
        "@ohos/flutter_module": "file:path/to/flutter_module.har",
        "plugin_a": "file:path/to/plugin_a.har",
        ...
      }

    3. Open ${globals.fs.path.join('<host project>/entry', 'oh-package.json5')}
    4. Add flutter and flutter_module to the dependencies list:

      "dependencies": {
        "@ohos/flutter_ohos": "",
        "@ohos/flutter_module": "",
      }
  ''');
  }

  @override
  Future<void> buildHsp({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  }) {
    // TODO: implement buildHsp
    throw UnimplementedError();
  }

  @override
  Future<void> buildApp({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  }) async {
    final Status status = _logger.startProgress(
      'Running Hvigor task assembleApp...',
    );
    updateLocalProperties(project: project, buildInfo: ohosBuildInfo.buildInfo);

    parseData(project, _logger);

    if (ohosBuildInfo.enableImpellerFlag != null) {
      await setImpellerEnableFlag(ohosProject, ohosBuildInfo);
    }

    final String hvigorwPath = getHvigorwPath(ohosRootPath, checkMod: true);

    /// invoke hvigow task generate hap file.
    final int errorCode1 = await assembleApp(
        processUtils: _processUtils,
        ohosRootPath: ohosRootPath,
        flavor: getFlavor(
            ohosProject.getBuildProfileFile(), ohosBuildInfo.buildInfo.flavor),
        hvigorwPath: hvigorwPath,
        ohosBuildInfo: ohosBuildInfo,
        target: target,
        logger: _logger);
    status.stop();
    if (errorCode1 != 0) {
      throwToolExit('assembleHap error! please check log.');
    }
    final BuildInfo buildInfo = ohosBuildInfo.buildInfo;
    File bundleFile = OhosProject.getSignedFile(
      modulePath: ohosProject.mainModuleDirectory.path,
      moduleName: ohosProject.mainModuleName,
      flavor: getFlavor(ohosProject.getBuildProfileFile(), buildInfo.flavor),
      type: OhosFileType.app,
      throwOnMissing: true,
    );
    if (bundleFile.existsSync()) {
      final String outputPath = globals.fs.path.join(
        ohosProject.parent.buildDirectory.path,
        'ohos',
        'app',
        bundleFile.basename,
      );
      bundleFile = bundleFile.copySync(outputPath);
    }
    final String appSize = (buildInfo.mode == BuildMode.debug)
        ? '' // Don't display the size when building a debug variant.
        : ' (${getSizeAsPlatformMB(bundleFile.lengthSync())})';
    _logger.printStatus(
      '${_logger.terminal.successMark} Built ${_fileSystem.path.relative(bundleFile.path)}$appSize.',
      color: TerminalColor.green,
    );
  }

  String _moduleNameWithFlavor(List<OhosModule> modules, String? flavor) {
    return modules
        .map((OhosModule module) => OhosModule.fromModulePath(
              modulePath: module.srcPath,
              flavor: getFlavor(
                globals.fs.file(globals.fs.path
                    .join(module.srcPath, 'build-profile.json5')),
                flavor,
              ),
            ))
        .map((OhosModule module) => '${module.name}@${module.flavor}')
        .join(',');
  }

  /// 生成所有 plugin 的 har
  Future<void> assembleHars(
    ProcessUtils processUtils,
    FlutterProject project,
    OhosBuildInfo ohosBuildInfo,
    Logger? logger,
    String target,
  ) async {
    final String ohosProjectPath = project.ohos.ohosRoot.path;
    final List<OhosModule> modules = ohosBuildData.harModules;
    if (modules.isEmpty) {
      return;
    }

    // compile hars. parallel compilation.
    final String hvigorwPath = getHvigorwPath(ohosProjectPath, checkMod: true);
    final String moduleName =
        _moduleNameWithFlavor(modules, ohosBuildInfo.buildInfo.flavor);
    final int errorCode = await assembleHar(
        processUtils: processUtils,
        workPath: ohosProjectPath,
        moduleName: moduleName,
        hvigorwPath: hvigorwPath,
        ohosBuildInfo: ohosBuildInfo,
        target: target,
        logger: logger);
    if (errorCode != 0) {
      throwToolExit('Oops! assembleHars failed! please check log.');
    }

    // copy hars
    for (final OhosModule module in modules) {
      final File originHar = globals.fs.file(globals.fs.path.join(
          module.srcPath,
          'build',
          'default',
          'outputs',
          module.flavor,
          '${module.name}.har'));
      if (!originHar.existsSync()) {
        throwToolExit('Oops! Failed to find: ${originHar.path}');
      }
      final String desPath = globals.fs.path.join(
        ohosProject.parent.buildDirectory.path,
        'ohos',
        'har',
        ohosBuildInfo.buildInfo.mode.name,
        '${module.name}.har',
      );
      ensureParentExists(desPath);
      originHar.copySync(desPath);
    }
  }

  Future<void> assembleHsps(
    ProcessUtils processUtils,
    FlutterProject project,
    OhosBuildInfo ohosBuildInfo,
    Logger? logger,
    String target,
  ) async {
    final String ohosProjectPath = project.ohos.ohosRoot.path;
    final List<OhosModule> modules = ohosBuildData.hspModules;
    if (modules.isEmpty) {
      return;
    }
    final String hvigorwPath = getHvigorwPath(ohosProjectPath, checkMod: true);
    final String moduleName =
        _moduleNameWithFlavor(modules, ohosBuildInfo.buildInfo.flavor);
    final int errorCode = await assembleHsp(
        processUtils: processUtils,
        workPath: ohosProjectPath,
        moduleName: moduleName,
        hvigorwPath: hvigorwPath,
        flavor: getFlavor(
            project.ohos.getBuildProfileFile(), ohosBuildInfo.buildInfo.flavor),
        ohosBuildInfo: ohosBuildInfo,
        target: target,
        logger: logger);
    if (errorCode != 0) {
      throwToolExit('Oops! assembleHsps failed! please check log.');
    }
  }
}

void _appendCommands(List<String> command, OhosBuildInfo ohosBuildInfo,
    {required String target, Logger? logger}) {
  command.add('-s');
  command.addAll(<String>['-p', 'FLUTTER_TARGET=$target']);
  if (logger != null && logger.isVerbose) {
    command.addAll(<String>['-p', 'VERBOSE_SCRIPT_LOGGING=true']);
  }
  final LocalEngineInfo? localEngineInfo = globals.artifacts?.localEngineInfo;
  if (localEngineInfo != null) {
    command.addAll(<String>[
      '-p',
      'FLUTTER_ENGINE=${globals.fs.path.dirname(localEngineInfo.targetOutPath)}'
    ]);
    command.addAll(
        <String>['-p', 'LOCAL_ENGINE=${localEngineInfo.localTargetName}']);
    command.addAll(
        <String>['-p', 'LOCAL_ENGINE_HOST=${localEngineInfo.localHostName}']);
  }
  final Iterable<String> targetArchs = ohosBuildInfo.targetArchs
      .map((OhosArch arch) => getPlatformNameForOhosArch(arch));
  if (targetArchs.isNotEmpty) {
    command.addAll(<String>['-p', 'TARGET_PLATFORM=${targetArchs.join(',')}']);
  }
  final Map<String, String> envConfig =
      ohosBuildInfo.buildInfo.toEnvironmentConfig();
  for (final MapEntry<String, String> config in envConfig.entries) {
    command.addAll(<String>['-p', '${config.key}=${config.value}']);
  }
}
