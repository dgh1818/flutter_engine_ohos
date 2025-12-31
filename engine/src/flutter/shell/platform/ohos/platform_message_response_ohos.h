/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_MESSAGE_RESPONSE_OHOS_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_MESSAGE_RESPONSE_OHOS_H_

#include "flutter/fml/macros.h"
#include "flutter/fml/task_runner.h"
#include "flutter/lib/ui/window/platform_message_response.h"
#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

#include "flutter/shell/platform/ohos/napi/platform_view_ohos_napi.h"

namespace flutter {

class PlatformMessageResponseOHOS : public flutter::PlatformMessageResponse {
 public:
  // |flutter::PlatformMessageResponse|
  void Complete(std::unique_ptr<fml::Mapping> data) override;

  // |flutter::PlatformMessageResponse|
  void CompleteEmpty() override;

 private:
  PlatformMessageResponseOHOS(
      int response_id,
      std::shared_ptr<PlatformViewOHOSNapi> napi_facade,
      fml::RefPtr<fml::TaskRunner> platform_task_runner);

  ~PlatformMessageResponseOHOS() override;

  const int response_id_;
  const std::shared_ptr<PlatformViewOHOSNapi> napi_facade_;
  const fml::RefPtr<fml::TaskRunner> platform_task_runner_;

  FML_FRIEND_MAKE_REF_COUNTED(PlatformMessageResponseOHOS);
  FML_DISALLOW_COPY_AND_ASSIGN(PlatformMessageResponseOHOS);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_PLATFORM_MESSAGE_RESPONSE_OHOS_H_