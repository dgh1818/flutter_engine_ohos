// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:code_assets/code_assets.dart';
import 'package:hooks_runner/hooks_runner.dart';

import '../native_assets.dart';

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
