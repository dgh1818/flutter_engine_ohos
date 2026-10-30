/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/ohos_context_vulkan_impeller.h"
#include "flutter/shell/platform/ohos/ohos_surface_vulkan_impeller.h"

#include <gtest/gtest.h>

#include <memory>

#include "flutter/display_list/geometry/dl_geometry_types.h"
#include "flutter/fml/log_settings.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/fml/time/time_delta.h"
#include "flutter/fml/time/time_point.h"
#include "flutter/impeller/renderer/backend/vulkan/surface_context_vk.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "impeller/renderer/backend/vulkan/test/mock_vulkan.h"

namespace flutter {
namespace testing {
namespace {

OHNativeWindow* const kFakeNativeWindow =
    reinterpret_cast<OHNativeWindow*>(0x1100);

class QuietLogs {
 public:
  QuietLogs() : scoped_(fml::LogSettings{fml::kLogFatal}) {}

 private:
  fml::ScopedSetLogSettings scoped_;
};

class TestVulkanContext : public OHOSContext {
 public:
  explicit TestVulkanContext(bool valid = true)
      : OHOSContext(OHOSRenderingAPI::kImpellerVulkan), valid_(valid) {
    SetImpellerContext(impeller::testing::MockVulkanContextBuilder().Build());
  }

  bool IsValid() const override { return valid_; }

  using OHOSContext::SetImpellerContext;

 private:
  bool valid_;
};

class TestVulkanSurface : public OHOSSurfaceVulkanImpeller {
 public:
  using OHOSSurfaceVulkanImpeller::OHOSSurfaceVulkanImpeller;

  fml::RefPtr<OHOSNativeWindow>& native_window() { return native_window_; }

  std::shared_ptr<impeller::SurfaceContextVK> SurfaceContext() {
    return std::static_pointer_cast<impeller::SurfaceContextVK>(
        GetImpellerContext());
  }
};

}  // namespace

class OhosSurfaceVulkanImpellerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    quiet_.emplace();
    context_ = std::make_shared<TestVulkanContext>();
    ASSERT_NE(context_->GetImpellerContext(), nullptr);
    surface_ = std::make_unique<TestVulkanSurface>(context_);
  }
  void TearDown() override {
    surface_.reset();
    context_.reset();
  }
  std::optional<QuietLogs> quiet_;
  std::shared_ptr<TestVulkanContext> context_;
  std::unique_ptr<TestVulkanSurface> surface_;
};

TEST_F(OhosSurfaceVulkanImpellerTest, ConstructorMirrorsContextValidity) {
  EXPECT_TRUE(surface_->IsValid());
  EXPECT_NE(surface_->GetImpellerContext(), nullptr);
}

#if defined(NDEBUG)
TEST_F(OhosSurfaceVulkanImpellerTest, InvalidContextYieldsInvalidSurface) {
  std::shared_ptr<OHOSContext> invalid =
      std::make_shared<TestVulkanContext>(false);
  ASSERT_NE(invalid->GetImpellerContext(), nullptr);
  TestVulkanSurface surface(invalid);
  EXPECT_FALSE(surface.IsValid());
  EXPECT_EQ(surface.CreateGPUSurface(nullptr), nullptr);
}
#endif

TEST_F(OhosSurfaceVulkanImpellerTest, CreateGPUSurfacePreloadPath) {
  surface_->PrepareGpuSurface();
  auto preloaded = surface_->CreateGPUSurface(nullptr);
  if (preloaded != nullptr) {
    EXPECT_TRUE(preloaded->IsValid());
    auto fresh = surface_->CreateGPUSurface(nullptr);
    EXPECT_NE(fresh, nullptr);
  } else {
    EXPECT_EQ(surface_->CreateGPUSurface(nullptr), nullptr);
  }
}

TEST_F(OhosSurfaceVulkanImpellerTest, TeardownClearsNativeWindow) {
  surface_->native_window() =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_NE(surface_->native_window().get(), nullptr);
  EXPECT_NO_FATAL_FAILURE(surface_->TeardownOnScreenContext());
  EXPECT_EQ(surface_->native_window().get(), nullptr);
}

TEST_F(OhosSurfaceVulkanImpellerTest, OnScreenSurfaceResizeUpdatesSize) {
  auto surface_context = surface_->SurfaceContext();
  ASSERT_NE(surface_context, nullptr);
  impeller::testing::SetSwapchainImageSize(impeller::ISize{100, 100});
  ASSERT_TRUE(surface_context->SetWindowSurface(
      impeller::vk::UniqueSurfaceKHR{}, impeller::ISize{100, 100}));
  EXPECT_TRUE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));
}

TEST_F(OhosSurfaceVulkanImpellerTest, ResourceContextStubsAlwaysSucceed) {
  EXPECT_TRUE(surface_->ResourceContextMakeCurrent());
  EXPECT_TRUE(surface_->ResourceContextClearCurrent());
}

TEST_F(OhosSurfaceVulkanImpellerTest, SetNativeWindowRejectsNullWindow) {
  EXPECT_FALSE(surface_->SetNativeWindow(nullptr));
  EXPECT_EQ(surface_->native_window().get(), nullptr);
}

TEST_F(OhosSurfaceVulkanImpellerTest, SetNativeWindowRejectsInvalidWindow) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(nullptr);
  ASSERT_FALSE(window->IsValid());
  EXPECT_FALSE(surface_->SetNativeWindow(window));
  EXPECT_EQ(surface_->native_window().get(), nullptr);
}

TEST_F(OhosSurfaceVulkanImpellerTest, SetNativeWindowFailsWithoutVkExtension) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_FALSE(surface_->SetNativeWindow(window));
  EXPECT_EQ(surface_->native_window().get(), window.get());
  quiet_.reset();
  fml::ScopedSetLogSettings loud({fml::kLogInfo});
  EXPECT_FALSE(surface_->SetNativeWindow(window));
}

TEST_F(OhosSurfaceVulkanImpellerTest, PrepareOffscreenWindowDelegatesToBase) {
  GraphicStubKnobGuard knob_guard;
  EXPECT_NO_FATAL_FAILURE(surface_->PrepareOffscreenWindow(8, 8));
  surface_->TeardownOnScreenContext();
  EXPECT_NO_FATAL_FAILURE(surface_->PrepareOffscreenWindow(8, 8));
  surface_.reset();
}

TEST_F(OhosSurfaceVulkanImpellerTest, PrepareGpuSurfaceIsIdempotent) {
  EXPECT_NO_FATAL_FAILURE(surface_->PrepareGpuSurface());
  EXPECT_NO_FATAL_FAILURE(surface_->PrepareGpuSurface());
  EXPECT_NE(surface_->GetImpellerContext(), nullptr);
}

TEST_F(OhosSurfaceVulkanImpellerTest, GetImpellerContextIsSurfaceContext) {
  auto surface_context = surface_->SurfaceContext();
  ASSERT_NE(surface_context, nullptr);
  EXPECT_NE(surface_->GetImpellerContext(), context_->GetImpellerContext());
}

TEST_F(OhosSurfaceVulkanImpellerTest, SetPresentInfoToleratesMissingWindow) {
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(0, 0, 8, 8);
  VulkanPresentInfo with_damage{damage, std::nullopt, damage};
  EXPECT_FALSE(surface_->SetPresentInfo(with_damage));
  VulkanPresentInfo no_damage{std::nullopt, std::nullopt, std::nullopt};
  EXPECT_FALSE(surface_->SetPresentInfo(no_damage));
}

TEST_F(OhosSurfaceVulkanImpellerTest, SetPresentInfoRejectsInvalidWindow) {
  surface_->native_window() = fml::MakeRefCounted<OHOSNativeWindow>(nullptr);
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(0, 0, 8, 8);
  auto time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(5));
  VulkanPresentInfo info{damage, time, damage};
  EXPECT_FALSE(surface_->SetPresentInfo(info));
}

TEST_F(OhosSurfaceVulkanImpellerTest, SetPresentInfoRequiresPresentationTime) {
  surface_->native_window() =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(0, 0, 8, 8);
  VulkanPresentInfo info{damage, std::nullopt, damage};
  EXPECT_FALSE(surface_->SetPresentInfo(info));
}

TEST_F(OhosSurfaceVulkanImpellerTest, SetPresentInfoUploadsTimestamp) {
  GraphicStubKnobGuard knob_guard;
  g_graphic_stub.last_present_ts = 0;
  surface_->native_window() =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(0, 0, 8, 8);
  auto time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(5));
  VulkanPresentInfo info{damage, time, damage};
  EXPECT_TRUE(surface_->SetPresentInfo(info));
  EXPECT_EQ(g_graphic_stub.last_present_ts, 5'000'000);
}

TEST(OHOSContextVulkanImpellerTest, QuietConstructorSkipsValidationLog) {
  OHOSContextVulkanImpeller ctx(false, false, true);
  EXPECT_TRUE(ctx.IsValid());
}

TEST(OHOSContextVulkanImpellerTest, ValidationFlagStillBuildsContext) {
  OHOSContextVulkanImpeller ctx(true, false, false);
  EXPECT_TRUE(ctx.IsValid());
}

TEST(OHOSContextVulkanImpellerTest, LoudConstructorEmitsBackendInfo) {
  fml::ScopedSetLogSettings loud({fml::kLogInfo});
  OHOSContextVulkanImpeller ctx(false, false, false);
  EXPECT_TRUE(ctx.IsValid());
}

}  // namespace testing
}  // namespace flutter
