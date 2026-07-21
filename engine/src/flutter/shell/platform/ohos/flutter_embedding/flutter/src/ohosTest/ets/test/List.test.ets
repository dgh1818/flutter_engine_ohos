/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import FlutterAbilityAndEntryDelegateTest from './embedding/ohos/FlutterAbilityAndEntryDelegateTest.test';
import FlutterAbilityTest from './embedding/ohos/FlutterAbilityTest.test';
import FlutterEntryTest from './embedding/ohos/FlutterEntryTest.test';
import FlutterManagerTest from './embedding/ohos/FlutterManagerTest.test';
import SettingTest from './embedding/ohos/SettingTest.test';
import BinaryCodecTest from './plugin/common/BinaryCodecTest.test';
import PlatformViewRegistryImplTest from './plugin/platform/PlatformViewRegistryImplTest.test';
import PlatformViewWrapperTest from './plugin/platform/PlatformViewWrapperTest.test';
import PlatformViewsControllerTest from './plugin/platform/PlatformViewsControllerTest.test';
import RawPointerCoordTest from './plugin/platform/RawPointerCoordTest.test';
import RootDvModelManagerTest from './plugin/platform/RootDvModelManagerTest.test';
import ByteBufferTest from './plugin/util/ByteBufferTest.test';
import PlatformChannelUnitTest from './embedding/engine/systemchannels/PlatformChannel.test';
import RestorationChannelUnitTest from './embedding/engine/systemchannels/RestorationChannel.test';
import SettingsChannelUnitTest from './embedding/engine/systemchannels/SettingsChannel.test';
import TextInputChannelUnitTest from './embedding/engine/systemchannels/TextInputChannel.test';
import JSONMethodCodecUnitTest from './plugin/common/JSONMethodCodec.test';
import MethodCallUnitTest from './plugin/common/MethodCall.test';

import DartMessengerTest from './embedding/engine/dart/DartMessengerTest.test';
import ApplicationInfoTest from './embedding/engine/loader/ApplicationInfoTest.test';
import FlutterApplicationInfoTest from './embedding/engine/loader/FlutterApplicationInfoTest.test';
import FlutterLoaderTest from './embedding/engine/loader/FlutterLoaderTest.test';
import FlutterMutatorViewTest from './embedding/engine/mutatorsstack/FlutterMutatorViewTest.test';
import FlutterMutatorsStackTest from './embedding/engine/mutatorsstack/FlutterMutatorsStackTest.test';
import FlutterRendererTest from './embedding/engine/renderer/FlutterRendererTest.test';
import TouchEventProcessorTest from './embedding/ohos/TouchEventProcessorTest.test';
import TouchEventTrackerTest from './embedding/ohos/TouchEventTrackerTest.test';
import WindowInfoRepositoryCallbackAdapterWrapperTest
  from './embedding/ohos/WindowInfoRepositoryCallbackAdapterWrapperTest.test';
import FlutterEngineTest from './embedding/engine/FlutterEngineTest.test';
import FlutterEngineCacheTest from './embedding/engine/FlutterEngineCacheTest.test';
import FlutterEngineConnectionRegistryTest from './embedding/engine/FlutterEngineConnectionRegistryTest.test';
import FlutterEngineGroupCacheTest from './embedding/engine/FlutterEngineGroupCacheTest.test';
import FlutterEngineGroupTest from './embedding/engine/FlutterEngineGroupTest.test';
import FlutterNapiTest from './embedding/engine/FlutterNapiTest.test';
import FlutterOverlaySurfaceTest from './embedding/engine/FlutterOverlaySurfaceTest.test';
import FlutterShellArgsTest from './embedding/engine/FlutterShellArgsTest.test';
import BinaryMessengerTest from './plugin/common/BinaryMessengerTest.test';
import DartExecutorTest from './embedding/engine/dart/DartExecutorTest.test';

import PathUtilsTest from './plugin/util/PathUtils.test';
import StringUtilsTest from './plugin/util/StringUtils.test';
import ToolUtilsTest from './plugin/util/ToolUtils.test';
import MessageChannelUtilsTest from './plugin/util/MessageChannelUtils.test';
import LogTest from './plugin/util/Log.test';
import TraceSectionTest from './plugin/util/TraceSection.test';
import FlutterInjectorTest from './FlutterInjector.test';
import SendableBinaryCodecTest from './plugin/common/SendableBinaryCodec.test';
import dynamicViewJsonTest from './view/DynamicView/dynamicViewJson.test';
import PlatformViewTest from './plugin/platform/PlatformView.test';
import FlutterRunArgumentsTest from './view/FlutterRunArguments.test';
import LifecycleChannelTest from './embedding/engine/systemchannels/LifecycleChannel.test';
import FlutterExceptionTest from './plugin/common/FlutterException.test';
import JSONMessageCodecTest from './plugin/common/JSONMessageCodec.test';
import SendableJSONMessageCodecTest from './plugin/common/SendableJSONMessageCodec.test';
import SendableJSONMethodCodecTest from './plugin/common/SendableJSONMethodCodec.test';
import SendableStringCodecTest from './plugin/common/SendableStringCodec.test';
import StandardMessageCodecTest from './plugin/common/StandardMessageCodec.test';
import StandardMethodCodecTest from './plugin/common/StandardMethodCodec.test';
import StringCodecTest from './plugin/common/StringCodec.test';
import ListenableEditingStateTest from './plugin/editing/ListenableEditingState.test';
import TextEditingDeltaTest from './plugin/editing/TextEditingDelta.test';
import TextInputPluginTest from './plugin/editing/TextInputPlugin.test';
import LocalizationPluginTest from './plugin/localization/LocalizationPlugin.test';
import SendableStandardMessageCodecTest from './plugin/common/SendableStandardMessageCodec.test';
import SendableStandardMethodCodecTest from './plugin/common/SendableStandardMethodCodec.test';

import json5ToJsonTest from './util/Json5ToJson.test';
import methodChannelTest from './plugin/common/MethodChannel.test';
import eventChannelTest from './plugin/common/EventChannel.test';
import basicMessageChannelTest from './plugin/common/BasicMessageChannel.test';
import backgroundMethodChannelTest from './plugin/common/BackgroundMethodChannel.test';
import backgroundBasicMessageChannelTest from './plugin/common/BackgroundBasicMessageChannel.test';
import previewSpanCoalescingTest from './plugin/editing/PreviewSpanCoalescing.test';
import previewTextMergeContextTest from './plugin/editing/PreviewTextMergeContext.test';
import previewTextSupportTest from './plugin/editing/PreviewTextSupport.test';
import previewTextStringMergeTest from './plugin/editing/PreviewTextStringMerge.test';
import previewTextCoordinatorTest from './plugin/editing/PreviewTextCoordinator.test';
import customTouchEventTest from './plugin/platform/CustomTouchEvent.test';
import mouseCursorPluginTest from './plugin/mouse/MouseCursorPlugin.test';
import splitViewConfigPluginTest from './plugin/splitview/SplitViewConfigPlugin.test';
import sensitiveContentPluginTest from './plugin/view/SensitiveContentPlugin.test';
import platformPluginTest from './plugin/platform/PlatformPlugin.test';
import accessibilityChannelTest from './embedding/engine/systemchannels/AccessibilityChannel.test';
import keyEventChannelTest from './embedding/engine/systemchannels/KeyEventChannel.test';
import navigationChannelTest from './embedding/engine/systemchannels/NavigationChannel.test';
import statusBarClickChannelTest from './embedding/engine/systemchannels/StatusBarClickChannel.test';
import sensitiveContentChannelTest from './embedding/engine/systemchannels/SensitiveContentChannel.test';
import platformViewsChannelTest from './embedding/engine/systemchannels/PlatformViewsChannel.test';
import flutterAbilityLaunchConfigsTest from './embedding/ohos/FlutterAbilityLaunchConfigs.test';
import platformViewInfoTest from './embedding/ohos/PlatformViewInfo.test';
import keyboardMapTest from './embedding/ohos/KeyboardMap.test';
import keyDataTest from './embedding/ohos/KeyData.test';
import keyEmbedderResponderTest from './embedding/ohos/KeyEmbedderResponder.test';
import keyboardChannelTest from './embedding/engine/systemchannels/KeyboardChannel.test';
import mouseCursorChannelTest from './embedding/engine/systemchannels/MouseCursorChannel.test';
import localizationChannelTest from './embedding/engine/systemchannels/LocalizationChannel.test';
import displayMetricsChannelTest from './embedding/engine/systemchannels/DisplayMetricsChannel.test';
import testChannelTest from './embedding/engine/systemchannels/TestChannel.test';
import nativeVsyncChannelTest from './embedding/engine/systemchannels/NativeVsyncChannel.test';
import splitViewConfigSystemChannelTest from './embedding/engine/systemchannels/SplitViewConfigSystemChannel.test';
import keyEventHandlerTest from './embedding/ohos/KeyEventHandler.test';
import keyboardManagerTest from './embedding/ohos/KeyboardManager.test';

export default function testsuite() {
  FlutterAbilityAndEntryDelegateTest();
  FlutterAbilityTest();
  FlutterEntryTest();
  FlutterManagerTest();
  SettingTest();
  BinaryCodecTest();
  PlatformViewRegistryImplTest();
  PlatformViewWrapperTest();
  PlatformViewsControllerTest();
  RawPointerCoordTest();
  RootDvModelManagerTest();
  ByteBufferTest();
  PlatformChannelUnitTest();
  RestorationChannelUnitTest();
  SettingsChannelUnitTest();
  TextInputChannelUnitTest();

  JSONMethodCodecUnitTest();
  MethodCallUnitTest();
  DartMessengerTest();
  ApplicationInfoTest();
  FlutterApplicationInfoTest();
  FlutterLoaderTest()
  FlutterMutatorViewTest();
  FlutterMutatorsStackTest();
  FlutterRendererTest();
  TouchEventProcessorTest();
  TouchEventTrackerTest();
  WindowInfoRepositoryCallbackAdapterWrapperTest();
  FlutterEngineTest();
  FlutterEngineCacheTest();
  FlutterEngineConnectionRegistryTest();
  FlutterEngineGroupCacheTest();
  FlutterEngineGroupTest();
  FlutterNapiTest();
  FlutterOverlaySurfaceTest();
  FlutterShellArgsTest();
  BinaryMessengerTest();
  DartExecutorTest();
  PathUtilsTest();
  StringUtilsTest();
  ToolUtilsTest();
  MessageChannelUtilsTest();
  LogTest();
  TraceSectionTest();
  FlutterInjectorTest();
  SendableBinaryCodecTest();
  dynamicViewJsonTest();
  PlatformViewTest();
  PlatformViewTest();
  FlutterRunArgumentsTest();
  LifecycleChannelTest();
  FlutterExceptionTest();
  JSONMessageCodecTest();
  SendableJSONMessageCodecTest();
  SendableJSONMethodCodecTest();
  SendableStandardMessageCodecTest();
  SendableStandardMethodCodecTest();
  SendableStringCodecTest();
  StandardMessageCodecTest();
  StandardMethodCodecTest();
  StringCodecTest();
  ListenableEditingStateTest();
  TextEditingDeltaTest();
  TextInputPluginTest();
  LocalizationPluginTest();
  json5ToJsonTest();
  methodChannelTest();
  eventChannelTest();
  basicMessageChannelTest();
  backgroundMethodChannelTest();
  backgroundBasicMessageChannelTest();
  previewSpanCoalescingTest();
  previewTextMergeContextTest();
  previewTextSupportTest();
  previewTextStringMergeTest();
  previewTextCoordinatorTest();
  customTouchEventTest();
  mouseCursorPluginTest();
  splitViewConfigPluginTest();
  sensitiveContentPluginTest();
  platformPluginTest();
  accessibilityChannelTest();
  keyEventChannelTest();
  navigationChannelTest();
  statusBarClickChannelTest();
  sensitiveContentChannelTest();
  platformViewsChannelTest();
  flutterAbilityLaunchConfigsTest();
  platformViewInfoTest();
  keyboardMapTest();
  keyDataTest();
  keyEmbedderResponderTest();
  keyboardChannelTest();
  mouseCursorChannelTest();
  localizationChannelTest();
  displayMetricsChannelTest();
  testChannelTest();
  nativeVsyncChannelTest();
  splitViewConfigSystemChannelTest();
  keyEventHandlerTest();
  keyboardManagerTest();
}
