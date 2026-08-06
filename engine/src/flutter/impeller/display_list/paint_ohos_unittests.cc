/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

// OHOS-only assertion for the FML_OS_OHOS branch in Paint::CreateContents
// (paint.cc:68): solid-color paints must propagate Paint::source_color_space
// into SolidColorContents via SetColorWithSpace. Registered only in
// flutter_ohos_unittests; the guard keeps this file an empty translation
// unit on non-OHOS builds.
#include "flutter/fml/build_config.h"

#include "impeller/display_list/paint.h"
#include "impeller/entity/contents/solid_color_contents.h"

#include "gtest/gtest.h"

#if defined(FML_OS_OHOS)

namespace impeller {
namespace testing {

TEST(PaintOhosTest, SolidColorContentsUsesSourceColorSpace) {
  Paint paint;
  paint.color = Color::Red();
  paint.source_color_space = ColorSpace::kDisplayP3;

  // With no color_source set, CreateContents produces SolidColorContents.
  std::shared_ptr<ColorSourceContents> contents = paint.CreateContents();
  ASSERT_TRUE(contents != nullptr);
  ASSERT_TRUE(contents->IsSolidColor());

  // -fno-rtti build: the concrete type is guaranteed by CreateContents when
  // color_source == nullptr, so static_cast is safe here.
  auto* solid = static_cast<SolidColorContents*>(contents.get());
  EXPECT_EQ(solid->GetColor(), Color::Red());
  EXPECT_EQ(solid->GetSourceColorSpace(), ColorSpace::kDisplayP3);
}

}  // namespace testing
}  // namespace impeller

#endif  // FML_OS_OHOS
