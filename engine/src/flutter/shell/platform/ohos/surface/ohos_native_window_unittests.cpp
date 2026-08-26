/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include <gtest/gtest.h>
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/shell/platform/ohos/test_stubs/ace_graphic_ndk_stub.h"

namespace flutter {
namespace testing {

namespace {
OHOSNativeWindow::Handle kFakeHandle =
    reinterpret_cast<OHOSNativeWindow::Handle>(0x1000);
}

TEST(OHOSNativeWindow, ValidWindowExposesHandleAndIsValid) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeHandle);
  EXPECT_TRUE(window->IsValid());
  EXPECT_EQ(window->Gethandle(), kFakeHandle);
  EXPECT_EQ(window->handle(), kFakeHandle);
}

TEST(OHOSNativeWindow, NullWindowIsInvalid) {
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(nullptr);
  EXPECT_FALSE(window->IsValid());
  DlISize size = window->GetSize();
  EXPECT_EQ(size.width, 0);
  EXPECT_EQ(size.height, 0);
  EXPECT_NO_FATAL_FAILURE(window->SetSize(1, 2));
}

TEST(OHOSNativeWindow, FakeWindowFlagRoundTrip) {
  auto fake = fml::MakeRefCounted<OHOSNativeWindow>(kFakeHandle, true);
  EXPECT_TRUE(fake->IsFakeWindow());
  auto real = fml::MakeRefCounted<OHOSNativeWindow>(kFakeHandle, false);
  EXPECT_FALSE(real->IsFakeWindow());
}

TEST(OHOSNativeWindow, GetSizeReturnsConfiguredGeometry) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 640;
  g_stub_geometry_height = 480;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeHandle);
  DlISize size = window->GetSize();
  EXPECT_EQ(size.width, 640);
  EXPECT_EQ(size.height, 480);
}

TEST(OHOSNativeWindow, GetSizeFailureReturnsZero) {
  GraphicStubKnobGuard guard;
  g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeHandle);
  DlISize size = window->GetSize();
  EXPECT_EQ(size.width, 0);
  EXPECT_EQ(size.height, 0);
}

TEST(OHOSNativeWindow, SetSizeCallsHandleOptSuccessAndFailure) {
  GraphicStubKnobGuard guard;
  g_stub_geometry_width = 640;
  g_stub_geometry_height = 480;
  auto window = fml::MakeRefCounted<OHOSNativeWindow>(kFakeHandle);
  EXPECT_NO_FATAL_FAILURE(window->SetSize(100, 200));
  g_stub_graphic_fail_mask = kStubFailWindowHandleOpt;
  EXPECT_NO_FATAL_FAILURE(window->SetSize(100, 200));
  g_stub_graphic_fail_mask = 0;
  DlISize size = window->GetSize();
  EXPECT_EQ(size.width, 640);
  EXPECT_EQ(size.height, 480);
}

}
}
