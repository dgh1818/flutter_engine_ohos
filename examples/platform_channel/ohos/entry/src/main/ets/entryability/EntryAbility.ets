/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

import { FlutterAbility, FlutterEngine } from '@ohos/flutter_ohos';
import { GeneratedPluginRegistrant } from '../plugins/GeneratedPluginRegistrant';
import MethodChannel, {
  MethodCallHandler,
  MethodResult
} from '@ohos/flutter_ohos/src/main/ets/plugin/common/MethodChannel';
import MethodCall from '@ohos/flutter_ohos/src/main/ets/plugin/common/MethodCall';
import EventChannel, { StreamHandler, EventSink } from '@ohos/flutter_ohos/src/main/ets/plugin/common/EventChannel';
import { batteryInfo } from '@kit.BasicServicesKit';
import { commonEventManager } from '@kit.BasicServicesKit';

const BATTERY_CHANNEL = 'samples.flutter.io/battery';
const CHARGING_CHANNEL = 'samples.flutter.io/charging';

export default class EntryAbility extends FlutterAbility {
  private methodChannel: MethodChannel | null = null;
  private eventChannel: EventChannel | null = null;
  private subscriber: commonEventManager.CommonEventSubscriber | null = null;
  private isStreamActive: boolean = false;
  private subscriberRequestId: number = 0;

  configureFlutterEngine(flutterEngine: FlutterEngine) {
    super.configureFlutterEngine(flutterEngine);
    GeneratedPluginRegistrant.registerWith(flutterEngine);

    // Register MethodChannel to handle battery level queries
    this.methodChannel = new MethodChannel(flutterEngine.getDartExecutor(), BATTERY_CHANNEL);
    this.methodChannel.setMethodCallHandler({
      onMethodCall: (call: MethodCall, result: MethodResult): void => {
        if (call.method === 'getBatteryLevel') {
          try {
            const chargingStatus = batteryInfo.chargingStatus;
            if (chargingStatus === batteryInfo.BatteryChargeState.NONE) {
              result.error('UNAVAILABLE', 'Battery level not available.', null);
              return;
            }
            const batteryLevel = batteryInfo.batterySOC;
            result.success(batteryLevel);
          } catch (error) {
            result.error('UNAVAILABLE', 'Battery level not available.', null);
          }
        } else {
          result.notImplemented();
        }
      }
    });

    // Register EventChannel to monitor charging status changes
    this.eventChannel = new EventChannel(flutterEngine.getDartExecutor(), CHARGING_CHANNEL);
    this.eventChannel.setStreamHandler({
      onListen: (args: ESObject, events: EventSink): void => {
        // Send initial charging status
        this.sendChargingStatus(events);

        // Mark stream as active to prevent race condition with onCancel
        this.isStreamActive = true;

        // Increment request ID to invalidate previous callbacks
        const currentRequestId = ++this.subscriberRequestId;

        // Subscribe to battery changed common event
        const subscribeInfo: commonEventManager.CommonEventSubscribeInfo = {
          events: ['usual.event.BATTERY_CHANGED']
        };

        commonEventManager.createSubscriber(subscribeInfo, (err, subscriber) => {
          // Check if this callback is still valid (not superseded by a newer onListen)
          if (err || !this.isStreamActive || currentRequestId !== this.subscriberRequestId) {
            if (subscriber) {
              commonEventManager.unsubscribe(subscriber, (unsubErr) => {
                if (unsubErr) {
                  console.error('Failed to unsubscribe early: ' + JSON.stringify(unsubErr));
                }
              });
            }
            if (err) {
              console.error('Failed to create subscriber: ' + JSON.stringify(err));
              events.error('UNAVAILABLE', 'Charging status unavailable', null);
            }
            return;
          }

          // Only the latest request reaches here
          this.subscriber = subscriber;

          commonEventManager.subscribe(this.subscriber, (err, data) => {
            if (err) {
              console.error('Failed to subscribe event: ' + JSON.stringify(err));
              return;
            }

            // Send new status when battery state changes
            this.sendChargingStatus(events);
          });
        });
      },

      onCancel: (args: ESObject): void => {
        // Mark stream as inactive to prevent late subscription
        this.isStreamActive = false;

        // Unsubscribe from battery events
        if (this.subscriber) {
          commonEventManager.unsubscribe(this.subscriber, (err) => {
            if (err) {
              console.error('Failed to unsubscribe event: ' + JSON.stringify(err));
            }
          });
          this.subscriber = null;
        }
      }
    });
  }

  private sendChargingStatus(events: EventSink): void {
    try {
      const chargingStatus = batteryInfo.chargingStatus;
      // Handle unknown battery status
      if (chargingStatus === batteryInfo.BatteryChargeState.NONE) {
        events.error('UNAVAILABLE', 'Charging status unavailable', null);
        return;
      }
      // ENABLE or FULL indicates charging
      const isCharging = (chargingStatus === batteryInfo.BatteryChargeState.ENABLE ||
        chargingStatus === batteryInfo.BatteryChargeState.FULL);
      events.success(isCharging ? 'charging' : 'discharging');
    } catch (error) {
      events.error('UNAVAILABLE', 'Charging status unavailable', null);
    }
  }

  onDestroy() {
    // Clean up resources
    this.isStreamActive = false;
    if (this.subscriber) {
      commonEventManager.unsubscribe(this.subscriber, (err) => {
        if (err) {
          console.error('Failed to unsubscribe event: ' + JSON.stringify(err));
        }
      });
      this.subscriber = null;
    }
    super.onDestroy();
  }
}
