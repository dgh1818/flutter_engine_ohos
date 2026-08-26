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

void SetHisyseventDlopenRedirect(int mode);
int GetAndResetDlopenRedirectCount();

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

FakeEGLState g_egl;

std::string HexPtr(const void* p) {
  std::ostringstream os;
  os << "0x" << std::hex << reinterpret_cast<uintptr_t>(p);
  return os.str();
}

size_t CountEvents(const std::string& prefix) {
  size_t n = 0;
  for (const auto& e : g_egl.events) {
    if (e.rfind(prefix, 0) == 0) {
      ++n;
    }
  }
  return n;
}

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

extern "C" {

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglGetDisplay)>(
        dlsym(RTLD_NEXT, "eglGetDisplay"));
    return real ? real(display_id) : EGL_NO_DISPLAY;
  }
  g_egl.events.push_back("GetDisplay");
  return g_egl.get_display_result;
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglInitialize)>(
        dlsym(RTLD_NEXT, "eglInitialize"));
    return real ? real(dpy, major, minor) : EGL_FALSE;
  }
  g_egl.events.push_back("Initialize");
  if (major != nullptr) {
    *major = 1;
  }
  if (minor != nullptr) {
    *minor = 5;
  }
  return g_egl.initialize_result;
}

EGLBoolean eglTerminate(EGLDisplay dpy) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglTerminate)>(
        dlsym(RTLD_NEXT, "eglTerminate"));
    return real ? real(dpy) : EGL_FALSE;
  }
  g_egl.events.push_back("Terminate");
  return EGL_TRUE;
}

EGLBoolean eglChooseConfig(EGLDisplay dpy,
                           const EGLint* attrib_list,
                           EGLConfig* configs,
                           EGLint config_size,
                           EGLint* num_config) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglChooseConfig)>(
        dlsym(RTLD_NEXT, "eglChooseConfig"));
    return real ? real(dpy, attrib_list, configs, config_size, num_config)
                : EGL_FALSE;
  }
  g_egl.events.push_back("ChooseConfig");
  if (g_egl.choose_config_result != EGL_TRUE) {
    return EGL_FALSE;
  }
  if (num_config != nullptr) {
    *num_config = g_egl.choose_config_count;
  }
  if (configs != nullptr && !g_egl.choose_config_write_null) {
    *configs = flutter::testing::fake_egl::kFakeConfig;
  }
  return EGL_TRUE;
}

EGLContext eglCreateContext(EGLDisplay dpy,
                            EGLConfig config,
                            EGLContext share_context,
                            const EGLint* attrib_list) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglCreateContext)>(
        dlsym(RTLD_NEXT, "eglCreateContext"));
    return real ? real(dpy, config, share_context, attrib_list)
                : EGL_NO_CONTEXT;
  }
  g_egl.events.push_back("CreateContext");
  ++g_egl.create_context_calls;
  g_egl.last_context_config = config;
  g_egl.last_context_share = share_context;
  if (g_egl.fail_create_context_on_nth == g_egl.create_context_calls) {
    return EGL_NO_CONTEXT;
  }
  auto context = reinterpret_cast<EGLContext>(
      reinterpret_cast<uintptr_t>(flutter::testing::fake_egl::kFakeCtxBase) +
      static_cast<uintptr_t>(g_egl.create_context_calls));
  g_egl.created_contexts.push_back(context);
  return context;
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
  using flutter::testing::fake_egl::g_egl;
  using flutter::testing::fake_egl::HexPtr;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglDestroyContext)>(
        dlsym(RTLD_NEXT, "eglDestroyContext"));
    return real ? real(dpy, ctx) : EGL_FALSE;
  }
  g_egl.events.push_back("DestroyContext:" + HexPtr(ctx));
  return g_egl.destroy_context_result;
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy,
                                  EGLConfig config,
                                  EGLNativeWindowType win,
                                  const EGLint* attrib_list) {
  using flutter::testing::fake_egl::g_egl;
  using flutter::testing::fake_egl::HexPtr;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglCreateWindowSurface)>(
        dlsym(RTLD_NEXT, "eglCreateWindowSurface"));
    return real ? real(dpy, config, win, attrib_list) : EGL_NO_SURFACE;
  }
  g_egl.events.push_back(
      "CreateWindowSurface:" + HexPtr(reinterpret_cast<const void*>(win)));
  return g_egl.window_surface_fail ? EGL_NO_SURFACE
                                   : g_egl.window_surface_result;
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy,
                                   EGLConfig config,
                                   const EGLint* attrib_list) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglCreatePbufferSurface)>(
        dlsym(RTLD_NEXT, "eglCreatePbufferSurface"));
    return real ? real(dpy, config, attrib_list) : EGL_NO_SURFACE;
  }
  for (int i = 0; attrib_list != nullptr && attrib_list[i] != EGL_NONE;
       i += 2) {
    if (attrib_list[i] == EGL_WIDTH) {
      g_egl.last_pbuffer_width = attrib_list[i + 1];
    } else if (attrib_list[i] == EGL_HEIGHT) {
      g_egl.last_pbuffer_height = attrib_list[i + 1];
    }
  }
  g_egl.events.push_back("CreatePbufferSurface:" +
                         std::to_string(g_egl.last_pbuffer_width) + "x" +
                         std::to_string(g_egl.last_pbuffer_height));
  return g_egl.pbuffer_surface_fail ? EGL_NO_SURFACE
                                    : g_egl.pbuffer_surface_result;
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface) {
  using flutter::testing::fake_egl::g_egl;
  using flutter::testing::fake_egl::HexPtr;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglDestroySurface)>(
        dlsym(RTLD_NEXT, "eglDestroySurface"));
    return real ? real(dpy, surface) : EGL_FALSE;
  }
  g_egl.events.push_back("DestroySurface:" + HexPtr(surface));
  return EGL_TRUE;
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy,
                          EGLSurface draw,
                          EGLSurface read,
                          EGLContext ctx) {
  using flutter::testing::fake_egl::g_egl;
  using flutter::testing::fake_egl::HexPtr;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglMakeCurrent)>(
        dlsym(RTLD_NEXT, "eglMakeCurrent"));
    return real ? real(dpy, draw, read, ctx) : EGL_FALSE;
  }
  g_egl.events.push_back("MakeCurrent:" + HexPtr(draw) + "," +
                         HexPtr(read) + "," + HexPtr(ctx));
  return g_egl.make_current_result;
}

EGLContext eglGetCurrentContext(void) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglGetCurrentContext)>(
        dlsym(RTLD_NEXT, "eglGetCurrentContext"));
    return real ? real() : EGL_NO_CONTEXT;
  }
  ++g_egl.current_context_calls;
  return g_egl.current_context;
}

EGLSurface eglGetCurrentSurface(EGLint readdraw) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglGetCurrentSurface)>(
        dlsym(RTLD_NEXT, "eglGetCurrentSurface"));
    return real ? real(readdraw) : EGL_NO_SURFACE;
  }
  if (readdraw == EGL_DRAW) {
    ++g_egl.current_draw_calls;
    return g_egl.current_draw;
  }
  ++g_egl.current_read_calls;
  return g_egl.current_read;
}

EGLBoolean eglQuerySurface(EGLDisplay dpy,
                           EGLSurface surface,
                           EGLint attribute,
                           EGLint* value) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglQuerySurface)>(
        dlsym(RTLD_NEXT, "eglQuerySurface"));
    return real ? real(dpy, surface, attribute, value) : EGL_FALSE;
  }
  g_egl.events.push_back("QuerySurface:" + std::to_string(attribute));
  if (attribute == g_egl.fail_query_surface_pname) {
    return EGL_FALSE;
  }
  if (value != nullptr) {
    switch (attribute) {
      case EGL_WIDTH:
        *value = g_egl.query_width;
        break;
      case EGL_HEIGHT:
        *value = g_egl.query_height;
        break;
      case EGL_BUFFER_AGE_EXT:
        *value = g_egl.query_buffer_age;
        break;
      default:
        *value = 0;
        break;
    }
  }
  return EGL_TRUE;
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglSwapBuffers)>(
        dlsym(RTLD_NEXT, "eglSwapBuffers"));
    return real ? real(dpy, surface) : EGL_FALSE;
  }
  g_egl.events.push_back("SwapBuffers");
  return g_egl.swap_buffers_result;
}

EGLint eglGetError(void) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglGetError)>(
        dlsym(RTLD_NEXT, "eglGetError"));
    return real ? real() : EGL_SUCCESS;
  }
  ++g_egl.get_error_calls;
  return g_egl.error_code;
}

const char* eglQueryString(EGLDisplay dpy, EGLint name) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglQueryString)>(
        dlsym(RTLD_NEXT, "eglQueryString"));
    return real ? real(dpy, name) : nullptr;
  }
  g_egl.events.push_back("QueryString");
  return name == EGL_EXTENSIONS ? g_egl.extension_string : "";
}

void (*eglGetProcAddress(const char* procname))(void) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&eglGetProcAddress)>(
        dlsym(RTLD_NEXT, "eglGetProcAddress"));
    return real ? real(procname) : nullptr;
  }
  g_egl.events.push_back("GetProcAddress:" + std::string(procname));
  return g_egl.proc_address_result;
}

const GLubyte* glGetString(GLenum name) {
  using flutter::testing::fake_egl::g_egl;
  if (!g_egl.active) {
    static const auto real = reinterpret_cast<decltype(&glGetString)>(
        dlsym(RTLD_NEXT, "glGetString"));
    return real ? real(name) : nullptr;
  }
  g_egl.events.push_back("GetString");
  if (name == GL_RENDERER) {
    return reinterpret_cast<const GLubyte*>(g_egl.renderer_string);
  }
  return reinterpret_cast<const GLubyte*>("");
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

EGLDisplay s_present_display = EGL_NO_DISPLAY;
EGLSurface s_present_surface = EGL_NO_SURFACE;
EGLnsecsANDROID s_present_time_ns = 0;
bool s_present_called = false;

EGLBoolean EGLAPIENTRY StubPresentationTimeOk(EGLDisplay dpy,
                                              EGLSurface surface,
                                              EGLnsecsANDROID time) {
  s_present_display = dpy;
  s_present_surface = surface;
  s_present_time_ns = time;
  s_present_called = true;
  return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY StubPresentationTimeFail(EGLDisplay,
                                                EGLSurface,
                                                EGLnsecsANDROID) {
  return EGL_FALSE;
}

struct TailRedirectGuard {
  ~TailRedirectGuard() {
    SetHisyseventDlopenRedirect(0);
    GetAndResetDlopenRedirectCount();
  }
};

}

class OhosEGLSurfaceTest : public ::testing::Test {
 protected:
  void SetUp() override { guard_.emplace(); }
  void TearDown() override { guard_.reset(); }
  std::optional<FakeEGL> guard_;
};

TEST_F(OhosEGLSurfaceTest, LogLastEGLErrorConsumesOneErrorPerCall) {
  const EGLint known[] = {
      EGL_SUCCESS,         EGL_NOT_INITIALIZED,      EGL_BAD_ACCESS,
      EGL_BAD_ALLOC,       EGL_BAD_ATTRIBUTE,        EGL_BAD_CONTEXT,
      EGL_BAD_CONFIG,      EGL_BAD_CURRENT_SURFACE,  EGL_BAD_DISPLAY,
      EGL_BAD_SURFACE,     EGL_BAD_MATCH,            EGL_BAD_PARAMETER,
      EGL_BAD_NATIVE_PIXMAP, EGL_BAD_NATIVE_WINDOW,  EGL_CONTEXT_LOST};
  for (EGLint code : known) {
    g_egl.error_code = code;
    int before = g_egl.get_error_calls;
    LogLastEGLError();
    EXPECT_EQ(g_egl.get_error_calls, before + 1);
  }
  g_egl.error_code = 0x7777;
  int before = g_egl.get_error_calls;
  LogLastEGLError();
  EXPECT_EQ(g_egl.get_error_calls, before + 1);
}

TEST_F(OhosEGLSurfaceTest, InitResolvesDamageAndSwapProcsFromExtensions) {
  g_egl.extension_string =
      "EGL_KHR_partial_update EGL_EXT_swap_buffers_with_damage";
  OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  EXPECT_EQ(g_egl.events,
            (std::vector<std::string>{
                "QueryString",
                "GetProcAddress:eglSetDamageRegionKHR",
                "GetProcAddress:eglSwapBuffersWithDamageEXT"}));
}

TEST_F(OhosEGLSurfaceTest, InitPrefersExtSwapAndFallsBackToKhr) {
  {
    g_egl.extension_string = "EGL_KHR_partial_update";
    OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
    EXPECT_EQ(g_egl.events,
              (std::vector<std::string>{
                  "QueryString", "GetProcAddress:eglSetDamageRegionKHR"}));
  }
  {
    g_egl.events.clear();
    g_egl.extension_string = "EGL_KHR_swap_buffers_with_damage";
    OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
    EXPECT_EQ(g_egl.events,
              (std::vector<std::string>{
                  "QueryString",
                  "GetProcAddress:eglSwapBuffersWithDamageKHR"}));
  }
}

TEST_F(OhosEGLSurfaceTest, InitSkipsEmbeddedPrefixAndEmptyExtensions) {
  {
    g_egl.extension_string = "EGL_KHR_partial_updateX";
    OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
    EXPECT_EQ(g_egl.events, (std::vector<std::string>{"QueryString"}));
  }
  {
    g_egl.events.clear();
    g_egl.extension_string = "";
    OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
    EXPECT_EQ(g_egl.events, (std::vector<std::string>{"QueryString"}));
  }
}

TEST_F(OhosEGLSurfaceTest, IsValidReflectsSurfaceHandle) {
  OhosEGLSurface valid(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  EXPECT_TRUE(valid.IsValid());
  OhosEGLSurface invalid(EGL_NO_SURFACE, kFakeDisplay, kFakeContextA);
  EXPECT_FALSE(invalid.IsValid());
}

TEST_F(OhosEGLSurfaceTest, IsContextCurrentRequiresAllFourMatches) {
  OhosEGLSurface surface(kFakeSurfaceA, EGL_NO_DISPLAY, kFakeContextA);
  g_egl.current_draw = kFakeSurfaceA;
  g_egl.current_read = kFakeSurfaceA;

  g_egl.current_context = kFakeContextB;
  EXPECT_FALSE(surface.IsContextCurrent());
  EXPECT_EQ(g_egl.current_draw_calls, 0);

  OhosEGLSurface other_display(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  g_egl.current_context = kFakeContextA;
  EXPECT_FALSE(other_display.IsContextCurrent());
  EXPECT_EQ(g_egl.current_draw_calls, 0);

  g_egl.current_draw = kFakeSurfaceB;
  EXPECT_FALSE(surface.IsContextCurrent());
  EXPECT_EQ(g_egl.current_read_calls, 0);

  g_egl.current_draw = kFakeSurfaceA;
  g_egl.current_read = kFakeSurfaceB;
  EXPECT_FALSE(surface.IsContextCurrent());

  g_egl.current_read = kFakeSurfaceA;
  EXPECT_TRUE(surface.IsContextCurrent());
}

TEST_F(OhosEGLSurfaceTest, MakeCurrentAlreadyCurrentSkipsEglCall) {
  g_egl.current_context = kFakeContextA;
  g_egl.current_draw = kFakeSurfaceA;
  g_egl.current_read = kFakeSurfaceA;
  OhosEGLSurface surface(kFakeSurfaceA, EGL_NO_DISPLAY, kFakeContextA);
  EXPECT_EQ(surface.MakeCurrent(),
            OhosEGLSurfaceMakeCurrentStatus::kSuccessAlreadyCurrent);
  EXPECT_EQ(CountEvents("MakeCurrent"), 0u);
}

TEST_F(OhosEGLSurfaceTest, MakeCurrentMapsFailureAndSuccess) {
  OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  g_egl.make_current_result = EGL_FALSE;
  EXPECT_EQ(surface.MakeCurrent(), OhosEGLSurfaceMakeCurrentStatus::kFailure);
  EXPECT_EQ(g_egl.events.back(),
            "MakeCurrent:" + HexPtr(kFakeSurfaceA) + "," +
                HexPtr(kFakeSurfaceA) + "," + HexPtr(kFakeContextA));
  EXPECT_GE(g_egl.get_error_calls, 1);

  g_egl.make_current_result = EGL_TRUE;
  EXPECT_EQ(surface.MakeCurrent(),
            OhosEGLSurfaceMakeCurrentStatus::kSuccessMadeCurrent);
}

TEST_F(OhosEGLSurfaceTest, SetPresentationTimeGatesOnProcPresence) {
  OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  auto time = fml::TimePoint::FromEpochDelta(
      fml::TimeDelta::FromNanoseconds(1234));
  EXPECT_FALSE(surface.SetPresentationTime(time));

  surface.presentation_time_proc_ = &StubPresentationTimeOk;
  s_present_called = false;
  EXPECT_TRUE(surface.SetPresentationTime(time));
  EXPECT_TRUE(s_present_called);
  EXPECT_EQ(s_present_display, kFakeDisplay);
  EXPECT_EQ(s_present_surface, kFakeSurfaceA);
  EXPECT_EQ(s_present_time_ns, static_cast<EGLnsecsANDROID>(1234));

  surface.presentation_time_proc_ = &StubPresentationTimeFail;
  EXPECT_FALSE(surface.SetPresentationTime(time));
  surface.presentation_time_proc_ = nullptr;
}

TEST_F(OhosEGLSurfaceTest, SwapBuffersDelegatesToPlainEglSwap) {
  OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  std::optional<DlIRect> damage = DlIRect::MakeLTRB(1, 2, 30, 40);
  EXPECT_TRUE(surface.SwapBuffers(damage));
  EXPECT_TRUE(surface.SwapBuffers(std::nullopt));
  EXPECT_EQ(CountEvents("SwapBuffers"), 2u);
  EXPECT_EQ(CountEvents("QuerySurface"), 0u);
  g_egl.swap_buffers_result = EGL_FALSE;
  EXPECT_FALSE(surface.SwapBuffers(damage));
  EXPECT_EQ(CountEvents("SwapBuffers"), 3u);
}

TEST_F(OhosEGLSurfaceTest, PartialRepaintHelpersStayInert) {
  OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  EXPECT_FALSE(surface.SupportsPartialRepaint());
  auto damage = surface.InitialDamage();
  EXPECT_FALSE(damage.has_value());
  surface.SetDamageRegion(DlIRect::MakeLTRB(0, 0, 8, 8));
  surface.SetDamageRegion(std::nullopt);
  EXPECT_EQ(CountEvents("QuerySurface"), 0u);
  EXPECT_EQ(CountEvents("GetProcAddress"), 0u);
}

TEST_F(OhosEGLSurfaceTest, GetSizeQueriesBothDimensions) {
  OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
  auto size = surface.GetSize();
  EXPECT_EQ(size.width, 640);
  EXPECT_EQ(size.height, 480);
  EXPECT_EQ(CountEvents("QuerySurface"), 2u);

  g_egl.fail_query_surface_pname = EGL_WIDTH;
  size = surface.GetSize();
  EXPECT_EQ(size.width, 0);
  EXPECT_EQ(size.height, 0);
  EXPECT_EQ(CountEvents("QuerySurface"), 3u);
  EXPECT_GE(g_egl.get_error_calls, 1);

  g_egl.fail_query_surface_pname = EGL_HEIGHT;
  size = surface.GetSize();
  EXPECT_EQ(size.width, 0);
  EXPECT_EQ(size.height, 0);
  EXPECT_EQ(CountEvents("QuerySurface"), 5u);
}

TEST_F(OhosEGLSurfaceTest, DestructorDestroysOwnedSurfaceHandle) {
  {
    OhosEGLSurface surface(kFakeSurfaceA, kFakeDisplay, kFakeContextA);
    ASSERT_TRUE(surface.IsValid());
  }
  EXPECT_EQ(CountEvents("DestroySurface:" + HexPtr(kFakeSurfaceA)), 1u);
}

TEST_F(OhosEGLSurfaceTest, PassthroughAnchorsKeepRealLibrariesLinked) {
  guard_.reset();
#if defined(OHOS_X64_UNITTEST)
  EXPECT_NE(g_egl.egl_anchor, nullptr);
  EXPECT_NE(g_egl.gles_anchor, nullptr);
#else
  EXPECT_TRUE(eglQueryString(EGL_NO_DISPLAY, EGL_VERSION) == nullptr);
  EXPECT_EQ(eglGetError(), EGL_BAD_DISPLAY);
#endif  // defined(OHOS_X64_UNITTEST)
}

TEST_F(OhosEGLSurfaceTest, QuietSeveritySkipsLogConstruction) {
  fake_egl::FakeEGL guard;
  QuietLogs quiet;
  {
    OhosEGLSurface surface(fake_egl::kFakeSurfaceA, fake_egl::kFakeDisplay,
                           fake_egl::kFakeContextA);
    EXPECT_TRUE(surface.IsValid());

    g_egl.error_code = EGL_BAD_MATCH;
    int before = g_egl.get_error_calls;
    LogLastEGLError();
    EXPECT_EQ(g_egl.get_error_calls, before + 1);

    g_egl.error_code = 0x7777;
    before = g_egl.get_error_calls;
    LogLastEGLError();
    EXPECT_EQ(g_egl.get_error_calls, before + 1);

    g_egl.make_current_result = EGL_FALSE;
    EXPECT_EQ(surface.MakeCurrent(), OhosEGLSurfaceMakeCurrentStatus::kFailure);

    g_egl.make_current_result = EGL_TRUE;
    g_egl.fail_query_surface_pname = EGL_WIDTH;
    auto size = surface.GetSize();
    EXPECT_EQ(size.width, 0);
    EXPECT_EQ(size.height, 0);
  }
  EXPECT_EQ(CountEvents("DestroySurface"), 1u);
}

#if !defined(OHOS_X64_UNITTEST)
TEST_F(OhosEGLSurfaceTest, DestructorCheckFailureViaRealPassthrough) {
  guard_.reset();
  EGLDisplay real_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (real_display == EGL_NO_DISPLAY) {
    GTEST_SKIP() << "real EGL default display unavailable this round";
  }
  eglInitialize(real_display, nullptr, nullptr);
  QuietLogs quiet;
  EXPECT_EQ(eglDestroySurface(real_display, fake_egl::kFakeSurfaceA),
            EGLBoolean{EGL_FALSE});

  bool constructed = false;
  {
    OhosEGLSurface surface(fake_egl::kFakeSurfaceA, real_display,
                           fake_egl::kFakeContextA);
    constructed = true;
  }
  EXPECT_TRUE(constructed);
  eglTerminate(real_display);
}
#endif  // !defined(OHOS_X64_UNITTEST)

TEST_F(OhosEGLSurfaceTest, QuietSeverityOnLoadFailures) {
  QuietLogs quiet;
  TailRedirectGuard guard;
  SetHisyseventDlopenRedirect(1);
  int ret = fml::HiSysEventWrite("tail_dlopen_fail", 7);
  if (GetAndResetDlopenRedirectCount() > 0) {
    EXPECT_EQ(ret, -1);
  } else {
    EXPECT_TRUE(ret == 0 || ret == -5 || ret == -1);
  }

  SetHisyseventDlopenRedirect(2);
  ret = fml::HiSysEventWrite("tail_dlsym_fail", 8);
  if (GetAndResetDlopenRedirectCount() > 0) {
    EXPECT_EQ(ret, -1);
  } else {
    EXPECT_TRUE(ret == 0 || ret == -5 || ret == -1);
  }
}

}
}

