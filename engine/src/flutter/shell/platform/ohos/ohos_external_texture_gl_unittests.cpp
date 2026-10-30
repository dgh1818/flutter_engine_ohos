/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <gtest/gtest.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "flutter/impeller/toolkit/egl/egl.h"

#include "flutter/fml/log_settings.h"
#include "flutter/shell/platform/ohos/ohos_main.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"

#define private public
#define protected public
#include "flutter/shell/platform/ohos/ohos_external_texture.h"
#include "flutter/shell/platform/ohos/ohos_external_texture_gl.h"
#undef protected
#undef private

namespace {
int g_fake_create_sync_native_fd = -1;

EGLSyncKHR FakeCreateSyncNoSync(EGLDisplay, EGLenum, const EGLint*) {
  return EGL_NO_SYNC_KHR;
}
EGLSyncKHR FakeCreateSyncValid(EGLDisplay, EGLenum, const EGLint* attribs) {
  g_fake_create_sync_native_fd = -1;
  if (attribs != nullptr) {
    for (int i = 0; attribs[i] != EGL_NONE; i += 2) {
      if (attribs[i] == EGL_SYNC_NATIVE_FENCE_FD_ANDROID &&
          attribs[i + 1] >= 0) {
        close(attribs[i + 1]);
        g_fake_create_sync_native_fd = attribs[i + 1];
      }
    }
  }
  return reinterpret_cast<EGLSyncKHR>(0x2);
}
EGLint FakeWaitSync(EGLDisplay, EGLSyncKHR, EGLint) {
  return EGL_TRUE;
}
EGLBoolean FakeDestroySync(EGLDisplay, EGLSyncKHR) {
  return EGL_TRUE;
}
EGLBoolean FakeDestroyImage(EGLDisplay, EGLImageKHR) {
  return EGL_TRUE;
}
EGLImageKHR FakeCreateImageNoImage(EGLDisplay,
                                   EGLContext,
                                   EGLenum,
                                   EGLClientBuffer,
                                   const EGLint*) {
  return EGL_NO_IMAGE_KHR;
}
EGLImageKHR FakeCreateImageValid(EGLDisplay,
                                 EGLContext,
                                 EGLenum,
                                 EGLClientBuffer,
                                 const EGLint*) {
  return reinterpret_cast<EGLImageKHR>(0x3);
}
}  // namespace

namespace flutter {
namespace testing {
extern EGLDisplay g_current_display_override;

class OhosExternalTextureGLTest : public ::testing::Test {
 protected:
  OhosExternalTextureGLTest() {
    OhosMain::NativeInit(nullptr, nullptr);
    saved_create_sync_ = OHOSExternalTextureGL::eglCreateSyncKHR_;
    saved_dup_fence_ = OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_;
    saved_destroy_sync_ = OHOSExternalTextureGL::eglDestroySyncKHR_;
    saved_wait_sync_ = OHOSExternalTextureGL::eglWaitSyncKHR_;
    saved_create_image_ = OHOSExternalTextureGL::eglCreateImageKHR_;
    saved_image_target_ = OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_;
    saved_destroy_image_ = OHOSExternalTextureGL::eglDestroyImageKHR_;
  }

  ~OhosExternalTextureGLTest() override {
    OHOSExternalTextureGL::eglCreateSyncKHR_ = saved_create_sync_;
    OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = saved_dup_fence_;
    OHOSExternalTextureGL::eglDestroySyncKHR_ = saved_destroy_sync_;
    OHOSExternalTextureGL::eglWaitSyncKHR_ = saved_wait_sync_;
    OHOSExternalTextureGL::eglCreateImageKHR_ = saved_create_image_;
    OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_ = saved_image_target_;
    OHOSExternalTextureGL::eglDestroyImageKHR_ = saved_destroy_image_;
    g_current_display_override = EGL_NO_DISPLAY;
  }

  static int MakeSignalledFenceFd() {
    return open("/dev/null", O_RDONLY | O_CLOEXEC);
  }

  static bool FdClosed(int fd) { return fcntl(fd, F_GETFD) == -1; }

  static bool FdReleasedOrReused(int handed_off, int keep_dup) {
    if (fcntl(handed_off, F_GETFD) == -1) {
      return true;
    }
    struct stat handed = {};
    struct stat kept = {};
    if (fstat(handed_off, &handed) != 0 || fstat(keep_dup, &kept) != 0) {
      return true;
    }
    return handed.st_dev != kept.st_dev || handed.st_ino != kept.st_ino;
  }

  PFNEGLCREATESYNCKHRPROC saved_create_sync_ = nullptr;
  PFNEGLDUPNATIVEFENCEFDANDROIDPROC saved_dup_fence_ = nullptr;
  PFNEGLDESTROYSYNCKHRPROC saved_destroy_sync_ = nullptr;
  PFNEGLWAITSYNCKHRPROC saved_wait_sync_ = nullptr;
  PFNEGLCREATEIMAGEKHRPROC saved_create_image_ = nullptr;
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC saved_image_target_ = nullptr;
  PFNEGLDESTROYIMAGEKHRPROC saved_destroy_image_ = nullptr;
};

TEST_F(OhosExternalTextureGLTest, ConstructorLoadsEglProcPointers) {
  OHOSExternalTextureGL texture(11, OH_OnFrameAvailableListener{});
  EXPECT_NE(OHOSExternalTextureGL::eglCreateSyncKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglDestroySyncKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglCreateImageKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglDestroyImageKHR_, nullptr);
  EXPECT_EQ(texture.is_emulator_, OhosMain::IsEmulator());
}

TEST_F(OhosExternalTextureGLTest, SetGPUFenceEarlyReturnsWithoutDisplay) {
  OHOSExternalTextureGL texture(12, OH_OnFrameAvailableListener{});
  int fence_fd = -1;
  texture.SetGPUFence(nullptr, &fence_fd);
  EXPECT_EQ(fence_fd, -1);
}

TEST_F(OhosExternalTextureGLTest, WaitGPUFenceEarlyReturnForInvalidFd) {
  OHOSExternalTextureGL texture(13, OH_OnFrameAvailableListener{});
  EXPECT_NO_FATAL_FAILURE(texture.WaitGPUFence(-1));
  EXPECT_NO_FATAL_FAILURE(texture.WaitGPUFence(0));
}

TEST_F(OhosExternalTextureGLTest, WaitGPUFenceEarlyReturnsWithoutDisplay) {
  OHOSExternalTextureGL texture(14, OH_OnFrameAvailableListener{});
  int fd = MakeSignalledFenceFd();
  ASSERT_GT(fd, 0);
  texture.WaitGPUFence(fd);
  EXPECT_FALSE(FdClosed(fd));
  close(fd);
}

class ScopedUnsignaledFence {
 public:
  ScopedUnsignaledFence() {
    UpdateFstatFunc([](int fd, struct stat* st) {
      if (__real_fstat(fd, st) != 0) {
        return -1;
      }
      st->st_mode = S_IFCHR | (st->st_mode & 0777);
      return 0;
    });
    int fds[2];
    if (pipe2(fds, O_NONBLOCK) == 0) {
      fd_ = fds[0];
      write_fd_ = fds[1];
    }
  }

  ~ScopedUnsignaledFence() {
    UpdateFstatFunc(nullptr);
    if (write_fd_ >= 0) {
      close(write_fd_);
    }
    if (fd_ >= 0 && fcntl(fd_, F_GETFD) != -1) {
      close(fd_);
    }
  }

  int get() const { return fd_; }

 private:
  int fd_ = -1;
  int write_fd_ = -1;
};

TEST_F(OhosExternalTextureGLTest, WaitGPUFenceClosesSignalledFence) {
  OHOSExternalTextureGL texture(15, OH_OnFrameAvailableListener{});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  int fd = MakeSignalledFenceFd();
  ASSERT_GT(fd, 0);
  int keep = dup(fd);
  ASSERT_GE(keep, 0);
  texture.WaitGPUFence(fd);
  EXPECT_TRUE(FdReleasedOrReused(fd, keep));
  close(keep);
}

TEST_F(OhosExternalTextureGLTest, WaitGPUFenceClosesFdWhenProcMissing) {
  OHOSExternalTextureGL texture(16, OH_OnFrameAvailableListener{});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  OHOSExternalTextureGL::eglCreateSyncKHR_ = nullptr;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = nullptr;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = nullptr;

  ScopedUnsignaledFence fence;
  ASSERT_GT(fence.get(), 0);
  int fd = fence.get();
  int keep = dup(fd);
  ASSERT_GE(keep, 0);
  texture.WaitGPUFence(fd);
  EXPECT_TRUE(FdReleasedOrReused(fd, keep));
  close(keep);
}

TEST_F(OhosExternalTextureGLTest, WaitGPUFenceClosesFdWhenSyncCreateFails) {
  OHOSExternalTextureGL texture(17, OH_OnFrameAvailableListener{});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  OHOSExternalTextureGL::eglCreateSyncKHR_ = FakeCreateSyncNoSync;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = FakeWaitSync;

  ScopedUnsignaledFence fence;
  ASSERT_GT(fence.get(), 0);
  int fd = fence.get();
  int keep = dup(fd);
  ASSERT_GE(keep, 0);
  texture.WaitGPUFence(fd);
  EXPECT_TRUE(FdReleasedOrReused(fd, keep));
  close(keep);
}

TEST_F(OhosExternalTextureGLTest, WaitGPUFenceStoresSyncAndWaits) {
  OHOSExternalTextureGL texture(18, OH_OnFrameAvailableListener{});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  OHOSExternalTextureGL::eglCreateSyncKHR_ = FakeCreateSyncValid;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = FakeWaitSync;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = FakeDestroySync;

  texture.now_key_ = 7;
  ScopedUnsignaledFence fence;
  ASSERT_GT(fence.get(), 0);
  const int fence_fd = fence.get();
  g_fake_create_sync_native_fd = -1;
  texture.WaitGPUFence(fence_fd);
  EXPECT_EQ(texture.gl_resources_.size(), 1u);
  EXPECT_EQ(texture.gl_resources_[texture.now_key_].wait_sync.get(),
            reinterpret_cast<EGLSyncKHR>(0x2));
  EXPECT_EQ(g_fake_create_sync_native_fd, fence_fd);
  texture.GPUResourceDestroy();
  EXPECT_EQ(texture.gl_resources_.size(), 0u);
}

TEST_F(OhosExternalTextureGLTest, GPUResourceDestroyClearsResources) {
  OHOSExternalTextureGL texture(19, OH_OnFrameAvailableListener{});
  OHOSExternalTextureGL::eglDestroySyncKHR_ = FakeDestroySync;
  texture.gl_resources_[1] = GlResource{};
  texture.gl_resources_[2] = GlResource{};
  ASSERT_EQ(texture.gl_resources_.size(), 2u);
  EXPECT_NO_FATAL_FAILURE(texture.GPUResourceDestroy());
  EXPECT_EQ(texture.gl_resources_.size(), 0u);
}

TEST_F(OhosExternalTextureGLTest, DeleteBufferGPUResourceZeroKeyIsNoop) {
  OHOSExternalTextureGL texture(20, OH_OnFrameAvailableListener{});
  texture.gl_resources_[5] = GlResource{};
  texture.DeleteBufferGPUResource(0);
  EXPECT_EQ(texture.gl_resources_.size(), 1u);
  texture.DeleteBufferGPUResource(5);
  EXPECT_EQ(texture.gl_resources_.size(), 0u);
}

TEST_F(OhosExternalTextureGLTest, CreateEGLImageFailurePaths) {
  OHOSExternalTextureGL texture(21, OH_OnFrameAvailableListener{});

  EXPECT_FALSE(texture.CreateEGLImage(nullptr).is_valid());

  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  EXPECT_FALSE(texture.CreateEGLImage(nullptr).is_valid());

  OHOSExternalTextureGL::eglCreateImageKHR_ = nullptr;
  auto* fake_buffer = reinterpret_cast<OHNativeWindowBuffer*>(0x10);
  EXPECT_FALSE(texture.CreateEGLImage(fake_buffer).is_valid());

  OHOSExternalTextureGL::eglCreateImageKHR_ = FakeCreateImageNoImage;
  auto failed = texture.CreateEGLImage(fake_buffer);
  EXPECT_TRUE(failed.is_valid());
  EXPECT_EQ(failed.get().image, EGL_NO_IMAGE_KHR);

  OHOSExternalTextureGL::eglCreateImageKHR_ = FakeCreateImageValid;
  OHOSExternalTextureGL::eglDestroyImageKHR_ = FakeDestroyImage;
  auto created = texture.CreateEGLImage(fake_buffer);
  EXPECT_TRUE(created.is_valid());
  EXPECT_EQ(created.get().image, reinterpret_cast<EGLImageKHR>(0x3));
  created = OHOSUniqueEGLImageKHR();
}

TEST_F(OhosExternalTextureGLTest, SetGPUFenceFailsWhenBufferConvertFails) {
  OHOSExternalTextureGL texture(22, OH_OnFrameAvailableListener{});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  GraphicStubKnobGuard guard;
  UpdateFromNativeWindowBufferFail(1);
  int fd = -1;
  texture.SetGPUFence(reinterpret_cast<OHNativeWindowBuffer*>(0x10), &fd);
  EXPECT_EQ(fd, -1);
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  texture.SetGPUFence(reinterpret_cast<OHNativeWindowBuffer*>(0x10), &fd);
  EXPECT_EQ(fd, -1);
}

TEST_F(OhosExternalTextureGLTest, SetGPUFenceLogsWhenSyncProcsMissing) {
  OHOSExternalTextureGL texture(23, OH_OnFrameAvailableListener{});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  GraphicStubKnobGuard guard;
  OHOSExternalTextureGL::eglCreateSyncKHR_ = nullptr;
  OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = nullptr;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = nullptr;
  int fd = -1;
  texture.SetGPUFence(reinterpret_cast<OHNativeWindowBuffer*>(0x10), &fd);
  EXPECT_EQ(fd, -1);
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  texture.SetGPUFence(reinterpret_cast<OHNativeWindowBuffer*>(0x10), &fd);
  EXPECT_EQ(fd, -1);
}

TEST_F(OhosExternalTextureGLTest, TraitsFreeNullAndValidDestroyProcs) {
  impeller::EGLImageKHRWithDisplay image{reinterpret_cast<EGLImageKHR>(0x3),
                                         reinterpret_cast<EGLDisplay>(0x1)};
  OHOSExternalTextureGL::eglDestroyImageKHR_ = nullptr;
  EXPECT_NO_FATAL_FAILURE(OHOSEGLImageKHRWithDisplayTraits::Free(image));
  OHOSExternalTextureGL::eglDestroyImageKHR_ = FakeDestroyImage;
  EXPECT_NO_FATAL_FAILURE(OHOSEGLImageKHRWithDisplayTraits::Free(image));

  OHOSExternalTextureGL::eglDestroySyncKHR_ = nullptr;
  EXPECT_NO_FATAL_FAILURE(
      EGLSyncKHRTraits::Free(reinterpret_cast<EGLSyncKHR>(0x2)));
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  OHOSExternalTextureGL::eglDestroySyncKHR_ = FakeDestroySync;
  EXPECT_NO_FATAL_FAILURE(
      EGLSyncKHRTraits::Free(reinterpret_cast<EGLSyncKHR>(0x2)));
}

TEST_F(OhosExternalTextureGLTest, CreateEGLImageOnFailures) {
  OHOSExternalTextureGL texture(25, OH_OnFrameAvailableListener{});
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  OHOSExternalTextureGL::eglCreateImageKHR_ = nullptr;
  EXPECT_FALSE(
      texture.CreateEGLImage(reinterpret_cast<OHNativeWindowBuffer*>(0x10))
          .is_valid());
  OHOSExternalTextureGL::eglCreateImageKHR_ = FakeCreateImageValid;
  OHOSExternalTextureGL::eglDestroyImageKHR_ = FakeDestroyImage;
  auto created =
      texture.CreateEGLImage(reinterpret_cast<OHNativeWindowBuffer*>(0x10));
  EXPECT_TRUE(created.is_valid());
  created = OHOSUniqueEGLImageKHR();
}

TEST_F(OhosExternalTextureGLTest, EglGetErrorLogsDoNotCrash) {
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  OHOSExternalTextureGL texture(26, OH_OnFrameAvailableListener{});
  g_current_display_override = reinterpret_cast<EGLDisplay>(0x1);
  GraphicStubKnobGuard guard;
  UpdateFromNativeWindowBufferFail(0);
  OHOSExternalTextureGL::eglCreateSyncKHR_ = FakeCreateSyncValid;
  OHOSExternalTextureGL::eglDupNativeFenceFDANDROID_ = nullptr;
  OHOSExternalTextureGL::eglDestroySyncKHR_ = FakeDestroySync;
  OHOSExternalTextureGL::eglWaitSyncKHR_ = FakeWaitSync;
  OHOSExternalTextureGL::eglCreateImageKHR_ = FakeCreateImageValid;
  OHOSExternalTextureGL::eglDestroyImageKHR_ = FakeDestroyImage;
  int fd = -1;
  texture.SetGPUFence(reinterpret_cast<OHNativeWindowBuffer*>(0x10), &fd);

  ScopedUnsignaledFence fence;
  ASSERT_GT(fence.get(), 0);
  texture.WaitGPUFence(fence.get());
  auto created =
      texture.CreateEGLImage(reinterpret_cast<OHNativeWindowBuffer*>(0x10));
  EXPECT_TRUE(created.is_valid());
  created = OHOSUniqueEGLImageKHR();
}

}  // namespace testing
}  // namespace flutter
