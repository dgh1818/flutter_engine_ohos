// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

import 'package:flutter/foundation.dart';

/// Orientation change notifier for platform-specific split view handling.
///
/// This notifier allows [SystemChrome.setPreferredOrientations] to notify
/// listeners about forced landscape changes without directly depending on
/// [SplitViewManager], maintaining proper separation of concerns.
///
/// Usage:
/// - [SystemChrome] calls [notifyLandscapeChange] when orientation preferences change
/// - [SplitViewManager] listens to this notifier and reacts accordingly
class OrientationChangeNotifier extends ChangeNotifier {
  static final OrientationChangeNotifier _instance =
      OrientationChangeNotifier._internal();

  factory OrientationChangeNotifier() => _instance;

  OrientationChangeNotifier._internal();

  bool _isForcedLandscape = false;

  /// Whether the app has requested forced landscape mode
  /// (only landscapeLeft/landscapeRight, no portrait orientations).
  bool get isForcedLandscape => _isForcedLandscape;

  /// Notify listeners about landscape mode change.
  ///
  /// Called by [SystemChrome.setPreferredOrientations] when the app
  /// requests orientation changes. This allows platform-specific handlers
  /// (like [SplitViewManager]) to react without creating direct dependencies.
  void notifyLandscapeChange(bool isForcedLandscape) {
    if (_isForcedLandscape != isForcedLandscape) {
      _isForcedLandscape = isForcedLandscape;
      notifyListeners();
    }
  }
}