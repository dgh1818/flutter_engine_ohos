/*
* Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_HW file.
*/
import 'dart:async';
import 'package:flutter/services.dart';

class ChannelMessageHandler {
  static const MethodChannel _channel = MethodChannel('flutter/statusBarClick');

  static bool isInit = false;
  static DateTime? lastCallTime;

  static final StreamController<dynamic> _streamController =
      StreamController<dynamic>.broadcast();

  // init Channel
  static void init() {
    if (isInit) {
      return;
    }
    isInit = true;
    _channel.setMethodCallHandler((call) async {
      throttle(() {
        _streamController.add({
        'method': call.method,
        'arguments': call.arguments,
      });
      }, const Duration(milliseconds: 1000));
    });
  }

  static throttle(Function() callback, Duration duration) {
    final now = DateTime.now();
    if (lastCallTime == null || now.difference(lastCallTime!) >= duration) {
      lastCallTime = now;
      callback();
    }
  }

  static Stream<dynamic> get messageStream => _streamController.stream;

  // close
  static void dispose() {
    _streamController.close();
  }
}
