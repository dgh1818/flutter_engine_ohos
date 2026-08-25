// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE_HW file.

// The lint rules `omit_obvious_local_variable_types` and
// `specify_nonobvious_local_variable_types` conflict for
// `MethodChannel.invokeMethod<T>()` results (the type is
// "obvious" from the explicit generic but "non-obvious" from
// the nullable return). Suppress both for the affected lines.
// ignore_for_file: omit_obvious_local_variable_types, specify_nonobvious_local_variable_types

import 'package:flutter/foundation.dart' show TargetPlatform, defaultTargetPlatform;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:split_view/main.dart';

/// Integration tests for the OHOS split view (parallel vision) feature.
///
/// These tests run on a real OHOS device or emulator with a wide screen
/// (tablet or 2-in-1). They verify end-to-end behavior that cannot be
/// fully tested in unit tests.
///
/// Covered feature areas:
/// 1. Navigation: push (named/unnamed), multi-level push, pop, popUntil
/// 2. Return: pop, popUntil home
/// 3. Dialogs: showDialog, AlertDialog, nested dialog
/// 4. Input method: TextField focus and text input
/// 5. Orientation switch: forced landscape / restore all orientations
/// 6. Fullscreen: config-based fullscreen route + force-landscape fullscreen
///
/// Prerequisites:
/// - OHOS device or emulator with a wide screen (≥ 600 logical pixels)
/// - Split view enabled in the app's config file (`split_config.json`)
/// - Screen in landscape orientation
void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  /// Minimum logical width for split view to activate.
  const double kMinSplitViewWidth = 600;

  /// Returns the center x-coordinate of the widget found by [finder].
  double centerX(WidgetTester tester, Finder finder) {
    final Rect rect = tester.getRect(finder);
    return rect.center.dx;
  }

  /// Returns the logical screen width.
  double screenWidth(WidgetTester tester) {
    return tester.view.physicalSize.width / tester.view.devicePixelRatio;
  }

  /// Returns the logical screen height.
  double screenHeight(WidgetTester tester) {
    return tester.view.physicalSize.height / tester.view.devicePixelRatio;
  }

  /// Verifies that the device meets split view prerequisites:
  /// - OHOS platform
  /// - Wide screen (≥ 600 logical pixels)
  /// - Landscape orientation (width > height)
  ///
  /// Call this at the beginning of every testWidgets that depends on
  /// split view being active.
  void verifySplitViewPrerequisites(WidgetTester tester) {
    if (defaultTargetPlatform != TargetPlatform.ohos) {
      fail(
        '[Device prerequisite not met] This is NOT a code error. '
        'These tests require an OHOS device or emulator, but the current '
        'platform is $defaultTargetPlatform. Please run on an OHOS device.',
      );
    }
    final double sw = screenWidth(tester);
    final double sh = screenHeight(tester);
    if (sw < kMinSplitViewWidth) {
      fail(
        '[Device prerequisite not met] This is NOT a code error. '
        'These tests require a wide screen (≥ $kMinSplitViewWidth logical '
        'pixels), but the current screen width is $sw. '
        'Please use a tablet or 2-in-1 device.',
      );
    }
    if (sw <= sh) {
      fail(
        '[Device prerequisite not met] This is NOT a code error. '
        'These tests require the device to be in landscape orientation, '
        'but the current screen size is ${sw}x$sh (portrait). '
        'Please rotate the device to landscape before running.',
      );
    }
  }

  /// Restores the device to landscape orientation with split view active.
  ///
  /// Call this at the end of orientation-switch tests to ensure subsequent
  /// tests start from a clean landscape split-view state.
  Future<void> restoreLandscapeSplitView(WidgetTester tester) async {
    // Force landscape first to rotate the device back to landscape.
    await SystemChrome.setPreferredOrientations(<DeviceOrientation>[
      DeviceOrientation.landscapeLeft,
      DeviceOrientation.landscapeRight,
    ]);
    await tester.pumpAndSettle(const Duration(milliseconds: 500));

    // Then restore all orientations to cancel the forced landscape fullscreen
    // mode, so split view can resume.
    await SystemChrome.setPreferredOrientations(<DeviceOrientation>[
      DeviceOrientation.portraitUp,
      DeviceOrientation.portraitDown,
      DeviceOrientation.landscapeLeft,
      DeviceOrientation.landscapeRight,
    ]);
    await tester.pumpAndSettle(const Duration(milliseconds: 500));
  }

  // ===========================================================================
  // 1. Navigation: push (named/unnamed), multi-level, pop, popUntil
  // ===========================================================================

  group('Navigation', () {
    testWidgets('home page is visible on initial load', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Home Title'), findsOneWidget);
    });

    testWidgets('push named route shows home and detail simultaneously (split view)', (
      WidgetTester tester,
    ) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();

      // Both home and detail should be visible simultaneously.
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);

      // Home on left half, detail on right half.
      final double sw = screenWidth(tester);
      expect(centerX(tester, find.byKey(const Key('home-text'))), lessThan(sw / 2));
      expect(centerX(tester, find.byKey(const Key('detail-text-1'))), greaterThanOrEqualTo(sw / 2));
    });

    testWidgets('push unnamed route also activates split view', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      await tester.tap(find.byKey(const Key('push-detail1-unnamed')));
      await tester.pumpAndSettle();

      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);
    });

    testWidgets('multi-level push: detail1 → detail2 → detail3', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Home → Detail 1
      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      expect(find.text('Page Detail 1'), findsOneWidget);
      expect(find.text('Page Home'), findsOneWidget);

      // Detail 1 → Detail 2
      await tester.tap(find.byKey(const Key('push-next')));
      await tester.pumpAndSettle();
      expect(find.text('Page Detail 2'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsNothing);
      // Home should still be visible on the left in split view.
      expect(find.text('Page Home'), findsOneWidget);

      // Detail 2 → Detail 3
      await tester.tap(find.byKey(const Key('push-next')));
      await tester.pumpAndSettle();
      expect(find.text('Page Detail 3'), findsOneWidget);
      expect(find.text('Page Home'), findsOneWidget);
    });

    testWidgets('pop from detail restores home-only view', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      expect(find.text('Page Detail 1'), findsOneWidget);

      await tester.tap(find.byKey(const Key('pop-detail')));
      await tester.pumpAndSettle();

      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsNothing);
    });

    testWidgets('popUntil home from deep stack', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Push three levels deep.
      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      await tester.tap(find.byKey(const Key('push-next')));
      await tester.pumpAndSettle();
      await tester.tap(find.byKey(const Key('push-next')));
      await tester.pumpAndSettle();
      expect(find.text('Page Detail 3'), findsOneWidget);

      // PopUntil home should clear all detail pages at once.
      await tester.tap(find.byKey(const Key('pop-until-home')));
      await tester.pumpAndSettle();

      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 3'), findsNothing);
      expect(find.text('Page Detail 2'), findsNothing);
      expect(find.text('Page Detail 1'), findsNothing);
    });

    testWidgets('multiple push/pop cycles maintain split view state', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      for (int i = 0; i < 3; i++) {
        await tester.tap(find.byKey(const Key('push-detail1-named')));
        await tester.pumpAndSettle();
        expect(find.text('Page Detail 1'), findsOneWidget);
        expect(find.text('Page Home'), findsOneWidget);

        await tester.tap(find.byKey(const Key('pop-detail')));
        await tester.pumpAndSettle();
        expect(find.text('Page Home'), findsOneWidget);
        expect(find.text('Page Detail 1'), findsNothing);
      }
    });
  });

  // ===========================================================================
  // 2. Dialogs: AlertDialog, nested dialog
  // ===========================================================================

  group('Dialogs', () {
    testWidgets('show AlertDialog in split view and close', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Scroll the button into view before tapping.
      await tester.ensureVisible(find.byKey(const Key('show-dialog')));
      await tester.pumpAndSettle();

      // Open dialog from home page.
      await tester.tap(find.byKey(const Key('show-dialog')));
      await tester.pumpAndSettle();

      // Dialog should be visible.
      expect(find.text('Test Dialog'), findsOneWidget);
      expect(find.text('Dialog in split view'), findsOneWidget);

      // Close the dialog.
      await tester.ensureVisible(find.byKey(const Key('close-dialog')));
      await tester.pumpAndSettle();
      await tester.tap(find.byKey(const Key('close-dialog')));
      await tester.pumpAndSettle();

      expect(find.text('Test Dialog'), findsNothing);
      // Home should still be visible after dialog closes.
      expect(find.text('Page Home'), findsOneWidget);
    });

    testWidgets('show nested dialog and close all', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Scroll the button into view before tapping.
      await tester.ensureVisible(find.byKey(const Key('show-nested-dialog')));
      await tester.pumpAndSettle();

      // Open outer dialog.
      await tester.tap(find.byKey(const Key('show-nested-dialog')));
      await tester.pumpAndSettle();
      expect(find.text('Outer Dialog'), findsOneWidget);

      // Open inner dialog from outer.
      await tester.tap(find.byKey(const Key('open-inner-dialog')));
      await tester.pumpAndSettle();
      expect(find.text('Inner Dialog'), findsOneWidget);
      expect(find.text('Nested dialog in split view'), findsOneWidget);

      // Close all dialogs.
      await tester.tap(find.byKey(const Key('close-inner-dialog')));
      await tester.pumpAndSettle();

      expect(find.text('Inner Dialog'), findsNothing);
      expect(find.text('Outer Dialog'), findsNothing);
      expect(find.text('Page Home'), findsOneWidget);
    });

    testWidgets('dialog works while detail page is shown in split view', (
      WidgetTester tester,
    ) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Push detail page (split view active).
      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      expect(find.text('Page Detail 1'), findsOneWidget);
      expect(find.text('Page Home'), findsOneWidget);

      // Scroll the button into view before tapping.
      await tester.ensureVisible(find.byKey(const Key('show-dialog')));
      await tester.pumpAndSettle();

      // Open dialog from home page (left side).
      await tester.tap(find.byKey(const Key('show-dialog')));
      await tester.pumpAndSettle();
      expect(find.text('Test Dialog'), findsOneWidget);

      // The dialog was opened from the home page, but since a detail page
      // is shown on the right side of split view, the dialog should be
      // positioned on the right half of the screen.
      final double sw = screenWidth(tester);
      final double dialogCenterX = centerX(tester, find.text('Test Dialog'));
      expect(dialogCenterX, greaterThanOrEqualTo(sw / 2));

      // Close dialog; split view should be restored.
      await tester.ensureVisible(find.byKey(const Key('close-dialog')));
      await tester.pumpAndSettle();
      await tester.tap(find.byKey(const Key('close-dialog')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);
    });
  });

  // ===========================================================================
  // 3. Input method: TextField focus, cursor follow, and soft keyboard
  // ===========================================================================

  group('Input method', () {
    testWidgets('home Dart TextField accepts text input', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      expect(find.text('Page Home'), findsOneWidget);

      // Tap the Dart TextField on the home page and enter text.
      await tester.tap(find.byKey(const Key('home-dart-input')));
      await tester.pumpAndSettle();

      // 断言 TextField 获得焦点
      final EditableText homeEditableText = tester.widget(
        find.descendant(
          of: find.byKey(const Key('home-dart-input')),
          matching: find.byType(EditableText),
        ),
      );
      expect(homeEditableText.focusNode.hasFocus, isTrue);

      await tester.enterText(find.byKey(const Key('home-dart-input')), 'flutter ohos 2026');
      await tester.pumpAndSettle();

      // Verify the text was accepted.
      expect(find.text('flutter ohos 2026'), findsWidgets);
    });

    testWidgets('native page Dart TextField accepts text input', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Navigate to the native view page.
      await tester.tap(find.byKey(const Key('push-native')));
      await tester.pumpAndSettle();
      expect(find.text('Page Native View'), findsOneWidget);

      // Tap the Dart TextField on the native page and enter text.
      await tester.tap(find.byKey(const Key('native-dart-input')));
      await tester.pumpAndSettle();

      // 断言 TextField 获得焦点
      final EditableText nativeEditableText = tester.widget(
        find.descendant(
          of: find.byKey(const Key('native-dart-input')),
          matching: find.byType(EditableText),
        ),
      );
      expect(nativeEditableText.focusNode.hasFocus, isTrue);

      await tester.enterText(find.byKey(const Key('native-dart-input')), 'arkts flutter 2026');
      await tester.pumpAndSettle();

      // Verify the text was accepted.
      expect(find.text('arkts flutter 2026'), findsWidgets);

      // Pop back to home.
      await tester.tap(find.byKey(const Key('pop-native')));
      await tester.pumpAndSettle();
    });
  });

  // ===========================================================================
  // 4. Orientation switch: forced landscape / restore all orientations
  // ===========================================================================

  group('Orientation switch', () {
    testWidgets('force landscape disables split view (fullscreen)', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Initially split view should be active (home visible).
      expect(find.text('Page Home'), findsOneWidget);

      // Push a detail page to confirm split view is active.
      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);

      // Force landscape orientation → should trigger fullscreen mode.
      // Call SystemChrome directly since the home page (with the button)
      // will be hidden after orientation change.
      await SystemChrome.setPreferredOrientations(<DeviceOrientation>[
        DeviceOrientation.landscapeLeft,
        DeviceOrientation.landscapeRight,
      ]);
      await tester.pumpAndSettle();

      // In forced landscape fullscreen, home page should be hidden
      // (split view disabled, detail page covers full screen).
      expect(find.text('Page Detail 1'), findsOneWidget);
      expect(find.text('Page Home'), findsNothing);

      // Restore all orientations → split view should resume.
      await restoreLandscapeSplitView(tester);
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);
    });

    testWidgets('restore orientation re-enables split view from home', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Force landscape from home page (no detail pushed).
      await SystemChrome.setPreferredOrientations(<DeviceOrientation>[
        DeviceOrientation.landscapeLeft,
        DeviceOrientation.landscapeRight,
      ]);
      await tester.pumpAndSettle();

      // Home page is still visible (it's the only page, shown fullscreen).
      expect(find.text('Page Home'), findsOneWidget);

      // Restore all orientations → split view should resume.
      await restoreLandscapeSplitView(tester);

      // Home page should still be visible after restoring orientation.
      expect(find.text('Page Home'), findsOneWidget);

      // Push detail — split view should be active again.
      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);
    });

    testWidgets('orientation switch dynamically toggles split view on and off', (
      WidgetTester tester,
    ) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Push detail page (split view active in landscape).
      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);

      // Force portrait → split view disabled, detail page fullscreen.
      await SystemChrome.setPreferredOrientations(<DeviceOrientation>[
        DeviceOrientation.portraitUp,
      ]);
      await tester.pumpAndSettle(const Duration(milliseconds: 500));
      expect(find.text('Page Detail 1'), findsOneWidget);

      // Detail page should cover the full screen width (no split view).
      final double sw = screenWidth(tester);
      final double detailCenterX = centerX(tester, find.byKey(const Key('detail-text-1')));
      expect(detailCenterX, closeTo(sw / 2, sw * 0.15));

      // Restore landscape orientation so subsequent tests start clean.
      await restoreLandscapeSplitView(tester);
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);
    });
  });

  // ===========================================================================
  // 5. Fullscreen: config-based fullscreen route + force-landscape fullscreen
  // ===========================================================================

  group('Fullscreen', () {
    testWidgets('config-based fullscreen route hides home page', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Navigate to fullscreen page (listed in split_config.json fullScreenPages).
      await tester.tap(find.byKey(const Key('push-fullscreen')));
      await tester.pumpAndSettle();

      expect(find.text('Page Fullscreen'), findsOneWidget);
      expect(find.text('Fullscreen Title'), findsOneWidget);

      // Home page should NOT be visible in fullscreen mode.
      expect(find.text('Page Home'), findsNothing);

      // Pop back to home.
      await tester.tap(find.byKey(const Key('pop-fullscreen')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
    });

    testWidgets('fullscreen route covers entire screen width', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      await tester.tap(find.byKey(const Key('push-fullscreen')));
      await tester.pumpAndSettle();

      // The fullscreen text should be centered on the full screen,
      // not just the right half.
      final double sw = screenWidth(tester);
      final double fullscreenCenterX = centerX(tester, find.byKey(const Key('fullscreen-text')));
      expect(fullscreenCenterX, greaterThan(sw * 0.25));
      expect(fullscreenCenterX, lessThan(sw * 0.75));
    });

    testWidgets('fullscreen → pop → push detail restores split view', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Go to fullscreen first.
      await tester.tap(find.byKey(const Key('push-fullscreen')));
      await tester.pumpAndSettle();
      expect(find.text('Page Fullscreen'), findsOneWidget);
      expect(find.text('Page Home'), findsNothing);

      // Pop back to home.
      await tester.tap(find.byKey(const Key('pop-fullscreen')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);

      // Now push detail — split view should be active again.
      await tester.tap(find.byKey(const Key('push-detail1-named')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Detail 1'), findsOneWidget);
    });
  });

  // ===========================================================================
  // 6. MediaQuery: enableSplitView reduces reported width by half
  // ===========================================================================

  group('MediaQuery', () {
    testWidgets('MediaQuery size is halved when split view is active', (WidgetTester tester) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Push MediaQuery page (split view active).
      await tester.tap(find.byKey(const Key('push-mediaquery')));
      await tester.pumpAndSettle();

      expect(find.text('Page MediaQuery'), findsOneWidget);
      expect(find.text('Page Home'), findsOneWidget);

      // When split view is active and enableReducedContainerSize is true,
      // MediaQuery.size.width should be approximately half the screen width.
      final double sw = screenWidth(tester);
      final String widthText = tester.widget<Text>(find.byKey(const Key('mediaquery-width'))).data!;
      final double mqWidth = double.parse(widthText.split(': ')[1]);

      // The MediaQuery width should be roughly half the screen width.
      expect(mqWidth, lessThan(sw * 0.6));
      expect(mqWidth, greaterThan(sw * 0.4));

      // Pop back.
      await tester.tap(find.byKey(const Key('pop-mediaquery')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
    });
  });

  // ===========================================================================
  // 7. Native view: OhosView embedded in split view detail page
  // ===========================================================================

  group('Native view', () {
    testWidgets('native view renders in split view detail page with correct size', (
      WidgetTester tester,
    ) async {
      verifySplitViewPrerequisites(tester);
      await tester.pumpWidget(const SplitViewTestApp());
      await tester.pumpAndSettle();

      // Navigate to native view page.
      await tester.tap(find.byKey(const Key('push-native')));
      await tester.pumpAndSettle();

      // Both home and native view page should be visible in split view.
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Native View'), findsOneWidget);

      // The native view container should be on the right half.
      final double sw = screenWidth(tester);
      final double containerCenterX = centerX(
        tester,
        find.byKey(const Key('native-view-container')),
      );
      expect(containerCenterX, greaterThanOrEqualTo(sw / 2));

      // The SizedBox constrains the native view height to 200.
      // Width is stretched by Column's CrossAxisAlignment.stretch,
      // so it fills the available width rather than being 300.
      final Rect containerRect = tester.getRect(find.byKey(const Key('native-view-container')));
      expect(containerRect.height, 200);
      expect(containerRect.width, greaterThan(0));

      // Pop back.
      await tester.tap(find.byKey(const Key('pop-native')));
      await tester.pumpAndSettle();
      expect(find.text('Page Home'), findsOneWidget);
      expect(find.text('Page Native View'), findsNothing);
    });
  });
}
