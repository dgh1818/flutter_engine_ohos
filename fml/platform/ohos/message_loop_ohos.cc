/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flutter/fml/platform/ohos/message_loop_ohos.h"
#include <sys/epoll.h>
#include "flutter/fml/eintr_wrapper.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/platform/linux/timerfd.h"

namespace fml {

static constexpr int kClockType = CLOCK_MONOTONIC;

void MessageLoopOhos::OnPollCallback(uv_poll_t* handle,
                                     int status,
                                     int events) {
  if (status < 0) {
    FML_DLOG(ERROR) << "Poll error:" << uv_strerror(status);
    return;
  }

  if (events & UV_READABLE) {
    reinterpret_cast<MessageLoopOhos*>(handle->data)->OnEventFired();
  }
}

MessageLoopOhos::MessageLoopOhos()
    : timer_fd_(::timerfd_create(kClockType, TFD_NONBLOCK | TFD_CLOEXEC)) {
  FML_CHECK(timer_fd_.is_valid());
  uv_loop_init(&loop_);
  uv_poll_init(&loop_, &poll_handle_, timer_fd_.get());
  poll_handle_.data = this;
  uv_poll_start(&poll_handle_, UV_READABLE, OnPollCallback);
}

MessageLoopOhos::~MessageLoopOhos() {
  if (uv_loop_alive(&loop_)) {
    uv_loop_close(&loop_);
  }
}

// |fml::MessageLoopImpl|
void MessageLoopOhos::Run() {
  uv_run(&loop_, UV_RUN_DEFAULT);
}

// |fml::MessageLoopImpl|
void MessageLoopOhos::Terminate() {
  WakeUp(fml::TimePoint::Now());
  uv_poll_stop(&poll_handle_);
  uv_stop(&loop_);
}

// |fml::MessageLoopImpl|
void MessageLoopOhos::WakeUp(fml::TimePoint time_point) {
  bool result = TimerRearm(timer_fd_.get(), time_point);
  (void)result;
  FML_DCHECK(result);
}

void MessageLoopOhos::OnEventFired() {
  if (TimerDrain(timer_fd_.get())) {
    RunExpiredTasksNow();
  }
}

}  // namespace fml
