/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <fcntl.h>
#include <gtest/gtest.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <memory>
#include "flutter/fml/log_settings.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/ohos_surface_software.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"
#include "flutter/shell/platform/ohos/types.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace flutter {

bool GetSkColorType(int32_t buffer_format,
                    SkColorType* color_type,
                    SkAlphaType* alpha_type);

namespace testing {

namespace {

std::shared_ptr<OHOSContext> MakeSoftwareContext() {
  return std::make_shared<OHOSContext>(OHOSRenderingAPI::kSoftware);
}

OHNativeWindow* const kFakeWindowHandle =
    reinterpret_cast<OHNativeWindow*>(0x2000);

fml::RefPtr<OHOSNativeWindow> MakeWindow(OHNativeWindow* handle) {
  return fml::MakeRefCounted<OHOSNativeWindow>(handle);
}

constexpr int kUtFdSize = 4 << 20;

int TestBackingFd() {
  static const int kFd = [] {
    char ut_fd_path[4096];
    snprintf(ut_fd_path, sizeof(ut_fd_path), "%s/.ohos_surface_sw_ut_fd",
             GetUtTmpDir());
    int fd = static_cast<int>(::syscall(SYS_openat, AT_FDCWD, ut_fd_path,
                                        O_CREAT | O_RDWR | O_TRUNC, 0600));
    if (fd >= 0 && ::ftruncate(fd, kUtFdSize) != 0) {
      fd = -1;
    }
    return fd;
  }();
  return kFd;
}

int StubFallbackOpen(const char* path, int /*flags*/) {
  // 与 ace_graphic_ndk_stub 的回退路径同源(都经 GetUtTmpDir 拼接),
  // 两边必须一致才能匹配。
  char expect[4096];
  snprintf(expect, sizeof(expect), "%s/.stub_graphic_buffer_fd", GetUtTmpDir());
  return ::strcmp(path, expect) == 0 ? TestBackingFd() : -1;
}

class StubBackingFdGuard {
 public:
  StubBackingFdGuard() { UpdateOpenFunc(&StubFallbackOpen); }
  ~StubBackingFdGuard() { UpdateOpenFunc(nullptr); }
};

bool MappableFdSourceAvailable() {
  if (TestBackingFd() >= 0) {
    return true;
  }
#if defined(SYS_memfd_create)
  int fd = static_cast<int>(::syscall(SYS_memfd_create, "ut_probe", 0));
  if (fd >= 0) {
    ::close(fd);
    return true;
  }
#endif
  return false;
}

}  // namespace

TEST(OHOSSurfaceSoftware, SupportsRgba8888) {
  SkColorType color_type = kUnknown_SkColorType;
  SkAlphaType alpha_type = kUnknown_SkAlphaType;
  EXPECT_TRUE(GetSkColorType(kPixelFmtRgba8888, &color_type, &alpha_type));
  EXPECT_EQ(color_type, kRGBA_8888_SkColorType);
  EXPECT_EQ(alpha_type, kPremul_SkAlphaType);
}

TEST(OHOSSurfaceSoftware, RejectsOtherFormats) {
  SkColorType color_type;
  SkAlphaType alpha_type;
  EXPECT_FALSE(GetSkColorType(0, &color_type, &alpha_type));
  EXPECT_FALSE(GetSkColorType(kPixelFmtRgba8888 + 1, &color_type, &alpha_type));
}

TEST(OHOSSurfaceSoftware, IsValidAlwaysTrue) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  EXPECT_TRUE(surface.IsValid());
}

TEST(OHOSSurfaceSoftware, ResourceContextIsNeverCurrent) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  EXPECT_FALSE(surface.ResourceContextMakeCurrent());
  EXPECT_FALSE(surface.ResourceContextClearCurrent());
}

TEST(OHOSSurfaceSoftware, CreateGPUSurfaceReturnsValidSoftwareSurface) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  auto gpu_surface = surface.CreateGPUSurface(nullptr);
  ASSERT_NE(gpu_surface, nullptr);
  EXPECT_TRUE(gpu_surface->IsValid());
}

TEST(OHOSSurfaceSoftware, TeardownAndResizeAreHarmless) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  EXPECT_NO_FATAL_FAILURE(surface.TeardownOnScreenContext());
  EXPECT_TRUE(surface.OnScreenSurfaceResize(DlISize(10, 10)));
}

TEST(OHOSSurfaceSoftware, SetNativeWindowRejectsNullAndInvalid) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  EXPECT_FALSE(surface.SetNativeWindow(fml::RefPtr<OHOSNativeWindow>()));
  EXPECT_FALSE(surface.SetNativeWindow(MakeWindow(nullptr)));
}

TEST(OHOSSurfaceSoftware, SetNativeWindowAcceptsValidWindow) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  EXPECT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
}

TEST(OHOSSurfaceSoftware, AcquireBackingStoreCreatesAndCaches) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  auto first = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->width(), 10);
  EXPECT_EQ(first->height(), 10);
  EXPECT_EQ(first->imageInfo().colorType(), kRGBA_8888_SkColorType);
  EXPECT_EQ(first->imageInfo().alphaType(), kPremul_SkAlphaType);
  auto same = surface.AcquireBackingStore(DlISize(10, 10));
  EXPECT_EQ(same.get(), first.get());
  auto grown = surface.AcquireBackingStore(DlISize(12, 10));
  ASSERT_NE(grown, nullptr);
  EXPECT_EQ(grown->width(), 12);
  EXPECT_NE(grown.get(), first.get());
  auto taller = surface.AcquireBackingStore(DlISize(12, 12));
  ASSERT_NE(taller, nullptr);
  EXPECT_EQ(taller->height(), 12);
  EXPECT_NE(taller.get(), grown.get());
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreRejectsNullBackingStore) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  EXPECT_FALSE(surface.PresentBackingStore(nullptr));
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreRejectsInvalidWindow) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  ASSERT_FALSE(surface.SetNativeWindow(MakeWindow(nullptr)));
  EXPECT_FALSE(surface.PresentBackingStore(backing));
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreRejectsPixellessBackingStore) {
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  auto pixelless = SkSurfaces::Null(16, 16);
  ASSERT_NE(pixelless, nullptr);
  EXPECT_FALSE(surface.PresentBackingStore(pixelless));
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreRequestBufferFailure) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailRequestBuffer;
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  EXPECT_FALSE(surface.PresentBackingStore(backing));
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreMissingBufferHandle) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailGetBufferHandle;
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  EXPECT_FALSE(surface.PresentBackingStore(backing));
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreMmapFailure) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubBufferHandleBadFd;
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  EXPECT_FALSE(surface.PresentBackingStore(backing));
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreSucceeds) {
  GraphicStubKnobGuard guard;
  StubBackingFdGuard fd_guard;
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  const bool presented = surface.PresentBackingStore(backing);
  if (!presented && !MappableFdSourceAvailable()) {
    GTEST_SKIP() << "no mappable fd source for the stub BufferHandle";
  }
  EXPECT_TRUE(presented);
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreFlushFailure) {
  GraphicStubKnobGuard guard;
  StubBackingFdGuard fd_guard;
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  const bool ok = surface.PresentBackingStore(backing);
  if (!ok && !MappableFdSourceAvailable()) {
    GTEST_SKIP() << "no mappable fd source for the stub BufferHandle";
  }
  ASSERT_TRUE(ok);
  g_stub_graphic_fail_mask = kStubFailFlushBuffer;
  EXPECT_FALSE(surface.PresentBackingStore(backing));
}

TEST(OHOSSurfaceSoftware, LogSeverityReplaySurfaceAndPresentEdges) {
  {
    fml::ScopedSetLogSettings loud({fml::kLogInfo});
    GraphicStubKnobGuard guard;
    OHOSSurfaceSoftware surface(MakeSoftwareContext());
    EXPECT_NE(surface.CreateGPUSurface(nullptr), nullptr);
    EXPECT_NO_FATAL_FAILURE(surface.TeardownOnScreenContext());
    EXPECT_TRUE(surface.OnScreenSurfaceResize(DlISize(8, 8)));
    EXPECT_FALSE(surface.SetNativeWindow(fml::RefPtr<OHOSNativeWindow>()));
    EXPECT_FALSE(surface.SetNativeWindow(MakeWindow(nullptr)));
    EXPECT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
    auto first = surface.AcquireBackingStore(DlISize(8, 8));
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(surface.AcquireBackingStore(DlISize(8, 8)).get(), first.get());
    EXPECT_FALSE(surface.PresentBackingStore(nullptr));
    g_stub_graphic_fail_mask = kStubFailRequestBuffer;
    EXPECT_FALSE(surface.PresentBackingStore(first));
    g_stub_graphic_fail_mask = kStubFailGetBufferHandle;
    EXPECT_FALSE(surface.PresentBackingStore(first));
  }
  {
    fml::ScopedSetLogSettings quiet({fml::kLogFatal});
    GraphicStubKnobGuard guard;
    OHOSSurfaceSoftware surface(MakeSoftwareContext());
    EXPECT_NE(surface.CreateGPUSurface(nullptr), nullptr);
    EXPECT_FALSE(surface.SetNativeWindow(MakeWindow(nullptr)));
    EXPECT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
    auto store = surface.AcquireBackingStore(DlISize(6, 6));
    ASSERT_NE(store, nullptr);
    EXPECT_FALSE(surface.PresentBackingStore(nullptr));
    g_stub_graphic_fail_mask = kStubFailRequestBuffer;
    EXPECT_FALSE(surface.PresentBackingStore(store));
  }
}

TEST(OHOSSurfaceSoftware, EmitsPresentInfoLogs) {
  fml::ScopedSetLogSettings loud({fml::kLogInfo});
  GraphicStubKnobGuard guard;
  StubBackingFdGuard fd_guard;
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  const bool presented = surface.PresentBackingStore(backing);
  if (!presented && !MappableFdSourceAvailable()) {
    GTEST_SKIP() << "no mappable fd source for the stub BufferHandle";
  }
  EXPECT_TRUE(presented);
  g_stub_graphic_fail_mask = kStubFailFlushBuffer;
  EXPECT_FALSE(surface.PresentBackingStore(backing));
}

TEST(OHOSSurfaceSoftware, PresentBackingStoreSkipsUnknownBufferFormat) {
  GraphicStubKnobGuard guard;
  g_stub_buffer_format = kPixelFmtRgba8888 + 1;
  StubBackingFdGuard fd_guard;
  OHOSSurfaceSoftware surface(MakeSoftwareContext());
  ASSERT_TRUE(surface.SetNativeWindow(MakeWindow(kFakeWindowHandle)));
  auto backing = surface.AcquireBackingStore(DlISize(10, 10));
  ASSERT_NE(backing, nullptr);
  const bool presented = surface.PresentBackingStore(backing);
  if (!presented && !MappableFdSourceAvailable()) {
    GTEST_SKIP() << "no mappable fd source for the stub BufferHandle";
  }
  EXPECT_TRUE(presented);
}

}  // namespace testing
}  // namespace flutter
