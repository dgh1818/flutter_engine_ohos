// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This test is used by devicelab, test "integration_ui_ios_screenshot".
// Its use of package:image is, at the time of writing, the only use of that
// package in this repository. If package:image is a problem, it is probably
// fine to just remove this test since the value of the test is probably not
// as much as the cost of the dependency.

import 'package:flutter_driver/flutter_driver.dart';
import 'package:image/image.dart';

import 'package:test/test.dart' hide TypeMatcher, isInstanceOf;

void main() {
  group('FlutterDriver', () {
    late FlutterDriver driver;

    setUpAll(() async {
      driver = await FlutterDriver.connect();
    });

    tearDownAll(() async {
      await driver.close();
    });

    test('should take screenshot', () async {
      final SerializableFinder toggleBtn = find.byValueKey('toggle');
      // Cards use a magic background color that we look for in the screenshots.
      // On OHOS devices with a wide-gamut display the swapchain format is
      // A2B10G10R10 (see SurfaceContextVK::SetWindowSurface), which flips the
      // Impeller content context's target color space to Display P3. The
      // driver screenshot re-rasterizes the layer tree offscreen through that
      // same context, so the PNG comes back P3-encoded: sRGB #FF0102 renders
      // as the mathematically equivalent P3 #EA3324. Accept either encoding;
      // devices whose swapchain is not wide-gamut produce the sRGB bytes.
      final Matcher cardsAreVisible = anyOf(
        contains(getColor(0xff, 0x01, 0x02)),
        contains(getColor(234, 51, 36)),
      );
      await driver.waitFor(toggleBtn);

      var cardsShouldBeVisible = false;
      Image? imageBefore = decodePng(await driver.screenshot());
      for (var i = 0; i < 10; i += 1) {
        await driver.tap(toggleBtn);
        // On OHOS the screenshot (a software re-render of the current layer
        // tree) consistently races ahead of the frame triggered by the tap;
        // wait for that frame before capturing.
        await driver.waitForCondition(const NoPendingFrame());
        cardsShouldBeVisible = !cardsShouldBeVisible;
        final Image? imageAfter = decodePng(await driver.screenshot());

        if (cardsShouldBeVisible) {
          expect(imageBefore?.data, isNot(cardsAreVisible));
          expect(imageAfter?.data, cardsAreVisible);
        } else {
          expect(imageBefore?.data, cardsAreVisible);
          expect(imageAfter?.data, isNot(cardsAreVisible));
        }

        imageBefore = imageAfter;
      }
    }, timeout: Timeout.none);
  });
}
