/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import 'dart:async';
import 'dart:ui' as ui;
import 'package:flutter/foundation.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/scheduler.dart';
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

/// Immutable snapshot of [OhosFlexOverflowStrategy]'s internal state.
///
/// Used solely by tests to verify intermediate state that cannot be observed
/// through the public API alone (e.g. timer flags, instance tracking counts).
@visibleForTesting
class OhosFlexOverflowDebugState {
  /// Creates a snapshot of the current overflow strategy state.
  const OhosFlexOverflowDebugState({
    required this.isReportingOverflow,
    required this.lastScaleFactor,
    required this.lastScreenHeight,
    required this.lastViewInsetsBottom,
    required this.hasSuppressedInitialKeyboardOverflow,
    required this.hasSuppressedInitialEditingOverflow,
    required this.overflowingInstanceCount,
    required this.frameSampleCount,
    required this.hasPendingOverflowTimer,
    required this.hasSuppressedOverflowTimer,
    required this.suppressionExpired,
    required this.confirmationExpired,
    required this.routeChanged,
  });

  /// Whether a DPR shrink has been committed and is currently active.
  final bool isReportingOverflow;

  /// The last scale factor reported to the native side via [SystemChannels.displayMetrics].
  final double lastScaleFactor;

  /// The last sampled physical screen height in device pixels.
  final double lastScreenHeight;

  /// The last sampled bottom view inset (keyboard) in device pixels.
  final double lastViewInsetsBottom;

  /// Whether the initial keyboard-open overflow suppression has fired.
  final bool hasSuppressedInitialKeyboardOverflow;

  /// Whether the initial editing-time overflow suppression has fired.
  final bool hasSuppressedInitialEditingOverflow;

  /// Number of [RenderFlex] instances currently tracked as overflowing.
  final int overflowingInstanceCount;

  /// Number of overflow samples collected in the current frame buffer.
  final int frameSampleCount;

  /// Whether the 180ms confirmation timer is currently running.
  final bool hasPendingOverflowTimer;

  /// Whether the 600ms suppression timer is currently running.
  final bool hasSuppressedOverflowTimer;

  /// Whether the suppression timer has expired (overflow persists past 600ms).
  final bool suppressionExpired;

  /// Whether the confirmation timer has expired (overflow persists past 180ms).
  final bool confirmationExpired;

  /// Whether a route change has been signalled since the last DPR reset.
  final bool routeChanged;
}

/// Strategy interface for handling overflow in flex layouts
abstract class FlexOverflowStrategy {
  /// Handles overflow for the given flex render object
  void handleOverflow(RenderFlex renderFlex, double actualSize, double allocatedSize);

  /// Called when the render object is disposed.
  ///
  /// [renderFlex] is the instance being disposed, so the strategy can
  /// precisely remove it from any tracking sets.
  void dispose(RenderFlex renderFlex);
}

/// Creates the default overflow strategy based on the platform
FlexOverflowStrategy createDefaultOverflowStrategy(Axis direction) {
  if (OhosFlexOverflowStrategy.isFlexOverflowEnabled && direction == Axis.vertical) {
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
  void dispose(RenderFlex renderFlex) {
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

  /// Combined feature gate: the flex overflow adaptation is enabled only
  /// when the compile-time toggle ([_kEnableFlexOverflow]), the runtime
  /// gate ([kReleaseMode]), and the platform check
  /// ([defaultTargetPlatform] == [TargetPlatform.ohos]) are all true.
  ///
  /// Initialized once at class load. Tests override it via
  /// [debugDynamicDpiEnabled] and reset via [resetState].
  static bool isFlexOverflowEnabled =
      _kEnableFlexOverflow && kReleaseMode && defaultTargetPlatform == TargetPlatform.ohos;

  /// Test-only override for [isFlexOverflowEnabled].
  ///
  /// Set to `true` in tests that need to exercise the release-mode-only
  /// code paths (e.g. [notifyRouteChanged]) without running in an actual
  /// release build. Reset via [resetState].
  @visibleForTesting
  static bool get debugDynamicDpiEnabled => isFlexOverflowEnabled;
  @visibleForTesting
  static set debugDynamicDpiEnabled(bool value) => isFlexOverflowEnabled = value;

  // Global tracking of all overflowing RenderFlex instances. Used solely by
  // [dispose] to detect when every tracked instance has been removed, which
  // gates the DPR reset.
  static final Set<WeakReference<RenderFlex>> _overflowingInstances = <WeakReference<RenderFlex>>{};

  // ── Per-frame aggregation state ──────────────────────────────────────
  // Scale factors keyed by RenderFlex instance. Only overflowing instances
  // are stored; a non-overflow result removes the entry. The post-frame
  // callback reads all entries and issues at most one _reportFlexOverflow per
  // frame (the minimum scale across all overflowing instances). Entries
  // persist across frames so that timer-scheduled frames (which may not
  // trigger performLayout) still see the last known overflow state. Entries
  // are removed by [dispose] or when the instance stops overflowing.
  static final Map<RenderFlex, double> _frameSamples = <RenderFlex, double>{};
  static bool _frameAggregationScheduled = false;

  // ── Centralised state ───────────────────────────────────────────────
  static bool _isReportingOverflow = false;
  static double _lastScreenHeight = 0.0;
  static double _lastScaleFactor = 1.0;
  static double _lastViewInsetsBottom = 0.0;
  static bool _hasSuppressedInitialKeyboardOverflow = false;
  static bool _hasSuppressedInitialEditingOverflow = false;
  static DateTime? _initialOverflowSuppressionDeadline;
  static Timer? _suppressedOverflowTimer;
  static Timer? _pendingOverflowTimer;
  // Flags set by timer callbacks. The next _aggregateAndReport pass checks
  // them to decide whether to advance to the next phase (confirmation or
  // commit). This avoids accessing RenderFlex state from timer callbacks.
  static bool _suppressionExpired = false;
  static bool _confirmationExpired = false;

  // ── Route-change tracking ───────────────────────────────────────────
  // Set by the navigation layer (via [notifyRouteChanged]) whenever a
  // push/pop/replace/remove occurs. DPR reset requires this flag to be true
  // AND all tracked instances to be disposed, preventing premature resets.
  static bool _routeChanged = false;

  /// Called by the navigation layer to signal that a route transition
  /// (push/pop/replace/remove) has occurred.
  ///
  /// This is a no-op when the flex overflow feature is disabled (via
  /// `ENABLE_FLEX_OVERFLOW`) or not in release mode, matching the conditions
  /// in [createDefaultOverflowStrategy].
  static void notifyRouteChanged() {
    if (!isFlexOverflowEnabled) {
      return;
    }
    _routeChanged = true;
    // Reset per-page suppression flags so they don't leak to the new page.
    // Without this, a suppression that fired on the previous page (but never
    // committed a DPR shrink) would prevent the same suppression from firing
    // on the new page.
    _hasSuppressedInitialKeyboardOverflow = false;
    _hasSuppressedInitialEditingOverflow = false;
    _initialOverflowSuppressionDeadline = null;
    // Cancel any running timers and clear their expiry flags. Otherwise a
    // suppression timer started on the previous page would continue running
    // and fire on the new page, prematurely setting _suppressionExpired and
    // bypassing the suppression phase. Similarly, a confirmation timer from
    // the previous page could fire and trigger an unwanted DPR shrink on the
    // new page.
    _suppressedOverflowTimer?.cancel();
    _suppressedOverflowTimer = null;
    _pendingOverflowTimer?.cancel();
    _pendingOverflowTimer = null;
    _suppressionExpired = false;
    _confirmationExpired = false;
  }

  /// Initializes the strategy state.
  ///
  /// All state is static (centralised), so this only needs to run once for
  /// the first instance. Subsequent instances reuse the existing global
  /// state.
  static bool _stateInitialized = false;
  void _initializeState() {
    if (_stateInitialized) {
      return;
    }
    _stateInitialized = true;
    _suppressedOverflowTimer?.cancel();
    _suppressedOverflowTimer = null;
    _pendingOverflowTimer?.cancel();
    _pendingOverflowTimer = null;
    _isReportingOverflow = false;
    _lastScreenHeight = 0.0;
    _lastScaleFactor = 1.0;
    _lastViewInsetsBottom = 0.0;
    _hasSuppressedInitialKeyboardOverflow = false;
    _hasSuppressedInitialEditingOverflow = false;
    _initialOverflowSuppressionDeadline = null;
    _suppressionExpired = false;
    _confirmationExpired = false;
  }

  /// Checks if overflow handling should be triggered
  bool _shouldHandleOverflow(RenderFlex renderFlex) {
    return isFlexOverflowEnabled && _isRootVerticalFlex(renderFlex);
  }

  /// Gets screen information for overflow calculations
  ScreenInfo _getScreenInfo() {
    final ui.FlutterView view = ServicesBinding.instance.platformDispatcher.views.first;
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
    return clampDouble(scale, _kMinScaleFactor, 1.0);
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

  /// Collects this instance's overflow scale into [_frameSamples] for
  /// per-frame aggregation.
  ///
  /// Each [RenderFlex.performLayout] calls this at the end. The computed
  /// scale is stored in [_frameSamples] and a single post-frame callback
  /// is registered. The callback aggregates all samples — taking the
  /// minimum scale across all overflowing instances — and issues at most
  /// one [_reportFlexOverflow] per frame. This ensures that when multiple
  /// root Columns overflow in the same frame, only a single DPR report is
  /// sent.
  void _onPostFrame(RenderFlex renderFlex, double actualSize, double allocatedSize) {
    final ScreenInfo screenInfo = _getScreenInfo();
    final double scale = _calculateScale(screenInfo, actualSize, allocatedSize);
    final bool hasOverflow = _getOverflowStatus(renderFlex);

    if (hasOverflow) {
      _frameSamples[renderFlex] = scale;
    } else {
      // No overflow this frame: remove the entry so that
      // _aggregateAndReport sees the current (non-overflowing) state.
      _frameSamples.remove(renderFlex);
    }

    _scheduleAggregation();
  }

  /// Schedules a single [_aggregateAndReport] pass via a post-frame callback.
  ///
  /// This is the **sole** scheduling point for aggregation. Both
  /// [_onPostFrame] (during layout) and [dispose] (when the last tracked
  /// instance is removed) call this method. The [_frameAggregationScheduled]
  /// guard ensures at most one callback is registered per frame, regardless
  /// of how many trigger sites call this.
  ///
  /// Having a single scheduling method (rather than each call site
  /// independently registering callbacks) makes it clear that there is one
  /// aggregation pass per frame, not two independent entry points that
  /// could conflict.
  void _scheduleAggregation() {
    if (_frameAggregationScheduled) {
      return;
    }
    _frameAggregationScheduled = true;
    SchedulerBinding.instance.addPostFrameCallback((_) {
      _frameAggregationScheduled = false;
      _aggregateAndReport();
    });
  }

  /// Aggregates all stored samples and issues at most one overflow report.
  ///
  /// Runs in a post-frame callback. It examines all stored samples,
  /// determines the minimum scale among overflowing instances, and runs
  /// the decision logic (suppress / confirm / commit / reset) exactly
  /// once. Samples persist across frames, so even timer-scheduled frames
  /// that do not trigger [performLayout] can still evaluate overflow state.
  void _aggregateAndReport() {
    final ScreenInfo screenInfo = _getScreenInfo();

    // Height changed (fold/unfold/rotation): native side already reset DPR.
    // Clear Dart-side scaling state so future overflow can be re-detected.
    // _frameSamples is intentionally NOT cleared — old samples persist so
    // timer-scheduled empty frames still see overflow; the next
    // performLayout will overwrite them with fresh values.
    if (screenInfo.isHeightChanged) {
      _suppressedOverflowTimer?.cancel();
      _suppressedOverflowTimer = null;
      _pendingOverflowTimer?.cancel();
      _pendingOverflowTimer = null;
      _suppressionExpired = false;
      _confirmationExpired = false;
      _overflowingInstances.clear();
      _isReportingOverflow = false;
      _lastScaleFactor = 1.0;
      _hasSuppressedInitialKeyboardOverflow = false;
      _hasSuppressedInitialEditingOverflow = false;
      _initialOverflowSuppressionDeadline = null;
    }

    // Collect scales of all attached overflowing instances.
    final overflowingScales = <double>[];
    for (final MapEntry<RenderFlex, double> entry in _frameSamples.entries) {
      if (entry.key.attached) {
        overflowingScales.add(entry.value);
      }
    }

    // Compute the minimum scale among all overflowing instances.
    final double minScale = overflowingScales.isEmpty
        ? 1.0
        : overflowingScales.reduce((double a, double b) => a < b ? a : b);

    // Determine whether any overflow should be reported.
    final bool shouldReport =
        overflowingScales.isNotEmpty &&
        (!_isReportingOverflow ||
            screenInfo.isHeightChanged ||
            (minScale >= _kMinScaleFactor && minScale < _lastScaleFactor));

    if (shouldReport) {
      if (screenInfo.isHeightChanged) {
        // Screen height changed (fold/unfold/rotation) — commit
        // immediately. Screen changes are the target scenario for this
        // feature: the native side has already reset DPR, and the app
        // needs the scaled DPR applied as fast as possible to avoid
        // visible overflow during the transition. Waiting 180ms here
        // would cause a noticeable flash of overflow content.
        _commitOverflowReport(screenInfo, minScale);
      } else if (_confirmationExpired) {
        // 180ms confirmation window has elapsed and overflow persists.
        _confirmationExpired = false;
        _commitOverflowReport(screenInfo, minScale);
      } else if (_suppressionExpired) {
        // 600ms suppression window has elapsed and overflow persists.
        // Advance to the 180ms confirmation phase.
        _suppressionExpired = false;
        _startConfirmationTimer();
      } else if (_shouldSuppressInitialKeyboardOverflow(screenInfo)) {
        // First keyboard-open overflow on a fresh page: suppress once,
        // extend the 600ms deadline, and start the suppression timer.
        // This absorbs the transient layout jitter from the keyboard
        // transition (viewInsets 0 → non-zero) without committing a
        // DPR shrink.
        _hasSuppressedInitialKeyboardOverflow = true;
        _extendInitialOverflowSuppressionWindow();
        _startSuppressionTimer();
      } else if (_shouldSuppressInitialEditingOverflow(screenInfo)) {
        // First editing-time overflow after the keyboard has settled:
        // suppress once, extend the 600ms deadline, and start the
        // suppression timer. The first SearchAnchor editing update can
        // still overlap with the initial settled keyboard layout even
        // when the keyboard-open suppression did not fire.
        _hasSuppressedInitialEditingOverflow = true;
        _extendInitialOverflowSuppressionWindow();
        _startSuppressionTimer();
      } else if (_shouldSuppressInitialOverflowCooldown(screenInfo)) {
        // 600ms cooldown: keep the suppression timer running but do NOT
        // extend the deadline. The cooldown is a fixed window that lets
        // the initial suppression expire so real overflow can eventually
        // be detected. Extending here would make the deadline never
        // arrive, trapping the strategy in perpetual suppression.
        _startSuppressionTimer();
      } else if (_pendingOverflowTimer == null) {
        _startConfirmationTimer();
      }
    } else if (overflowingScales.isEmpty) {
      // No instance is overflowing this frame — cancel all timers and clear
      // flags so transient pulses don't carry over.
      _cancelPendingOverflowReport();
      _cancelSuppressionTimer();
      // Reset DPR when a route transition occurred and all tracked
      // overflowing instances have been disposed. This covers the case
      // where the overflowing route was popped: its RenderFlex is disposed
      // and removed from [_overflowingInstances] by [dispose]. The set
      // becomes empty and the reset fires here.
      //
      // The [_overflowingInstances.isEmpty] check is essential to prevent
      // the oscillation loop. Without it, the following cycle occurs:
      //
      //   overflow → shrink DPR → layout changes → SearchAnchor's
      //   PopupRoute is dismissed → notifyRouteChanged re-arms
      //   _routeChanged → overflow disappears (DPR already shrunk) →
      //   reset DPR → overflow recurs → repeat.
      //
      // The root cause is that SearchAnchor uses PopupRoute internally.
      // A DPR shrink can dismiss the PopupRoute, which triggers
      // notifyRouteChanged as a *side effect* of the shrink — not a
      // genuine user navigation. The [_overflowingInstances.isEmpty]
      // check ensures the reset only fires when the overflowing
      // RenderFlex has truly been disposed (a real route pop), not when
      // it is still attached but merely stopped overflowing because of
      // the DPR shrink.
      //
      // Known limitation: if a non-disposed RenderFlex (e.g. the home
      // page Column that overflowed when the keyboard was open on a
      // pushed detail page) is still attached but no longer overflowing,
      // it stays in [_overflowingInstances] because only [dispose]
      // removes entries. In this case the reset will not fire and the
      // scaled DPR will leak until the RenderFlex is eventually disposed.
      // This is accepted as a trade-off to avoid the SearchAnchor
      // oscillation, which is the more severe user-visible issue.
      if (_isReportingOverflow && _overflowingInstances.isEmpty && _routeChanged) {
        _overflowingInstances.clear();
        _reportFlexOverflow(_kResetDpiScale);
        _isReportingOverflow = false;
        _lastScaleFactor = 1.0;
        _hasSuppressedInitialKeyboardOverflow = false;
        _hasSuppressedInitialEditingOverflow = false;
        _initialOverflowSuppressionDeadline = null;
        _routeChanged = false;
      }
    }

    _updateScreenState(screenInfo);
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

  /// Starts (or keeps) the 600ms suppression timer.
  ///
  /// The timer callback only sets [_suppressionExpired] = true; it does not
  /// access any RenderFlex. The next [_aggregateAndReport] pass checks the
  /// flag and, if overflow persists, advances to the 180ms confirmation
  /// phase.
  void _startSuppressionTimer() {
    if (_suppressedOverflowTimer != null) {
      return; // already running
    }
    final DateTime? deadline = _initialOverflowSuppressionDeadline;
    final Duration delay = deadline == null ? Duration.zero : deadline.difference(DateTime.now());
    _suppressedOverflowTimer = Timer(delay.isNegative ? Duration.zero : delay, () {
      _suppressedOverflowTimer = null;
      _suppressionExpired = true;
      _scheduleAggregation();
      SchedulerBinding.instance.ensureVisualUpdate();
    });
  }

  /// Starts (or keeps) the 180ms confirmation timer.
  ///
  /// The timer callback only sets [_confirmationExpired] = true; it does not
  /// access any RenderFlex. The next [_aggregateAndReport] pass checks the
  /// flag and, if overflow persists, commits the DPR shrink using the current
  /// frame's minScale.
  void _startConfirmationTimer() {
    if (_pendingOverflowTimer != null) {
      return; // already running
    }
    _pendingOverflowTimer = Timer(_kOverflowReportConfirmation, () {
      _pendingOverflowTimer = null;
      _confirmationExpired = true;
      _scheduleAggregation();
      SchedulerBinding.instance.ensureVisualUpdate();
    });
  }

  /// Commits a confirmed overflow report and updates the global tracking
  /// state.
  ///
  /// Reports the current frame's [minScale] and tracks all
  /// currently-overflowing instances in [_overflowingInstances] so that
  /// [dispose] can detect when every instance has been removed.
  void _commitOverflowReport(ScreenInfo screenInfo, double minScale) {
    _cancelSuppressionTimer();
    _cancelPendingOverflowReport();
    _trackOverflowingInstances();
    _isReportingOverflow = true;
    _reportFlexOverflow(minScale);
    _lastScaleFactor = minScale;
    // Consume any pending route-change flag: we are now scaling for the
    // current page, so a future overflow-disappear should NOT trigger a
    // reset unless a *new* navigation occurs.
    _routeChanged = false;
    _updateScreenState(screenInfo);
  }

  /// Adds every attached overflowing instance from the current frame's
  /// samples to [_overflowingInstances]. Called at commit time so that
  /// [dispose] can later detect when all tracked instances are gone.
  void _trackOverflowingInstances() {
    for (final RenderFlex renderFlex in _frameSamples.keys) {
      if (renderFlex.attached) {
        final bool isAlreadyTracked = _overflowingInstances.any(
          (WeakReference<RenderFlex> weakRef) => weakRef.target == renderFlex,
        );
        if (!isAlreadyTracked) {
          _overflowingInstances.add(WeakReference<RenderFlex>(renderFlex));
        }
      }
    }
  }

  /// Cancels the 180ms confirmation timer and clears its flag.
  void _cancelPendingOverflowReport() {
    _pendingOverflowTimer?.cancel();
    _pendingOverflowTimer = null;
    _confirmationExpired = false;
  }

  /// Cancels the 600ms suppression timer and clears its flag.
  void _cancelSuppressionTimer() {
    _suppressedOverflowTimer?.cancel();
    _suppressedOverflowTimer = null;
    _suppressionExpired = false;
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
    // Collect this instance's overflow sample into the frame buffer.
    // The actual decision and reporting happens once per frame in
    // _aggregateAndReport, via a post-frame callback.
    _onPostFrame(renderFlex, actualSize, allocatedSize);
  }

  @override
  void dispose(RenderFlex renderFlex) {
    // Remove the disposed RenderFlex from the sample map and tracking set.
    _frameSamples.remove(renderFlex);
    _overflowingInstances.removeWhere(
      (WeakReference<RenderFlex> weakRef) =>
          weakRef.target == null || identical(weakRef.target, renderFlex),
    );

    // Schedule a final aggregation pass to reset DPR when all tracked
    // instances are gone. [ensureVisualUpdate] is needed because dispose
    // may run outside of a frame — [addPostFrameCallback] alone does not
    // schedule a frame, so without this the callback could never fire and
    // the scaled DPR would leak.
    if (_isReportingOverflow && _overflowingInstances.isEmpty) {
      _scheduleAggregation();
      SchedulerBinding.instance.ensureVisualUpdate();
    }
  }

  /// Returns an immutable snapshot of all internal state for testing.
  ///
  /// This is the sole testing entry point for inspecting private state.
  /// It provides an atomic read of every field so tests can verify
  /// intermediate states (timer flags, instance counts, suppression flags)
  /// that are not observable through the public API.
  @visibleForTesting
  static OhosFlexOverflowDebugState get debugState => OhosFlexOverflowDebugState(
    isReportingOverflow: _isReportingOverflow,
    lastScaleFactor: _lastScaleFactor,
    lastScreenHeight: _lastScreenHeight,
    lastViewInsetsBottom: _lastViewInsetsBottom,
    hasSuppressedInitialKeyboardOverflow: _hasSuppressedInitialKeyboardOverflow,
    hasSuppressedInitialEditingOverflow: _hasSuppressedInitialEditingOverflow,
    overflowingInstanceCount: _overflowingInstances.length,
    frameSampleCount: _frameSamples.length,
    hasPendingOverflowTimer: _pendingOverflowTimer != null,
    hasSuppressedOverflowTimer: _suppressedOverflowTimer != null,
    suppressionExpired: _suppressionExpired,
    confirmationExpired: _confirmationExpired,
    routeChanged: _routeChanged,
  );

  /// Resets all static state to initial values.
  ///
  /// This is intended for testing only. It clears all tracking sets,
  /// cancels timers, and resets all flags so that each test starts with
  /// a clean slate.
  @visibleForTesting
  static void resetState() {
    _stateInitialized = false;
    isFlexOverflowEnabled =
        _kEnableFlexOverflow && kReleaseMode && defaultTargetPlatform == TargetPlatform.ohos;
    _suppressedOverflowTimer?.cancel();
    _suppressedOverflowTimer = null;
    _pendingOverflowTimer?.cancel();
    _pendingOverflowTimer = null;
    _overflowingInstances.clear();
    _frameSamples.clear();
    _frameAggregationScheduled = false;
    _isReportingOverflow = false;
    _lastScreenHeight = 0.0;
    _lastScaleFactor = 1.0;
    _lastViewInsetsBottom = 0.0;
    _hasSuppressedInitialKeyboardOverflow = false;
    _hasSuppressedInitialEditingOverflow = false;
    _initialOverflowSuppressionDeadline = null;
    _suppressionExpired = false;
    _confirmationExpired = false;
    _routeChanged = false;
  }
}
