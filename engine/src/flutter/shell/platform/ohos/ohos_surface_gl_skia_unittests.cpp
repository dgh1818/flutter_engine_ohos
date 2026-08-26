/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#ifndef FML_USED_ON_EMBEDDER
#define FML_USED_ON_EMBEDDER
#endif

#include <native_image/native_image.h>
#include "flutter/common/graphics/gl_context_switch.h"
#include "flutter/common/task_runners.h"
#include "flutter/display_list/geometry/dl_geometry_types.h"
#include "flutter/flow/surface.h"
#include "flutter/fml/log_settings.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/time/time_delta.h"
#include "flutter/fml/time/time_point.h"
#include "flutter/shell/common/platform_view.h"
#include "flutter/shell/gpu/gpu_surface_gl_delegate.h"
#include "flutter/shell/gpu/gpu_surface_gl_skia.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"
#include "flutter/shell/platform/ohos/ohos_environment_gl.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "third_party/skia/include/gpu/ganesh/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/mock/GrMockTypes.h"

#define private public
#define protected public
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"
#include "flutter/shell/platform/ohos/ohos_egl_surface.h"
#include "flutter/shell/platform/ohos/ohos_context_gl_skia.h"
#include "flutter/shell/platform/ohos/ohos_surface_gl_skia.h"
#undef private
#undef protected

#include "flutter/fml/platform/ohos/hisysevent_c.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {
namespace fake_egl {

const EGLDisplay kFakeDisplay = reinterpret_cast<EGLDisplay>(0x9001);
const EGLConfig kFakeConfig = reinterpret_cast<EGLConfig>(0x9002);
const EGLSurface kFakeOnscreenSurface = reinterpret_cast<EGLSurface>(0x3000);
const EGLSurface kFakePbufferSurface = reinterpret_cast<EGLSurface>(0x3100);
const EGLContext kFakeCtxBase = reinterpret_cast<EGLContext>(0x2000);
const EGLSurface kFakeSurfaceA = reinterpret_cast<EGLSurface>(0x3200);
const EGLSurface kFakeSurfaceB = reinterpret_cast<EGLSurface>(0x3300);
const EGLContext kFakeContextA = reinterpret_cast<EGLContext>(0x2100);
const EGLContext kFakeContextB = reinterpret_cast<EGLContext>(0x2200);
OHNativeWindow* const kFakeNativeWindow =
    reinterpret_cast<OHNativeWindow*>(0x1100);

struct FakeEGLState {
  bool active = false;

  EGLDisplay get_display_result = kFakeDisplay;
  EGLBoolean initialize_result = EGL_TRUE;

  EGLBoolean choose_config_result = EGL_TRUE;
  EGLint choose_config_count = 1;
  bool choose_config_write_null = false;

  int fail_create_context_on_nth = 0;
  int create_context_calls = 0;
  std::vector<EGLContext> created_contexts;
  EGLConfig last_context_config = nullptr;
  EGLContext last_context_share = EGL_NO_CONTEXT;

  EGLSurface window_surface_result = kFakeOnscreenSurface;
  bool window_surface_fail = false;
  EGLSurface pbuffer_surface_result = kFakePbufferSurface;
  bool pbuffer_surface_fail = false;
  EGLint last_pbuffer_width = 0;
  EGLint last_pbuffer_height = 0;

  EGLBoolean make_current_result = EGL_TRUE;
  EGLBoolean destroy_context_result = EGL_TRUE;
  EGLContext current_context = EGL_NO_CONTEXT;
  EGLSurface current_draw = EGL_NO_SURFACE;
  EGLSurface current_read = EGL_NO_SURFACE;
  int current_context_calls = 0;
  int current_draw_calls = 0;
  int current_read_calls = 0;

  EGLint fail_query_surface_pname = 0;
  EGLint query_width = 640;
  EGLint query_height = 480;
  EGLint query_buffer_age = 0;

  EGLBoolean swap_buffers_result = EGL_TRUE;
  EGLint error_code = EGL_SUCCESS;
  int get_error_calls = 0;
  const char* extension_string = "";
  void (*proc_address_result)(void) = nullptr;
  const char* renderer_string = "StubRenderer";

  std::vector<std::string> events;

  const void* egl_anchor = reinterpret_cast<const void*>(&eglSurfaceAttrib);
  const void* gles_anchor = reinterpret_cast<const void*>(&glGetIntegerv);
};

extern FakeEGLState g_egl;

std::string HexPtr(const void* p);
size_t CountEvents(const std::string& prefix);

class FakeEGL {
 public:
  FakeEGL() {
    g_egl = FakeEGLState{};
    g_egl.active = true;
  }
  ~FakeEGL() { g_egl.active = false; }
};

}
}
}

namespace flutter {
namespace testing {
namespace {

class QuietLogs {
 public:
  QuietLogs() : scoped_(fml::LogSettings{fml::kLogFatal}) {}

 private:
  fml::ScopedSetLogSettings scoped_;
};

using fake_egl::CountEvents;
using fake_egl::FakeEGL;
using fake_egl::g_egl;
using fake_egl::HexPtr;
using fake_egl::kFakeConfig;
using fake_egl::kFakeContextA;
using fake_egl::kFakeContextB;
using fake_egl::kFakeDisplay;
using fake_egl::kFakeNativeWindow;
using fake_egl::kFakeSurfaceA;
using fake_egl::kFakeSurfaceB;

TaskRunners MakeTestTaskRunners() {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
  return TaskRunners("ohos_gl_ut", runner, runner, runner, runner);
}

}

class OhosSurfaceGLSkiaTest : public ::testing::Test {
 protected:
  void SetUp() override {
    guard_.emplace();
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    context_ = std::make_shared<OhosContextGLSkia>(
        OHOSRenderingAPI::kOpenGLES, MakeTestTaskRunners());
    ASSERT_TRUE(context_->IsValid());
    surface_ = std::make_unique<OhosSurfaceGLSkia>(context_);
    ASSERT_TRUE(surface_->IsValid());
    g_egl.events.clear();
  }
  void TearDown() override {
    surface_.reset();
    context_.reset();
    guard_.reset();
  }
  std::optional<FakeEGL> guard_;
  std::shared_ptr<OhosContextGLSkia> context_;
  std::unique_ptr<OhosSurfaceGLSkia> surface_;
};

TEST_F(OhosSurfaceGLSkiaTest, ConstructorDropsInvalidOffscreenSurface) {
  g_egl.pbuffer_surface_fail = true;
  OhosSurfaceGLSkia broken(context_);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(broken.offscreen_surface_, nullptr);
  g_egl.pbuffer_surface_fail = false;
}

TEST_F(OhosSurfaceGLSkiaTest, ConstructionKeepsValidOffscreenSurface) {
  EXPECT_NE(surface_->offscreen_surface_, nullptr);
  EXPECT_TRUE(surface_->offscreen_surface_->IsValid());
  EXPECT_TRUE(surface_->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, TeardownOnScreenContextClearsWindowState) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  ASSERT_NE(surface_->GetOnscreenSurface(), nullptr);
  g_egl.events.clear();

  surface_->TeardownOnScreenContext();
  EXPECT_EQ(surface_->GetOnscreenSurface(), nullptr);
  EXPECT_EQ(surface_->native_window_.get(), nullptr);
  EXPECT_EQ(CountEvents("MakeCurrent"), 0u);
}

TEST_F(OhosSurfaceGLSkiaTest, TeardownToleratesMissingBackingContext) {
  surface_->ohos_context_ = nullptr;
  int before = g_egl.current_context_calls;
  surface_->TeardownOnScreenContext();
  EXPECT_EQ(g_egl.current_context_calls, before);
  EXPECT_EQ(surface_->GetOnscreenSurface(), nullptr);
  surface_->ohos_context_ = context_;
}

TEST_F(OhosSurfaceGLSkiaTest, OnScreenSurfaceResizeNeedsSurfaceAndWindow) {
  EXPECT_FALSE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));

  surface_->onscreen_surface_ =
      std::make_unique<OhosEGLSurface>(kFakeSurfaceA, kFakeDisplay,
                                       kFakeContextA);
  EXPECT_FALSE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));
  surface_->onscreen_surface_ = nullptr;
}

TEST_F(OhosSurfaceGLSkiaTest, OnScreenSurfaceResizeSkipsUnchangedSize) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  size_t creations = CountEvents("CreateWindowSurface");
  EXPECT_TRUE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));
  EXPECT_EQ(CountEvents("CreateWindowSurface"), creations);
}

TEST_F(OhosSurfaceGLSkiaTest, OnScreenSurfaceResizeRecreatesWhenChanged) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  size_t creations = CountEvents("CreateWindowSurface");
  EXPECT_TRUE(surface_->OnScreenSurfaceResize(DlISize(800, 600)));
  EXPECT_EQ(CountEvents("CreateWindowSurface"), creations + 1);

  surface_->onscreen_surface_ = std::make_unique<OhosEGLSurface>(
      EGL_NO_SURFACE, kFakeDisplay, kFakeContextA);
  surface_->native_window_ = window;
  EXPECT_TRUE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));
  EXPECT_EQ(CountEvents("CreateWindowSurface"), creations + 2);
}

TEST_F(OhosSurfaceGLSkiaTest, ResourceContextMakeCurrentRequiresValidSurface) {
  g_egl.pbuffer_surface_fail = true;
  OhosSurfaceGLSkia broken(context_);
  ASSERT_FALSE(broken.IsValid());
  EXPECT_FALSE(broken.ResourceContextMakeCurrent());
  g_egl.pbuffer_surface_fail = false;
}

TEST_F(OhosSurfaceGLSkiaTest, ResourceContextMakeCurrentMapsStatus) {
  EXPECT_TRUE(surface_->ResourceContextMakeCurrent());
  EXPECT_EQ(g_egl.events.back(),
            "MakeCurrent:" + HexPtr(fake_egl::kFakePbufferSurface) + "," +
                HexPtr(fake_egl::kFakePbufferSurface) + "," +
                HexPtr(context_->resource_context_));
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(surface_->ResourceContextMakeCurrent());
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLSkiaTest, ResourceContextClearCurrentUnbindsCompletely) {
  EXPECT_TRUE(surface_->ResourceContextClearCurrent());
  EXPECT_EQ(g_egl.events.back(),
            "MakeCurrent:" + HexPtr(EGL_NO_SURFACE) + "," +
                HexPtr(EGL_NO_SURFACE) + "," + HexPtr(EGL_NO_CONTEXT));
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(surface_->ResourceContextClearCurrent());
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLSkiaTest, SetNativeWindowRejectsNullWindow) {
  EXPECT_FALSE(surface_->SetNativeWindow(nullptr));
  EXPECT_EQ(surface_->GetOnscreenSurface(), nullptr);
  EXPECT_EQ(surface_->native_window_.get(), nullptr);
}

TEST_F(OhosSurfaceGLSkiaTest, SetNativeWindowCreatesOnscreenSurface) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_TRUE(surface_->SetNativeWindow(window));
  EXPECT_EQ(surface_->native_window_.get(), window.get());
  ASSERT_NE(surface_->GetOnscreenSurface(), nullptr);
  EXPECT_TRUE(surface_->GetOnscreenSurface()->IsValid());
  EXPECT_EQ(surface_->GetOnscreenSurface()->display_, kFakeDisplay);
  EXPECT_EQ(CountEvents("CreateWindowSurface:" + HexPtr(kFakeNativeWindow)),
            1u);
}

TEST_F(OhosSurfaceGLSkiaTest, SetNativeWindowFailsWithInvalidSurface) {
  g_egl.window_surface_fail = true;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_FALSE(surface_->SetNativeWindow(window));
  ASSERT_NE(surface_->GetOnscreenSurface(), nullptr);
  EXPECT_FALSE(surface_->GetOnscreenSurface()->IsValid());
  EXPECT_EQ(surface_->native_window_.get(), window.get());
  g_egl.window_surface_fail = false;
}

TEST_F(OhosSurfaceGLSkiaTest, SetNativeWindowRemakesCurrentAfterRecreate) {
  g_egl.current_context = kFakeContextA;
  g_egl.current_draw = kFakeSurfaceA;
  g_egl.current_read = kFakeSurfaceA;
  surface_->onscreen_surface_ = std::make_unique<OhosEGLSurface>(
      kFakeSurfaceA, EGL_NO_DISPLAY, kFakeContextA);
  surface_->native_window_ =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);

  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_TRUE(surface_->SetNativeWindow(window));
  EXPECT_EQ(CountEvents("CreateWindowSurface"), 1u);
  EXPECT_EQ(g_egl.events.back(),
            "MakeCurrent:" + HexPtr(fake_egl::kFakeOnscreenSurface) + "," +
                HexPtr(fake_egl::kFakeOnscreenSurface) + "," +
                HexPtr(context_->context_));
}

TEST_F(OhosSurfaceGLSkiaTest, GLContextMakeCurrentDefaultsWithoutOnscreen) {
  auto result = surface_->GLContextMakeCurrent();
  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->GetResult());
  EXPECT_EQ(CountEvents("MakeCurrent"), 0u);

  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  g_egl.events.clear();
  auto ok = surface_->GLContextMakeCurrent();
  ASSERT_NE(ok, nullptr);
  EXPECT_TRUE(ok->GetResult());

  g_egl.make_current_result = EGL_FALSE;
  auto failed = surface_->GLContextMakeCurrent();
  ASSERT_NE(failed, nullptr);
  EXPECT_FALSE(failed->GetResult());
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLSkiaTest, GLContextClearCurrentHandlesMissingContext) {
  int before = g_egl.current_context_calls;
  surface_->ohos_context_ = nullptr;
  EXPECT_FALSE(surface_->GLContextClearCurrent());
  EXPECT_EQ(g_egl.current_context_calls, before);
  surface_->ohos_context_ = context_;
  EXPECT_TRUE(surface_->GLContextClearCurrent());

  g_egl.current_context = context_->context_;
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(surface_->GLContextClearCurrent());
  EXPECT_EQ(CountEvents("MakeCurrent"), 1u);
  g_egl.current_context = EGL_NO_CONTEXT;
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLSkiaTest, FramebufferInfoDefaultsWithoutOnscreen) {
  auto info = surface_->GLContextFramebufferInfo();
  EXPECT_TRUE(info.supports_readback);
  EXPECT_FALSE(info.supports_partial_repaint);
  EXPECT_EQ(info.horizontal_clip_alignment, 1);
  EXPECT_EQ(info.vertical_clip_alignment, 1);
  EXPECT_FALSE(info.existing_damage.has_value());
}

TEST_F(OhosSurfaceGLSkiaTest, FramebufferInfoAlignsDamageWithOnscreen) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  auto info = surface_->GLContextFramebufferInfo();
  EXPECT_TRUE(info.supports_readback);
  EXPECT_FALSE(info.supports_partial_repaint);
  EXPECT_EQ(info.horizontal_clip_alignment, 32);
  EXPECT_EQ(info.vertical_clip_alignment, 32);
  EXPECT_FALSE(info.existing_damage.has_value());
}

TEST_F(OhosSurfaceGLSkiaTest, SetDamageRegionToleratesMissingOnscreen) {
  std::optional<DlIRect> region = DlIRect::MakeLTRB(0, 0, 8, 8);
  g_egl.events.clear();
  surface_->GLContextSetDamageRegion(region);
  EXPECT_TRUE(g_egl.events.empty());

  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  g_egl.events.clear();
  surface_->GLContextSetDamageRegion(region);
  surface_->GLContextSetDamageRegion(std::nullopt);
  EXPECT_TRUE(g_egl.events.empty());
}

TEST_F(OhosSurfaceGLSkiaTest, PresentRequiresOnscreenSurface) {
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(1, 1, 9, 9);
  GLPresentInfo info{0u, damage, std::nullopt, damage};
  EXPECT_FALSE(surface_->GLContextPresent(info));
  EXPECT_EQ(CountEvents("SwapBuffers"), 0u);
}

TEST_F(OhosSurfaceGLSkiaTest, PresentSwapsBuffersWithAndWithoutTiming) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(1, 1, 9, 9);

  GLPresentInfo no_time{0u, damage, std::nullopt, damage};
  EXPECT_TRUE(surface_->GLContextPresent(no_time));
  EXPECT_EQ(CountEvents("SwapBuffers"), 1u);

  surface_->native_window_ = fml::MakeRefCounted<OHOSNativeWindow>(nullptr);
  auto time = fml::TimePoint::FromEpochDelta(
      fml::TimeDelta::FromMilliseconds(5));
  GLPresentInfo with_time{0u, damage, time, damage};
  EXPECT_TRUE(surface_->GLContextPresent(with_time));
  EXPECT_EQ(CountEvents("SwapBuffers"), 2u);

  g_egl.swap_buffers_result = EGL_FALSE;
  EXPECT_FALSE(surface_->GLContextPresent(no_time));
  g_egl.swap_buffers_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLSkiaTest, PresentToleratesNativeWindowOptFailure) {
  GraphicStubKnobGuard knob_guard;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(0, 0, 4, 4);
  auto time = fml::TimePoint::FromEpochDelta(
      fml::TimeDelta::FromMilliseconds(5));
  GLPresentInfo info{0u, damage, time, damage};
  EXPECT_TRUE(surface_->GLContextPresent(info));
  g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
  EXPECT_TRUE(surface_->GLContextPresent(info));
  EXPECT_EQ(CountEvents("SwapBuffers"), 2u);
}

TEST_F(OhosSurfaceGLSkiaTest, FBOIsAlwaysTheDefaultFramebuffer) {
  auto without = surface_->GLContextFBO(GLFrameInfo{64u, 64u});
  EXPECT_EQ(without.fbo_id, 0u);
  EXPECT_FALSE(without.existing_damage.has_value());

  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  auto with = surface_->GLContextFBO(GLFrameInfo{64u, 64u});
  EXPECT_EQ(with.fbo_id, 0u);
  EXPECT_FALSE(with.existing_damage.has_value());
}

TEST_F(OhosSurfaceGLSkiaTest, GetGLInterfaceSkipsWorkaroundOffEmulator) {
  g_egl.renderer_string = "SwiftShader";
  int created = g_egl.create_context_calls;
  auto iface = surface_->GetGLInterface();
  EXPECT_EQ(iface, nullptr);
  EXPECT_EQ(g_egl.create_context_calls, created);

  g_egl.renderer_string = nullptr;
  iface = surface_->GetGLInterface();
  EXPECT_EQ(iface, nullptr);
  EXPECT_EQ(g_egl.create_context_calls, created);
}

TEST_F(OhosSurfaceGLSkiaTest, GetGLInterfaceWorkaroundWithoutNewContext) {
  g_egl.renderer_string = "Mali-G78 OHOS emulator";
  g_egl.fail_create_context_on_nth = g_egl.create_context_calls + 1;
  auto iface = surface_->GetGLInterface();
  EXPECT_EQ(iface, nullptr);
  EXPECT_EQ(CountEvents("MakeCurrent"), 0u);
  EXPECT_EQ(CountEvents("DestroyContext"), 0u);
}

TEST_F(OhosSurfaceGLSkiaTest, GetGLInterfaceWorkaroundDancesWithNewContext) {
  g_egl.renderer_string = "Mali-G78 OHOS emulator";
  g_egl.current_context = kFakeContextA;
  g_egl.current_draw = kFakeSurfaceA;
  g_egl.current_read = kFakeSurfaceB;
  size_t before = g_egl.events.size();
  auto iface = surface_->GetGLInterface();
  EXPECT_EQ(iface, nullptr);
  EGLContext fresh = g_egl.created_contexts.back();
  std::vector<std::string> dance;
  for (size_t i = before; i < g_egl.events.size(); ++i) {
    const std::string& e = g_egl.events[i];
    if (e.rfind("CreateContext", 0) == 0 || e.rfind("MakeCurrent", 0) == 0 ||
        e.rfind("DestroyContext", 0) == 0) {
      dance.push_back(e);
    }
  }
  EXPECT_EQ(dance,
            (std::vector<std::string>{
                "CreateContext",
                "MakeCurrent:" + HexPtr(kFakeSurfaceA) + "," +
                    HexPtr(kFakeSurfaceB) + "," + HexPtr(fresh),
                "MakeCurrent:" + HexPtr(kFakeSurfaceA) + "," +
                    HexPtr(kFakeSurfaceB) + "," + HexPtr(kFakeContextA),
                "DestroyContext:" + HexPtr(fresh)}));
}

TEST_F(OhosSurfaceGLSkiaTest, CreateGPUSurfaceWrapsProvidedContext) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  GrMockOptions mock_options;
  auto gr_context = GrDirectContext::MakeMock(&mock_options);
  ASSERT_NE(gr_context, nullptr);
  auto gpu_surface = surface_->CreateGPUSurface(gr_context.get());
  ASSERT_NE(gpu_surface, nullptr);
  EXPECT_TRUE(gpu_surface->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, CreateGPUSurfaceCannotBootstrapWithoutOnscreen) {
  auto gpu_surface = surface_->CreateGPUSurface(nullptr);
  ASSERT_NE(gpu_surface, nullptr);
  EXPECT_EQ(context_->GetMainSkiaContext(), nullptr);
  EXPECT_FALSE(gpu_surface->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, CreateGPUSurfaceReusesStoredMainContext) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  GrMockOptions mock_options;
  auto gr_context = GrDirectContext::MakeMock(&mock_options);
  ASSERT_NE(gr_context, nullptr);
  context_->SetMainSkiaContext(gr_context);
  auto gpu_surface = surface_->CreateGPUSurface(nullptr);
  ASSERT_NE(gpu_surface, nullptr);
  EXPECT_TRUE(gpu_surface->IsValid());
  EXPECT_EQ(context_->GetMainSkiaContext().get(), gr_context.get());
}

TEST_F(OhosSurfaceGLSkiaTest, CreateSnapshotSurfaceBootstrapsPbuffer) {
  auto snapshot = surface_->CreateSnapshotSurface();
  ASSERT_NE(snapshot, nullptr);
  EXPECT_EQ(CountEvents("CreatePbufferSurface:1x1"), 1u);
  EXPECT_EQ(context_->GetMainSkiaContext(), nullptr);
  EXPECT_FALSE(snapshot->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, CreateSnapshotSurfaceReplacesInvalidOnscreen) {
  surface_->onscreen_surface_ = std::make_unique<OhosEGLSurface>(
      EGL_NO_SURFACE, kFakeDisplay, context_->context_);
  auto snapshot = surface_->CreateSnapshotSurface();
  ASSERT_NE(snapshot, nullptr);
  EXPECT_EQ(CountEvents("CreatePbufferSurface:1x1"), 1u);
  ASSERT_NE(surface_->GetOnscreenSurface(), nullptr);
  EXPECT_TRUE(surface_->GetOnscreenSurface()->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, CreateSnapshotSurfaceKeepsValidOnscreen) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  GrMockOptions mock_options;
  auto gr_context = GrDirectContext::MakeMock(&mock_options);
  ASSERT_NE(gr_context, nullptr);
  context_->SetMainSkiaContext(gr_context);
  size_t pbuffers = CountEvents("CreatePbufferSurface");
  auto snapshot = surface_->CreateSnapshotSurface();
  ASSERT_NE(snapshot, nullptr);
  EXPECT_EQ(CountEvents("CreatePbufferSurface"), pbuffers);
  EXPECT_TRUE(snapshot->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, PaintOffscreenDataRejectsMissingPieces) {
  OHNativeWindowBuffer* buffer =
      reinterpret_cast<OHNativeWindowBuffer*>(0x7700);
  EXPECT_FALSE(surface_->PaintOffscreenData(buffer, -1));

  surface_->native_window_ = fml::MakeRefCounted<OHOSNativeWindow>(nullptr);
  EXPECT_FALSE(surface_->PaintOffscreenData(buffer, -1));

  surface_->native_window_ =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_FALSE(surface_->PaintOffscreenData(nullptr, -1));
  surface_->native_window_ = nullptr;
}

TEST_F(OhosSurfaceGLSkiaTest, PaintOffscreenDataAttachesFlushesDestroys) {
  GraphicStubKnobGuard knob_guard;
  surface_->native_window_ =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  OHNativeWindowBuffer* buffer =
      reinterpret_cast<OHNativeWindowBuffer*>(0x7700);
  EXPECT_TRUE(surface_->PaintOffscreenData(buffer, 3));

  g_stub_graphic_fail_mask = kStubFailFlushBuffer;
  EXPECT_TRUE(surface_->PaintOffscreenData(buffer, 4));
}

TEST_F(OhosSurfaceGLSkiaTest, QuietSeverityAndContextInvalidLeg) {
  QuietLogs quiet;
  {
    OhosSurfaceGLSkia fresh(context_);
    EXPECT_TRUE(fresh.IsValid());
    EXPECT_NE(fresh.offscreen_surface_, nullptr);
  }
  {
    g_egl.pbuffer_surface_fail = true;
    OhosSurfaceGLSkia broken(context_);
    EXPECT_FALSE(broken.IsValid());
    EXPECT_EQ(broken.offscreen_surface_, nullptr);
    g_egl.pbuffer_surface_fail = false;
  }
  context_->valid_ = false;
  EXPECT_FALSE(surface_->IsValid());
  context_->valid_ = true;
  EXPECT_TRUE(surface_->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, QuietSeverityOnGpuSurfaceBootstrap) {
  QuietLogs quiet;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));

  GrMockOptions mock_options;
  auto gr_context = GrDirectContext::MakeMock(&mock_options);
  ASSERT_NE(gr_context, nullptr);
  auto wrapped = surface_->CreateGPUSurface(gr_context.get());
  ASSERT_NE(wrapped, nullptr);
  EXPECT_TRUE(wrapped->IsValid());

  auto bootstrapped = surface_->CreateGPUSurface(nullptr);
  ASSERT_NE(bootstrapped, nullptr);
  EXPECT_FALSE(bootstrapped->IsValid());
  EXPECT_EQ(context_->GetMainSkiaContext(), nullptr);
}

TEST_F(OhosSurfaceGLSkiaTest, QuietSeverityOnResizePaths) {
  QuietLogs quiet;
  EXPECT_FALSE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));

  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  size_t creations = CountEvents("CreateWindowSurface");

  EXPECT_TRUE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));
  EXPECT_EQ(CountEvents("CreateWindowSurface"), creations);

  EXPECT_TRUE(surface_->OnScreenSurfaceResize(DlISize(640, 999)));
  EXPECT_EQ(CountEvents("CreateWindowSurface"), creations + 1);
}

TEST_F(OhosSurfaceGLSkiaTest, QuietSeverityOnContextOps) {
  QuietLogs quiet;
  {
    g_egl.pbuffer_surface_fail = true;
    OhosSurfaceGLSkia broken(context_);
    ASSERT_FALSE(broken.IsValid());
    EXPECT_FALSE(broken.ResourceContextMakeCurrent());
    g_egl.pbuffer_surface_fail = false;
  }

  EXPECT_TRUE(surface_->ResourceContextClearCurrent());
  EXPECT_EQ(g_egl.events.back(),
            "MakeCurrent:" + HexPtr(EGL_NO_SURFACE) + "," +
                HexPtr(EGL_NO_SURFACE) + "," + HexPtr(EGL_NO_CONTEXT));

  auto result = surface_->GLContextMakeCurrent();
  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->GetResult());

  surface_->ohos_context_ = nullptr;
  EXPECT_FALSE(surface_->GLContextClearCurrent());
  surface_->ohos_context_ = context_;

  auto info = surface_->GLContextFramebufferInfo();
  EXPECT_TRUE(info.supports_readback);
  EXPECT_FALSE(info.supports_partial_repaint);

  size_t events_before = g_egl.events.size();
  surface_->GLContextSetDamageRegion(DlIRect::MakeLTRB(0, 0, 8, 8));
  EXPECT_EQ(g_egl.events.size(), events_before);

  std::optional<DlIRect> damage = DlIRect::MakeLTRB(1, 1, 9, 9);
  GLPresentInfo present_info{0u, damage, std::nullopt, damage};
  EXPECT_FALSE(surface_->GLContextPresent(present_info));

  auto fbo = surface_->GLContextFBO(GLFrameInfo{64u, 64u});
  EXPECT_EQ(fbo.fbo_id, 0u);
}

TEST_F(OhosSurfaceGLSkiaTest, QuietSeverityOnPaintOffscreen) {
  QuietLogs quiet;
  GraphicStubKnobGuard knob_guard;
  surface_->native_window_ =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  OHNativeWindowBuffer* buffer =
      reinterpret_cast<OHNativeWindowBuffer*>(0x7701);

  EXPECT_TRUE(surface_->PaintOffscreenData(buffer, 5));

  g_stub_graphic_fail_mask = kStubFailFlushBuffer;
  EXPECT_TRUE(surface_->PaintOffscreenData(buffer, 6));
}

TEST_F(OhosSurfaceGLSkiaTest, QuietSeverityOnGetGLInterfaceDance) {
  QuietLogs quiet;
  int created = g_egl.create_context_calls;
  EXPECT_EQ(surface_->GetGLInterface(), nullptr);
  EXPECT_EQ(g_egl.create_context_calls, created);

  g_egl.renderer_string = "Mali-G78 tail dance";
  auto iface = surface_->GetGLInterface();
  EXPECT_EQ(iface, nullptr);
  EXPECT_EQ(g_egl.create_context_calls, created + 1);

  g_egl.current_context = kFakeContextA;
  g_egl.current_draw = kFakeSurfaceA;
  g_egl.current_read = kFakeSurfaceB;
  g_egl.make_current_result = EGL_FALSE;
  g_egl.destroy_context_result = EGL_FALSE;
  size_t events_before = g_egl.events.size();
  iface = surface_->GetGLInterface();
  EXPECT_EQ(iface, nullptr);
  EGLContext fresh = g_egl.created_contexts.back();
  std::vector<std::string> dance;
  for (size_t i = events_before; i < g_egl.events.size(); ++i) {
    const std::string& e = g_egl.events[i];
    if (e.rfind("CreateContext", 0) == 0 || e.rfind("MakeCurrent", 0) == 0 ||
        e.rfind("DestroyContext", 0) == 0) {
      dance.push_back(e);
    }
  }
  EXPECT_EQ(dance,
            (std::vector<std::string>{
                "CreateContext",
                "MakeCurrent:" + HexPtr(kFakeSurfaceA) + "," +
                    HexPtr(kFakeSurfaceB) + "," + HexPtr(fresh),
                "MakeCurrent:" + HexPtr(kFakeSurfaceA) + "," +
                    HexPtr(kFakeSurfaceB) + "," + HexPtr(kFakeContextA),
                "DestroyContext:" + HexPtr(fresh)}));
  g_egl.make_current_result = EGL_TRUE;
  g_egl.destroy_context_result = EGL_TRUE;
  g_egl.current_context = EGL_NO_CONTEXT;
  g_egl.current_draw = EGL_NO_SURFACE;
  g_egl.current_read = EGL_NO_SURFACE;
}

TEST_F(OhosSurfaceGLSkiaTest, EntryCheckFailuresAreLoggedNotFatal) {
  QuietLogs quiet;
  context_->valid_ = false;
  EXPECT_FALSE(surface_->IsValid());

  EXPECT_FALSE(surface_->OnScreenSurfaceResize(DlISize(1, 1)));
  EXPECT_TRUE(surface_->ResourceContextClearCurrent());
  {
    EXPECT_FALSE(surface_->SetNativeWindow(nullptr));
    EXPECT_EQ(surface_->GetOnscreenSurface(), nullptr);
  }
  {
    auto result = surface_->GLContextMakeCurrent();
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->GetResult());
  }
  {
    auto info = surface_->GLContextFramebufferInfo();
    EXPECT_TRUE(info.supports_readback);
  }
  {
    size_t events_before = g_egl.events.size();
    surface_->GLContextSetDamageRegion(DlIRect::MakeLTRB(0, 0, 4, 4));
    EXPECT_EQ(g_egl.events.size(), events_before);
  }
  {
    std::optional<DlIRect> damage = DlIRect::MakeLTRB(1, 1, 9, 9);
    GLPresentInfo info{0u, damage, std::nullopt, damage};
    EXPECT_FALSE(surface_->GLContextPresent(info));
  }
  {
    auto fbo = surface_->GLContextFBO(GLFrameInfo{32u, 32u});
    EXPECT_EQ(fbo.fbo_id, 0u);
  }

  context_->valid_ = true;
  EXPECT_TRUE(surface_->IsValid());
}

TEST_F(OhosSurfaceGLSkiaTest, PresentSkipsTimingWhenWindowCleared) {
  QuietLogs quiet;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  ASSERT_TRUE(surface_->SetNativeWindow(window));
  surface_->native_window_ = nullptr;
  ASSERT_NE(surface_->GetOnscreenSurface(), nullptr);
  ASSERT_TRUE(surface_->GetOnscreenSurface()->IsValid());

  std::optional<DlIRect> damage = DlIRect::MakeLTRB(0, 0, 4, 4);
  auto time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(5));
  GLPresentInfo info{0u, damage, time, damage};
  EXPECT_TRUE(surface_->GLContextPresent(info));
  EXPECT_EQ(CountEvents("SwapBuffers"), 1u);
}

TEST_F(OhosSurfaceGLSkiaTest, QuietSeverityOnSnapshotSurface) {
  QuietLogs quiet;
  auto snapshot = surface_->CreateSnapshotSurface();
  ASSERT_NE(snapshot, nullptr);
  EXPECT_EQ(CountEvents("CreatePbufferSurface:1x1"), 1u);
  EXPECT_EQ(context_->GetMainSkiaContext(), nullptr);
  EXPECT_FALSE(snapshot->IsValid());
}

}
}
