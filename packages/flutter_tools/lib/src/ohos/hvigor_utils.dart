/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*/
import 'dart:convert';

import 'package:json5/json5.dart';

import '../base/common.dart';
import '../base/file_system.dart';

import '../base/utils.dart';
import '../build_info.dart';
import '../cache.dart';
import '../globals.dart' as globals;
import '../project.dart';
import '../reporting/reporting.dart';
import 'ohos_sdk.dart';

const String FLAVOR_DEFAULT = 'default';
const String BUILD_NUMBER_DEFAULT = '1000000';
const String BUILD_NAME_DEFAULT = '1.0.0';

class HvigorUtils {}

/// Overwrite local.properties in the specified Flutter project's Harmony
/// sub-project, if needed.
///
/// If [requireHarmonySdk] is true (the default) and no Harmony SDK is found,
/// this will fail with a [ToolExit].
void updateLocalProperties({
  required FlutterProject project,
  BuildInfo? buildInfo,
  bool requireHarmonySdk = true,
}) {
  if (requireHarmonySdk && globals.hmosSdk == null) {
    exitWithNoSdkMessage();
  }
  final File localProperties = project.ohos.localPropertiesFile;
  bool changed = false;

  SettingsFile settings;
  if (localProperties.existsSync()) {
    settings = SettingsFile.parseFromFile(localProperties);
  } else {
    settings = SettingsFile();
    changed = true;
  }

  void changeIfNecessary(String key, String? value) {
    if (settings.values[key] == value) {
      return;
    }
    if (value == null) {
      settings.values.remove(key);
    } else {
      settings.values[key] = value;
    }
    changed = true;
  }

  final HmosSdk? hmosSdk = globals.hmosSdk;
  if (hmosSdk != null) {
    changeIfNecessary('hwsdk.dir', globals.fsUtils.escapePath(hmosSdk.sdkPath));
  }
  final String? nodeHome = globals.platform.environment['NODE_HOME'];
  if (nodeHome != null) {
    changeIfNecessary('nodejs.dir', globals.fsUtils.escapePath(nodeHome));
  }

  changeIfNecessary('flutter.sdk', globals.fsUtils.escapePath(Cache.flutterRoot!));

  if (buildInfo != null) {
    final String? buildName = validatedBuildNameForPlatform(
      TargetPlatform.ohos_arm,
      buildInfo.buildName ?? project.manifest.buildName,
      globals.logger,
    );
    changeIfNecessary('flutter.versionName', buildName);
    final String? buildNumber = validatedBuildNumberForPlatform(
      TargetPlatform.ohos_arm,
      buildInfo.buildNumber ?? project.manifest.buildNumber,
      globals.logger,
    );
    changeIfNecessary('flutter.versionCode', buildNumber);
  }

  if (changed) {
    settings.writeContents(localProperties);
  }
}

/// Writes standard Ohos local properties to the specified [properties] file.
///
/// Writes the path to the Ohos SDK, if known.
void writeLocalProperties(File properties) {
  final SettingsFile settings = SettingsFile();
  final HmosSdk? hmosSdk = globals.hmosSdk;
  if (hmosSdk != null) {
    settings.values['hwsdk.dir'] = globals.fsUtils.escapePath(hmosSdk.sdkPath);
  }
  final String? nodeHome = globals.platform.environment['NODE_HOME'];
  if (nodeHome != null) {
    settings.values['nodejs.dir'] = nodeHome;
  }
  settings.writeContents(properties);
}

void exitWithNoSdkMessage() {
  BuildEvent('unsupported-project',
          type: 'hvigor',
          eventError: 'hos-sdk-not-found',
          flutterUsage: globals.flutterUsage)
      .send();
  throwToolExit('${globals.logger.terminal.warningMark} No Hmos SDK found. '
      'Try setting the HOS_SDK_HOME environment variable.');
}

String getFlavor(File buildProfileFile, String? flavor) {
  if (flavor == null) {
    return FLAVOR_DEFAULT;
  }
  if (buildProfileFile.existsSync()) {
    final Map<String, dynamic> config = JSON5
        .parse(buildProfileFile.readAsStringSync()) as Map<String, dynamic>;
    final List<dynamic> targetList;
    // ignore: avoid_dynamic_calls
    if (config['app'] != null && config['app']['products'] != null) {
      // ohos/build-profile.json5
      // ignore: avoid_dynamic_calls
      targetList = config['app']['products'] as List<dynamic>;
    } else if (config['targets'] != null) {
      // ohos/entry/build-profile.json5
      targetList = config['targets'] as List<dynamic>;
    } else {
      // ignore: always_specify_types
      targetList = [];
    }
    for (final dynamic item in targetList) {
      final Map<String, dynamic> map = item as Map<String, dynamic>;
      if (flavor == (map['name'] as String)) {
        return flavor;
      }
    }
  }
  globals.logger.printWarning(
      'Flavor "$flavor" not exists in file ${buildProfileFile.path}, use "$FLAVOR_DEFAULT" instead.');
  return FLAVOR_DEFAULT;
}

void updateProjectVersion(FlutterProject project, BuildInfo? buildInfo) {
  final File targetFile = project.ohos.getAppJsonFile();
  if (targetFile.existsSync()) {
    final String? buildNumber = validatedBuildNumberForPlatform(
      TargetPlatform.ohos_arm,
      buildInfo?.buildNumber ?? project.manifest.buildNumber,
      globals.logger,
    );
    final String? buildName = validatedBuildNameForPlatform(
      TargetPlatform.ohos_arm,
      buildInfo?.buildName ?? project.manifest.buildName,
      globals.logger,
    );

    final Map<String, dynamic> config =
        JSON5.parse(targetFile.readAsStringSync()) as Map<String, dynamic>;
    if (config['app'] != null) {
      final Map<String, dynamic> map = config['app'] as Map<String, dynamic>;
      if (buildNumber != null) {
        map['versionCode'] = int.parse(buildNumber);
      }
      if (buildName != null) {
        map['versionName'] = buildName;
      }
      final String configNew =
          const JsonEncoder.withIndent('  ').convert(config);
      targetFile.writeAsStringSync(configNew, flush: true);
    }
  }
}

void installHvigorPlugin(OhosProject ohosProject) {
  if (!useHvigorTsBuilder(ohosProject)) {
    globals.logger.printTrace('Skip installHvigorPlugin.');
    return;
  }
  globals.logger.printTrace('Call installHvigorPlugin.');
  final String flutterRoot = globals.fs.path.absolute(Cache.flutterRoot!);
  final String hvigorAppPluginPath = globals.fs.path.join(
    flutterRoot,
    'packages',
    'flutter_tools',
    'hvigor',
  );
  final File packageJsonFile = ohosProject.ohosRoot.childFile('package.json');
  if (packageJsonFile.existsSync()) {
    packageJsonFile.deleteSync();
  }
  final Map<String, dynamic> packageJson = <String, dynamic>{
    'dependencies': <String, String>{
      'flutter-hvigor-plugin': 'file:$hvigorAppPluginPath'
    }
  };
  packageJsonFile.createSync();
  final String packageJsonContent = const JsonEncoder.withIndent('  ').convert(packageJson);
  packageJsonFile.writeAsStringSync(packageJsonContent);
  final List<String> command = <String>[
    'npm',
    'install',
  ];
  globals.processManager.runSync(
    command,
    workingDirectory: ohosProject.ohosRoot.path,
  );
}

bool useHvigorTsBuilder(OhosProject ohosProject) {
  final File file = ohosProject.isModule
    ? ohosProject.ohosRoot.childFile('include_flutter.ts')
    : ohosProject.ohosRoot.childFile('hvigorfile.ts');
  return file.existsSync() &&
      file.readAsStringSync().contains('flutter-hvigor-plugin');
}

bool useHvigorDartBuilder(OhosProject ohosProject) {
  return !useHvigorTsBuilder(ohosProject);
}
