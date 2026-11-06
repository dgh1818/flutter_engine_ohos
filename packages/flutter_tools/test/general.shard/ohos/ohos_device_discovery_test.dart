// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter_tools/src/ohos/ohos_device_discovery.dart';

import '../../src/common.dart';

void main() {
  group('parseHdcDeviceOutput', () {
    test('parses a USB connect key as a device id', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '0005000E00AB1234',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['0005000E00AB1234']);
      expect(diagnostics, isEmpty);
    });

    test('parses a real USB connect key (with non-hex letters) as a device id', () {
      // Real `hdc list targets` output for a USB-connected phone: the connect
      // key contains non-hex letters, so a hex-only pattern would drop it.
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '63Q0226206003511',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['63Q0226206003511']);
      expect(diagnostics, isEmpty);
    });

    test('parses a wireless ip:port target as a device id', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '192.168.31.67:35095',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['192.168.31.67:35095']);
      expect(diagnostics, isEmpty);
    });

    test('[Empty] output produces no devices and one diagnostic', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput('[Empty]', deviceIds: deviceIds, diagnostics: diagnostics);

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['[Empty]']);
    });

    test('[Fail]Connect failed is routed to diagnostics, not devices', () {
      // Real hdc output observed with `hdc tconn 127.0.0.1:1` (issue #2991).
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '[Fail]Connect failed',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['[Fail]Connect failed']);
    });

    test('[Fail][E001005] device-not-found line is routed to diagnostics', () {
      // Real hdc output observed with `hdc -t 127.0.0.1:1 shell echo hi`
      // (issue #2991).
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '[Fail][E001005] Device not found or connected',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['[Fail][E001005] Device not found or connected']);
    });

    test('status words are routed to diagnostics', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        'Offline\nunauthorized\nUNKNOWN',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['Offline', 'unauthorized', 'UNKNOWN']);
    });

    test('routes an unauthorized device line to diagnostics', () {
      // Real `hdc list targets` output while the USB debugging authorization
      // dialog has not been accepted: the connect key is followed by a tab
      // and a status word, so the whole line must not become a device id.
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '63Q0226206003511\tUnauthorized',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, isEmpty);
      expect(diagnostics, <String>['63Q0226206003511\tUnauthorized']);
    });

    test('mixed output lists only the real device and reports error lines', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        '192.168.31.67:35095\n[Fail]Connect failed',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['192.168.31.67:35095']);
      expect(diagnostics, <String>['[Fail]Connect failed']);
    });

    test('handles CRLF line endings', () {
      final deviceIds = <String>[];
      final diagnostics = <String>[];

      OhosDevices.parseHdcDeviceOutput(
        'A1234567890B\r\n192.168.1.5:5555\r\n',
        deviceIds: deviceIds,
        diagnostics: diagnostics,
      );

      expect(deviceIds, <String>['A1234567890B', '192.168.1.5:5555']);
      expect(diagnostics, isEmpty);
    });
  });
}
