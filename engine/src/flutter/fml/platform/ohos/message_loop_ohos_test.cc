/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "flutter/fml/message_loop.h"

#include <iostream>
#include <thread>

#include "flutter/fml/build_config.h"
#include "flutter/fml/concurrent_message_loop.h"
#include "flutter/fml/synchronization/count_down_latch.h"
#include "flutter/fml/synchronization/waitable_event.h"
#include "flutter/fml/task_runner.h"
#include "flutter/fml/time/chrono_timestamp_provider.h"
#include "gtest/gtest.h"

namespace fml {
namespace message_loop_test {

#define PLATFORM_SPECIFIC_CAPTURE(...) [__VA_ARGS__]

/**
 *   breif:  注册异步任务
 *
 */
void MessageLoopTestPostTask(void) {
  bool started = false;
  bool terminated = false;
  std::thread thread([&started, &terminated]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    ASSERT_TRUE(loop.GetTaskRunner());
    loop.GetTaskRunner()->PostTask([&terminated]() {
      std::cout << "MessageLoopTestPostTask running ..." << std::endl;
      fml::MessageLoop::GetCurrent().Terminate();
      terminated = true;
    });
    loop.Run();
    started = true;
  });
  thread.join();
  ASSERT_TRUE(started);
  ASSERT_TRUE(terminated);
}

/**
 *   breif:
 * 进行观察者测试，注册25个任务后，添加观察者，在回调中增加观察者回调打印
 *
 */
void MessageLoopTestObserverFire(void) {
  bool started = false;
  bool terminated = false;
  std::thread thread([&started, &terminated]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    const size_t count = 25;
    auto& loop = fml::MessageLoop::GetCurrent();
    size_t task_count = 0;
    size_t obs_count = 0;
    auto obs = PLATFORM_SPECIFIC_CAPTURE(&obs_count)() {
      obs_count++;
      std::cout << "MessageLoopTestObserverFire obs_count" << obs_count
                << std::endl;
    };
    for (size_t i = 0; i < count; i++) {
      loop.GetTaskRunner()->PostTask(
          PLATFORM_SPECIFIC_CAPTURE(&terminated, i, &task_count)() {
            ASSERT_EQ(task_count, i);
            task_count++;
            if (count == i + 1) {
              fml::MessageLoop::GetCurrent().Terminate();
              terminated = true;
            }
          });
    }
    loop.AddTaskObserver(0, obs);
    loop.Run();
    ASSERT_EQ(task_count, count);
    ASSERT_EQ(obs_count, count);
    started = true;
  });
  thread.join();
  ASSERT_TRUE(started);
  ASSERT_TRUE(terminated);
}

}  // namespace message_loop_test
}  // namespace fml