/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/build_config.h"

#if defined(FML_OS_OHOS)

#include "flutter/common/settings.h"
#include "flutter/impeller/base/flags.h"
#include "flutter/shell/common/rasterizer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

namespace flutter {
namespace {

class MockRasterizerDelegate : public Rasterizer::Delegate {
 public:
  MOCK_METHOD(void,
              OnFrameRasterized,
              (const FrameTiming& frame_timing),
              (override));
  MOCK_METHOD(fml::Milliseconds, GetFrameBudget, (), (override));
  MOCK_METHOD(fml::TimePoint, GetLatestFrameTargetTime, (), (const, override));
  MOCK_METHOD(const TaskRunners&, GetTaskRunners, (), (const, override));
  MOCK_METHOD(const fml::RefPtr<fml::RasterThreadMerger>,
              GetParentRasterThreadMerger,
              (),
              (const, override));
  MOCK_METHOD(std::shared_ptr<const fml::SyncSwitch>,
              GetIsGpuDisabledSyncSwitch,
              (),
              (const, override));
  MOCK_METHOD(const Settings&, GetSettings, (), (const, override));
  MOCK_METHOD(bool,
              ShouldDiscardLayerTree,
              (int64_t, const flutter::LayerTree&),
              (override));
};

}  // namespace

namespace testing {

TEST(RasterizerFlagsOhosTest, SettingsEnableGlyphRasterParallelizationDefault) {
  NiceMock<MockRasterizerDelegate> delegate;
  Settings settings;
  EXPECT_FALSE(settings.enable_glyph_raster_parallelization);
  ON_CALL(delegate, GetSettings()).WillByDefault(ReturnRef(settings));
  auto rasterizer = std::make_unique<Rasterizer>(delegate);
  EXPECT_NE(rasterizer, nullptr);
}

TEST(RasterizerFlagsOhosTest, SettingsEnableGlyphRasterParallelizationTrue) {
  NiceMock<MockRasterizerDelegate> delegate;
  Settings settings;
  settings.enable_glyph_raster_parallelization = true;
  ON_CALL(delegate, GetSettings()).WillByDefault(ReturnRef(settings));
  auto rasterizer = std::make_unique<Rasterizer>(delegate);
  EXPECT_NE(rasterizer, nullptr);
  EXPECT_TRUE(delegate.GetSettings().enable_glyph_raster_parallelization);
}

TEST(RasterizerFlagsOhosTest,
     SettingsGlyphRasterParallelizationMapsToImpellerFlags) {
  Settings settings;
  settings.enable_glyph_raster_parallelization = true;
  impeller::Flags flags;
  flags.glyph_raster_parallelization =
      settings.enable_glyph_raster_parallelization;
  EXPECT_TRUE(flags.glyph_raster_parallelization);

  settings.enable_glyph_raster_parallelization = false;
  flags.glyph_raster_parallelization =
      settings.enable_glyph_raster_parallelization;
  EXPECT_FALSE(flags.glyph_raster_parallelization);
}

}  // namespace testing
}  // namespace flutter

#endif  // defined(FML_OS_OHOS)
