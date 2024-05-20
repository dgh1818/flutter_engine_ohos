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
#include "flutter/fml/logging.h"
#include "flutter/fml/eintr_wrapper.h"
#include "flutter/fml/platform/linux/timerfd.h"

namespace fml {

static constexpr int kClockType = CLOCK_MONOTONIC;

void MessageLoopOhos::OnAsyncCallback(uv_async_t* handle) {
  reinterpret_cast<MessageLoopOhos*>(handle->data)->OnEventFired();
}

void MessageLoopOhos::OnAsyncHandleClose(uv_handle_t* handle) {}

MessageLoopOhos::MessageLoopOhos(void* platform_loop)
    : epoll_fd_(FML_HANDLE_EINTR(::epoll_create(1 /* unused */))),
      timer_fd_(::timerfd_create(kClockType, TFD_NONBLOCK | TFD_CLOEXEC)),
      running_(false) {
  FML_CHECK(epoll_fd_.is_valid());
  FML_CHECK(timer_fd_.is_valid());
	bool added_source = AddOrRemoveTimerSource(true);
	FML_CHECK(added_source);
  async_handle_.data = this;
  if (platform_loop != nullptr) {
    uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(platform_loop);
    uv_async_init(loop, &async_handle_, OnAsyncCallback);
  } else {
    uv_loop_init(&loop_);
    uv_async_init(&loop_, &async_handle_, OnAsyncCallback);
  }
	timerhandleThread = std::thread([this]() {
      running_ = true;
      TimerFdWatcher();
    });
}

MessageLoopOhos::~MessageLoopOhos() {
	if(timerhandleThread.joinable()) {
		timerhandleThread.join();
	}
  bool removed_source = AddOrRemoveTimerSource(false);
  FML_CHECK(removed_source);
	uv_close((uv_handle_t*)&async_handle_, OnAsyncHandleClose);
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
  running_ = false;
  WakeUp(fml::TimePoint::Now());
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

void MessageLoopOhos::TimerFdWatcher() {
  while (running_) {
    struct epoll_event event = {};

    int epoll_result = FML_HANDLE_EINTR(
        ::epoll_wait(epoll_fd_.get(), &event, 1, -1 /* timeout */));

    // Errors are fatal.
    if (event.events & (EPOLLERR | EPOLLHUP)) {
      running_ = false;
      continue;
    }

    // Timeouts are fatal since we specified an infinite timeout already.
    // Likewise, > 1 is not possible since we waited for one result.
    if (epoll_result != 1) {
      running_ = false;
      continue;
    }

    if (event.data.fd == timer_fd_.get()) {
      uv_async_send(&async_handle_);
    }
  }
}

bool MessageLoopOhos::AddOrRemoveTimerSource(bool add) {
  struct epoll_event event = {};

  event.events = EPOLLIN;
  // The data is just for informational purposes so we know when we were worken
  // by the FD.
  event.data.fd = timer_fd_.get();

  int ctl_result =
      ::epoll_ctl(epoll_fd_.get(), add ? EPOLL_CTL_ADD : EPOLL_CTL_DEL,
                  timer_fd_.get(), &event);
  return ctl_result == 0;
}

}  // namespace fml
