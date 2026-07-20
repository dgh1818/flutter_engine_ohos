// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:code_assets/code_assets.dart';
import 'package:hooks_runner/hooks_runner.dart';

import '../../../base/common.dart';
import '../../../base/file_system.dart';
import '../../../build_info.dart';
import '../../../globals.dart' as globals;
import '../../../ohos/ohos_sdk.dart';
import '../native_assets.dart';

/// Maps [OhosArch] to dart-lang/native [Architecture] for code asset builds.
Architecture getNativeOhosArchitecture(OhosArch ohosArch) {
  return switch (ohosArch) {
    OhosArch.armeabi_v7a => Architecture.arm,
    OhosArch.arm64_v8a => Architecture.arm64,
    OhosArch.x86_64 => Architecture.x64,
  };
}

/// [Architecture] to [OhosArch] for copying assets into libs/arch/.
OhosArch _getOhosArch(Architecture architecture) {
  return switch (architecture) {
    Architecture.arm => OhosArch.armeabi_v7a,
    Architecture.arm64 => OhosArch.arm64_v8a,
    Architecture.x64 => OhosArch.x86_64,
    _ => throwToolExit('Invalid architecture for OHOS: $architecture.'),
  };
}

/// Looks up the OHOS SDK LLVM toolchain (clang / llvm-ar / ld.lld).
///
/// Returns `null` if the SDK or LLVM directory cannot be found.
/// The LLVM toolchain lives at:
///   {sdkPath}/{version}/openharmony/native/llvm/bin/
Future<CCompilerConfig?> cCompilerConfigOhos() async {
  final HarmonySdk? sdk = globals.ohosSdk ?? globals.hmosSdk;
  if (sdk == null) {
    return null;
  }

  final String? llvmBinPath = _findLlvmBinPath(sdk.sdkPath);
  if (llvmBinPath == null) {
    return null;
  }

  final bool isWindows = globals.platform.isWindows;
  final Uri? compiler = _toOptionalFileUri(
    globals.fs.path.join(llvmBinPath, isWindows ? 'clang.exe' : 'clang'),
  );
  final Uri? archiver = _toOptionalFileUri(
    globals.fs.path.join(llvmBinPath, isWindows ? 'llvm-ar.exe' : 'llvm-ar'),
  );
  final Uri? linker = _toOptionalFileUri(
    globals.fs.path.join(llvmBinPath, isWindows ? 'ld.lld.exe' : 'ld.lld'),
  );
  if (compiler == null || archiver == null || linker == null) {
    return null;
  }
  return CCompilerConfig(compiler: compiler, archiver: archiver, linker: linker);
}

/// Searches the SDK directory for the LLVM bin path.
///
/// Tries each versioned sub-directory (e.g. "default", "12", etc.) looking for
/// `openharmony/native/llvm/bin/clang`.
String? _findLlvmBinPath(String sdkPath) {
  final Directory sdkDir = globals.fs.directory(sdkPath);
  if (!sdkDir.existsSync()) {
    return null;
  }

  for (final FileSystemEntity entity in sdkDir.listSync()) {
    if (entity is! Directory) {
      continue;
    }
    final String candidate = globals.fs.path.join(
      entity.path,
      'openharmony',
      'native',
      'llvm',
      'bin',
    );
    if (globals.fs.directory(candidate).existsSync()) {
      final String clang = globals.fs.path.join(
        candidate,
        globals.platform.isWindows ? 'clang.exe' : 'clang',
      );
      if (globals.fs.file(clang).existsSync()) {
        return candidate;
      }
    }
  }
  return null;
}

Uri? _toOptionalFileUri(String path) {
  if (!globals.fs.file(path).existsSync()) {
    return null;
  }
  return Uri.file(path);
}

/// Copies native code assets into the OHOS build output under `libs/<arch>/`
/// (e.g. libs/arm64-v8a/), matching the layout expected by the HAP package.
Future<List<File>> copyNativeCodeAssetsOhos(
  Uri buildUri,
  Map<FlutterCodeAsset, KernelAsset> assetTargetLocations,
  FileSystem fileSystem,
) async {
  assert(assetTargetLocations.isNotEmpty);
  final installedFiles = <File>[];
  final archDirs = <String>[
    for (final OhosArch arch in OhosArch.values) getNameForOhosArch(arch),
  ];
  for (final String archDir in archDirs) {
    final Uri archUri = buildUri.resolve('libs/$archDir/');
    await fileSystem.directory(archUri).create(recursive: true);
  }
  for (final MapEntry<FlutterCodeAsset, KernelAsset> assetMapping in assetTargetLocations.entries) {
    final Uri source = assetMapping.key.codeAsset.file!;
    final Uri target = (assetMapping.value.path as KernelAssetAbsolutePath).uri;
    final OhosArch ohosArch = _getOhosArch(assetMapping.value.target.architecture);
    final String archDir = getNameForOhosArch(ohosArch);
    final Uri archUri = buildUri.resolve('libs/$archDir/');
    final Uri targetUri = archUri.resolveUri(target);
    final String targetFullPath = targetUri.toFilePath();
    final File installedFile = await fileSystem.file(source).copy(targetFullPath);
    installedFiles.add(installedFile);
  }
  return installedFiles;
}

Map<FlutterCodeAsset, KernelAsset> assetTargetLocationsOhos(List<FlutterCodeAsset> nativeAssets) {
  return <FlutterCodeAsset, KernelAsset>{
    for (final FlutterCodeAsset asset in nativeAssets) asset: _targetLocationOhos(asset),
  };
}

/// Converts the `path` of [asset] as output from a `build.dart` invocation to
/// the path used inside the Flutter app bundle.
KernelAsset _targetLocationOhos(FlutterCodeAsset asset) {
  final LinkMode linkMode = asset.codeAsset.linkMode;
  final KernelAssetPath kernelAssetPath;
  switch (linkMode) {
    case DynamicLoadingSystem _:
      kernelAssetPath = KernelAssetSystemPath(linkMode.uri);
    case LookupInExecutable _:
      kernelAssetPath = KernelAssetInExecutable();
    case LookupInProcess _:
      kernelAssetPath = KernelAssetInProcess();
    case DynamicLoadingBundled _:
      final String fileName = asset.codeAsset.file!.pathSegments.last;
      kernelAssetPath = KernelAssetAbsolutePath(Uri(path: fileName));
    default:
      throw Exception('Unsupported asset link mode $linkMode in asset $asset');
  }
  return KernelAsset(id: asset.codeAsset.id, target: asset.target, path: kernelAssetPath);
}
