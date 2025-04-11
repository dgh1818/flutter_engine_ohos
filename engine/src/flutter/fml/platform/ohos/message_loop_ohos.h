/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_FML_PLATFORM_OHOS_MESSAGE_LOOP_OHOS_H_
#define FLUTTER_FML_PLATFORM_OHOS_MESSAGE_LOOP_OHOS_H_

#include <uv.h>
#include "flutter/fml/macros.h"
#include "flutter/fml/message_loop_impl.h"
#include "flutter/fml/unique_fd.h"
#include <thread>
#include <atomic>

namespace fml {

class MessageLoopOhos : public MessageLoopImpl {
 private:
  uv_async_t async_handle_;
	uv_poll_t poll_handle_;
  uv_loop_t loop_;
	fml::UniqueFD epoll_fd_;
  fml::UniqueFD timer_fd_;
  std::atomic<bool> running_;
  std::thread timerhandle_thread_;
	bool is_platform_loop_;

  explicit MessageLoopOhos(void* platform_loop);

  ~MessageLoopOhos() override;

  // |fml::MessageLoopImpl|
  void Run() override;

  // |fml::MessageLoopImpl|
  void Terminate() override;

  // |fml::MessageLoopImpl|
  void WakeUp(fml::TimePoint time_point) override;

  void OnEventFired();

  void TimerFdWatcher();

  bool AddOrRemoveTimerSource(bool add);

  FML_FRIEND_MAKE_REF_COUNTED(MessageLoopOhos);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MessageLoopOhos);
  FML_DISALLOW_COPY_AND_ASSIGN(MessageLoopOhos);

 public:
  static void OnAsyncCallback(uv_async_t* handle);
  static void OnAsyncHandleClose(uv_handle_t* handle);
	static void OnPollCallback(uv_poll_t* handle, int status, int events);
};

}  // namespace fml

#endif  // FLUTTER_FML_PLATFORM_OHOS_MESSAGE_LOOP_OHOS_H_
