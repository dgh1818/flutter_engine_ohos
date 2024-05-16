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
#include "flutter/fml/logging.h"

namespace fml {

void MessageLoopOhos::OnAsyncCallback(uv_async_t* handle)
{
    reinterpret_cast<MessageLoopOhos*>(handle->data)->OnEventFired();
}

MessageLoopOhos::MessageLoopOhos(void* platform_loop)
{
    async_handle_.data = this;
    if (platform_loop != nullptr) {
        uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(platform_loop);
        uv_async_init(loop, &async_handle_, OnAsyncCallback);
    } else {
        uv_loop_init(&loop_);
        uv_async_init(&loop_, &async_handle_, OnAsyncCallback);
    }
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
    uv_async_init(&loop_, &async_handle_, nullptr);
}

// |fml::MessageLoopImpl|
void MessageLoopOhos::WakeUp(fml::TimePoint time_point) {
    uv_async_send(&async_handle_);
}

void MessageLoopOhos::OnEventFired() {
    RunExpiredTasksNow();
}

}  // namespace fml
