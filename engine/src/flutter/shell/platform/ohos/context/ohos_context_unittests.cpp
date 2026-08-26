/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/context/ohos_context.h"
#include <gtest/gtest.h>
#include <memory>
#include "flutter/impeller/renderer/context.h"
#include "third_party/skia/include/gpu/ganesh/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/mock/GrMockTypes.h"

namespace flutter {
namespace testing {

namespace {

using impeller::Allocator;
using impeller::Capabilities;
using impeller::CommandBuffer;
using impeller::CommandQueue;
using impeller::PipelineLibrary;
using impeller::RuntimeStageBackend;
using impeller::SamplerLibrary;
using impeller::ShaderLibrary;

class FakeImpellerContext final : public impeller::Context {
 public:
  FakeImpellerContext() : impeller::Context(impeller::Flags{}) {}

  BackendType GetBackendType() const override {
    return BackendType::kOpenGLES;
  }
  std::string DescribeGpuModel() const override { return "FakeImpeller"; }
  bool IsValid() const override { return true; }
  const std::shared_ptr<const Capabilities>& GetCapabilities()
      const override {
    return capabilities_;
  }
  std::shared_ptr<Allocator> GetResourceAllocator() const override {
    return nullptr;
  }
  std::shared_ptr<ShaderLibrary> GetShaderLibrary() const override {
    return nullptr;
  }
  std::shared_ptr<SamplerLibrary> GetSamplerLibrary() const override {
    return nullptr;
  }
  std::shared_ptr<PipelineLibrary> GetPipelineLibrary() const override {
    return nullptr;
  }
  std::shared_ptr<CommandBuffer> CreateCommandBuffer() const override {
    return nullptr;
  }
  std::shared_ptr<CommandQueue> GetCommandQueue() const override {
    return nullptr;
  }
  void Shutdown() override { ++shutdown_count_; }
  RuntimeStageBackend GetRuntimeStageBackend() const override {
    return RuntimeStageBackend::kVulkan;
  }

  int shutdown_count_ = 0;

 private:
  std::shared_ptr<const Capabilities> capabilities_;
};

class TestOHOSContext final : public OHOSContext {
 public:
  explicit TestOHOSContext(OHOSRenderingAPI rendering_api)
      : OHOSContext(rendering_api) {}
  using OHOSContext::SetImpellerContext;
};

}

TEST(OHOSContext, RenderingApiRoundTrip) {
  EXPECT_EQ(OHOSContext(OHOSRenderingAPI::kSoftware).RenderingApi(),
            OHOSRenderingAPI::kSoftware);
  EXPECT_EQ(OHOSContext(OHOSRenderingAPI::kOpenGLES).RenderingApi(),
            OHOSRenderingAPI::kOpenGLES);
  EXPECT_EQ(OHOSContext(OHOSRenderingAPI::kImpellerVulkan).RenderingApi(),
            OHOSRenderingAPI::kImpellerVulkan);
}

TEST(OHOSContext, IsValidAlwaysTrue) {
  OHOSContext context(OHOSRenderingAPI::kSoftware);
  EXPECT_TRUE(context.IsValid());
}

TEST(OHOSContext, MainSkiaContextDefaultsToNull) {
  TestOHOSContext context(OHOSRenderingAPI::kSoftware);
  EXPECT_EQ(context.GetMainSkiaContext(), nullptr);
  context.SetMainSkiaContext(nullptr);
  EXPECT_EQ(context.GetMainSkiaContext(), nullptr);
}

TEST(OHOSContext, ImpellerContextDefaultsToNull) {
  TestOHOSContext context(OHOSRenderingAPI::kSoftware);
  EXPECT_EQ(context.GetImpellerContext(), nullptr);
}

TEST(OHOSContext, ImpellerFlagsRoundTrip) {
  TestOHOSContext context(OHOSRenderingAPI::kSoftware);
  EXPECT_FALSE(context.GetImpellerFlags().antialiased_lines);
  impeller::Flags flags;
  flags.antialiased_lines = true;
  flags.use_sdfs = true;
  context.SetImpellerFlags(flags);
  EXPECT_TRUE(context.GetImpellerFlags().antialiased_lines);
  EXPECT_TRUE(context.GetImpellerFlags().use_sdfs);
  EXPECT_FALSE(context.GetImpellerFlags().glyph_raster_parallelization);
}

TEST(OHOSContext, DestructorWithoutContextsIsHarmless) {
  EXPECT_NO_FATAL_FAILURE({
    TestOHOSContext context(OHOSRenderingAPI::kSoftware);
    context.SetImpellerContext(nullptr);
  });
}

TEST(OHOSContext, DestructorShutsDownImpellerContext) {
  auto impeller_context = std::make_shared<FakeImpellerContext>();
  {
    TestOHOSContext context(OHOSRenderingAPI::kSoftware);
    context.SetImpellerContext(impeller_context);
    EXPECT_EQ(context.GetImpellerContext().get(), impeller_context.get());
    EXPECT_EQ(impeller_context->shutdown_count_, 0);
  }
  EXPECT_EQ(impeller_context->shutdown_count_, 1);
}

TEST(OHOSContext, DestructorReleasesMainSkiaContext) {
  GrMockOptions mock_options;
  auto gr_context = GrDirectContext::MakeMock(&mock_options);
  ASSERT_NE(gr_context, nullptr);
  {
    TestOHOSContext context(OHOSRenderingAPI::kOpenGLES);
    context.SetMainSkiaContext(gr_context);
    EXPECT_EQ(context.GetMainSkiaContext().get(), gr_context.get());
    EXPECT_FALSE(gr_context->abandoned());
  }
  EXPECT_TRUE(gr_context->abandoned());
}

}  // namespace testing
}  // namespace flutter
