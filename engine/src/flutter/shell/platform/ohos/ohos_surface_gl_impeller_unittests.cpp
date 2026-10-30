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
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#ifndef FML_USED_ON_EMBEDDER
#define FML_USED_ON_EMBEDDER
#endif

#include "flutter/display_list/geometry/dl_geometry_types.h"
#include "flutter/flow/surface.h"
#include "flutter/fml/log_settings.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/memory/ref_ptr.h"
#include "flutter/shell/gpu/gpu_surface_gl_delegate.h"
#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "impeller/toolkit/egl/context.h"
#include "impeller/toolkit/egl/surface.h"

#define private public
#define protected public
#include "flutter/shell/platform/ohos/ohos_surface_gl_impeller.h"
#include "flutter/shell/platform/ohos/surface/ohos_surface.h"
#undef private
#undef protected

#include "gtest/gtest.h"

namespace flutter {
namespace testing {
namespace fake_egl {

const EGLDisplay kFakeDisplay = reinterpret_cast<EGLDisplay>(0x9001);
const EGLConfig kFakeConfig = reinterpret_cast<EGLConfig>(0x9002);
const EGLSurface kFakeOnscreenSurface = reinterpret_cast<EGLSurface>(0x3000);
const EGLSurface kFakePbufferSurface = reinterpret_cast<EGLSurface>(0x3100);
OHNativeWindow* const kFakeNativeWindow =
    reinterpret_cast<OHNativeWindow*>(0x1100);

struct FakeEGLState {
  bool active = false;

  EGLDisplay get_display_result = kFakeDisplay;
  EGLBoolean initialize_result = EGL_TRUE;

  EGLBoolean choose_config_result = EGL_TRUE;
  EGLint choose_config_count = 1;
  bool choose_config_write_null = false;
  int fail_choose_config_on_nth = 0;
  int choose_config_calls = 0;

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

}  // namespace fake_egl

namespace {

class QuietLogs {
 public:
  QuietLogs() : scoped_(fml::LogSettings{fml::kLogFatal}) {}

 private:
  fml::ScopedSetLogSettings scoped_;
};

using fake_egl::CountEvents;
using fake_egl::g_egl;
using fake_egl::HexPtr;
using fake_egl::kFakeNativeWindow;

class TestOHOSContext : public OHOSContext {
 public:
  TestOHOSContext() : OHOSContext(OHOSRenderingAPI::kOpenGLES) {}
};

impeller::egl::ConfigDescriptor OnscreenDescriptor() {
  impeller::egl::ConfigDescriptor desc;
  desc.api = impeller::egl::API::kOpenGLES2;
  desc.color_format = impeller::egl::ColorFormat::kRGBA8888;
  desc.depth_bits = impeller::egl::DepthBits::kZero;
  desc.stencil_bits = impeller::egl::StencilBits::kEight;
  desc.samples = impeller::egl::Samples::kFour;
  desc.surface_type = impeller::egl::SurfaceType::kWindow;
  return desc;
}

void InstallEglMembers(OHOSSurfaceGLImpeller* surface) {
  surface->display_ = std::make_unique<impeller::egl::Display>();
  ASSERT_TRUE(surface->display_->IsValid());
  surface->onscreen_config_ =
      surface->display_->ChooseConfig(OnscreenDescriptor());
  ASSERT_NE(surface->onscreen_config_, nullptr);
  impeller::egl::ConfigDescriptor offscreen_desc = OnscreenDescriptor();
  offscreen_desc.surface_type = impeller::egl::SurfaceType::kPBuffer;
  surface->offscreen_config_ = surface->display_->ChooseConfig(offscreen_desc);
  ASSERT_NE(surface->offscreen_config_, nullptr);
  surface->onscreen_context_ =
      surface->display_->CreateContext(*surface->onscreen_config_, nullptr);
  ASSERT_NE(surface->onscreen_context_, nullptr);
  surface->offscreen_context_ = surface->display_->CreateContext(
      *surface->offscreen_config_, surface->onscreen_context_.get());
  ASSERT_NE(surface->offscreen_context_, nullptr);
  surface->offscreen_surface_ = surface->display_->CreatePixelBufferSurface(
      *surface->offscreen_config_, 1u, 1u);
  ASSERT_NE(surface->offscreen_surface_, nullptr);
}

}  // namespace

class OhosSurfaceGLImpellerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    guard_.emplace();
    quiet_.emplace();
    context_ = std::make_shared<TestOHOSContext>();
    g_egl.pbuffer_surface_fail = true;
    surface_ = std::make_unique<OHOSSurfaceGLImpeller>(context_, false);
    g_egl.pbuffer_surface_fail = false;
    g_egl.events.clear();
  }
  void TearDown() override {
    surface_.reset();
    context_.reset();
    quiet_.reset();
    guard_.reset();
  }
  std::optional<fake_egl::FakeEGL> guard_;
  std::optional<QuietLogs> quiet_;
  std::shared_ptr<TestOHOSContext> context_;
  std::unique_ptr<OHOSSurfaceGLImpeller> surface_;
};

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnImpellerContext) {
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(broken.display_, nullptr);
  EXPECT_EQ(broken.onscreen_config_, nullptr);
  EXPECT_EQ(broken.onscreen_context_, nullptr);
  EXPECT_EQ(broken.offscreen_context_, nullptr);
  EXPECT_EQ(broken.offscreen_surface_, nullptr);
  EXPECT_EQ(broken.impeller_context_, nullptr);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnInvalidDisplay) {
  g_egl.get_display_result = EGL_NO_DISPLAY;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(CountEvents("ChooseConfig"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnInitialize) {
  g_egl.initialize_result = EGL_FALSE;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(CountEvents("ChooseConfig"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnOnscreenConfig) {
  g_egl.choose_config_result = EGL_FALSE;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(CountEvents("ChooseConfig"), 1u);
  EXPECT_EQ(CountEvents("CreateContext"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnOffscreenConfig) {
  g_egl.fail_choose_config_on_nth = g_egl.choose_config_calls + 2;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(CountEvents("ChooseConfig"), 2u);
  EXPECT_EQ(CountEvents("CreateContext"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnOnscreenContext) {
  g_egl.fail_create_context_on_nth = g_egl.create_context_calls + 1;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(CountEvents("CreateContext"), 1u);
  EXPECT_EQ(CountEvents("CreatePbufferSurface"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnOffscreenContext) {
  g_egl.fail_create_context_on_nth = g_egl.create_context_calls + 2;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(CountEvents("CreateContext"), 2u);
  EXPECT_EQ(CountEvents("CreatePbufferSurface"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnOffscreenSurface) {
  g_egl.pbuffer_surface_fail = true;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_EQ(CountEvents("CreatePbufferSurface:1x1"), 1u);
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailsOnMakeCurrent) {
  g_egl.make_current_result = EGL_FALSE;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  EXPECT_GE(CountEvents("MakeCurrent"), 1u);
}

TEST_F(OhosSurfaceGLImpellerTest, IsValidReflectsMember) {
  EXPECT_FALSE(surface_->IsValid());
  surface_->is_valid_ = true;
  EXPECT_TRUE(surface_->IsValid());
  surface_->is_valid_ = false;
  EXPECT_FALSE(surface_->IsValid());
}

TEST_F(OhosSurfaceGLImpellerTest, CreateGPUSurfaceNeedsImpellerContext) {
  EXPECT_EQ(surface_->CreateGPUSurface(nullptr), nullptr);
}

TEST_F(OhosSurfaceGLImpellerTest, TeardownClearsOnscreenSurface) {
  InstallEglMembers(surface_.get());
  surface_->onscreen_surface_ = surface_->display_->CreateWindowSurface(
      *surface_->onscreen_config_, (EGLNativeWindowType)kFakeNativeWindow);
  ASSERT_NE(surface_->onscreen_surface_, nullptr);
  size_t makes = CountEvents("MakeCurrent");
  surface_->TeardownOnScreenContext();
  EXPECT_EQ(surface_->onscreen_surface_, nullptr);
  EXPECT_GT(CountEvents("MakeCurrent"), makes);
}

TEST_F(OhosSurfaceGLImpellerTest, TeardownToleratesMissingMembers) {
  EXPECT_EQ(surface_->onscreen_surface_, nullptr);
  surface_->TeardownOnScreenContext();
  EXPECT_EQ(surface_->onscreen_surface_, nullptr);
}

TEST_F(OhosSurfaceGLImpellerTest, ResizeFailsWithoutNativeWindow) {
  EXPECT_FALSE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));
  EXPECT_EQ(CountEvents("CreateWindowSurface"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, ResizeRecreatesOnscreenSurface) {
  InstallEglMembers(surface_.get());
  surface_->native_window_ =
      fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_TRUE(surface_->OnScreenSurfaceResize(DlISize(640, 480)));
  EXPECT_EQ(CountEvents("CreateWindowSurface:" + HexPtr(kFakeNativeWindow)),
            1u);
  ASSERT_NE(surface_->onscreen_surface_, nullptr);
}

TEST_F(OhosSurfaceGLImpellerTest, SetNativeWindowRecreatesAndMakesCurrent) {
  InstallEglMembers(surface_.get());
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_TRUE(surface_->SetNativeWindow(window));
  EXPECT_EQ(surface_->native_window_.get(), window.get());
  EXPECT_EQ(CountEvents("CreateWindowSurface:" + HexPtr(kFakeNativeWindow)),
            1u);
  ASSERT_NE(surface_->onscreen_surface_, nullptr);
  EXPECT_GT(CountEvents("MakeCurrent"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, SetNativeWindowFailsOnSurfaceCreation) {
  InstallEglMembers(surface_.get());
  g_egl.window_surface_fail = true;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_FALSE(surface_->SetNativeWindow(window));
  EXPECT_EQ(surface_->onscreen_surface_, nullptr);
  EXPECT_EQ(surface_->native_window_.get(), window.get());
}

TEST_F(OhosSurfaceGLImpellerTest, SetNativeWindowFailsOnMakeCurrent) {
  InstallEglMembers(surface_.get());
  g_egl.make_current_result = EGL_FALSE;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeNativeWindow);
  EXPECT_FALSE(surface_->SetNativeWindow(window));
  ASSERT_NE(surface_->onscreen_surface_, nullptr);
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLImpellerTest, ResourceContextNeedsBackingMembers) {
  EXPECT_FALSE(surface_->ResourceContextMakeCurrent());
  EXPECT_FALSE(surface_->ResourceContextClearCurrent());

  InstallEglMembers(surface_.get());
  EXPECT_TRUE(surface_->ResourceContextMakeCurrent());
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(surface_->ResourceContextMakeCurrent());
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLImpellerTest, ResourceContextClearCurrentMapsStatus) {
  InstallEglMembers(surface_.get());
  EXPECT_TRUE(surface_->ResourceContextClearCurrent());
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(surface_->ResourceContextClearCurrent());
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLImpellerTest, GetImpellerContextDefaultsToNull) {
  EXPECT_EQ(surface_->GetImpellerContext(), nullptr);
}

TEST_F(OhosSurfaceGLImpellerTest, GLContextMakeCurrentWrapsResult) {
  auto result = surface_->GLContextMakeCurrent();
  ASSERT_NE(result, nullptr);
  EXPECT_FALSE(result->GetResult());

  InstallEglMembers(surface_.get());
  surface_->onscreen_surface_ = surface_->display_->CreateWindowSurface(
      *surface_->onscreen_config_, (EGLNativeWindowType)kFakeNativeWindow);
  auto ok = surface_->GLContextMakeCurrent();
  ASSERT_NE(ok, nullptr);
  EXPECT_TRUE(ok->GetResult());

  g_egl.make_current_result = EGL_FALSE;
  auto failed = surface_->GLContextMakeCurrent();
  ASSERT_NE(failed, nullptr);
  EXPECT_FALSE(failed->GetResult());
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLImpellerTest, GLContextClearCurrentNeedsMembers) {
  EXPECT_FALSE(surface_->GLContextClearCurrent());

  InstallEglMembers(surface_.get());
  surface_->onscreen_surface_ = surface_->display_->CreateWindowSurface(
      *surface_->onscreen_config_, (EGLNativeWindowType)kFakeNativeWindow);
  EXPECT_TRUE(surface_->GLContextClearCurrent());

  g_egl.make_current_result = EGL_FALSE;
  EXPECT_FALSE(surface_->GLContextClearCurrent());
  g_egl.make_current_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLImpellerTest, FramebufferInfoReportsCapabilities) {
  auto info = surface_->GLContextFramebufferInfo();
  EXPECT_TRUE(info.supports_readback);
  EXPECT_FALSE(info.supports_partial_repaint);
}

TEST_F(OhosSurfaceGLImpellerTest, SetDamageRegionAcceptsBothStates) {
  std::optional<DlIRect> region = DlIRect::MakeLTRB(0, 0, 8, 8);
  size_t events = g_egl.events.size();
  surface_->GLContextSetDamageRegion(region);
  surface_->GLContextSetDamageRegion(std::nullopt);
  EXPECT_EQ(g_egl.events.size(), events);
}

TEST_F(OhosSurfaceGLImpellerTest, PresentRequiresOnscreenSurface) {
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(1, 1, 9, 9);
  GLPresentInfo info{0u, damage, std::nullopt, damage};
  EXPECT_FALSE(surface_->GLContextPresent(info));
  EXPECT_EQ(CountEvents("SwapBuffers"), 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, PresentSwapsBuffers) {
  InstallEglMembers(surface_.get());
  surface_->onscreen_surface_ = surface_->display_->CreateWindowSurface(
      *surface_->onscreen_config_, (EGLNativeWindowType)kFakeNativeWindow);
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(1, 1, 9, 9);
  GLPresentInfo info{0u, damage, std::nullopt, damage};
  EXPECT_TRUE(surface_->GLContextPresent(info));
  EXPECT_EQ(CountEvents("SwapBuffers"), 1u);

  g_egl.swap_buffers_result = EGL_FALSE;
  EXPECT_FALSE(surface_->GLContextPresent(info));
  g_egl.swap_buffers_result = EGL_TRUE;
}

TEST_F(OhosSurfaceGLImpellerTest, FBOIsTheDefaultFramebuffer) {
  auto fbo = surface_->GLContextFBO(GLFrameInfo{64u, 64u});
  EXPECT_EQ(fbo.fbo_id, 0u);
}

TEST_F(OhosSurfaceGLImpellerTest, GetGLInterfaceReturnsNull) {
  EXPECT_EQ(surface_->GetGLInterface(), nullptr);
}

TEST_F(OhosSurfaceGLImpellerTest, ResourceAndOnscreenToleratePartialMembers) {
  InstallEglMembers(surface_.get());
  surface_->offscreen_surface_.reset();
  EXPECT_FALSE(surface_->ResourceContextMakeCurrent());
  EXPECT_FALSE(surface_->ResourceContextClearCurrent());

  surface_->offscreen_surface_ = surface_->display_->CreatePixelBufferSurface(
      *surface_->offscreen_config_, 1u, 1u);
  ASSERT_NE(surface_->offscreen_surface_, nullptr);
  surface_->offscreen_context_.reset();
  EXPECT_FALSE(surface_->ResourceContextMakeCurrent());
  EXPECT_FALSE(surface_->ResourceContextClearCurrent());

  surface_->onscreen_context_ =
      surface_->display_->CreateContext(*surface_->onscreen_config_, nullptr);
  ASSERT_NE(surface_->onscreen_context_, nullptr);
  surface_->onscreen_surface_.reset();
  EXPECT_FALSE(surface_->GLContextClearCurrent());
  EXPECT_FALSE(surface_->OnGLContextMakeCurrent());
}

TEST_F(OhosSurfaceGLImpellerTest, ConstructorFailEmitsLogs) {
  fml::ScopedSetLogSettings loud({fml::kLogInfo});
  g_egl.pbuffer_surface_fail = true;
  OHOSSurfaceGLImpeller broken(context_, false);
  EXPECT_FALSE(broken.IsValid());
  g_egl.pbuffer_surface_fail = false;
}

}  // namespace testing
}  // namespace flutter
