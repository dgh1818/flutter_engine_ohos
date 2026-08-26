/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/ohos_environment_gl.h"
#include <gtest/gtest.h>
#include "flutter/fml/memory/ref_counted.h"

namespace flutter {
namespace testing {

#if !defined(OHOS_X64_UNITTEST)
TEST(OhosEnvironmentGL, CreatesValidDisplay) {
  auto environment = fml::MakeRefCounted<OhosEnvironmentGL>();
  if (environment->Display() == EGL_NO_DISPLAY) {
    GTEST_SKIP() << "EGL display unavailable on emulator";
  }
  EXPECT_TRUE(environment->IsValid());
}
#endif  // !defined(OHOS_X64_UNITTEST)

#if !defined(OHOS_X64_UNITTEST)
TEST(OhosEnvironmentGL, DisplayHandleIsStableAcrossInstances) {
  EGLDisplay first_display = EGL_NO_DISPLAY;
  {
    auto environment = fml::MakeRefCounted<OhosEnvironmentGL>();
    first_display = environment->Display();
    if (first_display == EGL_NO_DISPLAY) {
      GTEST_SKIP() << "EGL display unavailable on emulator";
    }
  }
  auto second = fml::MakeRefCounted<OhosEnvironmentGL>();
  if (second->Display() == EGL_NO_DISPLAY) {
    GTEST_SKIP() << "EGL display unavailable on emulator";
  }
  EXPECT_EQ(second->Display(), first_display);
}
#endif  // !defined(OHOS_X64_UNITTEST)

}
}
