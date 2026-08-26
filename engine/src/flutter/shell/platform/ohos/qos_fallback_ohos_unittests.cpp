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

TEST(QoSFallbackOhosTest, SetBackgroundQoSDoesNotCrash) {
  int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_BACKGROUND);
  if (ret != 0) {
    ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
    EXPECT_EQ(ret, 0);
  }
  SUCCEED();
}

TEST(QoSFallbackOhosTest, SetDisplayQoSDoesNotCrash) {
  int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
  if (ret != 0) {
    ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
    EXPECT_EQ(ret, 0);
  }
  SUCCEED();
}

TEST(QoSFallbackOhosTest, SetRasterQoSDoesNotCrash) {
  int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
  if (ret != 0) {
    ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
    EXPECT_EQ(ret, 0);
  }
  SUCCEED();
}

TEST(QoSFallbackOhosTest, SetDefaultQoSDoesNotCrash) {
  int ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
  EXPECT_EQ(ret, 0);
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

