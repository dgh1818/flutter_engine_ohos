/*
 * Copyright 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include <atomic>

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
  std::atomic<int> ret(-1);
  thread.GetTaskRunner()->PostTask([&ret]() {
    ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_BACKGROUND);
    if (ret != 0) {
      ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
    }
  });
  thread.Join();
  EXPECT_EQ(ret.load(), 0);
}

// OH_QoS_SetThreadQoS fails on system-managed threads (NAPI/JS) but works on
// app-created threads; run on a self-created thread like engine production
// code does (ohos_shell_holder.cpp, fence_waiter_vk.cc).
TEST(QoSFallbackOhosTest, SetDisplayQoSDoesNotCrash) {
  fml::Thread thread("qos_test_display");
  std::atomic<int> ret(-1);
  thread.GetTaskRunner()->PostTask([&ret]() {
    ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
    if (ret != 0) {
      ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
    }
  });
  thread.Join();
  EXPECT_EQ(ret.load(), 0);
}

TEST(QoSFallbackOhosTest, SetDefaultQoSDoesNotCrash) {
  fml::Thread thread("qos_test_default");
  std::atomic<int> ret(-1);
  thread.GetTaskRunner()->PostTask(
      [&ret]() { ret = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT); });
  thread.Join();
  EXPECT_EQ(ret.load(), 0);
}

TEST(QoSFallbackOhosTest, ThreadConfigSetterDoesNotCrash) {
  fml::Thread thread("qos_test_thread");
  std::atomic<int> bg(-1);
  std::atomic<int> ui(-1);
  thread.GetTaskRunner()->PostTask([&bg, &ui]() {
    bg = OH_QoS_SetThreadQoS(QoS_Level::QOS_BACKGROUND);
    if (bg != 0) {
      bg = OH_QoS_SetThreadQoS(QoS_Level::QOS_DEFAULT);
    }
    ui = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
    if (ui != 0) {
      ui = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INITIATED);
    }
  });
  thread.Join();
  EXPECT_EQ(bg.load(), 0);
  EXPECT_EQ(ui.load(), 0);
}

}  // namespace testing
}  // namespace flutter
