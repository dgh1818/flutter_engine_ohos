/*
* Copyright 2014 The Flutter Authors. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*
*/

import 'package:json5/json5.dart';
import 'package:process/process.dart';

import '../artifacts.dart';
import '../base/common.dart';
import '../base/context.dart';
import '../base/file_system.dart';
import '../base/logger.dart';
import '../base/platform.dart' as base_platform;
import '../base/process.dart';
import '../base/terminal.dart';
import '../base/utils.dart';
import '../build_info.dart';
import '../build_system/build_system.dart';
import '../build_system/targets/ohos.dart';
import '../cache.dart';
import '../globals.dart' as globals;
import '../project.dart';
import '../reporting/reporting.dart';
import 'application_package.dart';
import 'hvigor.dart';
import 'hvigor_utils.dart';
import 'ohos_plugins_manager.dart';

/// The builder in the current context.
OhosBuilder? get ohosBuilder {
  return context.get<OhosBuilder>();
}

abstract class OhosBuilder {
  const OhosBuilder();

  /// build hap
  Future<void> buildHap({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  });

  /// build har
  Future<void> buildHar({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  });

  /// build app
  Future<void> buildApp({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  });

  /// build hsp
  Future<void> buildHsp({
    required FlutterProject project,
    required OhosBuildInfo ohosBuildInfo,
    required String target,
  });
}

/// flutter_ohos 旧版编译方式，无自定义hvigor插件
/// flutter鸿蒙化插件，先编译成har，再进行依赖
class OhosDartBuilder implements OhosBuilder {
  OhosDartBuilder({
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

  /// eg:entry/src/main/resources/rawfile
  String getProjectAssetsPath(String ohosRootPath, OhosProject ohosProject) {
    return globals.fs.path.join(ohosProject.flutterModuleDirectory.path,
        'src/main/resources/rawfile', FLUTTER_ASSETS_PATH);
  }

  /// eg:entry/libs/arm64-v8a/libapp.so
  String getAppSoPath(
      String ohosRootPath, OhosArch ohosArch, OhosProject ohosProject) {
    return globals.fs.path.join(ohosProject.flutterModuleDirectory.path, 'libs',
        getNameForOhosArch(ohosArch), APP_SO);
  }

  void copyFlutterAssets(String orgPath, String desPath, Logger? logger) {
    logger?.printTrace('copy from "$orgPath" to "$desPath"');
    final LocalFileSystem localFileSystem = globals.localFileSystem;
    copyDirectory(
        localFileSystem.directory(orgPath), localFileSystem.directory(desPath));
  }

  Future<void> copyFlutterBuildInfoFile(OhosProject ohosProject) async {
    final String rawfilePath = globals.fs.path.join(
        ohosProject.flutterModuleDirectory.path, 'src/main/resources/rawfile');
    final Directory rawfileDirectory =
        globals.localFileSystem.directory(rawfilePath);
    if (!await rawfileDirectory.exists()) {
      return;
    }

    final String buildinfoFilePath = globals.fs.path
        .join(ohosProject.flutterModuleDirectory.path, BUILD_INFO_JSON_PATH);
    final File sourceFile = globals.localFileSystem.file(buildinfoFilePath);
    final String fileName = globals.fs.path.basename(buildinfoFilePath);
    final String destinationFilePath =
        globals.fs.path.join(rawfilePath, fileName);
    final File destinationFile =
        globals.localFileSystem.file(destinationFilePath);

    if (!await sourceFile.exists()) {
      return;
    }

    if (!await destinationFile.exists()) {
      await sourceFile.copy(destinationFilePath);
    } else {
      return;
    }
    // delete sourceFile
    await sourceFile.delete();
  }

  /// flutter构建
  Future<String> flutterAssemble(FlutterProject flutterProject,
      OhosBuildInfo ohosBuildInfo, String targetFile) async {
    late String targetName;
    if (ohosBuildInfo.buildInfo.isDebug) {
      targetName = 'debug_ohos_application';
    } else if (ohosBuildInfo.buildInfo.isProfile) {
      // eg:ohos_aot_bundle_profile_ohos-arm64
      targetName =
          'ohos_aot_bundle_profile_${getPlatformNameForOhosArch(ohosBuildInfo.targetArchs.first)}';
    } else {
      // eg:ohos_aot_bundle_release_ohos-arm64
      targetName =
          'ohos_aot_bundle_release_${getPlatformNameForOhosArch(ohosBuildInfo.targetArchs.first)}';
    }
    final List<Target> selectTarget =
        ohosTargets.where((Target e) => targetName == e.name).toList();
    if (selectTarget.isEmpty) {
      throwToolExit('do not found compare target.');
    } else if (selectTarget.length > 1) {
      throwToolExit('more than one target match.');
    }
    final Target target = selectTarget[0];

    final Status status =
        globals.logger.startProgress('Compiling $targetName for the Ohos...');
    String output = globals.fs.directory(getOhosBuildDirectory()).path;
    // If path is relative, make it absolute from flutter project.
    output = getAbsolutePath(flutterProject, output);
    try {
      final String? flavor = ohosBuildInfo.buildInfo.flavor;
      final BuildResult result = await globals.buildSystem.build(
          target,
          Environment(
            projectDir: globals.fs.currentDirectory,
            packageConfigPath: '.dart_tool/package_config.json',
            outputDir: globals.fs.directory(output),
            buildDir: flutterProject.directory
                .childDirectory('.dart_tool')
                .childDirectory('flutter_build'),
            defines: <String, String>{
              ...ohosBuildInfo.buildInfo.toBuildSystemEnvironment(),
              if (flavor != null) kFlavor: flavor,
              kTargetFile: targetFile,
              kTargetPlatform:
                  getPlatformNameForOhosArch(ohosBuildInfo.targetArchs.first),
            },
            artifacts: globals.artifacts!,
            fileSystem: globals.fs,
            logger: globals.logger,
            processManager: globals.processManager,
            analytics: globals.analytics,
            platform: globals.platform,
            cacheDir: globals.cache.getRoot(),
            engineVersion: globals.artifacts!.usesLocalArtifacts
                ? null
                : globals.flutterVersion.engineRevision,
            flutterRootDir: globals.fs.directory(Cache.flutterRoot),
            generateDartPluginRegistry: true,
          ));
      if (!result.success) {
        for (final ExceptionMeasurement measurement
            in result.exceptions.values) {
          globals.printError(
            'Target ${measurement.target} failed: ${measurement.exception}',
            stackTrace: measurement.fatal ? measurement.stackTrace : null,
          );
        }
        throwToolExit('Failed to compile application for the Ohos.');
      } else {
        return output;
      }
    } on Exception catch (err) {
      throwToolExit(err.toString());
    } finally {
      status.stop();
    }
  }

  /// 清理和拷贝flutter产物和资源
  Future<void> cleanAndCopyFlutterAsset(
      OhosProject ohosProject,
      OhosBuildInfo ohosBuildInfo,
      Logger? logger,
      String ohosRootPath,
      String output) async {
    logger?.printTrace('copy flutter assets to project start');
    // clean flutter assets
    final String desFlutterAssetsPath =
        getProjectAssetsPath(ohosRootPath, ohosProject);
    final Directory desAssets = globals.fs.directory(desFlutterAssetsPath);
    if (desAssets.existsSync()) {
      desAssets.deleteSync(recursive: true);
    }

    /// copy flutter assets
    copyFlutterAssets(globals.fs.path.join(output, FLUTTER_ASSETS_PATH),
        desFlutterAssetsPath, logger);
    await copyFlutterBuildInfoFile(ohosProject);

    if (ohosBuildInfo.enableImpellerFlag != null) {
      await setImpellerEnableFlag(ohosProject, ohosBuildInfo);
    }

    final String desAppSoPath = getAppSoPath(
        ohosRootPath, ohosBuildInfo.targetArchs.first, ohosProject);
    if (ohosBuildInfo.buildInfo.isRelease ||
        ohosBuildInfo.buildInfo.isProfile) {
      // copy app.so
      final String appSoPath = globals.fs.path.join(output,
          getNameForOhosArch(ohosBuildInfo.targetArchs.first), APP_SO_ORIGIN);
      final File appSoFile = globals.localFileSystem.file(appSoPath);
      ensureParentExists(desAppSoPath);
      appSoFile.copySync(desAppSoPath);
    } else {
      // delete libapp.so
      final String dir =
          globals.fs.path.join(ohosProject.flutterModuleDirectory.path, 'libs');
      if (globals.fs.directory(dir).existsSync()) {
        final List<FileSystemEntity> files =
            globals.fs.directory(dir).listSync(recursive: true);
        for (final FileSystemEntity item in files) {
          if (item.basename == APP_SO && item.existsSync()) {
            item.deleteSync();
          }
        }
      }
    }
    logger?.printTrace('copy flutter assets to project end');
  }

  /// 清理和拷贝flutter运行时
  void cleanAndCopyFlutterRuntime(OhosProject ohosProject,
      OhosBuildInfo ohosBuildInfo, Logger? logger, String ohosRootPath) {
    logger?.printTrace('copy flutter runtime to project start');
    // 复制 flutter.har
    final String localEngineHarPath = globals.artifacts!.getArtifactPath(
      Artifact.flutterEngineHar,
      platform: getTargetPlatformForName(
          getPlatformNameForOhosArch(ohosBuildInfo.targetArchs.first)),
      mode: ohosBuildInfo.buildInfo.mode,
    );
    final String desHarPath =
        globals.fs.path.join(ohosRootPath, 'har', HAR_FILE_NAME);
    ensureParentExists(desHarPath);
    final File originHarFile = globals.localFileSystem.file(localEngineHarPath);
    originHarFile.copySync(desHarPath);
    logger?.printTrace('copy from "$localEngineHarPath" to "$desHarPath"');
    logger?.printTrace('copy flutter runtime to project end');
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

    updateProjectVersion(project, ohosBuildInfo.buildInfo);
    await flutterBuildPre(project, ohosBuildInfo, target);

    /// invoke hvigow task generate hap file.
    final int errorCode = await assembleHap(
        processUtils: _processUtils,
        ohosRootPath: ohosRootPath,
        hvigorwPath: getHvigorwPath(ohosRootPath, checkMod: true),
        flavor: getFlavor(
            ohosProject.getBuildProfileFile(), ohosBuildInfo.buildInfo.flavor),
        ohosBuildInfo: ohosBuildInfo,
        target: target,
        logger: _logger);
    status.stop();
    if (errorCode != 0) {
      throwToolExit('assembleHap error! please check log.');
    }

    if (ohosBuildInfo.shouldCodesign!) {
      checkOhosSignedInfo(ohosProject);
    }

    final BuildInfo buildInfo = ohosBuildInfo.buildInfo;
    final File bundleFile = OhosProject.getSignedFile(
      modulePath: ohosProject.mainModuleDirectory.path,
      moduleName: ohosProject.mainModuleName,
      flavor: getFlavor(ohosProject.getBuildProfileFile(), buildInfo.flavor),
      throwOnMissing: true,
      shouldCodesign: ohosBuildInfo.shouldCodesign!,
    );
    final String appSize = (buildInfo.mode == BuildMode.debug)
        ? '' // Don't display the size when building a debug variant.
        : ' (${getSizeAsPlatformMB(bundleFile.lengthSync())})';
    _logger.printStatus(
      '${_logger.terminal.successMark} Built ${_fileSystem.path.relative(bundleFile.path)}$appSize.',
      color: TerminalColor.green,
    );
  }

  Future<void> flutterBuildPre(FlutterProject project,
      OhosBuildInfo ohosBuildInfo, String target) async {
    /**
     * 1. execute flutter assemble
     * 2. copy flutter asset to flutter module
     * 3. copy flutter runtime
     * 4. ohpm install
     */

    await checkOhosPluginsDependencies(project);
    await addPluginsModules(project);
    await addFlutterModuleAndPluginsSrcOverrides(project);

    parseData(project, _logger);

    final String output = await flutterAssemble(project, ohosBuildInfo, target);

    await cleanAndCopyFlutterAsset(
        ohosProject, ohosBuildInfo, _logger, ohosRootPath, output);

    cleanAndCopyFlutterRuntime(
        ohosProject, ohosBuildInfo, _logger, ohosRootPath);

    // ohpm install for all modules
    // ohosProject.deleteOhModulesCache();
    await ohpmInstall(
      processUtils: _processUtils,
      workingDirectory: ohosRootPath,
      logger: _logger,
    );

    /// 生成所有 plugin 的 har
    await assembleHars(_processUtils, project, ohosBuildInfo, target, _logger);
    await assembleHsps(_processUtils, project, ohosBuildInfo, target, _logger);

    await removePluginsModules(project);
    await addFlutterModuleAndPluginsOverrides(project);
    // ohosProject.deleteOhModulesCache();
    await ohpmInstall(
      processUtils: _processUtils,
      workingDirectory: ohosRootPath,
      logger: _logger,
    );
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

    await flutterBuildPre(project, ohosBuildInfo, target);
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
    2. Add flutter_module to the dependencies list:

      "dependencies": {
        "@ohos/flutter_module": "file:path/to/har/flutter_module.har"
      }

    3. Override flutter and plugins dependencies:

      "overrides" {
        "@ohos/flutter_ohos": "file:path/to/har/flutter.har",
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
    if (!project.ohos.ohosBuildData.moduleInfo.hasEntryModule) {
      throwToolExit(
          "this ohos project don't have a entry module , can't build to a application.");
    }
    final Status status = _logger.startProgress(
      'Running Hvigor task assembleApp...',
    );
    updateProjectVersion(project, ohosBuildInfo.buildInfo);
    await flutterBuildPre(project, ohosBuildInfo, target);

    /// invoke hvigow task generate hap file.
    final int errorCode1 = await assembleApp(
        processUtils: _processUtils,
        ohosRootPath: ohosRootPath,
        flavor: getFlavor(
            ohosProject.getBuildProfileFile(), ohosBuildInfo.buildInfo.flavor),
        hvigorwPath: getHvigorwPath(ohosRootPath, checkMod: true),
        ohosBuildInfo: ohosBuildInfo,
        target: target,
        logger: _logger);
    status.stop();
    if (errorCode1 != 0) {
      throwToolExit('assembleApp error! please check log.');
    }

    if (ohosBuildInfo.shouldCodesign!) {
      checkOhosSignedInfo(ohosProject);
    }

    final BuildInfo buildInfo = ohosBuildInfo.buildInfo;
    final File bundleFile = OhosProject.getSignedFile(
      modulePath: ohosProject.mainModuleDirectory.path,
      moduleName: ohosProject.mainModuleName,
      flavor: getFlavor(ohosProject.getBuildProfileFile(), buildInfo.flavor),
      type: OhosFileType.app,
      throwOnMissing: true,
      shouldCodesign: ohosBuildInfo.shouldCodesign!,
    );
    final String appSize = (buildInfo.mode == BuildMode.debug)
        ? '' // Don't display the size when building a debug variant.
        : ' (${getSizeAsPlatformMB(bundleFile.lengthSync())})';
    _logger.printStatus(
      '${_logger.terminal.successMark} Built ${_fileSystem.path.relative(bundleFile.path)}$appSize.',
      color: TerminalColor.green,
    );
  }

  /// 生成所有 plugin 的 har
  Future<void> assembleHars(
    ProcessUtils processUtils,
    FlutterProject project,
    OhosBuildInfo ohosBuildInfo,
    String target,
    Logger? logger,
  ) async {
    final String ohosProjectPath = project.ohos.ohosRoot.path;
    final List<OhosModule> modules = ohosBuildData.harModules;
    if (modules.isEmpty) {
      return;
    }

    // compile hars. parallel compilation.
    final String hvigorwPath = getHvigorwPath(ohosProjectPath, checkMod: true);
    final String moduleName =
        moduleNameWithFlavor(modules, ohosBuildInfo.buildInfo.flavor);
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
      final String desPath =
          globals.fs.path.join(ohosRootPath, 'har', '${module.name}.har');
      ensureParentExists(desPath);
      originHar.copySync(desPath);
    }
  }

  Future<void> assembleHsps(
    ProcessUtils processUtils,
    FlutterProject project,
    OhosBuildInfo ohosBuildInfo,
    String target,
    Logger? logger,
  ) async {
    final String ohosProjectPath = project.ohos.ohosRoot.path;
    final List<OhosModule> modules = ohosBuildData.moduleInfo.moduleList
        .where((OhosModule element) => element.type == OhosModuleType.shared)
        .toList();
    if (modules.isEmpty) {
      return;
    }
    final String hvigorwPath = getHvigorwPath(ohosProjectPath, checkMod: true);
    final String moduleName =
        moduleNameWithFlavor(modules, ohosBuildInfo.buildInfo.flavor);
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
