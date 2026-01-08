/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import 'dart:async';
import 'dart:ui' as ui;
import 'package:flutter/foundation.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';

/// Data class to hold screen information
class ScreenInfo {
  ScreenInfo({
    required this.height,
    required this.devicePixelRatio,
    required this.isHeightChanged,
  });

  final double height;
  final double devicePixelRatio;
  final bool isHeightChanged;
}

/// Strategy interface for handling overflow in flex layouts
abstract class FlexOverflowStrategy {
  /// Handles overflow for the given flex render object
  void handleOverflow(
      RenderFlex renderFlex, double actualSize, double allocatedSize);

  /// Called when the render object is disposed
  void dispose();
}

/// Creates the default overflow strategy based on the platform
FlexOverflowStrategy createDefaultOverflowStrategy(Axis direction) {
  if (direction == Axis.vertical && defaultTargetPlatform == TargetPlatform.ohos && kReleaseMode) {
    return OhosFlexOverflowStrategy();
  } else {
    return DefaultFlexOverflowStrategy();
  }
}

/// Default implementation that doesn't handle overflow
class DefaultFlexOverflowStrategy implements FlexOverflowStrategy {
  @override
  void handleOverflow(
      RenderFlex renderFlex, double actualSize, double allocatedSize) {
    // No-op for default implementation
  }

  @override
  void dispose() {
    // No-op for default implementation
  }
}

/// OHOS-specific implementation for handling overflow
class OhosFlexOverflowStrategy implements FlexOverflowStrategy {
  /// Minimum scale factor set to 0.85 according to UX specifications
  /// to ensure content remains legible and not too small
  static const double _kMinScaleFactor = 0.85;

  /// Reset DPI scale value used to clear overflow state
  static const double _kResetDpiScale = -1.0;
  static const bool _kEnableFlexOverflow = bool.fromEnvironment(
    'ENABLE_FLEX_OVERFLOW',
    defaultValue: true,
  );

  // Global tracking of all overflowing RenderFlex instances
  static final Set<WeakReference<RenderFlex>> _overflowingInstances = {};
  // Global flag to track if any instance is reporting overflow
  static bool _anyInstanceReportingOverflow = false;
  // Local state for each strategy instance
  bool _isReportingOverflow = false;
  double _lastScreenHeight = 0.0;

  OhosFlexOverflowStrategy() {
    _initializeState();
  }

  /// Initializes the strategy state
  void _initializeState() {
    _isReportingOverflow = false;
    _lastScreenHeight = 0.0;
  }

  /// Checks if overflow handling should be triggered
  bool _shouldHandleOverflow(RenderFlex renderFlex) {
    return _kEnableFlexOverflow && _isOhos && _isRootVerticalFlex(renderFlex);
  }

  /// Gets screen information for overflow calculations
  ScreenInfo _getScreenInfo() {
    final ui.FlutterView view = ui.PlatformDispatcher.instance.views.first;
    return ScreenInfo(
      height: view.physicalSize.height,
      devicePixelRatio: view.devicePixelRatio,
      isHeightChanged: _lastScreenHeight != view.physicalSize.height,
    );
  }

  /// Calculates the safe scale factor for overflow handling
  double _calculateScale(
      ScreenInfo screenInfo, double actualSize, double allocatedSize) {
    final double scale = actualSize / allocatedSize;
    return scale.clamp(_kMinScaleFactor, 1.0);
  }

  /// Determines if overflow should be reported
  bool _shouldReportOverflow(RenderFlex renderFlex, ScreenInfo screenInfo) {
    // Only report overflow under the following conditions:
    // 1. There is overflow
    // 2. Never reported before, or screen height has changed
    return _getOverflowStatus(renderFlex) &&
        (!_isReportingOverflow || screenInfo.isHeightChanged);
  }

  /// Gets the overflow status from RenderFlex
  bool _getOverflowStatus(RenderFlex renderFlex) {
    // Use the public getter to check overflow status
    return renderFlex.hasOverflow;
  }

  /// Updates the screen state after processing
  set _updateScreenState(double screenHeight) {
    _lastScreenHeight = screenHeight;
  }

  /// Handles the overflow logic in a post-frame callback
  void _onPostFrame(
      RenderFlex renderFlex, double actualSize, double allocatedSize) {
    final ScreenInfo screenInfo = _getScreenInfo();
    final double scale = _calculateScale(screenInfo, actualSize, allocatedSize);

    if (_shouldReportOverflow(renderFlex, screenInfo)) {
      // Add this instance to the global tracking set
      _overflowingInstances.add(WeakReference(renderFlex));
      _isReportingOverflow = true;
      _anyInstanceReportingOverflow = true;
      _reportFlexOverflow(scale);
    }

    _updateScreenState = screenInfo.height;
  }

  /// Reports flex overflow by updating DPI through system channel
  Future<void> _reportFlexOverflow(double dpiScale) async {
    try {
      await SystemChannels.displayMetrics.invokeMethod<void>(
        'updateDpiScale',
        <String, dynamic>{
          'dpiScale': dpiScale,
        },
      );
    } catch (e) {
      // Silently handle overflow report failures
    }
  }

  /// Checks if the current platform is OHOS
  bool get _isOhos => defaultTargetPlatform == TargetPlatform.ohos;

  // Check if it's a root vertical Flex (Column)
  bool _isRootVerticalFlex(RenderFlex renderFlex) {
    // First check if it's vertical direction
    if (renderFlex.direction != Axis.vertical) {
      return false;
    }

    // Then check if there are other vertical Flex widgets in the parent chain
    RenderObject? ancestor = renderFlex.parent;
    while (ancestor != null) {
      if (ancestor is RenderFlex && ancestor.direction == Axis.vertical) {
        return false;
      }
      ancestor = ancestor.parent;
    }
    return true;
  }

  @override
  void handleOverflow(
      RenderFlex renderFlex, double actualSize, double allocatedSize) {
    if (!_shouldHandleOverflow(renderFlex)) {
      return;
    }
    _onPostFrame(renderFlex, actualSize, allocatedSize);
  }

  @override
  void dispose() {
    if (!_isReportingOverflow) {
      return;
    }

    // Remove all weak references that point to this instance or are garbage collected
    _overflowingInstances.removeWhere((weakRef) =>
        weakRef.target == null || weakRef.target?.attached == false);

    _lastScreenHeight = 0;
    _isReportingOverflow = false;

    // Check if any instances are still overflowing
    _anyInstanceReportingOverflow = _overflowingInstances.isNotEmpty;

    // Only send reset message if no more instances are overflowing
    if (!_anyInstanceReportingOverflow) {
      _reportFlexOverflow(_kResetDpiScale);
    }
  }
}
