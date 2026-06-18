// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter/services.dart';
import 'split_view_config.dart';

/// Builder callback for the split-view placeholder page.
/// Set by the app layer (WidgetsApp) so that navigator.dart (a foundational
/// component) does not need to depend on Text, Image, or other UI widgets.
typedef SplitViewPlaceholderBuilder = Widget Function(BuildContext context);

class SplitViewManager extends ChangeNotifier {
  static final SplitViewManager _instance = SplitViewManager._internal();

  factory SplitViewManager() {
    return _instance;
  }

  SplitViewManager._internal() {
    // Listen to orientation changes from SystemChrome
    OrientationChangeNotifier().addListener(_onOrientationChange);
  }

  /// Handle orientation change notifications from OrientationChangeNotifier.
  void _onOrientationChange() {
    final bool isForcedLandscape = OrientationChangeNotifier().isForcedLandscape;
    final config = SplitViewConfig();
    if (config.supportLandscapeFullscreen) {
      setLandscapeFullscreen(isForcedLandscape);
    }
  }

  /// Final determined home page route name.
  String? _realHomePage;

  String? get realHomePage => _realHomePage;

  void setRealHomePage(String? homePage) {
    _realHomePage = homePage;
  }

  /// Whether split screen is active on current page.
  bool _isSplitViewActive = false;

  bool get isSplitViewActive => _isSplitViewActive;

  void setSplitViewActive(bool value) {
    if (_isSplitViewActive == value) {
      return;
    }
    _isSplitViewActive = value;
    notifyListeners();
  }

  /// Whether current page is forced fullscreen.
  bool _isForceFullscreen = false;

  bool get isForceFullscreen => _isForceFullscreen;

  void setForceFullscreen(bool value) {
    if (_isForceFullscreen == value) {
      return;
    }
    _isForceFullscreen = value;
    notifyListeners();
  }

  /// Whether app requested forced landscape mode (landscapeLeft/landscapeRight only).
  /// When true, split view should be disabled.
  bool _isForcedLandscape = false;

  bool get isForcedLandscape => _isForcedLandscape;

  void setLandscapeFullscreen(bool value) {
    if (_isForcedLandscape == value) {
      return;
    }
    _isForcedLandscape = value;
    notifyListeners();
  }

  /// Placeholder builder for split-view right panel when no detail page is shown.
  /// Set by the app layer (WidgetsApp) so that navigator.dart does not need
  /// to depend on UI widgets like Text or Image.
  SplitViewPlaceholderBuilder? _placeholderBuilder;

  SplitViewPlaceholderBuilder? get placeholderBuilder => _placeholderBuilder;

  /// Initialize the default placeholder builder using SplitViewConfig.
  /// Creates a builder that renders an app icon (from placeholderIconBytes)
  /// centered on the placeholder background. When placeholderIconBytes is
  /// null (no icon configured), the placeholder shows an empty page with
  /// only the background color.
  /// Called from WidgetsApp after config is loaded.
  void initDefaultPlaceholderBuilder() {
    _placeholderBuilder = (BuildContext context) {
      final Uint8List? bytes = SplitViewConfig().placeholderIconBytes;
      if (bytes != null) {
        return ColoredBox(
          color: const Color(0xFFf1f3f5),
          child: Center(child: Image.memory(bytes, width: 48, height: 48, fit: BoxFit.contain)),
        );
      }
      return const ColoredBox(color: Color(0xFFf1f3f5));
    };
  }

  /// Resets Navigator-related state to default values.
  ///
  /// This is called by [SplitViewNavigatorPolicy.init] when a new Navigator
  /// is created, to ensure the singleton doesn't hold stale state from a
  /// previous Navigator instance (e.g., after hot reload).
  ///
  /// Note: [_realHomePage] is NOT reset here because it is determined by
  /// the app layer (WidgetsApp) from the split-view configuration file,
  /// which persists across Navigator instances.
  void reset() {
    _isSplitViewActive = false;
    _isForceFullscreen = false;
    _isForcedLandscape = false;
    notifyListeners();
  }
}
