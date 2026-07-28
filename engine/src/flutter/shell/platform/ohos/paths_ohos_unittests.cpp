/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/platform/ohos/paths_ohos.h"

#include <gtest/gtest.h>

#include "flutter/fml/unique_fd.h"

namespace flutter {
namespace testing {

// GetExecutablePath always returns {false, ""} on OHos
TEST(PathsOhosTest, GetExecutablePathReturnsFalseAndEmpty) {
  auto [valid, path] = fml::paths::GetExecutablePath();
  EXPECT_FALSE(valid);
  EXPECT_TRUE(path.empty());
}

// After initializing caches path, GetCachesDirectory should return a valid FD
TEST(PathsOhosTest, InitializeCachesPathAndGetDirectory) {
  fml::paths::InitializeOhosCachesPath("/data/local/tmp");
  fml::UniqueFD fd = fml::paths::GetCachesDirectory();
  EXPECT_TRUE(fd.is_valid());
}

// When initialized with a nonexistent path, GetCachesDirectory should return an invalid FD
TEST(PathsOhosTest, GetCachesDirectoryInvalidForNonexistentPath) {
  fml::paths::InitializeOhosCachesPath("/nonexistent/path/xyz");
  fml::UniqueFD fd = fml::paths::GetCachesDirectory();
  EXPECT_FALSE(fd.is_valid());
}

}  // namespace testing
}  // namespace flutter
