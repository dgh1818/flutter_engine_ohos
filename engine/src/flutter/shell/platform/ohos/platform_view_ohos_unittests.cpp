/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/platform_view_ohos.h"

#include <gtest/gtest.h>

#include <memory>

#include "flutter/common/settings.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"

namespace flutter {
namespace testing {

namespace {

// 软件渲染不需要 GPU，可在测试进程中安全构建完整 shell（与
// ohos_shell_holder_unittests.cpp 相同的模式）。
Settings MakeTestSettings() {
  Settings settings;
  settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  return settings;
}

}  // namespace

// HCPP 默认关闭：开关未打开时 IsHybridCompositionEnabled() 为 false，
// CreateExternalViewEmbedder() 返回 nullptr，走外部纹理 / TLHC 老路径。
TEST(PlatformViewOHOSHcpp, DisabledByDefault) {
  auto settings = MakeTestSettings();
  EXPECT_FALSE(settings.enable_ohos_hybrid_composition);

  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  auto platform_view = holder->GetPlatformView();
  ASSERT_NE(platform_view.get(), nullptr);

  EXPECT_FALSE(platform_view->IsHybridCompositionEnabled());
  // CreateExternalViewEmbedder 在 PlatformViewOHOS 中是 private override，
  // 经基类 public 虚接口调用触发虚表分发。
  PlatformView* base_view = platform_view.get();
  EXPECT_EQ(base_view->CreateExternalViewEmbedder(), nullptr);
}

// HCPP 以 Settings.enable_ohos_hybrid_composition 方式打开，但软件渲染后端
// 被排除：开关不无条件生效（HCPP 依赖 ArkUI 系统合成层）。
TEST(PlatformViewOHOSHcpp, DisabledWithSoftwareRenderingDespiteSettings) {
  auto settings = MakeTestSettings();
  settings.enable_ohos_hybrid_composition = true;

  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  auto platform_view = holder->GetPlatformView();
  ASSERT_NE(platform_view.get(), nullptr);

  EXPECT_FALSE(platform_view->IsHybridCompositionEnabled());
  PlatformView* base_view = platform_view.get();
  EXPECT_EQ(base_view->CreateExternalViewEmbedder(), nullptr);
}

// HCPP 关闭时 overlay 窗口的生命周期方法必须安全：
// - SetHybridCompositionOverlayWindow 在 embedder 未创建时走 stash 分支；
// - ClearHybridCompositionOverlayWindowSync 在无 embedder 时直接返回，
//   不会阻塞等待 raster 线程。
TEST(PlatformViewOHOSHcpp, OverlayWindowCallsSafeWhenDisabled) {
  auto settings = MakeTestSettings();
  auto napi_facade = std::make_shared<PlatformViewOHOSNapi>(nullptr);
  auto holder =
      std::make_unique<OHOSShellHolder>(settings, napi_facade, nullptr);
  ASSERT_TRUE(holder->IsValid());
  auto platform_view = holder->GetPlatformView();
  ASSERT_NE(platform_view.get(), nullptr);

  int dummy_window = 0;
  platform_view->SetHybridCompositionOverlayWindow(&dummy_window);
  platform_view->ClearHybridCompositionOverlayWindowSync();
  // 清空路径（nullptr）同样安全。
  platform_view->SetHybridCompositionOverlayWindow(nullptr);
  platform_view->ClearHybridCompositionOverlayWindowSync();
}

}  // namespace testing
}  // namespace flutter
