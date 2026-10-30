// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:io' show Platform;
import 'dart:ui';

import 'package:display_cutout_rotation/main.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';

/// Logical-pixel tolerance for rect comparisons: values pass through a
/// fractional device pixel ratio and can differ by rounding.
const double _coordinateTolerance = 1.0;

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  group('end-to-end test', () {
    // The cutout under test is whichever device provides it: the synthetic
    // notch injected by the android harness, or the physical punch-hole
    // reported by ohos. Assertions are geometric invariants (rotation
    // mapping, in-screen sanity), not absolute offsets, so they hold for an
    // edge-attached notch and an inset punch-hole alike.
    testWidgets('cutout is reported in portrait mode', (WidgetTester tester) async {
      // Force rotation
      await setOrientationAndWaitUntilRotation(tester, DeviceOrientation.portraitUp);
      // Load app widget.
      await tester.pumpWidget(const MyApp());
      final BuildContext context = tester.element(find.byType(Text));
      if (!context.mounted) {
        fail('BuildContext not mounted');
      }
      final Iterable<DisplayFeature> displayFeatures = getCutouts(tester, context);
      if (displayFeatures.isEmpty) {
        // OHOS has no cutout-injection mechanism; a panel without a cutout
        // has nothing to assert. Other platforms always report a cutout
        // (injected by the harness), so an empty list there is a regression.
        if (!Platform.isOhos) {
          fail('No cutout display feature reported by this device');
        }
        print('No cutout display feature reported by this device; skipping.');
        return;
      }
      // One cutout is expected: injected on android, the single punch-hole
      // on ohos.
      expect(displayFeatures.length, 1, reason: 'Single cutout display feature expected');
      final Rect portraitBounds = displayFeatures.first.bounds;
      final Size portraitSize = MediaQuery.of(context).size;
      // Sanity only — a real punch-hole is inset by the system avoid-margin,
      // so no absolute offset is asserted.
      expect(portraitBounds.left, greaterThanOrEqualTo(0));
      expect(portraitBounds.top, greaterThanOrEqualTo(0));
      expect(portraitBounds.right, lessThanOrEqualTo(portraitSize.width));
      expect(portraitBounds.bottom, lessThanOrEqualTo(portraitSize.height));
    });

    testWidgets('cutout is reported in landscape left', (WidgetTester tester) async {
      await setOrientationAndWaitUntilRotation(tester, DeviceOrientation.landscapeLeft);
      // Load app widget.
      await tester.pumpWidget(const MyApp());
      final BuildContext context = tester.element(find.byType(Text));
      if (!context.mounted) {
        fail('BuildContext not mounted');
      }
      // Verify that app code reports the cutout in landscape too.
      final Iterable<DisplayFeature> displayFeatures = getCutouts(tester, context);
      if (displayFeatures.isEmpty) {
        if (!Platform.isOhos) {
          fail('No cutout display feature reported by this device');
        }
        print('No cutout display feature reported by this device; skipping.');
        return;
      }
      expect(displayFeatures.length, 1, reason: 'Single cutout display feature expected');
      final Rect landscapeBounds = displayFeatures.first.bounds;
      final Size landscapeSize = MediaQuery.of(context).size;
      expect(landscapeBounds.left, greaterThanOrEqualTo(0));
      expect(landscapeBounds.top, greaterThanOrEqualTo(0));
      expect(landscapeBounds.right, lessThanOrEqualTo(landscapeSize.width));
      expect(landscapeBounds.bottom, lessThanOrEqualTo(landscapeSize.height));
    });

    testWidgets('cutout handles rotation', (WidgetTester tester) async {
      await setOrientationAndWaitUntilRotation(tester, DeviceOrientation.portraitUp);
      const widgetUnderTest = MyApp();
      // Load app widget.
      await tester.pumpWidget(widgetUnderTest);
      BuildContext context = tester.element(find.byType(Text));
      if (!context.mounted) {
        fail('BuildContext not mounted');
      }
      Iterable<DisplayFeature> displayFeatures = getCutouts(tester, context);
      if (displayFeatures.isEmpty) {
        if (!Platform.isOhos) {
          fail('No cutout display feature reported by this device');
        }
        print('No cutout display feature reported by this device; skipping.');
        return;
      }
      // Test is expecting one cutout setup in the test harness.
      expect(displayFeatures.length, 1, reason: 'Single cutout display feature expected');
      final Rect portraitBounds = displayFeatures.first.bounds;
      final double portraitWidth = MediaQuery.of(context).size.width;
      await setOrientationAndWaitUntilRotation(tester, DeviceOrientation.landscapeLeft);
      await tester.pumpWidget(widgetUnderTest);

      // Requery for display features after rotation.
      context = tester.element(find.byType(Text));
      if (!context.mounted) {
        fail('BuildContext not mounted');
      }
      displayFeatures = getCutouts(tester, context);
      expect(displayFeatures.length, 1, reason: 'Single cutout display feature expected');
      final Rect landscapeBounds = displayFeatures.first.bounds;

      // landscapeLeft rotates 90 degrees counterclockwise from portraitUp:
      // the former top edge becomes the left edge, the former left edge
      // becomes the bottom edge, and width and height swap.
      expect(
        landscapeBounds.width,
        closeTo(portraitBounds.height, _coordinateTolerance),
        reason: 'cutout width and height should swap after a 90° rotation',
      );
      expect(
        landscapeBounds.height,
        closeTo(portraitBounds.width, _coordinateTolerance),
        reason: 'cutout width and height should swap after a 90° rotation',
      );
      expect(
        landscapeBounds.left,
        closeTo(portraitBounds.top, _coordinateTolerance),
        reason: 'the former top inset should become the left inset',
      );
      expect(
        landscapeBounds.bottom,
        closeTo(portraitWidth - portraitBounds.left, _coordinateTolerance),
        reason: 'the former left inset should become the bottom inset',
      );
    });

    tearDown(() async {
      // After each test reset to device perfered orientations to avoid
      // test pollution.
      await SystemChrome.setPreferredOrientations(<DeviceOrientation>[]);
    });
  });
}

/*
 * Force rotation then poll to ensure rotation has happened.
 *
 * Rotations have an async communication to engine which then has an async
 * communication to the android operating system.
 */
Future<void> setOrientationAndWaitUntilRotation(
  WidgetTester tester,
  DeviceOrientation orientation,
) async {
  await SystemChrome.setPreferredOrientations(<DeviceOrientation>[orientation]);
  Orientation expectedOrientation;
  switch (orientation) {
    case DeviceOrientation.portraitUp:
    case DeviceOrientation.portraitDown:
      expectedOrientation = Orientation.portrait;
    case DeviceOrientation.landscapeRight:
    case DeviceOrientation.landscapeLeft:
      expectedOrientation = Orientation.landscape;
  }
  while (true) {
    final BuildContext context = tester.element(find.byType(Text));
    if (context.mounted && expectedOrientation == MediaQuery.of(context).orientation) {
      break;
    }
    await tester.pumpAndSettle();
  }
}

Iterable<DisplayFeature> getCutouts(WidgetTester tester, BuildContext context) {
  final List<DisplayFeature> displayFeatures = MediaQuery.of(context).displayFeatures;
  return displayFeatures.where(
    (DisplayFeature feature) => feature.type == DisplayFeatureType.cutout,
  );
}
