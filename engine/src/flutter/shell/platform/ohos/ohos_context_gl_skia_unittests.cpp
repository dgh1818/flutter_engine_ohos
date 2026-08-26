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

void ResetEnvironmentKnobs() {
  g_egl.get_display_result = fake_egl::kFakeDisplay;
  g_egl.initialize_result = EGL_TRUE;
  g_egl.choose_config_result = EGL_TRUE;
}

}

class OhosContextGLSkiaTest : public ::testing::Test {
 protected:
  void SetUp() override {
    guard_.emplace();
    fml::MessageLoop::EnsureInitializedForCurrentThread();
  }
  void TearDown() override { guard_.reset(); }
  std::optional<FakeEGL> guard_;

  OhosContextGLSkia MakeContext() {
    return OhosContextGLSkia(OHOSRenderingAPI::kOpenGLES,
                             MakeTestTaskRunners());
  }
};

TEST_F(OhosContextGLSkiaTest, ConstructorFailsWithoutDisplay) {
  g_egl.get_display_result = EGL_NO_DISPLAY;
  {
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    EXPECT_EQ(context.Environment()->Display(), EGL_NO_DISPLAY);
    EXPECT_EQ(CountEvents("Initialize"), 0u);
    context.context_ = EGL_NO_CONTEXT;
    context.resource_context_ = EGL_NO_CONTEXT;
  }
  EXPECT_EQ(CountEvents("DestroyContext"), 0u);
  EXPECT_EQ(CountEvents("Terminate"), 0u);
}

TEST_F(OhosContextGLSkiaTest, ConstructorFailsWhenInitializeFails) {
  g_egl.initialize_result = EGL_FALSE;
  auto context = MakeContext();
  EXPECT_FALSE(context.IsValid());
  EXPECT_EQ(context.Environment()->Display(), kFakeDisplay);
  EXPECT_EQ(CountEvents("ChooseConfig"), 0u);
  context.context_ = EGL_NO_CONTEXT;
  context.resource_context_ = EGL_NO_CONTEXT;
}

TEST_F(OhosContextGLSkiaTest, ConstructorFailsWhenChooseConfigFails) {
  g_egl.choose_config_result = EGL_FALSE;
  auto context = MakeContext();
  EXPECT_FALSE(context.IsValid());
  EXPECT_EQ(CountEvents("ChooseConfig"), 1u);
  EXPECT_EQ(CountEvents("CreateContext"), 0u);
  EXPECT_GE(g_egl.get_error_calls, 1);
  context.context_ = EGL_NO_CONTEXT;
  context.resource_context_ = EGL_NO_CONTEXT;
}

TEST_F(OhosContextGLSkiaTest, ConstructorFailsWithoutUsableConfig) {
  {
    g_egl.choose_config_count = 0;
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    context.context_ = EGL_NO_CONTEXT;
    context.resource_context_ = EGL_NO_CONTEXT;
  }
  {
    g_egl.events.clear();
    g_egl.choose_config_count = 1;
    g_egl.choose_config_write_null = true;
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    EXPECT_EQ(CountEvents("ChooseConfig"), 1u);
    context.context_ = EGL_NO_CONTEXT;
    context.resource_context_ = EGL_NO_CONTEXT;
  }
}

TEST_F(OhosContextGLSkiaTest, ConstructorFailsWithoutMainContext) {
  g_egl.fail_create_context_on_nth = 1;
  auto context = MakeContext();
  EXPECT_FALSE(context.IsValid());
  EXPECT_EQ(g_egl.create_context_calls, 1);
  context.resource_context_ = EGL_NO_CONTEXT;
}

TEST_F(OhosContextGLSkiaTest, ConstructorFailsWithoutResourceContext) {
  g_egl.fail_create_context_on_nth = 2;
  auto context = MakeContext();
  EXPECT_FALSE(context.IsValid());
  EXPECT_EQ(g_egl.create_context_calls, 2);
  EXPECT_NE(context.context_, EGL_NO_CONTEXT);
  context.resource_context_ = EGL_NO_CONTEXT;
}

TEST_F(OhosContextGLSkiaTest, ConstructorSucceedsAndSharesMainContext) {
  auto context = MakeContext();
  EXPECT_TRUE(context.IsValid());
  EXPECT_EQ(g_egl.create_context_calls, 2);
  EXPECT_EQ(g_egl.last_context_share, context.context_);
  EXPECT_EQ(context.Config(), kFakeConfig);
  EXPECT_EQ(context.Environment()->Display(), kFakeDisplay);
  EXPECT_NE(context.resource_context_, EGL_NO_CONTEXT);
}

TEST_F(OhosContextGLSkiaTest, ClearCurrentShortCircuitsForOtherContext) {
  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  EXPECT_TRUE(context.ClearCurrent());
  EXPECT_EQ(CountEvents("MakeCurrent"), 0u);
}

TEST_F(OhosContextGLSkiaTest, ClearCurrentUnbindsOwnedContext) {
  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  g_egl.current_context = context.context_;
  g_egl.make_current_result = EGL_TRUE;
  EXPECT_TRUE(context.ClearCurrent());
  EXPECT_EQ(g_egl.events.back(),
            "MakeCurrent:" + HexPtr(EGL_NO_SURFACE) + "," +
                HexPtr(EGL_NO_SURFACE) + "," + HexPtr(EGL_NO_CONTEXT));
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(context.ClearCurrent());
  EXPECT_GE(g_egl.get_error_calls, 1);
}

TEST_F(OhosContextGLSkiaTest, CreateNewContextReusesStoredConfig) {
  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  EGLContext fresh = context.CreateNewContext();
  EXPECT_NE(fresh, EGL_NO_CONTEXT);
  EXPECT_EQ(g_egl.create_context_calls, 3);
  EXPECT_EQ(g_egl.last_context_config, kFakeConfig);
  EXPECT_EQ(g_egl.last_context_share, EGL_NO_CONTEXT);

  g_egl.fail_create_context_on_nth = g_egl.create_context_calls + 1;
  EXPECT_EQ(context.CreateNewContext(), EGL_NO_CONTEXT);
}

TEST_F(OhosContextGLSkiaTest, OnscreenSurfaceSelectionByWindowKind) {
  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  {
    auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
    auto surface = context.CreateOnscreenSurface(window);
    ASSERT_TRUE(surface->IsValid());
    EXPECT_EQ(surface->display_, kFakeDisplay);
    EXPECT_EQ(surface->context_, context.context_);
    EXPECT_EQ(CountEvents("CreateWindowSurface:" + HexPtr(kFakeNativeWindow)),
              1u);
    EXPECT_EQ(CountEvents("CreatePbufferSurface"), 0u);
  }
  {
    auto fake = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow, true);
    auto surface = context.CreateOnscreenSurface(fake);
    ASSERT_TRUE(surface->IsValid());
    EXPECT_EQ(CountEvents("CreateWindowSurface"), 1u);
    EXPECT_EQ(CountEvents("CreatePbufferSurface:1x1"), 1u);
    EXPECT_EQ(surface->context_, context.context_);
  }
}

TEST_F(OhosContextGLSkiaTest, OffscreenAndPbufferSurfaceParameters) {
  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  auto offscreen = context.CreateOffscreenSurface();
  ASSERT_TRUE(offscreen->IsValid());
  EXPECT_EQ(g_egl.last_pbuffer_width, 1);
  EXPECT_EQ(g_egl.last_pbuffer_height, 1);
  EXPECT_EQ(offscreen->context_, context.resource_context_);
  EXPECT_EQ(offscreen->display_, kFakeDisplay);

  auto pbuffer = context.CreatePbufferSurface(8, 9);
  ASSERT_TRUE(pbuffer->IsValid());
  EXPECT_EQ(g_egl.last_pbuffer_width, 8);
  EXPECT_EQ(g_egl.last_pbuffer_height, 9);
  EXPECT_EQ(pbuffer->context_, context.context_);
  EXPECT_EQ(CountEvents("CreatePbufferSurface:8x9"), 1u);
}

TEST_F(OhosContextGLSkiaTest, SurfaceCreationFailuresYieldInvalidSurfaces) {
  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  g_egl.window_surface_fail = true;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_FALSE(context.CreateOnscreenSurface(window)->IsValid());
  g_egl.window_surface_fail = false;
  g_egl.pbuffer_surface_fail = true;
  EXPECT_FALSE(context.CreateOffscreenSurface()->IsValid());
  EXPECT_FALSE(context.CreatePbufferSurface(2, 3)->IsValid());
}

TEST_F(OhosContextGLSkiaTest, DestructorReleasesSkiaContextAndTearsDown) {
  GrMockOptions mock_options;
  auto gr_context = GrDirectContext::MakeMock(&mock_options);
  ASSERT_NE(gr_context, nullptr);
  {
    auto context = MakeContext();
    ASSERT_TRUE(context.IsValid());
    context.SetMainSkiaContext(gr_context);
    EXPECT_FALSE(gr_context->abandoned());
  }
  EXPECT_TRUE(gr_context->abandoned());
  EXPECT_EQ(CountEvents("CreatePbufferSurface:1x1"), 1u);
  EXPECT_EQ(CountEvents("DestroyContext"), 2u);
  EXPECT_EQ(CountEvents("Terminate"), 1u);
}

TEST_F(OhosContextGLSkiaTest, DestructorToleratesContextTeardownFailure) {
  g_egl.destroy_context_result = EGL_FALSE;
  {
    auto context = MakeContext();
    ASSERT_TRUE(context.IsValid());
    EXPECT_EQ(g_egl.get_error_calls, 0);
  }
  EXPECT_EQ(CountEvents("DestroyContext"), 2u);
  EXPECT_GE(g_egl.get_error_calls, 2);
}

TEST_F(OhosContextGLSkiaTest, DestructorSkipsReleaseWhenMakeCurrentFails) {
  GrMockOptions mock_options;
  auto gr_context = GrDirectContext::MakeMock(&mock_options);
  ASSERT_NE(gr_context, nullptr);
  {
    auto context = MakeContext();
    ASSERT_TRUE(context.IsValid());
    context.SetMainSkiaContext(gr_context);
    g_egl.make_current_result = EGL_FALSE;
  }
  EXPECT_FALSE(gr_context->abandoned());
  EXPECT_EQ(CountEvents("MakeCurrent"), 1u);
  EXPECT_EQ(CountEvents("DestroyContext"), 2u);
}

TEST_F(OhosContextGLSkiaTest, QuietSeverityOnFailingConstructors) {
  QuietLogs quiet;
  {
    g_egl.get_display_result = EGL_NO_DISPLAY;
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    EXPECT_EQ(context.Environment()->Display(), EGL_NO_DISPLAY);
    context.context_ = EGL_NO_CONTEXT;
    context.resource_context_ = EGL_NO_CONTEXT;
  }
  {
    ResetEnvironmentKnobs();
    g_egl.initialize_result = EGL_FALSE;
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    context.context_ = EGL_NO_CONTEXT;
    context.resource_context_ = EGL_NO_CONTEXT;
  }
  {
    ResetEnvironmentKnobs();
    g_egl.choose_config_result = EGL_FALSE;
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    EXPECT_EQ(CountEvents("ChooseConfig"), 1u);
    context.context_ = EGL_NO_CONTEXT;
    context.resource_context_ = EGL_NO_CONTEXT;
  }
  {
    ResetEnvironmentKnobs();
    g_egl.fail_create_context_on_nth = g_egl.create_context_calls + 1;
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    EXPECT_EQ(context.context_, EGL_NO_CONTEXT);
    context.resource_context_ = EGL_NO_CONTEXT;
  }
  {
    ResetEnvironmentKnobs();
    g_egl.fail_create_context_on_nth = g_egl.create_context_calls + 2;
    auto context = MakeContext();
    EXPECT_FALSE(context.IsValid());
    EXPECT_NE(context.context_, EGL_NO_CONTEXT);
    context.resource_context_ = EGL_NO_CONTEXT;
  }
}

TEST_F(OhosContextGLSkiaTest, QuietSeverityOnSuccessAndSurfaceCreation) {
  QuietLogs quiet;
  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  EXPECT_NE(context.context_, EGL_NO_CONTEXT);
  EXPECT_NE(context.resource_context_, EGL_NO_CONTEXT);
  EXPECT_EQ(context.Config(), fake_egl::kFakeConfig);

  auto fake_window =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow, true);
  auto onscreen = context.CreateOnscreenSurface(fake_window);
  ASSERT_TRUE(onscreen->IsValid());
  EXPECT_EQ(onscreen->context_, context.context_);
  EXPECT_EQ(CountEvents("CreatePbufferSurface:1x1"), 1u);

  auto offscreen = context.CreateOffscreenSurface();
  ASSERT_TRUE(offscreen->IsValid());
  EXPECT_EQ(offscreen->context_, context.resource_context_);
  EXPECT_EQ(g_egl.last_pbuffer_width, 1);
  EXPECT_EQ(g_egl.last_pbuffer_height, 1);

  auto pbuffer = context.CreatePbufferSurface(8, 9);
  ASSERT_TRUE(pbuffer->IsValid());
  EXPECT_EQ(pbuffer->context_, context.context_);
  EXPECT_EQ(CountEvents("CreatePbufferSurface:8x9"), 1u);
}

TEST_F(OhosContextGLSkiaTest, QuietSeverityOnTeardownAndClearFailure) {
  QuietLogs quiet;
  g_egl.destroy_context_result = EGL_FALSE;
  {
    auto context = MakeContext();
    ASSERT_TRUE(context.IsValid());
  }
  EXPECT_EQ(CountEvents("DestroyContext"), 2u);

  auto context = MakeContext();
  ASSERT_TRUE(context.IsValid());
  g_egl.current_context = context.context_;
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(context.ClearCurrent());
  g_egl.make_current_result = EGL_TRUE;
  g_egl.current_context = EGL_NO_CONTEXT;
}

}
}
