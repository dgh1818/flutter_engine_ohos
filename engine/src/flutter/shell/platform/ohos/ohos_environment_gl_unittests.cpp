/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <gtest/gtest.h>
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/shell/platform/ohos/ohos_environment_gl.h"

namespace flutter {
namespace testing {

TEST(OhosEnvironmentGL, CreatesValidDisplay) {
  auto environment = fml::MakeRefCounted<OhosEnvironmentGL>();
  ASSERT_NE(environment->Display(), EGL_NO_DISPLAY);
  EXPECT_TRUE(environment->IsValid());
}

TEST(OhosEnvironmentGL, DisplayHandleIsStableAcrossInstances) {
  EGLDisplay first_display = EGL_NO_DISPLAY;
  {
    auto environment = fml::MakeRefCounted<OhosEnvironmentGL>();
    first_display = environment->Display();
    ASSERT_NE(first_display, EGL_NO_DISPLAY);
  }
  auto second = fml::MakeRefCounted<OhosEnvironmentGL>();
  ASSERT_NE(second->Display(), EGL_NO_DISPLAY);
  EXPECT_EQ(second->Display(), first_display);
}

}  // namespace testing
}  // namespace flutter
