/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/surface/ohos_surface.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"

namespace flutter {

extern std::map<uint64_t, bool> g_surface_is_alive;
extern std::mutex g_surface_alive_mutex;

namespace testing {

namespace {

std::shared_ptr<OHOSContext> MakeContext() {
  return std::make_shared<OHOSContext>(OHOSRenderingAPI::kSoftware);
}

[[maybe_unused]] OHNativeWindow* const kFakeWindowHandle =
    reinterpret_cast<OHNativeWindow*>(0x1000);

fml::RefPtr<OHOSNativeWindow> MakeWindow(OHNativeWindow* handle) {
  return fml::MakeRefCounted<OHOSNativeWindow>(handle);
}

class FakeOHOSSurface : public OHOSSurface {
 public:
  explicit FakeOHOSSurface(const std::shared_ptr<OHOSContext>& context)
      : OHOSSurface(context) {}

  bool IsValid() const override { return true; }
  void TeardownOnScreenContext() override { ++teardown_count_; }
  bool OnScreenSurfaceResize(const DlISize& size) override {
    ++resize_count_;
    last_resize_ = size;
    return true;
  }
  bool ResourceContextMakeCurrent() override { return false; }
  bool ResourceContextClearCurrent() override { return false; }
  bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) override {
    ++set_window_count_;
    native_window_ = std::move(window);
    return native_window_ && native_window_->IsValid();
  }
  std::unique_ptr<Surface> CreateGPUSurface(GrDirectContext*) override {
    return nullptr;
  }
  bool PaintOffscreenData(OHNativeWindowBuffer* buffer, int fence_fd) override {
    ++paint_count_;
    last_paint_buffer_ = buffer;
    last_paint_fence_fd_ = fence_fd;
    return paint_result_;
  }

  const std::shared_ptr<OHOSContext>& context() const { return ohos_context_; }

  int teardown_count_ = 0;
  int resize_count_ = 0;
  int set_window_count_ = 0;
  int paint_count_ = 0;
  bool paint_result_ = false;
  DlISize last_resize_ = {0, 0};
  OHNativeWindowBuffer* last_paint_buffer_ = nullptr;
  int last_paint_fence_fd_ = -1;
};

OHNativeWindow* const kHandleA = reinterpret_cast<OHNativeWindow*>(0x1000);
OHNativeWindow* const kHandleB = reinterpret_cast<OHNativeWindow*>(0x2000);

bool RegisteredAlive(const void* surface) {
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  auto it = g_surface_is_alive.find(reinterpret_cast<uint64_t>(surface));
  return it != g_surface_is_alive.end() && it->second;
}

class BranchFakeSurface : public OHOSSurface {
 public:
  explicit BranchFakeSurface(const std::shared_ptr<OHOSContext>& context)
      : OHOSSurface(context) {}

  bool IsValid() const override { return true; }
  void TeardownOnScreenContext() override {}
  bool OnScreenSurfaceResize(const DlISize& size) override {
    ++resize_count_;
    last_resize_ = size;
    return true;
  }
  bool ResourceContextMakeCurrent() override { return true; }
  bool ResourceContextClearCurrent() override { return true; }
  bool SetNativeWindow(fml::RefPtr<OHOSNativeWindow> window) override {
    ++set_window_count_;
    native_window_ = std::move(window);
    return native_window_ && native_window_->IsValid();
  }
  std::unique_ptr<Surface> CreateGPUSurface(GrDirectContext*) override {
    return nullptr;
  }
  bool PaintOffscreenData(OHNativeWindowBuffer* buffer, int fence_fd) override {
    ++paint_count_;
    last_paint_buffer_ = buffer;
    last_paint_fence_fd_ = fence_fd;
    return paint_result_;
  }

  const std::shared_ptr<OHOSContext>& context() const { return ohos_context_; }

  int resize_count_ = 0;
  int set_window_count_ = 0;
  int paint_count_ = 0;
  bool paint_result_ = false;
  DlISize last_resize_ = {0, 0};
  OHNativeWindowBuffer* last_paint_buffer_ = nullptr;
  int last_paint_fence_fd_ = -1;
};

}

TEST(OHOSSurface, ConstructorKeepsContext) {
  auto context = MakeContext();
  FakeOHOSSurface surface(context);
  EXPECT_EQ(surface.context().get(), context.get());
}

TEST(OHOSSurface, CreateSnapshotSurfaceReturnsNull) {
  FakeOHOSSurface surface(MakeContext());
  EXPECT_EQ(surface.CreateSnapshotSurface(), nullptr);
}

TEST(OHOSSurface, GetImpellerContextReturnsNull) {
  FakeOHOSSurface surface(MakeContext());
  EXPECT_EQ(surface.GetImpellerContext(), nullptr);
}

TEST(OHOSSurface, SetDisplayWindowRejectsNullWindow) {
  FakeOHOSSurface surface(MakeContext());
  EXPECT_FALSE(surface.SetDisplayWindow(nullptr));
  EXPECT_EQ(surface.set_window_count_, 0);
}

TEST(OHOSSurface, SetDisplayWindowRejectsInvalidWindow) {
  FakeOHOSSurface surface(MakeContext());
  EXPECT_FALSE(surface.SetDisplayWindow(MakeWindow(nullptr)));
  EXPECT_EQ(surface.set_window_count_, 0);
}

TEST(OHOSSurface, ReleaseOffscreenWindowWithoutStateIsNoop) {
  FakeOHOSSurface surface(MakeContext());
  EXPECT_NO_FATAL_FAILURE(surface.ReleaseOffscreenWindow());
}

TEST(OHOSSurface, OnFrameAvailableForUnknownSurfaceIsIgnored) {
  int unknown = 0;
  OHOSSurface::OnFrameAvailable(&unknown);
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  auto it = g_surface_is_alive.find((uint64_t)&unknown);
  ASSERT_NE(it, g_surface_is_alive.end());
  EXPECT_FALSE(it->second);
  g_surface_is_alive.erase(it);
}

TEST(OHOSSurface, NeedNewFrameDefaultsToFalse) {
  FakeOHOSSurface surface(MakeContext());
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurfaceBranch, BaseAccessorsKeepContextAndDefaultState) {
  auto context = MakeContext();
  BranchFakeSurface surface(context);
  EXPECT_EQ(surface.context().get(), context.get());
  EXPECT_EQ(surface.CreateSnapshotSurface(), nullptr);
  EXPECT_EQ(surface.GetImpellerContext(), nullptr);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurface, PrepareOffscreenWindowSucceeds) {
  GraphicStubKnobGuard guard;
  FakeOHOSSurface surface(MakeContext());
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_EQ(surface.set_window_count_, 1);
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  EXPECT_TRUE(g_surface_is_alive[(uint64_t)&surface]);
}

TEST(OHOSSurface, PrepareOffscreenWindowSameSizeEarlyOuts) {
  GraphicStubKnobGuard guard;
  FakeOHOSSurface surface(MakeContext());
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_EQ(surface.set_window_count_, 1);
}

TEST(OHOSSurface, PrepareOffscreenWindowImageCreateFailure) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailNativeImageCreate;
  FakeOHOSSurface surface(MakeContext());
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 8));
}

TEST(OHOSSurface, PrepareOffscreenWindowAcquireWindowFailure) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailAcquireNativeWindow;
  FakeOHOSSurface surface(MakeContext());
  EXPECT_FALSE(surface.PrepareOffscreenWindow(8, 8));
}

TEST(OHOSSurface, PrepareOffscreenWindowSetGeometryFailure) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
  FakeOHOSSurface surface(MakeContext());
  EXPECT_FALSE(surface.PrepareOffscreenWindow(8, 8));
}

TEST(OHOSSurface, PrepareOffscreenWindowListenerFailureStillSucceeds) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailFrameAvailableListener;
  FakeOHOSSurface surface(MakeContext());
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 8));
}

TEST(OHOSSurface, SetDisplayWindowSameWindowResizesInsteadOfReplacing) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 640;
  g_stub_geometry_height = 480;
  FakeOHOSSurface surface(MakeContext());
  auto window = MakeWindow(kFakeWindowHandle);
  ASSERT_TRUE(surface.SetNativeWindow(window));
  EXPECT_TRUE(surface.SetDisplayWindow(window));
  EXPECT_EQ(surface.resize_count_, 1);
  EXPECT_EQ(surface.last_resize_.width, 640);
  EXPECT_EQ(surface.last_resize_.height, 480);
  EXPECT_EQ(surface.set_window_count_, 1);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurface, SetDisplayWindowWithoutOffscreenWindowReplacesWindow) {
  GraphicStubKnobGuard guard;
  FakeOHOSSurface surface(MakeContext());
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kFakeWindowHandle)));
  EXPECT_EQ(surface.set_window_count_, 1);
  EXPECT_EQ(surface.paint_count_, 0);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurface, SetDisplayWindowSizeMismatchReplacesWindow) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 99;
  g_stub_geometry_height = 99;
  FakeOHOSSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kFakeWindowHandle)));
  EXPECT_EQ(surface.set_window_count_, 2);
  EXPECT_EQ(surface.paint_count_, 0);
}

TEST(OHOSSurface, SetDisplayWindowOffscreenReusePaints) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 8;
  FakeOHOSSurface surface(MakeContext());
  surface.paint_result_ = true;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kFakeWindowHandle)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurface, SetDisplayWindowOffscreenPaintFailureSchedulesFrame) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 8;
  FakeOHOSSurface surface(MakeContext());
  surface.paint_result_ = false;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kFakeWindowHandle)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_TRUE(surface.NeedNewFrame());
}

TEST(OHOSSurface, SetDisplayWindowPaintsAcquiredOffscreenBuffer) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 8;
  g_stub_graphic_fail_mask = kStubAcquireBufferSuccess;
  FakeOHOSSurface surface(MakeContext());
  surface.paint_result_ = true;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  OHOSSurface::OnFrameAvailable(&surface);
  ASSERT_TRUE(surface.SetDisplayWindow(MakeWindow(kFakeWindowHandle)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_NE(surface.last_paint_buffer_, nullptr);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurface, OnFrameAvailableAcquireFailure) {
  GraphicStubKnobGuard guard;
  FakeOHOSSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_NO_FATAL_FAILURE(OHOSSurface::OnFrameAvailable(&surface));
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  EXPECT_TRUE(g_surface_is_alive[(uint64_t)&surface]);
}

TEST(OHOSSurface, OnFrameAvailableAcquireAndRelease) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubAcquireBufferSuccess;
  FakeOHOSSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 8));
  EXPECT_NO_FATAL_FAILURE({
    OHOSSurface::OnFrameAvailable(&surface);
    OHOSSurface::OnFrameAvailable(&surface);
  });
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  EXPECT_TRUE(g_surface_is_alive[(uint64_t)&surface]);
}

TEST(OHOSSurface, OnFrameAvailableReleaseFailureDestroysBuffer) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask =
      kStubAcquireBufferSuccess | kStubFailReleaseWindowBuffer;
  FakeOHOSSurface* surface = new FakeOHOSSurface(MakeContext());
  ASSERT_TRUE(surface->PrepareOffscreenWindow(8, 8));
  EXPECT_NO_FATAL_FAILURE({
    OHOSSurface::OnFrameAvailable(surface);
    OHOSSurface::OnFrameAvailable(surface);
    delete surface;
  });
  std::lock_guard<std::mutex> lock(g_surface_alive_mutex);
  EXPECT_EQ(g_surface_is_alive.count((uint64_t)surface), 0u);
}

TEST(OHOSSurfaceBranch, PrepareFirstCreateSetsWindowAndRegistersAlive) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 1);
  EXPECT_TRUE(RegisteredAlive(&surface));
}

TEST(OHOSSurfaceBranch, PrepareSameSizeEarlyOutSkipsRecreate) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 1);
}

TEST(OHOSSurfaceBranch, PrepareSizeChangeRecreatesOffscreenWindow) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 32));
  EXPECT_EQ(surface.set_window_count_, 2);
  EXPECT_TRUE(surface.PrepareOffscreenWindow(16, 32));
  EXPECT_EQ(surface.set_window_count_, 3);
}

TEST(OHOSSurfaceBranch, PrepareImageCreateFailureStillSucceeds) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailNativeImageCreate;
  BranchFakeSurface surface(MakeContext());
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 1);
  EXPECT_TRUE(RegisteredAlive(&surface));
}

TEST(OHOSSurfaceBranch, PrepareAcquireWindowFailureReturnsFalse) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailAcquireNativeWindow;
  BranchFakeSurface surface(MakeContext());
  EXPECT_FALSE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 0);
  EXPECT_FALSE(RegisteredAlive(&surface));
}

TEST(OHOSSurfaceBranch, PrepareSetGeometryFailureReturnsFalse) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
  BranchFakeSurface surface(MakeContext());
  EXPECT_FALSE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 0);
  EXPECT_FALSE(RegisteredAlive(&surface));
}

TEST(OHOSSurfaceBranch, PrepareListenerFailureStillSucceeds) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailFrameAvailableListener;
  BranchFakeSurface surface(MakeContext());
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 1);
  EXPECT_TRUE(RegisteredAlive(&surface));
}

TEST(OHOSSurfaceBranch, ReleaseOnFreshSurfaceIsNoop) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  surface.ReleaseOffscreenWindow();
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 1);
}

TEST(OHOSSurfaceBranch, ReleaseClearsPendingBufferAndDestroysImage) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubAcquireBufferSuccess;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  OHOSSurface::OnFrameAvailable(&surface);
  surface.ReleaseOffscreenWindow();
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 2);
  EXPECT_TRUE(RegisteredAlive(&surface));
}

TEST(OHOSSurfaceBranch, ReleaseBufferFailureStillDestroysImage) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask =
      kStubAcquireBufferSuccess | kStubFailReleaseWindowBuffer;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  OHOSSurface::OnFrameAvailable(&surface);
  surface.ReleaseOffscreenWindow();
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 2);
}

TEST(OHOSSurfaceBranch, SetDisplayWindowRejectsNullThenInvalidWindow) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  EXPECT_FALSE(surface.SetDisplayWindow(nullptr));
  EXPECT_EQ(surface.set_window_count_, 0);
  EXPECT_FALSE(surface.SetDisplayWindow(MakeWindow(nullptr)));
  EXPECT_EQ(surface.set_window_count_, 0);
  EXPECT_EQ(surface.resize_count_, 0);
}

TEST(OHOSSurfaceBranch, SetDisplayWindowSameHandleSecondRefPtrResizes) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 640;
  g_stub_geometry_height = 480;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kHandleA)));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.resize_count_, 1);
  EXPECT_EQ(surface.last_resize_.width, 640);
  EXPECT_EQ(surface.last_resize_.height, 480);
  EXPECT_EQ(surface.set_window_count_, 1);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurfaceBranch, SetDisplayWindowStaleInvalidStoredWindowReplaces) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  surface.SetNativeWindow(MakeWindow(nullptr));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.resize_count_, 0);
  EXPECT_EQ(surface.set_window_count_, 2);
}

TEST(OHOSSurfaceBranch, SetDisplayWindowDifferentHandleReplaces) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kHandleA)));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleB)));
  EXPECT_EQ(surface.resize_count_, 0);
  EXPECT_EQ(surface.set_window_count_, 2);
}

TEST(OHOSSurfaceBranch, SetDisplayWindowOffscreenWidthMismatchReplaces) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 7;
  g_stub_geometry_height = 16;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.paint_count_, 0);
  EXPECT_EQ(surface.set_window_count_, 2);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurfaceBranch, SetDisplayWindowOffscreenHeightMismatchReplaces) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 17;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.paint_count_, 0);
  EXPECT_EQ(surface.set_window_count_, 2);
}

TEST(OHOSSurfaceBranch, SetDisplayWindowOffscreenMatchPaintsAcquiredBuffer) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubAcquireBufferSuccess;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 16;
  BranchFakeSurface surface(MakeContext());
  surface.paint_result_ = true;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  OHOSSurface::OnFrameAvailable(&surface);
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_NE(surface.last_paint_buffer_, nullptr);
  EXPECT_EQ(surface.last_paint_fence_fd_, -1);
  EXPECT_FALSE(surface.NeedNewFrame());
  EXPECT_EQ(surface.set_window_count_, 2);
  EXPECT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 3);
}

TEST(OHOSSurfaceBranch, SetDisplayWindowOffscreenPaintFailureSchedulesFrame) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 16;
  BranchFakeSurface surface(MakeContext());
  surface.paint_result_ = false;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_EQ(surface.last_paint_buffer_, nullptr);
  EXPECT_TRUE(surface.NeedNewFrame());
}

TEST(OHOSSurfaceBranch, OnFrameAvailableUnregisteredPointerIgnored) {
  GraphicStubKnobGuard guard;
  int not_a_surface = 0;
  OHOSSurface::OnFrameAvailable(&not_a_surface);
  EXPECT_FALSE(RegisteredAlive(&not_a_surface));
}

TEST(OHOSSurfaceBranch, OnFrameAvailableEmptyQueueKeepsSurfaceAlive) {
  GraphicStubKnobGuard guard;
  BranchFakeSurface surface(MakeContext());
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  OHOSSurface::OnFrameAvailable(&surface);
  EXPECT_TRUE(RegisteredAlive(&surface));
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurfaceBranch, OnFrameAvailableReleaseAcquireCycleFeedsPaint) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubAcquireBufferSuccess;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 16;
  BranchFakeSurface surface(MakeContext());
  surface.paint_result_ = true;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  OHOSSurface::OnFrameAvailable(&surface);
  OHOSSurface::OnFrameAvailable(&surface);
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_NE(surface.last_paint_buffer_, nullptr);
  EXPECT_FALSE(surface.NeedNewFrame());
}

TEST(OHOSSurfaceBranch, OnFrameAvailableReleaseFailureDestroysAndReacquires) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask =
      kStubAcquireBufferSuccess | kStubFailReleaseWindowBuffer;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 16;
  BranchFakeSurface surface(MakeContext());
  surface.paint_result_ = true;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  OHOSSurface::OnFrameAvailable(&surface);
  OHOSSurface::OnFrameAvailable(&surface);
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_NE(surface.last_paint_buffer_, nullptr);
}

TEST(OHOSSurfaceBranch, OnFrameAvailableNullImageStillAcquiresForPaint) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask =
      kStubFailNativeImageCreate | kStubAcquireBufferSuccess;
  g_stub_geometry_width = 8;
  g_stub_geometry_height = 16;
  BranchFakeSurface surface(MakeContext());
  surface.paint_result_ = true;
  ASSERT_TRUE(surface.PrepareOffscreenWindow(8, 16));
  EXPECT_EQ(surface.set_window_count_, 1);
  OHOSSurface::OnFrameAvailable(&surface);
  EXPECT_TRUE(surface.SetDisplayWindow(MakeWindow(kHandleA)));
  EXPECT_EQ(surface.paint_count_, 1);
  EXPECT_NE(surface.last_paint_buffer_, nullptr);
}

TEST(OHOSSurfaceBranch, DestructorErasesAliveEntryAndReleasesPendingBuffer) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask =
      kStubAcquireBufferSuccess | kStubFailReleaseWindowBuffer;
  auto* surface = new BranchFakeSurface(MakeContext());
  ASSERT_TRUE(surface->PrepareOffscreenWindow(8, 16));
  OHOSSurface::OnFrameAvailable(surface);
  EXPECT_TRUE(RegisteredAlive(surface));
  delete surface;
  EXPECT_FALSE(RegisteredAlive(surface));
}

}
}
