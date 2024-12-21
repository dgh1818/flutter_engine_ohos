/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "native_accessibility_channel.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_accessibility_bridge.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

namespace flutter {

NativeAccessibilityChannel::NativeAccessibilityChannel() {}

NativeAccessibilityChannel::~NativeAccessibilityChannel() {}

/**
 * 通知flutter框架ohos平台无障碍屏幕朗读已开启
 */
void NativeAccessibilityChannel::OnOhosAccessibilityEnabled(
    int64_t shellHolderId) {
  FML_DLOG(INFO) << "NativeAccessibilityChannel -> OnOhosAccessibilityEnabled";
  this->SetSemanticsEnabled(shellHolderId, true);
}

/**
 * 通知flutter框架ohos平台无障碍屏幕朗读未开启
 */
void NativeAccessibilityChannel::OnOhosAccessibilityDisabled(
    int64_t shellHolderId) {
  FML_DLOG(INFO) << "NativeAccessibilityChannel -> OnOhosAccessibilityDisabled";
  this->SetSemanticsEnabled(shellHolderId, false);
}

/**
 * Native无障碍通道传递语义感知，若开启则实时更新语义树信息
 */
void NativeAccessibilityChannel::SetSemanticsEnabled(int64_t shellHolderId,
                                                     bool enabled) {
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shellHolderId);
  ohos_shell_holder->GetPlatformView()->SetSemanticsEnabled(enabled);
}

/**
 * Native无障碍通道设置无障碍特征类型，如:无障碍导航、字体加粗等
 */
void NativeAccessibilityChannel::SetAccessibilityFeatures(int64_t shellHolderId,
                                                          int32_t flags) {
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shellHolderId);
  ohos_shell_holder->GetPlatformView()->SetAccessibilityFeatures(flags);
}

/**
 * Native无障碍通道分发flutter屏幕语义动作，如:点击、滑动等
 */
void NativeAccessibilityChannel::DispatchSemanticsAction(
    int64_t shellHolderId,
    int32_t id,
    flutter::SemanticsAction action,
    fml::MallocMapping args) {
  auto ohos_shell_holder = reinterpret_cast<OHOSShellHolder*>(shellHolderId);
  ohos_shell_holder->GetPlatformView()->PlatformView::DispatchSemanticsAction(
      id, action, fml::MallocMapping());
}

/**
 * 更新flutter无障碍相关语义信息
 */
void NativeAccessibilityChannel::UpdateSemantics(
    flutter::SemanticsNodeUpdates update,
    flutter::CustomAccessibilityActionUpdates actions) {
  auto ohos_a11y_bridge = OhosAccessibilityBridge::GetInstance();
  ohos_a11y_bridge->UpdateSemantics(update, actions);
}

/**
 * 设置无障碍消息处理器，通过无障碍通道发送处理dart侧传递的相关信息
 */
void NativeAccessibilityChannel::SetAccessibilityMessageHandler(
    std::shared_ptr<AccessibilityMessageHandler> handler) {
  this->handler = handler;
}

/**
 * 利用通道内部类AccessibilityMessageHandler处理主动播报事件
 */
void NativeAccessibilityChannel::AccessibilityMessageHandler::Announce(
    std::unique_ptr<char[]>& message) {
  auto ohos_a11y_bridge = OhosAccessibilityBridge::GetInstance();
  ohos_a11y_bridge->Announce(message);
}
}  // namespace flutter