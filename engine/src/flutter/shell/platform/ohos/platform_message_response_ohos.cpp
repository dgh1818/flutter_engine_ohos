/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#include "flutter/shell/platform/ohos/platform_message_response_ohos.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {

PlatformMessageResponseOHOS::PlatformMessageResponseOHOS(
    int response_id,
    std::shared_ptr<PlatformViewOHOSNapi> napi_facade,
    fml::RefPtr<fml::TaskRunner> platform_task_runner)
    : response_id_(response_id),
      napi_facade_(std::move(napi_facade)),
      platform_task_runner_(std::move(platform_task_runner)) {}

PlatformMessageResponseOHOS::~PlatformMessageResponseOHOS() = default;

void PlatformMessageResponseOHOS::Complete(std::unique_ptr<fml::Mapping> data) {
  // async post

  platform_task_runner_->PostTask(
      fml::MakeCopyable([response_id = response_id_, data = std::move(data),
                         napi_facede = napi_facade_]() mutable {
        napi_facede->FlutterViewHandlePlatformMessageResponse(response_id,
                                                              std::move(data));
      }));
}

void PlatformMessageResponseOHOS::CompleteEmpty() {
  platform_task_runner_->PostTask(
      fml::MakeCopyable([response_id = response_id_,  //
                         napi_facede = napi_facade_   //
  ]() {
        // Make the response call into Java.
        napi_facede->FlutterViewHandlePlatformMessageResponse(response_id,
                                                              nullptr);
      }));
}

}  // namespace flutter