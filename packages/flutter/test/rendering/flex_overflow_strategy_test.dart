// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';

// This test file verifies OhosFlexOverflowStrategy behavior using
// testWidgets (TestWidgetsFlutterBinding).
//
// notifyRouteChanged() is a no-op in debug mode (kReleaseMode guard),
// so tests 6 and 9 are skipped — they can only run in release mode.

/// A widget that creates a [RenderFlex] with a custom [overflowStrategy].
///
/// The standard [Column] widget does not expose [overflowStrategy], so this
/// widget is needed to inject [OhosFlexOverflowStrategy] in testWidgets tests.
class _OverflowColumn extends MultiChildRenderObjectWidget {
  const _OverflowColumn({required this.overflowStrategy, super.children});

  final FlexOverflowStrategy overflowStrategy;

  @override
  RenderFlex createRenderObject(BuildContext context) {
    return RenderFlex(
      direction: Axis.vertical,
      textDirection: TextDirection.ltr,
      overflowStrategy: overflowStrategy,
    );
  }

  @override
  void updateRenderObject(BuildContext context, covariant RenderFlex renderObject) {
    renderObject.overflowStrategy = overflowStrategy;
  }
}

void main() {
  // Captured displayMetrics channel calls.
  late List<MethodCall> methodCalls;

  setUp(() {
    // Reset all static state so each test starts clean.
    OhosFlexOverflowStrategy.resetState();

    // Enable the release-mode-only dynamic DPI gate so that
    // notifyRouteChanged() and createDefaultOverflowStrategy() exercise
    // their full logic in debug-mode tests.
    OhosFlexOverflowStrategy.debugDynamicDpiEnabled = true;

    // Capture displayMetrics channel calls.
    methodCalls = <MethodCall>[];
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(
      SystemChannels.displayMetrics,
      (MethodCall call) {
        methodCalls.add(call);
        return null;
      },
    );
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(
      SystemChannels.displayMetrics,
      null,
    );
  });

  /// Pumps an overflowing vertical RenderFlex whose view height equals
  /// [constraintHeight]. Children total [childHeight] × 2, so overflow
  /// occurs when childHeight × 2 > constraintHeight.
  ///
  /// The view's physical size is set to [constraintHeight] (with DPR 1.0),
  /// so the RenderFlex receives a tight height constraint equal to
  /// [constraintHeight] — matching how a real Scaffold body constrains its
  /// child Column.
  Future<RenderFlex> pumpOverflowingColumn(
    WidgetTester tester, {
    double constraintHeight = 200,
    double childHeight = 150,
    double width = 100,
  }) async {
    tester.view.devicePixelRatio = 1.0;
    tester.view.physicalSize = Size(width, constraintHeight);
    addTearDown(tester.view.reset);

    await tester.pumpWidget(
      Directionality(
        textDirection: TextDirection.ltr,
        child: _OverflowColumn(
          overflowStrategy: OhosFlexOverflowStrategy(),
          children: <Widget>[
            SizedBox(width: width, height: childHeight),
            SizedBox(width: width, height: childHeight),
          ],
        ),
      ),
    );
    await tester.pump();

    // The overflow assertion is expected — drain it so it doesn't fail
    // the test. The overflow is the whole point of this helper.
    tester.takeException();

    final flex = tester.renderObject(find.byType(_OverflowColumn)) as RenderFlex;
    return flex;
  }

  // ===========================================================================
  // 1. Overflow / Recovery
  // ===========================================================================

  group('OhosFlexOverflowStrategy - Overflow / Recovery', () {
    // -- Case 1: overflow triggers DPR shrink report -----------------------------
    testWidgets(
      'overflow triggers DPR report via displayMetrics channel',
      (WidgetTester tester) async {
        final RenderFlex flex = await pumpOverflowingColumn(tester);
        expect(flex.hasOverflow, isTrue);

        // No DPR report yet — 180ms confirmation timer is running.
        expect(methodCalls, isEmpty);

        // Verify intermediate state: confirmation timer started, not yet committed.
        final OhosFlexOverflowDebugState stateDuringConfirmation =
            OhosFlexOverflowStrategy.debugState;
        expect(stateDuringConfirmation.isReportingOverflow, isFalse);
        expect(stateDuringConfirmation.hasPendingOverflowTimer, isTrue);
        expect(stateDuringConfirmation.confirmationExpired, isFalse);
        expect(stateDuringConfirmation.frameSampleCount, 1);

        // Wait for the 180ms confirmation timer to fire.
        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        // After confirmation, the DPR shrink should be reported.
        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.method, 'updateDpiScale');
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        // Verify committed state.
        final OhosFlexOverflowDebugState stateAfterCommit = OhosFlexOverflowStrategy.debugState;
        expect(stateAfterCommit.isReportingOverflow, isTrue);
        expect(stateAfterCommit.lastScaleFactor, 0.85);
        expect(stateAfterCommit.overflowingInstanceCount, 1);
        expect(stateAfterCommit.hasPendingOverflowTimer, isFalse);
        expect(stateAfterCommit.routeChanged, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 2: multiple overflowing instances report minimum scale -------------
    testWidgets(
      'multiple overflowing instances report minimum scale',
      (WidgetTester tester) async {
        // Children 300px, constraint 200px → scale = 200/300 ≈ 0.667 → clamped 0.85
        final RenderFlex flex = await pumpOverflowingColumn(tester);
        expect(flex.hasOverflow, isTrue);

        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.frameSampleCount, 1);
        expect(state1.hasPendingOverflowTimer, isTrue);

        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.isReportingOverflow, isTrue);
        expect(state2.lastScaleFactor, 0.85);
        expect(state2.overflowingInstanceCount, 1);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 3: scale factor is clamped to [0.85, 1.0] -------------------------
    testWidgets(
      'scale factor is clamped to minimum 0.85',
      (WidgetTester tester) async {
        // Extreme overflow: children 1000px, constraint 200px → scale = 0.2 → clamped to 0.85
        final RenderFlex flex = await pumpOverflowingColumn(tester, childHeight: 500);
        expect(flex.hasOverflow, isTrue);

        expect(methodCalls, isEmpty);

        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        // Even with extreme overflow, scale is clamped to 0.85.
        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        final OhosFlexOverflowDebugState state = OhosFlexOverflowStrategy.debugState;
        expect(state.lastScaleFactor, 0.85);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 4: screen height change commits scale immediately ------------------
    testWidgets(
      'screen height change commits scale immediately',
      (WidgetTester tester) async {
        // Set initial physical size (devicePixelRatio = 1.0 so logical = physical).
        // View height = 200 so the RenderFlex gets a tight height=200 constraint.
        // Children total 300px → overflow.
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        tester.view.physicalSize = const Size(100, 200);

        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );

        // Pump to start the confirmation timer.
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();
        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.hasPendingOverflowTimer, isTrue);
        expect(state1.isReportingOverflow, isFalse);

        // Change screen height (fold/unfold/rotation).
        // Keep height < 300 so overflow still exists after the change.
        tester.view.physicalSize = const Size(100, 250);

        // Pump a frame — _aggregateAndReport sees isHeightChanged=true and
        // commits immediately, bypassing the 180ms confirmation window.
        await tester.pump();
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.method, 'updateDpiScale');
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.isReportingOverflow, isTrue);
        expect(state2.lastScaleFactor, 0.85);
        expect(state2.hasPendingOverflowTimer, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 5: screen height change clears suppression state ------------------
    testWidgets(
      'screen height change clears suppression state',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        // View height = 200 so the RenderFlex gets a tight height=200 constraint.
        // Children total 300px → overflow.
        tester.view.physicalSize = const Size(100, 200);

        // Simulate keyboard opening: viewInsets transitions from 0 to non-zero.
        // _lastViewInsetsBottom starts at 0 (from resetState).
        tester.view.viewInsets = const FakeViewPadding(bottom: 300);

        // Pump an overflowing column — this triggers the keyboard-open
        // suppression path (viewInsets 0→non-zero).
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        // Verify suppression was triggered.
        expect(state1.hasSuppressedInitialKeyboardOverflow, isTrue);
        expect(state1.hasSuppressedOverflowTimer, isTrue);
        expect(state1.isReportingOverflow, isFalse);

        // Change screen height — should clear all suppression state.
        tester.view.physicalSize = const Size(100, 400);
        await tester.pump();

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.hasSuppressedInitialKeyboardOverflow, isFalse);
        expect(state2.hasSuppressedInitialEditingOverflow, isFalse);
        expect(state2.hasSuppressedOverflowTimer, isFalse);
        expect(state2.hasPendingOverflowTimer, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 6: dispose all instances with route change resets DPR -------------
    testWidgets(
      'dispose all instances with route change resets DPR',
      (WidgetTester tester) async {
        // First, commit a DPR shrink.
        final RenderFlex flex = await pumpOverflowingColumn(tester);
        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.isReportingOverflow, isTrue);
        expect(state1.overflowingInstanceCount, 1);
        expect(state1.routeChanged, isFalse);

        // Signal a route change — sets _routeChanged = true.
        OhosFlexOverflowStrategy.notifyRouteChanged();

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.routeChanged, isTrue);

        // Dispose the RenderFlex by removing the widget.
        await tester.pumpWidget(
          const Directionality(
            textDirection: TextDirection.ltr,
            child: SizedBox(width: 100, height: 200),
          ),
        );
        await tester.pump();

        // A reset DPR call (-1.0) should have been sent because:
        // - _isReportingOverflow was true
        // - _overflowingInstances is now empty (disposed)
        // - _routeChanged was true (from notifyRouteChanged)
        expect(methodCalls, hasLength(2));
        expect(methodCalls[1].method, 'updateDpiScale');
        expect(methodCalls[1].arguments, <String, dynamic>{'dpiScale': -1.0});

        final OhosFlexOverflowDebugState state3 = OhosFlexOverflowStrategy.debugState;
        expect(state3.isReportingOverflow, isFalse);
        expect(state3.overflowingInstanceCount, 0);
        expect(state3.routeChanged, isFalse);
        expect(state3.lastScaleFactor, 1.0);

        // Reference flex to avoid unused warning.
        expect(flex.attached, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 7: dispose without route change does not reset DPR ----------------
    testWidgets(
      'dispose without route change does not reset DPR',
      (WidgetTester tester) async {
        // First, commit a DPR shrink.
        final RenderFlex flex = await pumpOverflowingColumn(tester);
        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        // Do NOT call notifyRouteChanged — routeChanged stays false.
        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.routeChanged, isFalse);

        // Dispose the RenderFlex by removing the widget.
        await tester.pumpWidget(
          const Directionality(
            textDirection: TextDirection.ltr,
            child: SizedBox(width: 100, height: 200),
          ),
        );
        await tester.pump();

        // No reset should be sent — only the original DPR shrink.
        expect(methodCalls, hasLength(1));

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        // isReportingOverflow stays true because the reset condition
        // (_routeChanged && _overflowingInstances.isEmpty) was not met.
        expect(state2.isReportingOverflow, isTrue);
        expect(state2.overflowingInstanceCount, 0);
        expect(state2.routeChanged, isFalse);

        // Reference flex to avoid unused warning.
        expect(flex.attached, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 8: overflow disappears but instance alive does not reset DPR ------
    testWidgets(
      'overflow disappears but instance alive does not reset DPR',
      (WidgetTester tester) async {
        // First, commit a DPR shrink.
        final RenderFlex flex = await pumpOverflowingColumn(tester);
        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.isReportingOverflow, isTrue);
        expect(state1.overflowingInstanceCount, 1);

        // The instance is still alive (not disposed).
        expect(flex.attached, isTrue);

        // Pump another frame — overflow still present, no new report
        // (same scale, already reporting).
        await tester.pump();
        expect(methodCalls, hasLength(1));

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.isReportingOverflow, isTrue);
        expect(state2.overflowingInstanceCount, 1);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 9: notifyRouteChanged sets route flag and resets suppression ------
    testWidgets(
      'notifyRouteChanged sets route flag and resets suppression',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        tester.view.physicalSize = const Size(100, 200);
        tester.view.viewInsets = const FakeViewPadding(bottom: 300);

        // Set up keyboard-open suppression.
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.hasSuppressedOverflowTimer, isTrue);
        expect(state1.hasSuppressedInitialKeyboardOverflow, isTrue);
        expect(state1.isReportingOverflow, isFalse);
        expect(state1.routeChanged, isFalse);

        // Signal a route change.
        OhosFlexOverflowStrategy.notifyRouteChanged();

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        // Route flag is set.
        expect(state2.routeChanged, isTrue);
        // Per-page suppression flags are reset.
        expect(state2.hasSuppressedInitialKeyboardOverflow, isFalse);
        expect(state2.hasSuppressedInitialEditingOverflow, isFalse);
        // Timers are cancelled.
        expect(state2.hasSuppressedOverflowTimer, isFalse);
        expect(state2.hasPendingOverflowTimer, isFalse);
        // Suppression/confirmation expiry flags are cleared.
        expect(state2.suppressionExpired, isFalse);
        expect(state2.confirmationExpired, isFalse);

        // Clean up any pending timers.
        OhosFlexOverflowStrategy.resetState();
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 10: nested vertical flex does not trigger overflow handling -------
    testWidgets(
      'nested vertical flex does not trigger overflow handling',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        // View height = 400 so the outer flex gets a tight height=400 constraint.
        // The inner flex is wrapped in a SizedBox(height: 200) so it gets a
        // tight height=200 constraint. The inner flex has children totaling
        // 300px → overflow, but it is NOT a root vertical flex (it has a
        // vertical flex ancestor), so _isRootVerticalFlex returns false and
        // handleOverflow is a no-op for the inner flex.
        // The outer flex has a single child (SizedBox height=200) which fits
        // within 400px, so the outer flex does not overflow either.
        tester.view.physicalSize = const Size(100, 400);

        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: <Widget>[
                SizedBox(
                  height: 200,
                  child: _OverflowColumn(
                    overflowStrategy: OhosFlexOverflowStrategy(),
                    children: const <Widget>[
                      SizedBox(width: 100, height: 150),
                      SizedBox(width: 100, height: 150),
                    ],
                  ),
                ),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion from the inner flex.
        // The inner flex overflows (children 300px > constraint 200px), but
        // _isRootVerticalFlex returns false so handleOverflow is a no-op.
        tester.takeException();

        // No overflow handling should have been triggered because:
        // - The outer flex doesn't overflow (child fits within constraint)
        // - The inner flex overflows but is not a root vertical flex
        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state = OhosFlexOverflowStrategy.debugState;
        expect(state.isReportingOverflow, isFalse);
        expect(state.frameSampleCount, 0);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 11: resetState clears all state to initial values -----------------
    testWidgets(
      'resetState clears all state to initial values',
      (WidgetTester tester) async {
        // First, create some state by committing a DPR shrink.
        await pumpOverflowingColumn(tester);
        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        // Verify state is non-initial.
        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.isReportingOverflow, isTrue);
        expect(state1.lastScaleFactor, 0.85);
        expect(state1.overflowingInstanceCount, 1);

        // Reset state.
        OhosFlexOverflowStrategy.resetState();

        // Verify all fields are at initial values.
        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.isReportingOverflow, isFalse);
        expect(state2.lastScaleFactor, 1.0);
        expect(state2.lastScreenHeight, 0.0);
        expect(state2.lastViewInsetsBottom, 0.0);
        expect(state2.hasSuppressedInitialKeyboardOverflow, isFalse);
        expect(state2.hasSuppressedInitialEditingOverflow, isFalse);
        expect(state2.overflowingInstanceCount, 0);
        expect(state2.frameSampleCount, 0);
        expect(state2.hasPendingOverflowTimer, isFalse);
        expect(state2.hasSuppressedOverflowTimer, isFalse);
        expect(state2.suppressionExpired, isFalse);
        expect(state2.confirmationExpired, isFalse);
        expect(state2.routeChanged, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );
  });

  // ===========================================================================
  // 2. 180ms Confirmation Window
  // ===========================================================================

  group('OhosFlexOverflowStrategy - 180ms Confirmation Window', () {
    // -- Case 12: confirmation timer starts on first overflow -------------------
    testWidgets(
      'confirmation timer starts on first overflow',
      (WidgetTester tester) async {
        final RenderFlex flex = await pumpOverflowingColumn(tester);
        expect(flex.hasOverflow, isTrue);

        // No DPR report yet — confirmation timer is running.
        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state = OhosFlexOverflowStrategy.debugState;
        expect(state.hasPendingOverflowTimer, isTrue);
        expect(state.confirmationExpired, isFalse);
        expect(state.isReportingOverflow, isFalse);
        expect(state.frameSampleCount, 1);

        // Clean up the pending timer so it doesn't trip _verifyInvariants.
        OhosFlexOverflowStrategy.resetState();
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 13: confirmation timer cancelled when overflow disappears --------
    testWidgets(
      'confirmation timer cancelled when overflow disappears',
      (WidgetTester tester) async {
        // Start the confirmation timer.
        await pumpOverflowingColumn(tester);

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.hasPendingOverflowTimer, isTrue);
        expect(state1.confirmationExpired, isFalse);

        // Simulate overflow disappearing: remove the overflowing widget.
        await tester.pumpWidget(
          const Directionality(
            textDirection: TextDirection.ltr,
            child: SizedBox(width: 100, height: 200),
          ),
        );

        // Wait for the 180ms confirmation timer to fire. The timer callback
        // sets _confirmationExpired = true and calls _scheduleAggregation().
        await tester.pump(const Duration(milliseconds: 250));

        // After aggregation, the timer flag is cleared and no report was sent.
        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.hasPendingOverflowTimer, isFalse);
        expect(state2.confirmationExpired, isFalse);
        expect(methodCalls, isEmpty);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 14: confirmation timer expiry commits DPR shrink -----------------
    testWidgets(
      'confirmation timer expiry commits DPR shrink',
      (WidgetTester tester) async {
        await pumpOverflowingColumn(tester);

        // Confirmation timer is running, no report yet.
        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.hasPendingOverflowTimer, isTrue);
        expect(state1.confirmationExpired, isFalse);

        // Wait for the 180ms confirmation timer to fire. The timer callback
        // sets _confirmationExpired = true and calls _scheduleAggregation()
        // + ensureVisualUpdate().
        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        final OhosFlexOverflowDebugState state3 = OhosFlexOverflowStrategy.debugState;
        expect(state3.isReportingOverflow, isTrue);
        expect(state3.lastScaleFactor, 0.85);
        expect(state3.confirmationExpired, isFalse);
        expect(state3.hasPendingOverflowTimer, isFalse);
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );
  });

  // ===========================================================================
  // 3. 600ms Suppression
  // ===========================================================================

  group('OhosFlexOverflowStrategy - 600ms Suppression', () {
    // -- Case 15: first keyboard-open overflow is suppressed --------------------
    testWidgets(
      'first keyboard-open overflow is suppressed',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        // View height = 200 so the RenderFlex gets a tight height=200 constraint.
        // Children total 300px → overflow.
        tester.view.physicalSize = const Size(100, 200);

        // Simulate keyboard opening: viewInsets transitions from 0 to non-zero.
        // _lastViewInsetsBottom starts at 0 (from resetState).
        tester.view.viewInsets = const FakeViewPadding(bottom: 300);

        // Pump an overflowing column. The overflow happens while
        // viewInsets just transitioned from 0 to non-zero, so
        // _shouldSuppressInitialKeyboardOverflow returns true.
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();

        // No DPR report — overflow was suppressed.
        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state = OhosFlexOverflowStrategy.debugState;
        expect(state.hasSuppressedInitialKeyboardOverflow, isTrue);
        expect(state.hasSuppressedOverflowTimer, isTrue);
        expect(state.isReportingOverflow, isFalse);
        expect(state.hasPendingOverflowTimer, isFalse);
        expect(state.lastViewInsetsBottom, 300.0);

        // Clean up the pending suppression timer so it doesn't trip _verifyInvariants.
        OhosFlexOverflowStrategy.resetState();
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 16: first editing-time overflow is suppressed ---------------------
    testWidgets(
      'first editing-time overflow is suppressed',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        // View height = 200 so the RenderFlex gets a tight height=200 constraint.
        tester.view.physicalSize = const Size(100, 200);

        // Pre-set viewInsets to non-zero so _lastViewInsetsBottom > 0
        // after the first pump.
        tester.view.viewInsets = const FakeViewPadding(bottom: 300);

        // Pump a non-overflowing column first so _lastViewInsetsBottom gets
        // set to 300 via _aggregateAndReport. Using a RenderFlex (not a
        // plain SizedBox) ensures handleOverflow is called, which triggers
        // _onPostFrame → _scheduleAggregation → _aggregateAndReport →
        // _updateScreenState (which sets _lastViewInsetsBottom = 300).
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 50),
                SizedBox(width: 100, height: 50),
              ],
            ),
          ),
        );
        await tester.pump();

        // Now _lastViewInsetsBottom = 300. The next overflow with
        // viewInsets still > 0 and _lastScaleFactor == 1.0 should trigger
        // _shouldSuppressInitialEditingOverflow.
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();

        // No DPR report — overflow was suppressed.
        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state = OhosFlexOverflowStrategy.debugState;
        expect(state.hasSuppressedInitialEditingOverflow, isTrue);
        expect(state.hasSuppressedOverflowTimer, isTrue);
        expect(state.isReportingOverflow, isFalse);
        expect(state.hasPendingOverflowTimer, isFalse);

        // Clean up the pending suppression timer so it doesn't trip _verifyInvariants.
        OhosFlexOverflowStrategy.resetState();
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 17: suppression timer expiry advances to confirmation phase ------
    testWidgets(
      'suppression timer expiry advances to confirmation phase',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        // View height = 200 so the RenderFlex gets a tight height=200 constraint.
        tester.view.physicalSize = const Size(100, 200);
        tester.view.viewInsets = const FakeViewPadding(bottom: 300);

        // Set up keyboard-open suppression.
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.hasSuppressedOverflowTimer, isTrue);
        expect(state1.suppressionExpired, isFalse);
        expect(state1.isReportingOverflow, isFalse);

        // Wait for the 600ms suppression timer to fire.
        // The timer callback sets _suppressionExpired = true and calls
        // _scheduleAggregation(). The post-frame callback runs
        // _aggregateAndReport in the same pump, which sees
        // _suppressionExpired=true, resets it to false, and starts the
        // 180ms confirmation timer.
        await tester.pump(const Duration(milliseconds: 700));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        // After the suppression timer fired and _aggregateAndReport ran:
        // - _suppressionExpired was reset to false
        // - The suppression timer is no longer running
        // - The confirmation timer has started
        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.suppressionExpired, isFalse);
        expect(state2.hasSuppressedOverflowTimer, isFalse);
        expect(state2.hasPendingOverflowTimer, isTrue);
        expect(state2.isReportingOverflow, isFalse);

        // Now wait for the 180ms confirmation timer.
        await tester.pump(const Duration(milliseconds: 250));
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        // After confirmation, DPR shrink is committed.
        expect(methodCalls, hasLength(1));
        expect(methodCalls.first.arguments, <String, dynamic>{'dpiScale': 0.85});

        final OhosFlexOverflowDebugState state4 = OhosFlexOverflowStrategy.debugState;
        expect(state4.isReportingOverflow, isTrue);
        expect(state4.lastScaleFactor, 0.85);

        // Clean up any pending timers so they don't trip _verifyInvariants.
        OhosFlexOverflowStrategy.resetState();
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 18: suppression cooldown does not extend deadline ----------------
    testWidgets(
      'suppression cooldown does not extend deadline',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        // View height = 200 so the RenderFlex gets a tight height=200 constraint.
        tester.view.physicalSize = const Size(100, 200);
        tester.view.viewInsets = const FakeViewPadding(bottom: 300);

        // Set up keyboard-open suppression.
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.hasSuppressedOverflowTimer, isTrue);
        expect(state1.hasSuppressedInitialKeyboardOverflow, isTrue);

        // The suppression timer is running with a 600ms deadline.
        // If we pump another frame while overflow persists and the
        // cooldown is active, _shouldSuppressInitialOverflowCooldown
        // returns true and _startSuppressionTimer is called again,
        // but since the timer is already running, it's a no-op.
        // The deadline should NOT be extended.
        await tester.pump();
        // Drain the expected overflow assertion from the re-layout.
        tester.takeException();

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        // Timer still running, not expired yet.
        expect(state2.hasSuppressedOverflowTimer, isTrue);
        expect(state2.suppressionExpired, isFalse);
        expect(state2.isReportingOverflow, isFalse);

        // Clean up the pending suppression timer so it doesn't trip _verifyInvariants.
        OhosFlexOverflowStrategy.resetState();
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );

    // -- Case 19: suppression timer cancelled when overflow disappears ----------
    testWidgets(
      'suppression timer cancelled when overflow disappears',
      (WidgetTester tester) async {
        addTearDown(tester.view.reset);
        tester.view.devicePixelRatio = 1.0;
        // View height = 200 so the RenderFlex gets a tight height=200 constraint.
        tester.view.physicalSize = const Size(100, 200);
        tester.view.viewInsets = const FakeViewPadding(bottom: 300);

        // Set up keyboard-open suppression.
        await tester.pumpWidget(
          Directionality(
            textDirection: TextDirection.ltr,
            child: _OverflowColumn(
              overflowStrategy: OhosFlexOverflowStrategy(),
              children: const <Widget>[
                SizedBox(width: 100, height: 150),
                SizedBox(width: 100, height: 150),
              ],
            ),
          ),
        );
        await tester.pump();
        // Drain the expected overflow assertion.
        tester.takeException();

        final OhosFlexOverflowDebugState state1 = OhosFlexOverflowStrategy.debugState;
        expect(state1.hasSuppressedOverflowTimer, isTrue);
        expect(state1.suppressionExpired, isFalse);

        // Simulate overflow disappearing: remove the overflowing widget.
        await tester.pumpWidget(
          const Directionality(
            textDirection: TextDirection.ltr,
            child: SizedBox(width: 100, height: 200),
          ),
        );
        await tester.pump();

        // The suppression timer is still running — dispose() does not
        // schedule aggregation when not in the reporting phase. But when
        // the timer fires, _aggregateAndReport sees overflowingScales is
        // empty and cleans up without committing a DPR shrink.
        await tester.pump(const Duration(milliseconds: 700));

        // No DPR report was ever sent — overflow disappeared before commit.
        expect(methodCalls, isEmpty);

        final OhosFlexOverflowDebugState state2 = OhosFlexOverflowStrategy.debugState;
        expect(state2.hasSuppressedOverflowTimer, isFalse);
        expect(state2.isReportingOverflow, isFalse);

        // Clean up any pending timers so they don't trip _verifyInvariants.
        OhosFlexOverflowStrategy.resetState();
      },
      variant: TargetPlatformVariant.only(TargetPlatform.ohos),
    );
  });
}
