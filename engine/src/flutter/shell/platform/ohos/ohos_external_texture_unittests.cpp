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
#include "flutter/shell/platform/ohos/ohos_external_texture.h"
#undef private

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <fcntl.h>
#include <unistd.h>

#include "flutter/shell/platform/ohos/ohos_external_texture_gl.h"
#include "flutter/shell/platform/ohos/ohos_external_texture_vulkan.h"
#include "flutter/shell/platform/ohos/test_stubs/unittest_x64/libc_wrapper_stub.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {
namespace {

// Counting frame-available callback used to verify listener registration.
void CountingOnFrameAvailable(void* context) {
  if (context != nullptr) {
    ++(*static_cast<int*>(context));
  }
}

OH_OnFrameAvailableListener MakeListener(int* counter) {
  OH_OnFrameAvailableListener listener;
  listener.context = counter;
  listener.onFrameAvailable = &CountingOnFrameAvailable;
  return listener;
}

constexpr int64_t kTestTextureId = 1;

}  // namespace

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

  int listener_call_count_ = 0;
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
  OHOSExternalTexture::DefaultOnFrameAvailableWithLock(nullptr);
}

class OhosExternalTextureGLTest : public ::testing::Test {
 protected:
  void SetUp() override {
    texture_ = std::make_shared<OHOSExternalTextureGL>(
        kTestTextureId, MakeListener(&listener_call_count_));
  }

  int listener_call_count_ = 0;
  std::shared_ptr<OHOSExternalTextureGL> texture_;
};

TEST_F(OhosExternalTextureGLTest, InitEGLFunPtrResolvesSymbols) {
  // The constructor already ran InitEGLFunPtr(). On an OHOS device these
  // symbols are provided by libEGL.so (core EGL 1.5 or the extensions
  // this class relies on in production).
  EXPECT_NE(OHOSExternalTextureGL::eglCreateSyncKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglDestroySyncKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::eglCreateImageKHR_, nullptr);
  EXPECT_NE(OHOSExternalTextureGL::glEGLImageTargetTexture2DOES_, nullptr);
}

TEST_F(OhosExternalTextureGLTest, AcquiresAndReleasesBuffer) {
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

class OhosExternalTextureVulkanTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // A real impeller::ContextVK cannot be created inside
    // flutter_ohos_unittests (no Vulkan device/surface), so only the
    // paths that do not dereference impeller_context_ are exercised here.
    texture_ = std::make_shared<OHOSExternalTextureVulkan>(
        nullptr, kTestTextureId, MakeListener(&listener_call_count_));
  }

  int listener_call_count_ = 0;
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

}  // namespace testing
}  // namespace flutter
