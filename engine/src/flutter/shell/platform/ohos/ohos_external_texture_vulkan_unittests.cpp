/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/ohos_external_texture_vulkan.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory>
#include <optional>

#include "flutter/fml/log_settings.h"
#include "flutter/impeller/core/formats.h"
#include "flutter/impeller/core/texture_descriptor.h"
#include "flutter/shell/platform/ohos/ohos_external_texture.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"
#include "flutter/shell/platform/ohos/test_stubs/libc_wrapper_stub.h"
#include "impeller/renderer/backend/vulkan/test/mock_vulkan.h"

namespace flutter {
namespace testing {

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

class TestVulkanTexture : public OHOSExternalTextureVulkan {
 public:
  using OHOSExternalTextureVulkan::OHOSExternalTextureVulkan;

  using OHOSExternalTextureVulkan::DeleteBufferGPUResource;
  using OHOSExternalTextureVulkan::GPUResourceDestroy;
  using OHOSExternalTextureVulkan::SetGPUFence;
  using OHOSExternalTextureVulkan::WaitGPUFence;

  std::unordered_map<NativeBufferKey, VkResource>& resources() {
    return vk_resources_;
  }

  void SetGPUFenceDirect(OHNativeWindowBuffer* buffer, int* fence_fd) {
    OHOSExternalTextureVulkan::SetGPUFence(buffer, fence_fd);
  }
};

class OhosExternalTextureVkUt : public ::testing::Test {
 protected:
  void SetUp() override {
    knob_guard_.emplace();
    context_ = impeller::testing::MockVulkanContextBuilder().Build();
    ASSERT_NE(context_, nullptr);
    texture_ = std::make_unique<TestVulkanTexture>(
        context_, 31, OH_OnFrameAvailableListener{});
  }

  void TearDown() override {
    texture_.reset();
    context_.reset();
    knob_guard_.reset();
  }

  static int MakeFenceFd(bool signalled) {
    return open("/dev/null", (signalled ? O_RDONLY : O_WRONLY) | O_CLOEXEC);
  }

  std::optional<GraphicStubKnobGuard> knob_guard_;
  std::shared_ptr<impeller::ContextVK> context_;
  std::unique_ptr<TestVulkanTexture> texture_;
};

TEST_F(OhosExternalTextureVkUt, ConstructStoresContext) {
  EXPECT_EQ(texture_->resources().size(), 0u);
}

TEST_F(OhosExternalTextureVkUt, SetGPUFenceRejectsNullFencePointer) {
  EXPECT_NO_FATAL_FAILURE(texture_->SetGPUFence(
      reinterpret_cast<OHNativeWindowBuffer*>(0x1), nullptr));
}

TEST_F(OhosExternalTextureVkUt, SetGPUFenceRejectsNullWindowBuffer) {
  int fd = 5;
  texture_->SetGPUFence(nullptr, &fd);
  EXPECT_EQ(fd, 5);
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  texture_->SetGPUFence(nullptr, &fd);
  EXPECT_EQ(fd, 5);
}

TEST_F(OhosExternalTextureVkUt, GPUResourceDestroyOnEmptyMap) {
  ASSERT_EQ(texture_->resources().size(), 0u);
  EXPECT_NO_FATAL_FAILURE(texture_->GPUResourceDestroy());
  EXPECT_EQ(texture_->resources().size(), 0u);
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  EXPECT_NO_FATAL_FAILURE(texture_->GPUResourceDestroy());
}

TEST_F(OhosExternalTextureVkUt, SetGPUFenceClosesOldFdAndFailsConvert) {
  UpdateFromNativeWindowBufferFail(1);
  int fd = MakeFenceFd(false);
  ASSERT_GT(fd, 0);
  texture_->SetGPUFenceDirect(reinterpret_cast<OHNativeWindowBuffer*>(0x10),
                              &fd);
  EXPECT_EQ(fd, -1);
  fml::ScopedSetLogSettings quiet({fml::kLogFatal});
  int again = -1;
  texture_->SetGPUFenceDirect(reinterpret_cast<OHNativeWindowBuffer*>(0x10),
                              &again);
  EXPECT_EQ(again, -1);
}

TEST_F(OhosExternalTextureVkUt, SetGPUFenceInvalidOldFdSkipsClose) {
  UpdateFromNativeWindowBufferFail(1);
  int fd = -1;
  texture_->SetGPUFence(reinterpret_cast<OHNativeWindowBuffer*>(0x10), &fd);
  EXPECT_EQ(fd, -1);
}

TEST_F(OhosExternalTextureVkUt, SetGPUFenceConvertOkWithoutTrackedTexture) {
  UpdateFromNativeWindowBufferFail(0);
  int fd = -1;
  texture_->SetGPUFence(reinterpret_cast<OHNativeWindowBuffer*>(0x10), &fd);
  EXPECT_EQ(fd, -1);
}

TEST_F(OhosExternalTextureVkUt, WaitGPUFenceInvalidFdNoSubmit) {
  EXPECT_NO_FATAL_FAILURE(texture_->WaitGPUFence(-1));
  EXPECT_NO_FATAL_FAILURE(texture_->WaitGPUFence(0));
}

TEST_F(OhosExternalTextureVkUt, WaitGPUFenceSignaledFdNoSubmit) {
  int fd = MakeFenceFd(true);
  ASSERT_GT(fd, 0);
  int keep = dup(fd);
  ASSERT_GE(keep, 0);
  EXPECT_NO_FATAL_FAILURE(texture_->WaitGPUFence(fd));
  bool closed = fcntl(fd, F_GETFD) == -1;
  if (!closed) {
    struct stat handed = {};
    struct stat kept = {};
    ASSERT_EQ(fstat(fd, &handed), 0);
    ASSERT_EQ(fstat(keep, &kept), 0);
    closed = handed.st_dev != kept.st_dev || handed.st_ino != kept.st_ino;
  }
  EXPECT_TRUE(closed);
  close(keep);
}

TEST_F(OhosExternalTextureVkUt, WaitGPUFenceUnsignaledTriesImportThenBails) {
  ScopedUnsignaledFence fence;
  ASSERT_GE(fence.get(), 0);
  int fd = fence.get();
  EXPECT_NO_FATAL_FAILURE(texture_->WaitGPUFence(fd));
}

TEST_F(OhosExternalTextureVkUt, DeleteBufferGPUResourceZeroAndUnknown) {
  texture_->resources()[7] = VkResource{};
  texture_->DeleteBufferGPUResource(0);
  EXPECT_EQ(texture_->resources().size(), 1u);
  texture_->DeleteBufferGPUResource(7);
  EXPECT_EQ(texture_->resources().size(), 0u);
  EXPECT_NO_FATAL_FAILURE(texture_->DeleteBufferGPUResource(99));
}

}  // namespace testing
}  // namespace flutter
