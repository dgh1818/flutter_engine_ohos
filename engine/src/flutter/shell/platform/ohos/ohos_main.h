/*
 * Copyright 2013 The Flutter Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_OHOS_MAIN_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_OHOS_MAIN_H_
#define FML_USED_ON_EMBEDDER
#include "flutter/common/settings.h"
#include "flutter/fml/macros.h"
#include "flutter/runtime/dart_service_isolate.h"
#include "napi/native_api.h"
#include "napi_common.h"

namespace flutter {
class OhosMain {
 public:
  ~OhosMain();
  static OhosMain& Get();
  const flutter::Settings& GetSettings() const;
  static napi_value NativeInit(napi_env env, napi_callback_info info);
  static bool IsEmulator();

 private:
  const flutter::Settings settings_;
  DartServiceIsolate::CallbackHandle observatory_uri_callback_;

  explicit OhosMain(const flutter::Settings& settings);

  static napi_value Init(napi_env env, napi_callback_info info);

  void SetupObservatoryUriCallback(napi_env env, napi_callback_info info);
  FML_DISALLOW_COPY_AND_ASSIGN(OhosMain);
};
}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_OHOS_MAIN_H_
