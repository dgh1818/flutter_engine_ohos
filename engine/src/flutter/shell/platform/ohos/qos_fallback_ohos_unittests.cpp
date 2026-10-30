/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <qos/qos.h>
#include "flutter/fml/thread.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

// OH_QoS_SetThreadQoS fails on system-managed threads (NAPI/JS) but works on
// app-created threads; run on a self-created thread like engine production
// code does (ohos_shell_holder.cpp, fence_waiter_vk.cc).
TEST(QoSFallbackOhosTest, SetBackgroundQoSDoesNotCrash) {
  fml::Thread thread("qos_test_bg");
  thread.GetTaskRunner()->PostTask([]() {
    int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_BACKGROUND);
    if (ret != 0) {
      ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
      EXPECT_EQ(ret, 0);
    }
  });
  thread.Join();
  SUCCEED();
}

// OH_QoS_SetThreadQoS fails on system-managed threads (NAPI/JS) but works on
// app-created threads; run on a self-created thread like engine production
// code does (ohos_shell_holder.cpp, fence_waiter_vk.cc).
TEST(QoSFallbackOhosTest, SetDisplayQoSDoesNotCrash) {
  fml::Thread thread("qos_test_display");
  thread.GetTaskRunner()->PostTask([]() {
    int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
    if (ret != 0) {
      ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
      EXPECT_EQ(ret, 0);
    }
  });
  thread.Join();
  SUCCEED();
}

TEST(QoSFallbackOhosTest, SetDefaultQoSDoesNotCrash) {
  fml::Thread thread("qos_test_default");
  thread.GetTaskRunner()->PostTask([]() {
    int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
    EXPECT_EQ(ret, 0);
  });
  thread.Join();
  SUCCEED();
}

TEST(QoSFallbackOhosTest, ThreadConfigSetterDoesNotCrash) {
  fml::Thread thread("qos_test_thread");
  thread.GetTaskRunner()->PostTask([]() {
    {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_BACKGROUND);
      if (ret != 0) {
        OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
      }
    }
    {
      int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
      if (ret != 0) {
        OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
      }
    }
  });
  thread.Join();
  SUCCEED();
}

}  // namespace testing
}  // namespace flutter
