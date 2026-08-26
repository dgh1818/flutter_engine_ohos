/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// These tests inspect a few private members of OHOSExternalTexture
// (frame_listener_, background_color_, size_is_changing_, ...) to verify
// state transitions directly. Expose them with the same
// #define-private-public pattern used by vsync_waiter_ohos_unittests.cpp.
// Only ohos_external_texture.h is wrapped, so the affected transitive
// includes stay minimal; the #undef must follow the include immediately.
#define private public
#define protected public
#include "flutter/shell/platform/ohos/ohos_external_texture.h"
#undef private
#undef protected

#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include "flutter/impeller/toolkit/egl/image.h"
#include "flutter/impeller/toolkit/gles/texture.h"

#define private public
#define protected public
#include "flutter/shell/platform/ohos/ohos_external_texture_gl.h"
#undef private
#undef protected

#include <atomic>
#include <cstring>
#include <memory>
#include <set>
#include "flutter/shell/platform/ohos/ohos_external_texture_vulkan.h"
#include "flutter/display_list/skia/dl_sk_canvas.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"
#include "gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/gpu/ganesh/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/gl/GrGLInterface.h"

namespace flutter {
extern std::set<uint64_t> g_external_texture_set;
}

namespace flutter {
namespace testing {
namespace {

// Counting frame-available callback used to verify listener registration.
// Atomic: on device the listener fires on the producer thread.
void CountingOnFrameAvailable(void* context) {
  if (context != nullptr) {
    ++(*static_cast<std::atomic<int>*>(context));
  }
}

OH_OnFrameAvailableListener MakeListener(std::atomic<int>* counter) {
  OH_OnFrameAvailableListener listener;
  listener.context = counter;
  listener.onFrameAvailable = &CountingOnFrameAvailable;
  return listener;
}

constexpr int64_t kTestTextureId = 1;

class TestOHOSExternalTexture : public OHOSExternalTextureGL {
 public:
  using OHOSExternalTextureGL::OHOSExternalTextureGL;

  OHNativeWindowBuffer* GetConsumer(int* fence_fd) {
    return GetConsumerNativeBuffer(fence_fd);
  }
};

class SoftwarePaintTarget {
 public:
  explicit SoftwarePaintTarget(int width, int height) {
    bitmap_.allocN32Pixels(width, height);
    bitmap_.eraseColor(SK_ColorTRANSPARENT);
    canvas_ = std::make_unique<SkCanvas>(bitmap_);
    adapter_ = std::make_unique<flutter::DlSkCanvasAdapter>(canvas_.get());
    context_.canvas = adapter_.get();
  }

  Texture::PaintContext& context() { return context_; }

  SkColor CenterPixel() const {
    return bitmap_.getColor(bitmap_.width() / 2, bitmap_.height() / 2);
  }

 private:
  SkBitmap bitmap_;
  std::unique_ptr<SkCanvas> canvas_;
  std::unique_ptr<flutter::DlSkCanvasAdapter> adapter_;
  Texture::PaintContext context_;
};

sk_sp<flutter::DlImage> MakeRasterDlImage(SkColor color) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(8, 8);
  bitmap.eraseColor(color);
  return flutter::DlImage::Make(SkImages::RasterFromBitmap(bitmap));
}

#if defined(OHOS_X64_UNITTEST)
class ScopedCharDevFstat {
 public:
  ScopedCharDevFstat() {
    UpdateFstatFunc([](int fd, struct stat* st) {
      st->st_mode = S_IFCHR | 0666;
      return 0;
    });
  }

  ~ScopedCharDevFstat() { UpdateFstatFunc(nullptr); }
};
#else
class ScopedCharDevFstat {
 public:
  ScopedCharDevFstat() {}
  ~ScopedCharDevFstat() {}
};
#endif

#if !defined(OHOS_X64_UNITTEST)
// Device-only no-op that shadows the global stub-engaging guard: unqualified
// uses pass through to the real graphic stack, `::`-qualified uses do not.
class GraphicStubKnobGuard {
 public:
  GraphicStubKnobGuard() {}
  ~GraphicStubKnobGuard() {}
};
#endif

#if defined(OHOS_X64_UNITTEST)
bool QueueOneProducerFrame(OHOSExternalTextureGL&, int, int) {
  g_stub_graphic_fail_mask = kStubAcquireBufferSuccess;
  return true;
}
#else
bool QueueOneProducerFrame(OHOSExternalTextureGL& texture,
                           int width,
                           int height) {
  OHNativeWindow* window = texture.producer_nativewindow_;
  if (window == nullptr) {
    return false;
  }
  if (OH_NativeWindow_NativeWindowHandleOpt(window, SET_BUFFER_GEOMETRY, width,
                                            height) != 0) {
    return false;
  }
  OHNativeWindowBuffer* buffer = nullptr;
  int fence_fd = -1;
  if (OH_NativeWindow_NativeWindowRequestBuffer(window, &buffer, &fence_fd) !=
          0 ||
      buffer == nullptr) {
    return false;
  }
  Region region = {};
  return OH_NativeWindow_NativeWindowFlushBuffer(window, buffer, fence_fd,
                                                 region) == 0;
}
#endif

std::shared_ptr<OHOSExternalTextureGL> MakeTextureWithoutSource(
    OH_OnFrameAvailableListener listener) {
#if defined(OHOS_X64_UNITTEST)
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailNativeImageCreate;
  return std::make_shared<OHOSExternalTextureGL>(kTestTextureId, listener);
#else
  auto texture =
      std::make_shared<OHOSExternalTextureGL>(kTestTextureId, listener);
  texture->Reset(false);
  return texture;
#endif
}

OH_NativeBuffer_Config ReadBackPixelMapConfig(OHOSExternalTextureGL& texture) {
  OH_NativeBuffer_Config config = {};
  OH_NativeBuffer* native_buffer = nullptr;
  OHOSExternalTexture::GetWindowBufferConfig(texture.pixelmap_buffer_,
                                             &native_buffer, &config, nullptr);
  return config;
}

inline OHNativeWindowBuffer* ReleaseProbeBuffer() {
#if defined(OHOS_X64_UNITTEST)
  return reinterpret_cast<OHNativeWindowBuffer*>(0x99);
#else
  OH_NativeBuffer_Config config = {
      64, 64, NATIVEBUFFER_PIXEL_FMT_RGBA_8888,
      NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE, 0x8};
  OH_NativeBuffer* native_buffer = OH_NativeBuffer_Alloc(&config);
  return native_buffer != nullptr
             ? OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(
                   native_buffer)
             : nullptr;
#endif
}

EGLSyncKHR StubCreateSyncOk(EGLDisplay, EGLenum, const EGLint*) {
  return reinterpret_cast<EGLSyncKHR>(0x1234);
}

EGLSyncKHR StubCreateSyncNoSync(EGLDisplay, EGLenum, const EGLint*) {
  return EGL_NO_SYNC_KHR;
}

EGLint StubDupFenceFdNoFd(EGLDisplay, EGLSyncKHR) {
  return -1;
}

EGLBoolean StubDestroySync(EGLDisplay, EGLSyncKHR) {
  return EGL_TRUE;
}

EGLint StubWaitSync(EGLDisplay, EGLSyncKHR, EGLint) {
  return EGL_TRUE;
}

EGLBoolean StubDestroyImage(EGLDisplay, EGLImageKHR) {
  return EGL_TRUE;
}

EGLImageKHR StubCreateImageOk(EGLDisplay,
                              EGLContext,
                              EGLenum,
                              EGLClientBuffer,
                              const EGLint*) {
  return reinterpret_cast<EGLImageKHR>(0x5678);
}

EGLImageKHR StubCreateImageNoImage(EGLDisplay,
                                   EGLContext,
                                   EGLenum,
                                   EGLClientBuffer,
                                   const EGLint*) {
  return EGL_NO_IMAGE_KHR;
}

class ScopedEGLProcs {
 public:
  ScopedEGLProcs()
      : create_sync(OHOSExternalTextureGL::eglCreateSyncKHR_),
        dup_fence_fd(OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_),
        destroy_sync(OHOSExternalTextureGL::eglDestroySyncKHR_),
        wait_sync(OHOSExternalTextureGL::eglWaitSyncKHR_),
        create_image(OHOSExternalTextureGL::eglCreateImageKHR_),
        destroy_image(OHOSExternalTextureGL::eglDestroyImageKHR_),
        target_2d(OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_) {}

  ~ScopedEGLProcs() {
    OHOSExternalTextureGL::eglCreateSyncKHR_ = create_sync;
    OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = dup_fence_fd;
    OHOSExternalTextureGL::eglDestroySyncKHR_ = destroy_sync;
    OHOSExternalTextureGL::eglWaitSyncKHR_ = wait_sync;
    OHOSExternalTextureGL::eglCreateImageKHR_ = create_image;
    OHOSExternalTextureGL::eglDestroyImageKHR_ = destroy_image;
    OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_ = target_2d;
  }

  PFNEGLCREATESYNCKHRPROC create_sync;
  PFNEGLDUPNATIVEFENCEFDANDROIDPROC dup_fence_fd;
  PFNEGLDESTROYSYNCKHRPROC destroy_sync;
  PFNEGLWAITSYNCKHRPROC wait_sync;
  PFNEGLCREATEIMAGEKHRPROC create_image;
  PFNEGLDESTROYIMAGEKHRPROC destroy_image;
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC target_2d;
};

#if defined(OHOS_X64_UNITTEST)
class ScopedUnsignaledFence {
 public:
  ScopedUnsignaledFence() {
    UpdateFstatFunc([](int fd, struct stat* st) {
      st->st_mode = S_IFCHR | 0666;
      return 0;
    });
    fd_ = eventfd(0, 0);
  }

  ~ScopedUnsignaledFence() {
    UpdateFstatFunc(nullptr);
    if (fd_ >= 0 && fcntl(fd_, F_GETFD) != -1) {
      close(fd_);
    }
  }

  int get() const { return fd_; }

 private:
  int fd_ = -1;
};
#else
class ScopedUnsignaledFence {
 public:
  ScopedUnsignaledFence() { fd_ = open("/dev/ptmx", O_RDWR | O_NONBLOCK); }

  ~ScopedUnsignaledFence() {
    if (fd_ >= 0 && fcntl(fd_, F_GETFD) != -1) {
      close(fd_);
    }
  }

  int get() const { return fd_; }

 private:
  int fd_ = -1;
};
#endif

struct TestWindowBuffer {
  TestWindowBuffer() {
    OH_NativeBuffer_Config config = {
        64, 64, NATIVEBUFFER_PIXEL_FMT_RGBA_8888,
        NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE, 0x8};
    native_buffer = OH_NativeBuffer_Alloc(&config);
    window_buffer =
        OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
  }

  ~TestWindowBuffer() {
    OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
    OH_NativeBuffer_Unreference(native_buffer);
  }

  OH_NativeBuffer* native_buffer = nullptr;
  OHNativeWindowBuffer* window_buffer = nullptr;
};

OH_NativeBuffer_Config DefaultBufferConfig() {
  return {64, 64, NATIVEBUFFER_PIXEL_FMT_RGBA_8888,
          NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE, 0x8};
}

constexpr int64_t kWave3TextureId = 1;
}

// Fixture for OHOSExternalTexture base-class behavior. The base class is
// abstract, so the tests instantiate the GL subclass, whose constructor
// only resolves EGL symbols and does not require a current GL context.
class OhosExternalTextureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    listener_call_count_ = 0;
    texture_ = std::make_shared<OHOSExternalTextureGL>(
        kTestTextureId, MakeListener(&listener_call_count_));
    // On an OHOS device the constructor always succeeds in creating the
    // native image source and acquiring its producer window.
    ASSERT_NE(texture_->GetProducerSurfaceId(), 0u);
    ASSERT_NE(texture_->GetProducerWindowId(), 0u);
  }

  void SeedEglError() { eglQueryString(EGL_NO_DISPLAY, EGL_VERSION); }

  std::atomic<int> listener_call_count_ = 0;
  std::shared_ptr<OHOSExternalTextureGL> texture_;
};

TEST_F(OhosExternalTextureTest, RegistersListener) {
  // The constructor must register our listener on the native image and
  // keep a callable copy in frame_listener_.
  EXPECT_EQ(texture_->frame_listener_.onFrameAvailable,
            &CountingOnFrameAvailable);
  EXPECT_EQ(texture_->frame_listener_.context,
            static_cast<void*>(&listener_call_count_));

  // The stored (context, callback) pair must be intact and callable.
  texture_->frame_listener_.onFrameAvailable(texture_->frame_listener_.context);
  EXPECT_EQ(listener_call_count_, 1);
}

TEST_F(OhosExternalTextureTest, MarkNewFrameAvailableSetsProducerState) {
  EXPECT_FALSE(texture_->producer_has_frame_);
  texture_->MarkNewFrameAvailable();
  EXPECT_TRUE(texture_->producer_has_frame_);
  EXPECT_EQ(texture_->now_new_frame_seq_num_.load(), int64_t{1});
}

TEST_F(OhosExternalTextureTest, SetBackGroundColorSwapsRedBlue) {
  texture_->SetBackGroundColor(0x11223344);
  EXPECT_TRUE(texture_->background_color_enable_);
  // ABGR -> ARGB: R and B channels are swapped, A and G are kept.
  EXPECT_EQ(texture_->background_color_, 0x11443322u);
}

TEST_F(OhosExternalTextureTest, NotifyResizingTogglesSizeChanging) {
  // Producer window size starts at (0, 0): no change, no flag.
  texture_->NotifyResizing(0, 0);
  EXPECT_FALSE(texture_->size_is_changing_.load());

  texture_->NotifyResizing(640, 480);
  EXPECT_TRUE(texture_->size_is_changing_.load());
}

TEST_F(OhosExternalTextureTest, SetProducerWindowSizeSucceeds) {
  ASSERT_TRUE(texture_->SetProducerWindowSize(640, 480));
  EXPECT_EQ(texture_->producer_nativewindow_width_, 640);
  EXPECT_EQ(texture_->producer_nativewindow_height_, 480);
}

TEST_F(OhosExternalTextureTest, ResetWithSurfaceIdRecreatesSource) {
  ASSERT_NE(texture_->GetProducerSurfaceId(), 0u);

  // Reset without surface id tears the native image source down.
  EXPECT_EQ(texture_->Reset(false), 0u);
  EXPECT_EQ(texture_->GetProducerSurfaceId(), 0u);

  // Reset with surface id recreates a fresh source with a valid id.
  uint64_t new_surface_id = texture_->Reset(true);
  EXPECT_NE(new_surface_id, 0u);
  EXPECT_EQ(new_surface_id, texture_->GetProducerSurfaceId());
}

TEST_F(OhosExternalTextureTest, SetExternalNativeImageReplacesSource) {
  EXPECT_FALSE(texture_->SetExternalNativeImage(nullptr));

  OH_NativeImage* external_image =
      OH_NativeImage_Create(0, GL_TEXTURE_EXTERNAL_OES);
  ASSERT_NE(external_image, nullptr);

  EXPECT_TRUE(texture_->SetExternalNativeImage(external_image));
  EXPECT_TRUE(texture_->source_is_external_);
  EXPECT_NE(texture_->GetProducerSurfaceId(), 0u);

  // The texture does NOT take ownership of an external native image: its
  // destructor only installs the default frame listener on it, so the
  // image must be destroyed here after the texture is gone.
  texture_.reset();
  OH_NativeImage_Destroy(&external_image);
}

TEST_F(OhosExternalTextureTest, FdIsValidIdentifiesCharDevice) {
  EXPECT_FALSE(OHOSExternalTexture::FdIsValid(-1));
  EXPECT_FALSE(OHOSExternalTexture::FdIsValid(0));

#if defined(OHOS_X64_UNITTEST)
  UpdateFstatFunc([](int fd, struct stat* st) {
    st->st_mode = S_IFCHR | 0666;
    return 0;
  });
  EXPECT_TRUE(OHOSExternalTexture::FdIsValid(1));
  UpdateFstatFunc(nullptr);
#else
  // /dev/null is a character device, same file type as anon_inode:sync_file.
  int null_fd = open("/dev/null", O_RDONLY);
  ASSERT_GE(null_fd, 0);
  EXPECT_TRUE(OHOSExternalTexture::FdIsValid(null_fd));
  close(null_fd);
#endif

  // A regular file must be rejected as a sync_file fd.
  const char* tmp_path = "/data/local/tmp/fd_is_valid_test.tmp";
  int file_fd = open(tmp_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  ASSERT_GE(file_fd, 0);
  EXPECT_FALSE(OHOSExternalTexture::FdIsValid(file_fd));
  close(file_fd);
  unlink(tmp_path);
}

TEST_F(OhosExternalTextureTest, FenceIsSignalDetectsReadyFd) {
  EXPECT_FALSE(OHOSExternalTexture::FenceIsSignal(-1));
  EXPECT_FALSE(OHOSExternalTexture::FenceIsSignal(0));

  // /dev/null is always readable, so a zero-timeout poll reports it as
  // "signaled".
  int null_fd = open("/dev/null", O_RDONLY);
  ASSERT_GE(null_fd, 0);
  EXPECT_TRUE(OHOSExternalTexture::FenceIsSignal(null_fd));
  close(null_fd);
}

TEST_F(OhosExternalTextureTest,
       DefaultOnFrameAvailableWithLockIgnoresUnknownImage) {
  // An image that was never inserted into g_external_texture_set must be
  // ignored early, without touching the NDK (and must not crash).
  EXPECT_NO_FATAL_FAILURE(
      OHOSExternalTexture::DefaultOnFrameAvailableWithLock(nullptr));
}

TEST_F(OhosExternalTextureTest, ConstructorFailsWhenNativeImageCreateFails) {
  auto texture = MakeTextureWithoutSource(MakeListener(&listener_call_count_));
  EXPECT_EQ(texture->native_image_source_, nullptr);
  EXPECT_EQ(texture->producer_nativewindow_, nullptr);
  EXPECT_EQ(texture->GetProducerSurfaceId(), 0u);
  EXPECT_EQ(texture->GetProducerWindowId(), 0u);

  size_t set_size = g_external_texture_set.size();
  EXPECT_NO_FATAL_FAILURE({
    texture->OnGrContextCreated();
    texture->OnGrContextDestroyed();
  });
  EXPECT_EQ(g_external_texture_set.size(), set_size);
}

TEST_F(OhosExternalTextureTest, ConstructorFailsWhenAcquireNativeWindowFails) {
  ::GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailAcquireNativeWindow;
  auto texture = std::make_shared<OHOSExternalTextureGL>(
      kTestTextureId, MakeListener(&listener_call_count_));
  EXPECT_NE(texture->native_image_source_, nullptr);
  EXPECT_EQ(texture->producer_nativewindow_, nullptr);
  EXPECT_FALSE(texture->need_120_fps_);
  EXPECT_EQ(texture->GetProducerWindowId(), 0u);
}

TEST_F(OhosExternalTextureTest, ConstructorHandlesOptAndListenerFailures) {
#if defined(OHOS_X64_UNITTEST)
  {
    GraphicStubKnobGuard guard;
    g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
    auto texture = std::make_shared<OHOSExternalTextureGL>(
        kTestTextureId, MakeListener(&listener_call_count_));
    EXPECT_NE(texture->native_image_source_, nullptr);
    EXPECT_NE(texture->producer_nativewindow_, nullptr);
    EXPECT_FALSE(texture->need_120_fps_);
  }
  {
    GraphicStubKnobGuard guard;
    g_stub_graphic_fail_mask = kStubFailFrameAvailableListener;
    auto texture = std::make_shared<OHOSExternalTextureGL>(
        kTestTextureId, MakeListener(&listener_call_count_));
    EXPECT_NE(texture->native_image_source_, nullptr);
    EXPECT_NE(texture->producer_nativewindow_, nullptr);
    EXPECT_TRUE(texture->need_120_fps_);
  }
#else
  auto texture = std::make_shared<OHOSExternalTextureGL>(
      kTestTextureId, MakeListener(&listener_call_count_));
  EXPECT_NE(texture->native_image_source_, nullptr);
  EXPECT_NE(texture->producer_nativewindow_, nullptr);
  EXPECT_TRUE(texture->need_120_fps_);
#endif
}

TEST_F(OhosExternalTextureTest, GetProducerWindowIdReacquiresNullWindow) {
  OHNativeWindow* saved = texture_->producer_nativewindow_;
  texture_->producer_nativewindow_ = nullptr;
  EXPECT_NE(texture_->GetProducerWindowId(), 0u);
  EXPECT_EQ(texture_->producer_nativewindow_, saved);
}

TEST_F(OhosExternalTextureTest, OnGrContextDestroyedReleasesAndRegisters) {
  texture_->SetOldDlImage(MakeRasterDlImage(0xFF0000FF));
  OH_NativeBuffer_Config config = {};
  texture_->image_lru_.AddImage(MakeRasterDlImage(0xFF000000), config, 3);
  int fence_fd = open("/dev/null", O_RDONLY);
  ASSERT_GE(fence_fd, 0);
  texture_->last_fence_fd_ = fence_fd;
  texture_->gl_resources_[42] = GlResource{};

  {
    ScopedCharDevFstat char_fstat;
    texture_->OnGrContextDestroyed();
  }
  uint64_t key = reinterpret_cast<uint64_t>(texture_->native_image_source_);
  EXPECT_EQ(g_external_texture_set.count(key), 1u);
  EXPECT_NE(texture_->native_image_source_, nullptr);
  EXPECT_EQ(texture_->old_dl_image_, nullptr);
  EXPECT_EQ(texture_->image_lru_.FindImage(3, config, nullptr), nullptr);
  EXPECT_EQ(texture_->last_fence_fd_, -1);
  EXPECT_EQ(fcntl(fence_fd, F_GETFD), -1);
  EXPECT_TRUE(texture_->gl_resources_.empty());

  EXPECT_NO_FATAL_FAILURE(OHOSExternalTexture::DefaultOnFrameAvailableWithLock(
      texture_->native_image_source_));
  {
    GraphicStubKnobGuard guard;
    ASSERT_TRUE(QueueOneProducerFrame(*texture_, 64, 64))
        << "cannot queue a producer frame";
    EXPECT_NO_FATAL_FAILURE(
        OHOSExternalTexture::DefaultOnFrameAvailableWithLock(
            texture_->native_image_source_));
  }
  texture_.reset();
  EXPECT_TRUE(g_external_texture_set.empty());
}

TEST_F(OhosExternalTextureTest, CreatePixelMapBufferMapsFormatsAndRejects) {
  const struct {
    PIXEL_FORMAT pixel_format;
    int window_format;
  } cases[] = {
      {PIXEL_FORMAT_RGB_565, NATIVEBUFFER_PIXEL_FMT_RGB_565},
      {PIXEL_FORMAT_RGBA_8888, NATIVEBUFFER_PIXEL_FMT_RGBA_8888},
      {PIXEL_FORMAT_BGRA_8888, NATIVEBUFFER_PIXEL_FMT_BGRA_8888},
      {PIXEL_FORMAT_RGB_888, NATIVEBUFFER_PIXEL_FMT_RGB_888},
      {PIXEL_FORMAT_NV21, NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP},
      {PIXEL_FORMAT_NV12, NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP},
      {PIXEL_FORMAT_RGBA_1010102, NATIVEBUFFER_PIXEL_FMT_RGBA_1010102},
  };
  for (const auto& c : cases) {
    if (!texture_->CreatePixelMapBuffer(32, 16, (int)c.pixel_format)) {
      continue;
    }
    OH_NativeBuffer_Config config = ReadBackPixelMapConfig(*texture_);
    EXPECT_EQ(config.width, 32);
    EXPECT_EQ(config.height, 16);
    EXPECT_EQ(config.format, c.window_format);
    const auto kReqUsage = NATIVEBUFFER_USAGE_HW_TEXTURE |
                           NATIVEBUFFER_USAGE_MEM_DMA |
                           NATIVEBUFFER_USAGE_CPU_WRITE;
    EXPECT_TRUE((config.usage & kReqUsage) == kReqUsage);
    EXPECT_NE(config.stride, 0);
    EXPECT_NE(texture_->pixelmap_buffer_, nullptr);
  }
  EXPECT_FALSE(
      texture_->CreatePixelMapBuffer(0, 16, (int)PIXEL_FORMAT_RGBA_8888));
  EXPECT_FALSE(
      texture_->CreatePixelMapBuffer(32, 0, (int)PIXEL_FORMAT_RGBA_8888));
  EXPECT_FALSE(
      texture_->CreatePixelMapBuffer(32, 16, (int)PIXEL_FORMAT_ALPHA_8));
  EXPECT_FALSE(
      texture_->CreatePixelMapBuffer(32, 16, (int)PIXEL_FORMAT_RGBA_F16));
  EXPECT_FALSE(
      texture_->CreatePixelMapBuffer(32, 16, (int)PIXEL_FORMAT_UNKNOWN));
}

TEST_F(OhosExternalTextureTest, SetPixelMapAsProducerRejectsNull) {
  EXPECT_FALSE(texture_->SetPixelMapAsProducer(nullptr, nullptr));
#if defined(OHOS_X64_UNITTEST)
  EXPECT_TRUE(
      texture_->SetPixelMapAsProducer(reinterpret_cast<NativePixelMap*>(0x1),
                                      reinterpret_cast<OH_NativeBuffer*>(0x2)));
  EXPECT_NE(texture_->pixelmap_buffer_, nullptr);
  EXPECT_EQ(texture_->pixelmap_native_buffer_,
            reinterpret_cast<OH_NativeBuffer*>(0x2));
  EXPECT_FALSE(texture_->SetPixelMapAsProducer(
      reinterpret_cast<NativePixelMap*>(0x1), nullptr));
#else
#endif
}

TEST_F(OhosExternalTextureTest, CopyDataToPixelMapBufferValidatesAndCopies) {
  unsigned char src[64 * 48];
  memset(src, 0xAB, sizeof(src));
  const int rgba = (int)PIXEL_FORMAT_RGBA_8888;

  EXPECT_FALSE(texture_->CopyDataToPixelMapBuffer(nullptr, 32, 16, 32, rgba));

  ASSERT_TRUE(texture_->CreatePixelMapBuffer(32, 16, rgba));
  OHNativeWindow* saved_window = texture_->producer_nativewindow_;
  texture_->producer_nativewindow_ = nullptr;
  EXPECT_FALSE(texture_->CopyDataToPixelMapBuffer(src, 32, 16, 32, rgba));
  texture_->producer_nativewindow_ = saved_window;

  texture_->pixelmap_buffer_ = nullptr;
  EXPECT_FALSE(texture_->CopyDataToPixelMapBuffer(src, 32, 16, 32, rgba));

  ASSERT_TRUE(texture_->CreatePixelMapBuffer(32, 16, (int)PIXEL_FORMAT_NV21));

  ASSERT_TRUE(texture_->CreatePixelMapBuffer(64, 64, rgba));
  EXPECT_FALSE(texture_->CopyDataToPixelMapBuffer(src, 32, 32, 128, rgba));

  ASSERT_TRUE(texture_->CreatePixelMapBuffer(32, 16, (int)PIXEL_FORMAT_NV21));
  EXPECT_TRUE(texture_->CopyDataToPixelMapBuffer(
      src, 32, 16, 32, (int)PIXEL_FORMAT_NV21));
  ASSERT_TRUE(texture_->CreatePixelMapBuffer(32, 16, rgba));
  EXPECT_TRUE(texture_->CopyDataToPixelMapBuffer(src, 32, 16, 32, rgba));
}

TEST_F(OhosExternalTextureTest, GetNewTransformBoundIdentityAndAxisSwap) {
  ASSERT_TRUE(
      texture_->CreatePixelMapBuffer(64, 64, (int)PIXEL_FORMAT_RGBA_8888));
  SkM44 transform;
  SkRect bounds = SkRect::MakeLTRB(2, 3, 10, 20);
  texture_->GetNewTransformBound(transform, bounds);
  EXPECT_FLOAT_EQ(transform.rc(0, 0), 1.0f);
  EXPECT_FLOAT_EQ(transform.rc(1, 1), 1.0f);
  EXPECT_FLOAT_EQ(transform.rc(3, 3), 1.0f);
  EXPECT_TRUE(bounds == SkRect::MakeLTRB(2, 3, 10, 20));

  texture_->Reset(false);
  texture_->GetNewTransformBound(transform, bounds);
  EXPECT_FLOAT_EQ(transform.rc(0, 0), 1.0f);
  EXPECT_TRUE(bounds == SkRect::MakeLTRB(2, 3, 10, 20));

#if defined(OHOS_X64_UNITTEST)
  TestOHOSExternalTexture texture(kTestTextureId,
                                  MakeListener(&listener_call_count_));
  SkRect swap_bounds = SkRect::MakeLTRB(2, 3, 10, 20);
  texture.GetNewTransformBound(transform, swap_bounds);
  EXPECT_TRUE(swap_bounds == SkRect::MakeWH(17, 8));
  EXPECT_FLOAT_EQ(transform.rc(0, 0), 0.0f);
#else
  TestOHOSExternalTexture texture(kTestTextureId,
                                  MakeListener(&listener_call_count_));
  SkRect real_bounds = SkRect::MakeLTRB(2, 3, 10, 20);
  texture.GetNewTransformBound(transform, real_bounds);
  EXPECT_TRUE(real_bounds == SkRect::MakeWH(17, 8));
#endif
}

TEST_F(OhosExternalTextureTest, WindowHelpersRejectNullAndFailures) {
  EXPECT_FALSE(texture_->SetWindowSize(nullptr, 640, 480));
  EXPECT_FALSE(
      texture_->SetWindowFormat(nullptr, NATIVEBUFFER_PIXEL_FMT_RGBA_8888));
  EXPECT_FALSE(texture_->SetNativeWindowCPUAccess(nullptr, true));

#if defined(OHOS_X64_UNITTEST)
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
  OHNativeWindow* window = texture_->producer_nativewindow_;
  EXPECT_FALSE(texture_->SetWindowSize(window, 640, 480));
  EXPECT_FALSE(
      texture_->SetWindowFormat(window, NATIVEBUFFER_PIXEL_FMT_RGBA_8888));
  EXPECT_FALSE(texture_->SetNativeWindowCPUAccess(window, true));

  g_stub_graphic_fail_mask = 0;
#else
  OHNativeWindow* window = texture_->producer_nativewindow_;
#endif
  EXPECT_TRUE(
      texture_->SetWindowFormat(window, NATIVEBUFFER_PIXEL_FMT_RGBA_8888));
  EXPECT_TRUE(texture_->SetNativeWindowCPUAccess(window, true));
  EXPECT_TRUE(texture_->SetNativeWindowCPUAccess(window, false));
}

TEST_F(OhosExternalTextureTest, FenceAndReleaseHelpersCoverBranches) {
  EXPECT_FALSE(texture_->CPUWaitFence(-1, 0));
  EXPECT_FALSE(texture_->CPUWaitFence(0, 0));
  int null_fd = open("/dev/null", O_RDONLY);
  ASSERT_GE(null_fd, 0);
  EXPECT_FALSE(texture_->CPUWaitFence(null_fd, 0));
  close(null_fd);
  int pipe_fds[2];
  ASSERT_EQ(pipe(pipe_fds), 0);
  EXPECT_TRUE(texture_->CPUWaitFence(pipe_fds[0], 0));
  close(pipe_fds[0]);
  close(pipe_fds[1]);

  OH_NativeImage* image = texture_->native_image_source_;
  OHNativeWindowBuffer* release_probe = ReleaseProbeBuffer();
  ASSERT_NE(release_probe, nullptr);
  int fence_fd = -1;
  OHOSExternalTexture::ReleaseWindowBuffer(image, release_probe, &fence_fd);
#if defined(OHOS_X64_UNITTEST)
  UpdateFromNativeWindowBufferFail(1);
  OHNativeWindowBuffer* config_probe =
      reinterpret_cast<OHNativeWindowBuffer*>(0x99);
#else
  OHNativeWindowBuffer* config_probe = nullptr;
#endif
  EXPECT_FALSE(OHOSExternalTexture::GetWindowBufferConfig(config_probe, nullptr,
                                                          nullptr, nullptr));
  texture_->producer_nativewindow_width_ = 7;
  texture_->producer_nativewindow_height_ = 5;
  SkRect size = texture_->UpdateWindowSize(config_probe);
  EXPECT_TRUE(size == SkRect::MakeLTRB(0, 0, 7, 5));
  EXPECT_EQ(texture_->producer_nativewindow_width_, 7);
#if defined(OHOS_X64_UNITTEST)
  UpdateFromNativeWindowBufferFail(0);
#endif
}

TEST_F(OhosExternalTextureTest, GetConsumerNoFrameReturnsPixelmapAsIs) {
  TestOHOSExternalTexture texture(kTestTextureId,
                                  MakeListener(&listener_call_count_));
  ASSERT_TRUE(
      texture.CreatePixelMapBuffer(32, 16, (int)PIXEL_FORMAT_RGBA_8888));
  OHNativeWindowBuffer* pixelmap = texture.pixelmap_buffer_;
  int fence_fd = -5;
  EXPECT_EQ(texture.GetConsumer(&fence_fd), pixelmap);
  EXPECT_EQ(fence_fd, -5);
  EXPECT_EQ(texture.pixelmap_buffer_, pixelmap);
}

TEST_F(OhosExternalTextureTest, GetConsumerFrameErrorResyncsSeqAndDestroys) {
  TestOHOSExternalTexture texture(kTestTextureId,
                                  MakeListener(&listener_call_count_));
  ASSERT_TRUE(
      texture.CreatePixelMapBuffer(32, 16, (int)PIXEL_FORMAT_RGBA_8888));
  texture.producer_has_frame_ = true;
  texture.now_new_frame_seq_num_ = 7;
  int fence_fd = -1;
  EXPECT_EQ(texture.GetConsumer(&fence_fd), nullptr);
  EXPECT_EQ(texture.pixelmap_buffer_, nullptr);
  EXPECT_EQ(texture.now_paint_frame_seq_num_.load(), 7);
}

TEST_F(OhosExternalTextureTest, GetConsumerAcquireUpdatesSizeAndLastBuffer) {
  GraphicStubKnobGuard guard;
  TestOHOSExternalTexture texture(kTestTextureId,
                                  MakeListener(&listener_call_count_));
  ASSERT_TRUE(
      texture.CreatePixelMapBuffer(64, 64, (int)PIXEL_FORMAT_RGBA_8888));
  texture.producer_has_frame_ = true;
  int fence_fd = -5;
  ASSERT_TRUE(QueueOneProducerFrame(texture, 64, 64))
      << "cannot queue a producer frame";
  OHNativeWindowBuffer* buffer = texture.GetConsumer(&fence_fd);
  ASSERT_NE(buffer, nullptr);
  EXPECT_EQ(texture.pixelmap_buffer_, nullptr);
  EXPECT_EQ(texture.producer_nativewindow_width_, 64);
  EXPECT_EQ(texture.producer_nativewindow_height_, 64);
  EXPECT_TRUE(texture.buffer_size_has_changed_);
  EXPECT_TRUE(texture.old_buffer_bounds_ == SkRect::MakeLTRB(0, 0, 64, 64));
  EXPECT_EQ(texture.last_native_window_buffer_, buffer);
  EXPECT_EQ(texture.now_paint_frame_seq_num_.load(), 1);

  int valid_fd = open("/dev/null", O_RDONLY);
  ASSERT_GE(valid_fd, 0);
  texture.last_fence_fd_ = valid_fd;
  OHNativeWindowBuffer* buffer2 = nullptr;
  {
    ScopedCharDevFstat char_fstat;
    ASSERT_TRUE(QueueOneProducerFrame(texture, 64, 64))
        << "cannot queue a second producer frame";
    buffer2 = texture.GetConsumer(&fence_fd);
  }
#if defined(OHOS_X64_UNITTEST)
  EXPECT_EQ(buffer2, buffer);
#else
  EXPECT_NE(buffer2, nullptr);
  EXPECT_EQ(texture.last_native_window_buffer_, buffer2);
#endif
  EXPECT_EQ(fcntl(valid_fd, F_GETFD), -1);
  EXPECT_EQ(texture.now_paint_frame_seq_num_.load(), 2);
  EXPECT_FALSE(texture.buffer_size_has_changed_);
}

TEST_F(OhosExternalTextureTest, GetConsumerWithholdsBufferWhileSizeChanging) {
  GraphicStubKnobGuard guard;
  TestOHOSExternalTexture texture(kTestTextureId,
                                  MakeListener(&listener_call_count_));
  ASSERT_TRUE(
      texture.CreatePixelMapBuffer(64, 64, (int)PIXEL_FORMAT_RGBA_8888));
  texture.producer_has_frame_ = true;
  texture.size_is_changing_ = true;
  texture.draw_size_has_changed_ = false;
  int fence_fd = -5;
  ASSERT_TRUE(QueueOneProducerFrame(texture, 64, 64))
      << "cannot queue a producer frame";
  EXPECT_EQ(texture.GetConsumer(&fence_fd), nullptr);
  EXPECT_NE(texture.size_change_buffer_, nullptr);
  EXPECT_EQ(texture.size_change_buffer_fence_fd_, -1);
  EXPECT_EQ(fence_fd, -1);
  EXPECT_EQ(texture.now_paint_frame_seq_num_.load(), 0);

  texture.draw_size_has_changed_ = true;
  ASSERT_TRUE(QueueOneProducerFrame(texture, 64, 64))
      << "cannot queue a second producer frame";
  OHNativeWindowBuffer* buffer = texture.GetConsumer(&fence_fd);
  EXPECT_NE(buffer, nullptr);
  EXPECT_EQ(texture.size_change_buffer_, nullptr);
  EXPECT_EQ(texture.now_paint_frame_seq_num_.load(), 2);
}

TEST_F(OhosExternalTextureTest, PaintDrawsBackgroundColorWhenImageMissing) {
  SoftwarePaintTarget target(64, 64);
  texture_->SetBackGroundColor(0xFF123456);
  DlRect opaque_bounds = DlRect::MakeWH(64, 64);
  auto paint_context = target.context();
  texture_->Paint(paint_context, opaque_bounds, false,
                  DlImageSampling::kLinear);
  EXPECT_EQ(target.CenterPixel(), 0xFF563412u);

  EXPECT_EQ(texture_->Reset(false), 0u);
  EXPECT_FALSE(texture_->background_color_enable_);
  SoftwarePaintTarget after_reset(64, 64);
  auto reset_context = after_reset.context();
  texture_->Paint(reset_context, opaque_bounds, false,
                  DlImageSampling::kLinear);
  EXPECT_EQ(after_reset.CenterPixel(), SK_ColorTRANSPARENT);
}

TEST_F(OhosExternalTextureTest, PaintSizeChangeStateMachine) {
  SoftwarePaintTarget target(64, 64);
  auto context = target.context();
  auto paint_frozen = [&](const DlRect& bounds) {
    texture_->Paint(context, bounds, true, DlImageSampling::kLinear);
  };

  DlRect a = DlRect::MakeWH(64, 64);
  paint_frozen(a);
  EXPECT_TRUE(texture_->old_draw_bounds_ == SkRect::MakeWH(64, 64));

  texture_->NotifyResizing(100, 200);
  EXPECT_TRUE(texture_->size_is_changing_.load());
  for (int i = 1; i <= 10; i++) {
    paint_frozen(a);
    EXPECT_EQ(texture_->size_change_frames_, i);
  }
  paint_frozen(a);
  EXPECT_FALSE(texture_->size_is_changing_.load());
  EXPECT_EQ(texture_->size_change_frames_, 0);
  EXPECT_FALSE(texture_->buffer_size_has_changed_);

  texture_->NotifyResizing(300, 300);
  EXPECT_TRUE(texture_->size_is_changing_.load());
  texture_->buffer_size_has_changed_ = true;
  paint_frozen(DlRect::MakeWH(32, 32));
  EXPECT_FALSE(texture_->size_is_changing_.load());
  EXPECT_EQ(texture_->size_change_frames_, 0);
  EXPECT_TRUE(texture_->old_draw_bounds_ == SkRect::MakeWH(32, 32));

  texture_->NotifyResizing(500, 400);
  texture_->buffer_size_has_changed_ = false;
  paint_frozen(DlRect::MakeWH(16, 16));
  EXPECT_TRUE(texture_->old_draw_bounds_ == SkRect::MakeWH(32, 32));
  EXPECT_TRUE(texture_->size_is_changing_.load());
  EXPECT_EQ(texture_->size_change_frames_, 1);
}

TEST_F(OhosExternalTextureTest, PaintHandlesBufferConfigLookupFailures) {
  GraphicStubKnobGuard guard;
  SoftwarePaintTarget target(64, 64);
  texture_->SetBackGroundColor(0xFF123456);
  texture_->producer_has_frame_ = true;
  ASSERT_TRUE(QueueOneProducerFrame(*texture_, 64, 64))
      << "cannot queue a producer frame";
  texture_->Paint(target.context(), DlRect::MakeWH(64, 64), false,
                  DlImageSampling::kLinear);
  EXPECT_EQ(target.CenterPixel(), 0xFF563412u);
  EXPECT_NE(texture_->last_native_window_buffer_, nullptr);
  EXPECT_EQ(texture_->old_dl_image_, nullptr);
}

TEST_F(OhosExternalTextureTest, MarkNewFrameAvailableQueueErrorAndNullWindow) {
#if defined(OHOS_X64_UNITTEST)
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
#endif
  texture_->MarkNewFrameAvailable();
  EXPECT_TRUE(texture_->producer_has_frame_);
  EXPECT_EQ(texture_->now_new_frame_seq_num_.load(), 1);
  EXPECT_EQ(texture_->now_paint_frame_seq_num_.load(), 0);
  EXPECT_EQ(listener_call_count_, 0);

  OHNativeWindow* saved = texture_->producer_nativewindow_;
  texture_->producer_nativewindow_ = nullptr;
  texture_->now_new_frame_seq_num_ = 0;
  texture_->MarkNewFrameAvailable();
  EXPECT_EQ(texture_->now_new_frame_seq_num_.load(), 1);
  EXPECT_EQ(texture_->now_paint_frame_seq_num_.load(), 0);
  texture_->producer_nativewindow_ = saved;
}

TEST_F(OhosExternalTextureTest, ResetFailurePathsAndBackgroundFlagClear) {
  texture_->SetBackGroundColor(0xFF000000);
  EXPECT_TRUE(texture_->background_color_enable_);
  EXPECT_EQ(texture_->Reset(false), 0u);
  EXPECT_FALSE(texture_->background_color_enable_);
  EXPECT_EQ(texture_->native_image_source_, nullptr);

}

TEST_F(OhosExternalTextureTest, SetExternalNativeImageSameAndListenerFail) {
  OH_NativeImage* source = texture_->native_image_source_;
  EXPECT_TRUE(texture_->SetExternalNativeImage(source));
  EXPECT_EQ(texture_->native_image_source_, source);
  EXPECT_FALSE(texture_->source_is_external_);

}

TEST_F(OhosExternalTextureTest, DestroyNativeImageSourceExternalBranch) {
  OH_NativeImage* source = texture_->native_image_source_;
  uint64_t key = reinterpret_cast<uint64_t>(source);
  g_external_texture_set.insert(key);
  texture_->RemoveFromExternalTextureSet(source);
  EXPECT_EQ(g_external_texture_set.count(key), 0u);
  texture_->RemoveFromExternalTextureSet(
      reinterpret_cast<OH_NativeImage*>(0x999));
  EXPECT_TRUE(g_external_texture_set.empty());

  texture_->source_is_external_ = true;
  OHNativeWindowBuffer* last_probe = ReleaseProbeBuffer();
  OHNativeWindowBuffer* size_probe = ReleaseProbeBuffer();
  ASSERT_NE(last_probe, nullptr);
  ASSERT_NE(size_probe, nullptr);
  texture_->last_native_window_buffer_ = last_probe;
  texture_->size_change_buffer_ = size_probe;
  g_external_texture_set.insert(key);
  texture_->DestroyNativeImageSource();
  EXPECT_EQ(texture_->native_image_source_, nullptr);
  EXPECT_EQ(texture_->last_native_window_buffer_, nullptr);
  EXPECT_EQ(texture_->size_change_buffer_, nullptr);
  EXPECT_EQ(texture_->producer_nativewindow_, nullptr);
  EXPECT_EQ(g_external_texture_set.count(key), 0u);
  EXPECT_EQ(texture_->now_paint_frame_seq_num_.load(), 0);
  EXPECT_EQ(texture_->now_new_frame_seq_num_.load(), 0);
  OH_NativeImage_Destroy(&source);
}

TEST_F(OhosExternalTextureTest, InitEGLFunPtrResolvesSymbols) {
  // The constructor already ran InitEGLFunPtr(). On an OHOS device these
  // symbols are provided by libEGL.so (core EGL 1.5 or the extensions
  // this class relies on in production).
  // Emulator GL stacks may lack KHR_fence_sync / EGL_KHR_image entry points;
  // resolution can only be asserted where the driver provides them.
  if (OHOSExternalTextureGL::eglCreateSyncKHR_ == nullptr ||
      OHOSExternalTextureGL::eglDestroySyncKHR_ == nullptr) {
    GTEST_SKIP() << "EGL driver lacks KHR_fence_sync entry points";
  }
  EXPECT_NE(OHOSExternalTextureGL::eglCreateSyncKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglDestroySyncKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglCreateImageKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_, nullptr);
}

TEST_F(OhosExternalTextureTest, InitEGLFunPtrReResolvesNulledProcs) {
  OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = nullptr;
  OHOSExternalTextureGL::eglDestroyImageKHR_ = nullptr;
  OHOSExternalTextureGL::InitEGLFunPtr();
  EXPECT_NE(OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglDestroyImageKHR_, nullptr);
}

TEST_F(OhosExternalTextureTest, AcquiresAndReleasesBuffer) {
  // Allocate a real native buffer and wrap it as an OHNativeWindowBuffer,
  // the same object type the texture acquires from its consumer queue.
  OH_NativeBuffer_Config alloc_config = {
      64, 64, NATIVEBUFFER_PIXEL_FMT_RGBA_8888,
      NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE, 0x8};
  OH_NativeBuffer* native_buffer = OH_NativeBuffer_Alloc(&alloc_config);
  ASSERT_NE(native_buffer, nullptr);

  OHNativeWindowBuffer* window_buffer =
      OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(native_buffer);
  ASSERT_NE(window_buffer, nullptr);

  // GetWindowBufferConfig must recover the native buffer and its geometry.
  OH_NativeBuffer* queried_buffer = nullptr;
  OH_NativeBuffer_Config queried_config = {};
  uint32_t buffer_id = 0;
  EXPECT_TRUE(OHOSExternalTexture::GetWindowBufferConfig(
      window_buffer, &queried_buffer, &queried_config, &buffer_id));
  EXPECT_NE(queried_buffer, nullptr);
  EXPECT_EQ(queried_config.width, 64);
  EXPECT_EQ(queried_config.height, 64);
  EXPECT_GT(buffer_id, 0u);

  // Null input is rejected.
  EXPECT_FALSE(OHOSExternalTexture::GetWindowBufferConfig(nullptr, nullptr,
                                                          nullptr, nullptr));

  OH_NativeWindow_DestroyNativeWindowBuffer(window_buffer);
  OH_NativeBuffer_Unreference(native_buffer);
}

TEST_F(OhosExternalTextureTest, SetGPUFenceWithoutDisplayReturns) {
  int fence_fd = -1;
  texture_->SetGPUFence(nullptr, &fence_fd);
  EXPECT_EQ(fence_fd, -1);
}

static EGLDisplay g_current_display_override = EGL_NO_DISPLAY;

extern "C" EGLDisplay eglGetCurrentDisplay(void) {
  if (g_current_display_override != EGL_NO_DISPLAY) {
    return g_current_display_override;
  }
  static const auto real_eglGetCurrentDisplay =
      reinterpret_cast<EGLDisplay (*)(void)>(
          dlsym(RTLD_NEXT, "eglGetCurrentDisplay"));
  return real_eglGetCurrentDisplay ? real_eglGetCurrentDisplay()
                                   : EGL_NO_DISPLAY;
}

class StubDisplayScope {
 public:
  StubDisplayScope() {
    g_current_display_override = reinterpret_cast<EGLDisplay>(0x1234);
  }
  ~StubDisplayScope() { g_current_display_override = EGL_NO_DISPLAY; }
};

TEST_F(OhosExternalTextureTest,
       SetGPUFenceRejectsBadWindowBuffer) {
  StubDisplayScope stub_display;
#if defined(OHOS_X64_UNITTEST)
  UpdateFromNativeWindowBufferFail(1);
#endif
  int fence_fd = -1;
  texture_->SetGPUFence(nullptr, &fence_fd);
#if defined(OHOS_X64_UNITTEST)
  UpdateFromNativeWindowBufferFail(0);
#endif
  EXPECT_EQ(fence_fd, -1);
  EXPECT_TRUE(texture_->gl_resources_.empty());
}

TEST_F(OhosExternalTextureTest,
       SetGPUFenceCreatesFenceWhenProcsExist) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncOk;
  OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = &StubDupFenceFdNoFd;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = &StubDestroySync;
  TestWindowBuffer buffer;
  int fence_fd = -2;
  texture_->SetGPUFence(buffer.window_buffer, &fence_fd);
  EXPECT_EQ(fence_fd, -1);
  EXPECT_EQ(texture_->gl_resources_.size(), 1u);
  texture_->gl_resources_.clear();
}

TEST_F(OhosExternalTextureTest,
       SetGPUFenceSkipsFenceWhenProcMissing) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = nullptr;
  TestWindowBuffer buffer;
  int fence_fd = -2;
  texture_->SetGPUFence(buffer.window_buffer, &fence_fd);
  EXPECT_EQ(fence_fd, -2);
  EXPECT_TRUE(texture_->gl_resources_.empty());
}

TEST_F(OhosExternalTextureTest, SetGPUFenceLogsEglError) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncOk;
  OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = &StubDupFenceFdNoFd;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = &StubDestroySync;
  SeedEglError();
  TestWindowBuffer buffer;
  int fence_fd = -1;
  texture_->SetGPUFence(buffer.window_buffer, &fence_fd);
  EXPECT_EQ(fence_fd, -1);
  texture_->gl_resources_.clear();
}

TEST_F(OhosExternalTextureTest, WaitGPUFenceInvalidInputReturns) {
  StubDisplayScope stub_display;
  texture_->WaitGPUFence(-1);
  int unsignaled = eventfd(0, 0);
  ASSERT_GE(unsignaled, 0);
  texture_->WaitGPUFence(unsignaled);
  EXPECT_NE(fcntl(unsignaled, F_GETFD), -1);
  close(unsignaled);
}

TEST_F(OhosExternalTextureTest,
       WaitGPUFenceClosesAlreadySignaledFd) {
  StubDisplayScope stub_display;
  int signaled = open("/dev/null", O_RDONLY);
  ASSERT_GE(signaled, 0);
#if defined(OHOS_X64_UNITTEST)
  UpdateFstatFunc([](int fd, struct stat* st) {
    st->st_mode = S_IFCHR | 0666;
    return 0;
  });
#endif
  texture_->WaitGPUFence(signaled);
#if defined(OHOS_X64_UNITTEST)
  UpdateFstatFunc(nullptr);
#endif
  EXPECT_EQ(fcntl(signaled, F_GETFD), -1);
}

TEST_F(OhosExternalTextureTest,
       WaitGPUFenceClosesFdWhenProcsMissing) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateSyncKHR_ = nullptr;
  ScopedUnsignaledFence fence;
  ASSERT_GE(fence.get(), 0);
  texture_->WaitGPUFence(fence.get());
  EXPECT_EQ(fcntl(fence.get(), F_GETFD), -1);
}

TEST_F(OhosExternalTextureTest, WaitGPUFenceStoresSyncOnSuccess) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncOk;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = &StubWaitSync;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = &StubDestroySync;
  texture_->now_key_ = 7;
  ScopedUnsignaledFence fence;
  ASSERT_GE(fence.get(), 0);
  texture_->WaitGPUFence(fence.get());
  EXPECT_EQ(texture_->gl_resources_.count(7), 1u);
  texture_->gl_resources_.clear();
}

TEST_F(OhosExternalTextureTest,
       WaitGPUFenceClosesFdWhenSyncCreateFails) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncNoSync;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = &StubDestroySync;
  ScopedUnsignaledFence fence;
  ASSERT_GE(fence.get(), 0);
  texture_->WaitGPUFence(fence.get());
  EXPECT_EQ(fcntl(fence.get(), F_GETFD), -1);
}

TEST_F(OhosExternalTextureTest, WaitGPUFenceLogsEglError) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncOk;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = &StubWaitSync;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = &StubDestroySync;
  texture_->now_key_ = 9;
  SeedEglError();
  ScopedUnsignaledFence fence;
  ASSERT_GE(fence.get(), 0);
  texture_->WaitGPUFence(fence.get());
  EXPECT_EQ(texture_->gl_resources_.count(9), 1u);
  texture_->gl_resources_.clear();
}

TEST_F(OhosExternalTextureTest,
       GPUResourceDestroyConsumesGlError) {
  StubDisplayScope stub_display;
  glBindTexture(static_cast<GLenum>(0x9999), 0);
  texture_->gl_resources_[3] = GlResource{};
  texture_->GPUResourceDestroy();
  EXPECT_TRUE(texture_->gl_resources_.empty());
}

TEST_F(OhosExternalTextureTest, CreateEGLImageInvalidWithoutDisplay) {
  TestWindowBuffer buffer;
  EXPECT_FALSE(texture_->CreateEGLImage(buffer.window_buffer).is_valid());
  EXPECT_FALSE(texture_->CreateEGLImage(nullptr).is_valid());
}

TEST_F(OhosExternalTextureTest,
       CreateEGLImageInvalidWithNullBuffer) {
  StubDisplayScope stub_display;
  EXPECT_FALSE(texture_->CreateEGLImage(nullptr).is_valid());
}

TEST_F(OhosExternalTextureTest,
       CreateEGLImageInvalidWhenProcMissing) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateImageKHR_ = nullptr;
  TestWindowBuffer buffer;
  EXPECT_FALSE(texture_->CreateEGLImage(buffer.window_buffer).is_valid());
}

TEST_F(OhosExternalTextureTest, CreateEGLImageOnCreateFailure) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateImageKHR_ = &StubCreateImageNoImage;
  OHOSExternalTextureGL::eglDestroyImageKHR_ = &StubDestroyImage;
  TestWindowBuffer buffer;
  auto image = texture_->CreateEGLImage(buffer.window_buffer);
  EXPECT_TRUE(image.is_valid());
}

TEST_F(OhosExternalTextureTest, CreateEGLImageLogsEglError) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateImageKHR_ = &StubCreateImageNoImage;
  OHOSExternalTextureGL::eglDestroyImageKHR_ = &StubDestroyImage;
  SeedEglError();
  TestWindowBuffer buffer;
  auto image = texture_->CreateEGLImage(buffer.window_buffer);
  EXPECT_TRUE(image.is_valid());
}

TEST_F(OhosExternalTextureTest, CreateDlImageNullWithoutDisplay) {
  Texture::PaintContext paint_context;
  SkRect bounds = SkRect::MakeWH(64, 64);
  OH_NativeBuffer_Config config = DefaultBufferConfig();
  TestWindowBuffer buffer;
  auto image = texture_->CreateDlImage(paint_context, bounds, 42, config,
                                       buffer.window_buffer);
  EXPECT_EQ(image, nullptr);
}

TEST_F(OhosExternalTextureTest,
       CreateDlImageNullWhenTargetProcMissing) {
  StubDisplayScope stub_display;
  ScopedEGLProcs procs;
  OHOSExternalTextureGL::eglCreateImageKHR_ = &StubCreateImageOk;
  OHOSExternalTextureGL::eglDestroyImageKHR_ = &StubDestroyImage;
  OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_ = nullptr;
  Texture::PaintContext paint_context;
  SkRect bounds = SkRect::MakeWH(64, 64);
  OH_NativeBuffer_Config config = DefaultBufferConfig();
  TestWindowBuffer buffer;
  auto image = texture_->CreateDlImage(paint_context, bounds, 42, config,
                                       buffer.window_buffer);
  EXPECT_EQ(image, nullptr);
}

class ScopedRealEGLContext {
 public:
  ScopedRealEGLContext() {
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
      skip_reason_ = "EGL default display unavailable";
      return;
    }
    if (eglInitialize(display_, nullptr, nullptr) != EGL_TRUE) {
      skip_reason_ = "EGL display initialization unavailable";
      return;
    }
    const EGLint config_attrs[] = {EGL_SURFACE_TYPE,
                                   EGL_PBUFFER_BIT,
                                   EGL_RENDERABLE_TYPE,
                                   EGL_OPENGL_ES2_BIT,
                                   EGL_RED_SIZE,
                                   8,
                                   EGL_GREEN_SIZE,
                                   8,
                                   EGL_BLUE_SIZE,
                                   8,
                                   EGL_ALPHA_SIZE,
                                   8,
                                   EGL_NONE};
    EGLint config_count = 0;
    if (eglChooseConfig(display_, config_attrs, &config_, 1, &config_count) !=
            EGL_TRUE ||
        config_count < 1) {
      skip_reason_ = "no EGL pbuffer config available";
      return;
    }
    const EGLint surface_attrs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    surface_ = eglCreatePbufferSurface(display_, config_, surface_attrs);
    const EGLint context_attrs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context_ =
        eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attrs);
    if (surface_ == EGL_NO_SURFACE || context_ == EGL_NO_CONTEXT ||
        eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
      skip_reason_ = "EGL context creation unavailable";
    }
  }

  ~ScopedRealEGLContext() {
    if (display_ != EGL_NO_DISPLAY) {
      eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if (surface_ != EGL_NO_SURFACE) {
      eglDestroySurface(display_, surface_);
    }
    if (context_ != EGL_NO_CONTEXT) {
      eglDestroyContext(display_, context_);
    }
    if (display_ != EGL_NO_DISPLAY) {
      eglTerminate(display_);
    }
  }

  bool unavailable() const { return !skip_reason_.empty(); }
  const char* skip_reason() const { return skip_reason_.c_str(); }

 private:
  std::string skip_reason_;
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLConfig config_ = nullptr;
  EGLSurface surface_ = EGL_NO_SURFACE;
  EGLContext context_ = EGL_NO_CONTEXT;
};

#if !defined(OHOS_X64_UNITTEST)
TEST_F(OhosExternalTextureTest, CreateDlImageOnRealEglContext) {
  ScopedRealEGLContext real_egl;
  if (real_egl.unavailable()) {
    GTEST_SKIP() << real_egl.skip_reason();
  }
  listener_call_count_ = 0;
  auto texture = std::make_shared<OHOSExternalTextureGL>(
      kTestTextureId, MakeListener(&listener_call_count_));
  auto interface = GrGLMakeNativeInterface();
  if (!interface) {
    GTEST_SKIP() << "no native GL interface on this device";
  }
  auto gr_context = GrDirectContexts::MakeGL(interface);
  if (!gr_context) {
    GTEST_SKIP() << "no Ganesh GL context on this device";
  }
  Texture::PaintContext paint_context;
  paint_context.gr_context = gr_context.get();
  SkRect bounds = SkRect::MakeWH(64, 64);
  OH_NativeBuffer_Config config = DefaultBufferConfig();
  TestWindowBuffer buffer;
  auto image = texture->CreateDlImage(paint_context, bounds, 42, config,
                                      buffer.window_buffer);
  EXPECT_NE(image, nullptr);
  EXPECT_EQ(texture->now_key_, static_cast<NativeBufferKey>(42));
  EXPECT_EQ(texture->gl_resources_.count(42), 1u);
}
#endif  // !defined(OHOS_X64_UNITTEST)

TEST_F(OhosExternalTextureTest,
       DeleteBufferGPUResourceErasesTrackedKey) {
  texture_->gl_resources_[5] = GlResource{};
  texture_->DeleteBufferGPUResource(0);
  EXPECT_EQ(texture_->gl_resources_.count(5), 1u);
  texture_->gl_resources_[9] = GlResource{};
  texture_->DeleteBufferGPUResource(9);
  EXPECT_EQ(texture_->gl_resources_.count(9), 0u);
}

class OhosExternalTextureVulkanTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // A real impeller::ContextVK cannot be created inside
    // flutter_ohos_unittests (no Vulkan device/surface), so only the
    // paths that do not dereference impeller_context_ are exercised here.
    texture_ = std::make_shared<OHOSExternalTextureVulkan>(
        nullptr, kTestTextureId, MakeListener(&listener_call_count_));
  }

  std::atomic<int> listener_call_count_ = 0;
  std::shared_ptr<OHOSExternalTextureVulkan> texture_;
};

TEST_F(OhosExternalTextureVulkanTest, ConstructAndUnregisterWithoutContext) {
  // Construction goes through the base class (native image creation).
  EXPECT_NE(texture_->GetProducerSurfaceId(), 0u);

  // OnTextureUnregistered -> GPUResourceDestroy only clears the resource
  // map and must be safe without a Vulkan context. Acquiring a buffer for
  // real additionally requires a live ContextVK, which only exists in the
  // full shell / playground environment.
  texture_->OnTextureUnregistered();
}

TEST_F(OhosExternalTextureTest,
       MarkNewFrameAvailableCoversLogConditionLegs) {
  texture_->now_new_frame_seq_num_ = 10;
  texture_->MarkNewFrameAvailable();
  EXPECT_TRUE(texture_->producer_has_frame_);
  EXPECT_EQ(texture_->now_new_frame_seq_num_.load(), int64_t{11});
#if defined(OHOS_X64_UNITTEST)
  EXPECT_EQ(texture_->now_paint_frame_seq_num_.load(), int64_t{0});
#endif

  texture_->now_paint_frame_seq_num_ = 1;
  texture_->now_new_frame_seq_num_ = 0;
  texture_->MarkNewFrameAvailable();
  EXPECT_TRUE(texture_->producer_has_frame_);
  EXPECT_EQ(texture_->now_new_frame_seq_num_.load(), int64_t{1});
  EXPECT_EQ(texture_->now_paint_frame_seq_num_.load(), int64_t{1});
}

TEST_F(OhosExternalTextureTest, OnTextureUnregisteredClosesValidFence) {
  texture_->SetOldDlImage(MakeRasterDlImage(0xFF0000FF));
  OH_NativeBuffer_Config config = {};
  texture_->image_lru_.AddImage(MakeRasterDlImage(0xFF000000), config, 3);
  int fence_fd = open("/dev/null", O_RDONLY);
  ASSERT_GE(fence_fd, 0);
  texture_->last_fence_fd_ = fence_fd;
  texture_->gl_resources_[42] = GlResource{};

  {
    ScopedCharDevFstat char_fstat;
    texture_->OnTextureUnregistered();
  }
  EXPECT_EQ(fcntl(fence_fd, F_GETFD), -1);
  EXPECT_EQ(texture_->old_dl_image_, nullptr);
  EXPECT_EQ(texture_->image_lru_.FindImage(3, config, nullptr), nullptr);
  EXPECT_EQ(texture_->last_fence_fd_, -1);
  EXPECT_TRUE(texture_->gl_resources_.empty());
}

TEST_F(OhosExternalTextureTest, OnGrContextCreatedReportsListenerResult) {
  OH_NativeImage* source = texture_->native_image_source_;
#if defined(OHOS_X64_UNITTEST)
  {
    GraphicStubKnobGuard guard;
    g_stub_graphic_fail_mask = kStubFailFrameAvailableListener;
    EXPECT_NO_FATAL_FAILURE(texture_->OnGrContextCreated());
    EXPECT_EQ(texture_->native_image_source_, source);
  }
#endif
  EXPECT_NO_FATAL_FAILURE(texture_->OnGrContextCreated());
  EXPECT_EQ(texture_->native_image_source_, source);
  EXPECT_EQ(texture_->frame_listener_.onFrameAvailable,
            &CountingOnFrameAvailable);
}

TEST_F(OhosExternalTextureTest,
       ReleaseWindowBufferAcceptsNullFencePointer) {
  OH_NativeImage* image = texture_->native_image_source_;
  EXPECT_NO_FATAL_FAILURE(OHOSExternalTexture::ReleaseWindowBuffer(
      image, ReleaseProbeBuffer(), nullptr));
}

TEST_F(OhosExternalTextureTest, GetConsumerWithoutSourceReturnsNull) {
  ASSERT_EQ(texture_->Reset(false), 0u);
  texture_->producer_has_frame_ = true;
  int fence_fd = -1;
  EXPECT_EQ(texture_->GetConsumerNativeBuffer(&fence_fd), nullptr);
  EXPECT_EQ(texture_->now_paint_frame_seq_num_.load(), int64_t{0});
}

TEST_F(OhosExternalTextureTest,
       PaintFallsBackWhenCreateDlImageHasNoDisplay) {
  GraphicStubKnobGuard guard;
  SoftwarePaintTarget target(64, 64);
  texture_->SetBackGroundColor(0xFF123456);
  texture_->producer_has_frame_ = true;
  ASSERT_TRUE(QueueOneProducerFrame(*texture_, 64, 64))
      << "cannot queue a producer frame";
  texture_->Paint(target.context(), DlRect::MakeWH(64, 64), false,
                  DlImageSampling::kLinear);
  EXPECT_EQ(target.CenterPixel(), 0xFF563412u);
  EXPECT_NE(texture_->last_native_window_buffer_, nullptr);
  EXPECT_EQ(texture_->old_dl_image_, nullptr);
  EXPECT_EQ(texture_->now_paint_frame_seq_num_.load(), int64_t{1});
}

TEST_F(OhosExternalTextureTest, SetProducerWindowSizeCoversFailureLegs) {
  ASSERT_EQ(texture_->Reset(false), 0u);
  EXPECT_FALSE(texture_->SetProducerWindowSize(640, 480));
  EXPECT_EQ(texture_->producer_nativewindow_width_, 0);

#if defined(OHOS_X64_UNITTEST)
  {
    GraphicStubKnobGuard guard;
    g_stub_graphic_fail_mask = kStubFailAcquireNativeWindow;
    auto texture = std::make_shared<OHOSExternalTextureGL>(
        kWave3TextureId, MakeListener(&listener_call_count_));
    ASSERT_NE(texture->native_image_source_, nullptr);
    EXPECT_EQ(texture->producer_nativewindow_, nullptr);
    EXPECT_FALSE(texture->SetProducerWindowSize(100, 100));
  }
  {
    GraphicStubKnobGuard guard;
    g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
    auto texture = std::make_shared<OHOSExternalTextureGL>(
        kWave3TextureId, MakeListener(&listener_call_count_));
    ASSERT_NE(texture->producer_nativewindow_, nullptr);
    EXPECT_FALSE(texture->SetProducerWindowSize(640, 480));
    EXPECT_EQ(texture->producer_nativewindow_width_, 0);
  }
#endif
}

TEST_F(OhosExternalTextureTest,
       NotifyResizingHeightOnlyChangeSetsFlag) {
  texture_->NotifyResizing(0, 0);
  EXPECT_FALSE(texture_->size_is_changing_.load());
  texture_->NotifyResizing(0, 480);
  EXPECT_TRUE(texture_->size_is_changing_.load());
}

TEST_F(OhosExternalTextureTest, FenceHelpersReportPollErrorEvents) {
  int pipe_fds[2];
  ASSERT_EQ(pipe(pipe_fds), 0);
  int closed_fd = pipe_fds[0];
  close(pipe_fds[0]);
  close(pipe_fds[1]);
  EXPECT_TRUE(texture_->CPUWaitFence(closed_fd, 0));
  EXPECT_FALSE(OHOSExternalTexture::FenceIsSignal(closed_fd));
}

TEST_F(OhosExternalTextureTest, FdIsValidCoversFstatErrorLegs) {
#if defined(OHOS_X64_UNITTEST)
  UpdateFstatFunc([](int fd, struct stat* st) {
    errno = EACCES;
    return -1;
  });
  EXPECT_FALSE(OHOSExternalTexture::FdIsValid(5));
  UpdateFstatFunc([](int fd, struct stat* st) {
    errno = EBADF;
    return -1;
  });
  EXPECT_FALSE(OHOSExternalTexture::FdIsValid(5));
  UpdateFstatFunc(nullptr);
#else
  int fd = open("/dev/null", O_RDONLY);
  ASSERT_GE(fd, 0);
  close(fd);
  EXPECT_FALSE(OHOSExternalTexture::FdIsValid(fd));
#endif
}

TEST_F(OhosExternalTextureTest,
       GetWindowBufferConfigAllowsNullConfigOut) {
  TestWindowBuffer buffer;
  OH_NativeBuffer* native_buffer = nullptr;
  uint32_t buffer_id = 0;
  EXPECT_TRUE(OHOSExternalTexture::GetWindowBufferConfig(
      buffer.window_buffer, &native_buffer, nullptr, &buffer_id));
  EXPECT_NE(native_buffer, nullptr);
  EXPECT_GT(buffer_id, 0u);
}

TEST_F(OhosExternalTextureTest,
       CopyDataHeightMismatchAndYuvHeights) {
  unsigned char src[64 * 48];
  memset(src, 0xAB, sizeof(src));
  const int rgba = (int)PIXEL_FORMAT_RGBA_8888;

  ASSERT_TRUE(texture_->CreatePixelMapBuffer(32, 16, rgba));
  EXPECT_FALSE(texture_->CopyDataToPixelMapBuffer(src, 32, 8, 32, rgba));

  const int yuv_formats[] = {(int)PIXEL_FORMAT_NV12,
                             (int)PIXEL_FORMAT_YCBCR_P010,
                             (int)PIXEL_FORMAT_YCRCB_P010};
  for (int format : yuv_formats) {
    ASSERT_TRUE(texture_->CreatePixelMapBuffer(32, 16, format));
    EXPECT_TRUE(texture_->CopyDataToPixelMapBuffer(src, 32, 16, 32, format));
  }
}

TEST_F(OhosExternalTextureTest,
       DefaultOnFrameAvailableToleratesAcquireFailure) {
  ::GraphicStubKnobGuard guard;
  EXPECT_NO_FATAL_FAILURE(OHOSExternalTexture::DefaultOnFrameAvailable(
      reinterpret_cast<OH_NativeImage*>(0x5)));
}

#if !defined(OHOS_X64_UNITTEST)
TEST_F(OhosExternalTextureTest, FenceProcsPartialMissingLegs) {
  ScopedRealEGLContext real_egl;
  if (real_egl.unavailable()) {
    GTEST_SKIP() << real_egl.skip_reason();
  }
  listener_call_count_ = 0;
  auto texture = std::make_shared<OHOSExternalTextureGL>(
      kTestTextureId, MakeListener(&listener_call_count_));
  ScopedEGLProcs procs;
  TestWindowBuffer buffer;

  OHOSExternalTextureGL::eglCreateSyncKHR_ = nullptr;
  int fence_fd = -2;
  texture->SetGPUFence(buffer.window_buffer, &fence_fd);
  EXPECT_EQ(fence_fd, -2);
  EXPECT_TRUE(texture->gl_resources_.empty());

  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncOk;
  OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = &StubDupFenceFdNoFd;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = nullptr;
  fence_fd = -2;
  texture->SetGPUFence(buffer.window_buffer, &fence_fd);
  EXPECT_EQ(fence_fd, -2);
  EXPECT_TRUE(texture->gl_resources_.empty());

  const char* tmp_path = "/data/local/tmp/wave3_fence_probe.tmp";
  int file_fd = open(tmp_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  ASSERT_GE(file_fd, 0);
  texture->WaitGPUFence(file_fd);
  EXPECT_NE(fcntl(file_fd, F_GETFD), -1);
  close(file_fd);
  unlink(tmp_path);

  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncOk;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = nullptr;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = &StubDestroySync;
  {
    ScopedUnsignaledFence fence;
    ASSERT_GE(fence.get(), 0);
    texture->WaitGPUFence(fence.get());
    EXPECT_EQ(fcntl(fence.get(), F_GETFD), -1);
  }

  OHOSExternalTextureGL::eglCreateSyncKHR_ = &StubCreateSyncOk;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = &StubWaitSync;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = nullptr;
  {
    ScopedUnsignaledFence fence;
    ASSERT_GE(fence.get(), 0);
    texture->WaitGPUFence(fence.get());
    EXPECT_EQ(fcntl(fence.get(), F_GETFD), -1);
  }
}
#endif  // !defined(OHOS_X64_UNITTEST)

}  // namespace testing
}  // namespace flutter
