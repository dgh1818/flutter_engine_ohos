/*
* Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE_KHZG file.
*
* Based on NavigationChannel.java originally written by
* Copyright (C) 2013 The Flutter Authors.
*
*/

import JSONMethodCodec from '../../../plugin/common/JSONMethodCodec';
import MethodCall from '../../../plugin/common/MethodCall';
import MethodChannel, { MethodCallHandler, MethodResult } from '../../../plugin/common/MethodChannel';
import Log from '../../../util/Log';
import DartExecutor from '../dart/DartExecutor';

/**
 * Channel for handling navigation-related communication between Flutter and OpenHarmony.
 * This channel manages route navigation, including setting initial routes, pushing routes,
 * and popping routes.
 */
export default class NavigationChannel {
  private static TAG = "NavigationChannel";
  private channel: MethodChannel;

  /**
   * Constructs a new NavigationChannel instance.
   * @param dartExecutor - The DartExecutor for sending messages to Dart
   */
  constructor(dartExecutor: DartExecutor) {
    this.channel = new MethodChannel(dartExecutor, "flutter/navigation", JSONMethodCodec.INSTANCE);
    // Provide a default handler that returns an empty response to any messages
    // on this channel.
    this.channel.setMethodCallHandler(new NavigationCallback());
  }

  /**
   * Sets the initial route for navigation.
   * @param initialRoute - The initial route string
   */
  setInitialRoute(initialRoute: string): void {
    Log.i(NavigationChannel.TAG, "Sending message to set initial route to '" + initialRoute + "'");
    this.channel.invokeMethod("setInitialRoute", initialRoute);
  }

  /**
   * Pushes a new route onto the navigation stack.
   * @param route - The route string to push
   */
  pushRoute(route: string): void {
    Log.i(NavigationChannel.TAG, "Sending message to push route '" + route + "'");
    this.channel.invokeMethod("pushRoute", route);
  }

  /**
   * Pushes route information onto the navigation stack.
   * @param route - The route information string to push
   */
  pushRouteInformation(route: string): void {
    Log.i(NavigationChannel.TAG, "Sending message to push route information '" + route + "'");
    this.channel.invokeMethod("pushRouteInformation", new Map().set("location", route));
  }

  /**
   * Pops the current route from the navigation stack.
   */
  popRoute(): void {
    Log.i(NavigationChannel.TAG, "Sending message to pop route.");
    this.channel.invokeMethod("popRoute", null);
  }

  /**
   * Sets a custom method call handler for navigation events.
   * @param handler - The MethodCallHandler to handle navigation method calls
   */
  setMethodCallHandler(handler: MethodCallHandler) {
    this.channel.setMethodCallHandler(handler);
  }
}

/**
 * Default callback handler for navigation channel that returns empty responses.
 */
class NavigationCallback implements MethodCallHandler {
  /**
   * Handles method calls and returns a null success response.
   * @param call - The method call from Dart
   * @param result - The result callback to send a response
   */
  onMethodCall(call: MethodCall, result: MethodResult) {
    result.success(null);
  }
}