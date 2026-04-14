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
  /// Creates a snapshot of the current screen state used by overflow handling.
  ScreenInfo({required this.height, required this.viewInsetsBottom, required this.isHeightChanged});

  /// Physical screen height in device pixels.
  final double height;

  /// Bottom inset reported by the platform keyboard in device pixels.
  final double viewInsetsBottom;

  /// Whether the physical screen height changed since the previous sample.
  final bool isHeightChanged;
}

/// Strategy interface for handling overflow in flex layouts
abstract class FlexOverflowStrategy {
  /// Handles overflow for the given flex render object
  void handleOverflow(RenderFlex renderFlex, double actualSize, double allocatedSize);

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
  void handleOverflow(RenderFlex renderFlex, double actualSize, double allocatedSize) {
    // No-op for default implementation
  }

  @override
  void dispose() {
    // No-op for default implementation
  }
}

/// OHOS-specific implementation for handling overflow
class OhosFlexOverflowStrategy implements FlexOverflowStrategy {
  /// Creates a new strategy instance.
  OhosFlexOverflowStrategy() {
    _initializeState();
  }

  /// Minimum scale factor set to 0.85 according to UX specifications
  /// to ensure content remains legible and not too small
  static const double _kMinScaleFactor = 0.85;

  /// OHOS release builds alone enable a platform-specific Column overflow
  /// adaptation path that shrinks viewport DPR when a root vertical
  /// [RenderFlex] reports overflow. Debug builds and other platforms do not
  /// take this path.
  ///
  /// Non-fullscreen [SearchAnchor] overlays can briefly perturb the root
  /// Column layout during open/close transitions, which makes the OHOS
  /// release-only adaptation treat that temporary layout jitter as a real
  /// overflow and immediately collapse the search view again.
  ///
  /// A short confirmation window lets transient overlay pulses disappear before
  /// the OHOS DPR shrink is committed, while still preserving the original
  /// overflow adaptation for real, persistent Column overflow.
  ///
  /// This is intentionally not a blanket suppression for every keyboard-driven
  /// relayout. If the page really overflows after the keyboard appears, OHOS
  /// should still be allowed to apply its original Column overflow adaptation.
  ///
  /// The implementation also suppresses the first overflow reported during the
  /// initial keyboard transition from zero to non-zero viewInsets. That extra
  /// guard is limited to a single fresh-page interaction and exists because the
  /// first SearchAnchor open can overlap with keyboard startup on OHOS release.
  /// It also suppresses the first overflow reported immediately after typing
  /// begins while the keyboard is already visible on a fresh page, because the
  /// first non-fullscreen SearchAnchor editing update can still overlap with
  /// the initial settled keyboard layout even when the earlier keyboard-open
  /// suppression did not fire.
  ///
  /// Trade-off: the first real overflow on a newly built page now waits for a
  /// 180ms confirmation window before shrinking, and the first keyboard-open
  /// and first editing-time overflow on a fresh page are each ignored once, so
  /// the adaptation can be delayed slightly on OHOS release builds.
  ///
  /// Residual risk: if OHOS emits another delayed transient pulse after this
  /// short suppression window has already elapsed, the first interaction still
  /// falls back to the original release-mode adaptation path.
  static const Duration _kOverflowReportConfirmation = Duration(milliseconds: 180);
  static const Duration _kInitialOverflowSuppressionCooldown = Duration(milliseconds: 600);

  /// Reset DPI scale value used to clear overflow state
  static const double _kResetDpiScale = -1.0;
  static const bool _kEnableFlexOverflow = bool.fromEnvironment(
    'ENABLE_FLEX_OVERFLOW',
    defaultValue: true,
  );

  // Global tracking of all overflowing RenderFlex instances
  static final Set<WeakReference<RenderFlex>> _overflowingInstances = <WeakReference<RenderFlex>>{};
  // Root Flexes that are still inside the confirmation window.
  static final Set<WeakReference<RenderFlex>> _pendingOverflowInstances =
      <WeakReference<RenderFlex>>{};
  // Global flag to track if any instance is reporting overflow
  static bool _anyInstanceReportingOverflow = false;
  // Local state for each strategy instance
  bool _isReportingOverflow = false;
  double _lastScreenHeight = 0.0;
  double _lastScaleFactor = 1.0;
  double _lastViewInsetsBottom = 0.0;
  bool _hasSuppressedInitialKeyboardOverflow = false;
  bool _hasSuppressedInitialEditingOverflow = false;
  DateTime? _initialOverflowSuppressionDeadline;
  Timer? _suppressedOverflowTimer;
  WeakReference<RenderFlex>? _suppressedOverflowRenderFlex;
  double _suppressedScaleFactor = 1.0;
  Timer? _pendingOverflowTimer;
  WeakReference<RenderFlex>? _pendingOverflowRenderFlex;
  double _pendingScaleFactor = 1.0;

  /// Initializes the strategy state
  void _initializeState() {
    _cancelPendingOverflowReport();
    _isReportingOverflow = false;
    _lastScreenHeight = 0.0;
    _lastScaleFactor = 1.0;
    _lastViewInsetsBottom = 0.0;
    _hasSuppressedInitialKeyboardOverflow = false;
    _hasSuppressedInitialEditingOverflow = false;
    _initialOverflowSuppressionDeadline = null;
    _suppressedScaleFactor = 1.0;
    _pendingScaleFactor = 1.0;
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
      viewInsetsBottom: view.viewInsets.bottom,
      // Treat the first sampled height as baseline state instead of a real
      // height transition. Otherwise the first SearchAnchor open can bypass
      // the confirmation window simply because `_lastScreenHeight` starts at 0.
      isHeightChanged: _lastScreenHeight > 0.0 && _lastScreenHeight != view.physicalSize.height,
    );
  }

  /// Calculates the safe scale factor for overflow handling
  double _calculateScale(ScreenInfo screenInfo, double actualSize, double allocatedSize) {
    final double scale = actualSize / allocatedSize;
    return scale.clamp(_kMinScaleFactor, 1.0);
  }

  /// Determines if overflow should be reported
  bool _shouldReportOverflow(RenderFlex renderFlex, ScreenInfo screenInfo, double scale) {
    // Only report overflow under the following conditions:
    // 1. There is overflow
    // 2. Never reported before, or screen height has changed
    // 3. Scale is within valid range and smaller than previous scale
    return _getOverflowStatus(renderFlex) &&
        (!_isReportingOverflow ||
            screenInfo.isHeightChanged ||
            (scale >= _kMinScaleFactor && scale < _lastScaleFactor));
  }

  /// Gets the overflow status from RenderFlex
  bool _getOverflowStatus(RenderFlex renderFlex) {
    // Use the public getter to check overflow status
    return renderFlex.hasOverflow;
  }

  /// Updates cached screen state after processing.
  void _updateScreenState(ScreenInfo screenInfo) {
    _lastScreenHeight = screenInfo.height;
    _lastViewInsetsBottom = screenInfo.viewInsetsBottom;
  }

  /// Handles the overflow logic in a post-frame callback
  void _onPostFrame(RenderFlex renderFlex, double actualSize, double allocatedSize) {
    final ScreenInfo screenInfo = _getScreenInfo();
    final double scale = _calculateScale(screenInfo, actualSize, allocatedSize);

    if (_shouldReportOverflow(renderFlex, screenInfo, scale)) {
      if (_shouldSuppressInitialKeyboardOverflow(screenInfo)) {
        _hasSuppressedInitialKeyboardOverflow = true;
        _extendInitialOverflowSuppressionWindow();
        _deferSuppressedOverflow(renderFlex, scale);
      } else if (_shouldSuppressInitialEditingOverflow(screenInfo)) {
        _hasSuppressedInitialEditingOverflow = true;
        _extendInitialOverflowSuppressionWindow();
        _deferSuppressedOverflow(renderFlex, scale);
      } else if (_shouldSuppressInitialOverflowCooldown(screenInfo)) {
        _deferSuppressedOverflow(renderFlex, scale);
      } else if (_shouldConfirmOverflowBeforeReporting(screenInfo)) {
        _scheduleOverflowReport(renderFlex, scale);
      } else {
        _commitOverflowReport(renderFlex, screenInfo, scale);
      }
    } else {
      _cancelPendingOverflowReport();
    }

    _updateScreenState(screenInfo);
  }

  /// Returns true when the first overflow should survive a confirmation window
  /// before mutating viewport DPR.
  bool _shouldConfirmOverflowBeforeReporting(ScreenInfo screenInfo) {
    return !screenInfo.isHeightChanged;
  }

  /// Suppresses the first keyboard-open overflow once on a fresh page.
  ///
  /// This keeps the initial SearchAnchor open interaction stable when the
  /// keyboard transition from zero to non-zero viewInsets overlaps with the
  /// overlay layout change. Later interactions still use the normal OHOS
  /// overflow adaptation path.
  bool _shouldSuppressInitialKeyboardOverflow(ScreenInfo screenInfo) {
    return !_isReportingOverflow &&
        !_hasSuppressedInitialKeyboardOverflow &&
        _lastViewInsetsBottom == 0.0 &&
        screenInfo.viewInsetsBottom > 0.0;
  }

  /// Suppresses the first editing-time overflow once after the keyboard has
  /// already settled on a fresh page.
  bool _shouldSuppressInitialEditingOverflow(ScreenInfo screenInfo) {
    return !_isReportingOverflow &&
        !_hasSuppressedInitialEditingOverflow &&
        _lastScaleFactor == 1.0 &&
        _lastViewInsetsBottom > 0.0 &&
        screenInfo.viewInsetsBottom > 0.0 &&
        !screenInfo.isHeightChanged;
  }

  /// Keeps a short suppression window alive after the first keyboard-open or
  /// first editing-time overflow so follow-up relayout pulses cannot immediately
  /// re-arm OHOS DPR shrink on the same fresh-page interaction.
  bool _shouldSuppressInitialOverflowCooldown(ScreenInfo screenInfo) {
    final DateTime? deadline = _initialOverflowSuppressionDeadline;
    if (deadline == null) {
      return false;
    }
    return !_isReportingOverflow &&
        _lastScaleFactor == 1.0 &&
        screenInfo.viewInsetsBottom > 0.0 &&
        DateTime.now().isBefore(deadline);
  }

  void _extendInitialOverflowSuppressionWindow() {
    _initialOverflowSuppressionDeadline = DateTime.now().add(_kInitialOverflowSuppressionCooldown);
  }

  /// Defers a suppressed overflow and rechecks it once the suppression window
  /// ends, so genuine release-mode overflow adaptation is delayed rather than
  /// dropped outright.
  void _deferSuppressedOverflow(RenderFlex renderFlex, double scale) {
    _cancelPendingOverflowReport();
    final RenderFlex? previousSuppressedRenderFlex = _suppressedOverflowRenderFlex?.target;
    if (previousSuppressedRenderFlex != null && previousSuppressedRenderFlex != renderFlex) {
      _removePendingInstance(previousSuppressedRenderFlex);
    }
    _suppressedScaleFactor = scale < _suppressedScaleFactor ? scale : _suppressedScaleFactor;
    _suppressedOverflowRenderFlex = WeakReference<RenderFlex>(renderFlex);
    _addPendingInstance(renderFlex);

    final DateTime? deadline = _initialOverflowSuppressionDeadline;
    final Duration delay = deadline == null ? Duration.zero : deadline.difference(DateTime.now());
    _suppressedOverflowTimer?.cancel();
    _suppressedOverflowTimer = Timer(delay.isNegative ? Duration.zero : delay, () {
      final RenderFlex? suppressedRenderFlex = _suppressedOverflowRenderFlex?.target;
      _suppressedOverflowTimer = null;
      _suppressedOverflowRenderFlex = null;
      final double suppressedScale = _suppressedScaleFactor;
      _suppressedScaleFactor = 1.0;
      if (suppressedRenderFlex == null) {
        return;
      }
      if (!suppressedRenderFlex.attached) {
        _removePendingInstance(suppressedRenderFlex);
        return;
      }

      final ScreenInfo screenInfo = _getScreenInfo();
      final DateTime? currentDeadline = _initialOverflowSuppressionDeadline;
      if (currentDeadline != null && DateTime.now().isBefore(currentDeadline)) {
        _deferSuppressedOverflow(suppressedRenderFlex, suppressedScale);
        return;
      }
      _removePendingInstance(suppressedRenderFlex);
      if (!_shouldReportOverflow(suppressedRenderFlex, screenInfo, suppressedScale)) {
        return;
      }

      if (_shouldConfirmOverflowBeforeReporting(screenInfo)) {
        _scheduleOverflowReport(suppressedRenderFlex, suppressedScale);
      } else {
        _commitOverflowReport(suppressedRenderFlex, screenInfo, suppressedScale);
      }
    });
  }

  /// Schedules a one-shot confirmation for the initial overflow report.
  void _scheduleOverflowReport(RenderFlex renderFlex, double scale) {
    _pendingScaleFactor = scale < _pendingScaleFactor ? scale : _pendingScaleFactor;
    _pendingOverflowRenderFlex ??= WeakReference<RenderFlex>(renderFlex);
    if (_pendingOverflowTimer != null) {
      return;
    }

    _addPendingInstance(renderFlex);
    _pendingOverflowTimer = Timer(_kOverflowReportConfirmation, () {
      _pendingOverflowTimer = null;
      _removePendingInstance(renderFlex);
      _pendingOverflowRenderFlex = null;
      if (!renderFlex.attached) {
        return;
      }

      final ScreenInfo screenInfo = _getScreenInfo();
      final double scale = _pendingScaleFactor;
      if (!_shouldReportOverflow(renderFlex, screenInfo, scale)) {
        return;
      }
      _commitOverflowReport(renderFlex, screenInfo, scale);
    });
  }

  /// Commits a confirmed overflow report and updates the global tracking state.
  void _commitOverflowReport(RenderFlex renderFlex, ScreenInfo screenInfo, double scale) {
    _cancelSuppressedOverflowReport();
    final bool isAlreadyTracked = _overflowingInstances.any(
      (WeakReference<RenderFlex> weakRef) => weakRef.target == renderFlex,
    );
    if (!isAlreadyTracked) {
      _overflowingInstances.add(WeakReference<RenderFlex>(renderFlex));
    }
    _isReportingOverflow = true;
    _anyInstanceReportingOverflow = true;
    _reportFlexOverflow(scale);
    _lastScaleFactor = scale;
    _updateScreenState(screenInfo);
  }

  void _addPendingInstance(RenderFlex renderFlex) {
    final bool isAlreadyTracked = _pendingOverflowInstances.any(
      (WeakReference<RenderFlex> weakRef) => weakRef.target == renderFlex,
    );
    if (!isAlreadyTracked) {
      _pendingOverflowInstances.add(WeakReference<RenderFlex>(renderFlex));
    }
  }

  void _removePendingInstance(RenderFlex renderFlex) {
    _pendingOverflowInstances.removeWhere(
      (WeakReference<RenderFlex> weakRef) => weakRef.target == null || weakRef.target == renderFlex,
    );
  }

  /// Clears any overflow report that has not been committed yet.
  void _cancelPendingOverflowReport() {
    final RenderFlex? pendingRenderFlex = _pendingOverflowRenderFlex?.target;
    _pendingOverflowTimer?.cancel();
    _pendingOverflowTimer = null;
    _pendingOverflowRenderFlex = null;
    _pendingScaleFactor = 1.0;
    if (pendingRenderFlex != null) {
      _removePendingInstance(pendingRenderFlex);
    }
    _pendingOverflowInstances.removeWhere(
      (WeakReference<RenderFlex> weakRef) =>
          weakRef.target == null || weakRef.target?.attached == false,
    );
    _cleanupOverflowTrackingAndMaybeReset();
  }

  void _cancelSuppressedOverflowReport() {
    final RenderFlex? suppressedRenderFlex = _suppressedOverflowRenderFlex?.target;
    _suppressedOverflowTimer?.cancel();
    _suppressedOverflowTimer = null;
    _suppressedOverflowRenderFlex = null;
    _suppressedScaleFactor = 1.0;
    if (suppressedRenderFlex != null) {
      _removePendingInstance(suppressedRenderFlex);
    }
  }

  /// Reports flex overflow by updating DPI through system channel
  Future<void> _reportFlexOverflow(double dpiScale) async {
    try {
      await SystemChannels.displayMetrics.invokeMethod<void>('updateDpiScale', <String, dynamic>{
        'dpiScale': dpiScale,
      });
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
  void handleOverflow(RenderFlex renderFlex, double actualSize, double allocatedSize) {
    if (actualSize <= 0 || allocatedSize <= 0) {
      return;
    }
    if (!_shouldHandleOverflow(renderFlex)) {
      return;
    }
    _onPostFrame(renderFlex, actualSize, allocatedSize);
  }

  @override
  void dispose() {
    _cancelSuppressedOverflowReport();
    _cancelPendingOverflowReport();
    if (!_isReportingOverflow) {
      return;
    }

    _lastScreenHeight = 0;
    _isReportingOverflow = false;
    _cleanupOverflowTrackingAndMaybeReset();
  }

  void _cleanupOverflowTrackingAndMaybeReset() {
    _overflowingInstances.removeWhere(
      (WeakReference<RenderFlex> weakRef) =>
          weakRef.target == null || weakRef.target?.attached == false,
    );
    _pendingOverflowInstances.removeWhere(
      (WeakReference<RenderFlex> weakRef) =>
          weakRef.target == null || weakRef.target?.attached == false,
    );
    _anyInstanceReportingOverflow = _overflowingInstances.isNotEmpty;
    if (!_anyInstanceReportingOverflow && _pendingOverflowInstances.isEmpty) {
      _reportFlexOverflow(_kResetDpiScale);
    }
  }
}
