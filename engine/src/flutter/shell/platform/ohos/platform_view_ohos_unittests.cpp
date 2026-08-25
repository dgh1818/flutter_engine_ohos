/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/platform_view_ohos.h"

#include <gtest/gtest.h>

#include <memory>

#include "flutter/common/settings.h"
#include "flutter/common/task_runners.h"
#include "flutter/fml/thread.h"
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

#if defined(OHOS_X64_UNITTEST)

// Minimal delegate: no-op overrides except settings, which the
// PlatformViewOHOS constructor queries for the rendering API and HCPP flag.
class NullDelegate : public PlatformView::Delegate {
 public:
  void OnPlatformViewCreated(std::unique_ptr<Surface> surface) override {}
  void OnPlatformViewDestroyed() override {}
  void OnPlatformViewScheduleFrame() override {}
  void OnPlatformViewAddView(int64_t view_id,
                             const ViewportMetrics& viewport_metrics,
                             AddViewCallback callback) override {}
  void OnPlatformViewRemoveView(int64_t view_id,
                                RemoveViewCallback callback) override {}
  void OnPlatformViewSendViewFocusEvent(const ViewFocusEvent& event) override {}
  void OnPlatformViewSetNextFrameCallback(const fml::closure& closure) override {}
  void OnPlatformViewSetViewportMetrics(int64_t view_id,
                                        const ViewportMetrics& metrics) override {}
  void OnPlatformViewDispatchPlatformMessage(
      std::unique_ptr<PlatformMessage> message) override {}
  void OnPlatformViewDispatchPointerDataPacket(
      std::unique_ptr<PointerDataPacket> packet) override {}
  void OnPlatformViewDispatchSemanticsAction(int64_t view_id,
                                             int32_t node_id,
                                             SemanticsAction action,
                                             fml::MallocMapping args) override {}
  void OnPlatformViewSetSemanticsEnabled(bool enabled) override {}
  void OnPlatformViewSetAccessibilityFeatures(int32_t flags) override {}
  void OnPlatformViewRegisterTexture(std::shared_ptr<Texture> texture) override {}
  void OnPlatformViewUnregisterTexture(int64_t texture_id) override {}
  void OnPlatformViewMarkTextureFrameAvailable(int64_t texture_id) override {}
  void LoadDartDeferredLibrary(intptr_t loading_unit_id,
                               std::unique_ptr<const fml::Mapping> snapshot_data,
                               std::unique_ptr<const fml::Mapping> snapshot_instructions) override {}
  void LoadDartDeferredLibraryError(intptr_t loading_unit_id,
                                    const std::string error_message,
                                    bool transient) override {}
  void UpdateAssetResolverByType(std::unique_ptr<AssetResolver> updated_asset_resolver,
                                 AssetResolver::AssetResolverType type) override {}
  const Settings& OnPlatformViewGetSettings() const override { return settings_; }

  Settings settings_;
};

// x64 模拟器：hdc shell 起不了 JIT VM（mmap 无权限），绕开 OHOSShellHolder
// 直接构造 PlatformViewOHOS。
class TestViewHandle {
 public:
  explicit TestViewHandle(const Settings& settings)
      : runners_("test",
                 platform_thread_.GetTaskRunner(),
                 raster_thread_.GetTaskRunner(),
                 ui_thread_.GetTaskRunner(),
                 io_thread_.GetTaskRunner()) {
    delegate_.settings_ = settings;
    napi_facade_ = std::make_shared<PlatformViewOHOSNapi>(nullptr);
    view_ = std::make_unique<PlatformViewOHOS>(
        delegate_, runners_, napi_facade_, /*use_software_rendering=*/true);
  }

  bool IsValid() const { return view_ != nullptr; }
  PlatformViewOHOS* view() { return view_.get(); }

 private:
  NullDelegate delegate_;
  fml::Thread platform_thread_;
  fml::Thread raster_thread_;
  fml::Thread ui_thread_;
  fml::Thread io_thread_;
  TaskRunners runners_;
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  std::unique_ptr<PlatformViewOHOS> view_;
};

#else

// 真机：原路径，完整 Shell + Dart VM。
class TestViewHandle {
 public:
  explicit TestViewHandle(const Settings& settings)
      : napi_facade_(std::make_shared<PlatformViewOHOSNapi>(nullptr)),
        holder_(
            std::make_unique<OHOSShellHolder>(settings, napi_facade_, nullptr)) {}

  bool IsValid() const { return holder_->IsValid(); }
  PlatformViewOHOS* view() { return holder_->GetPlatformView().get(); }

 private:
  std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  std::unique_ptr<OHOSShellHolder> holder_;
};

#endif  // defined(OHOS_X64_UNITTEST)

}  // namespace

// HCPP 默认关闭：开关未打开时 IsHybridCompositionEnabled() 为 false，
// CreateExternalViewEmbedder() 返回非 null 的默认 embedder
// （HCPP embedder 仅在 hybrid_composition_enabled_ 时创建）。
TEST(PlatformViewOHOSHcpp, DisabledByDefault) {
  auto settings = MakeTestSettings();
  EXPECT_FALSE(settings.enable_ohos_hybrid_composition);

  TestViewHandle handle(settings);
  ASSERT_TRUE(handle.IsValid());
  auto platform_view = handle.view();
  ASSERT_NE(platform_view, nullptr);

  EXPECT_FALSE(platform_view->IsHybridCompositionEnabled());
  // CreateExternalViewEmbedder 在 PlatformViewOHOS 中是 private override，
  // 经基类 public 虚接口调用触发虚表分发。
  PlatformView* base_view = platform_view;
  auto embedder = base_view->CreateExternalViewEmbedder();
  ASSERT_NE(embedder, nullptr);
  EXPECT_EQ(embedder->CompositeEmbeddedView(1), nullptr);
}

// HCPP 以 Settings.enable_ohos_hybrid_composition 方式打开，但软件渲染后端
// 被排除：开关不无条件生效（HCPP 依赖 ArkUI 系统合成层）。
TEST(PlatformViewOHOSHcpp, DisabledWithSoftwareRenderingDespiteSettings) {
  auto settings = MakeTestSettings();
  settings.enable_ohos_hybrid_composition = true;

  TestViewHandle handle(settings);
  ASSERT_TRUE(handle.IsValid());
  auto platform_view = handle.view();
  ASSERT_NE(platform_view, nullptr);

  EXPECT_FALSE(platform_view->IsHybridCompositionEnabled());
  PlatformView* base_view = platform_view;
  auto embedder = base_view->CreateExternalViewEmbedder();
  ASSERT_NE(embedder, nullptr);
  EXPECT_EQ(embedder->CompositeEmbeddedView(1), nullptr);
}

// HCPP 关闭时 overlay 窗口的生命周期方法必须安全：
// - SetHybridCompositionOverlayWindow 在 embedder 未创建时走 stash 分支；
// - ClearHybridCompositionOverlayWindowSync 在无 embedder 时直接返回，
//   不会阻塞等待 raster 线程。
TEST(PlatformViewOHOSHcpp, OverlayWindowCallsSafeWhenDisabled) {
  auto settings = MakeTestSettings();

  TestViewHandle handle(settings);
  ASSERT_TRUE(handle.IsValid());
  auto platform_view = handle.view();
  ASSERT_NE(platform_view, nullptr);

  int dummy_window = 0;
  platform_view->SetHybridCompositionOverlayWindow(&dummy_window);
  platform_view->ClearHybridCompositionOverlayWindowSync();
  // 清空路径（nullptr）同样安全。
  platform_view->SetHybridCompositionOverlayWindow(nullptr);
  platform_view->ClearHybridCompositionOverlayWindowSync();
}

}  // namespace testing
}  // namespace flutter
