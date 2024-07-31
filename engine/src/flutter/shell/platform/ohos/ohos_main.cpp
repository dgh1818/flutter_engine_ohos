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

#include "flutter/shell/platform/ohos/ohos_main.h"

#include "common/settings.h"
#include "flutter/fml/command_line.h"
#include "flutter/fml/file.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/macros.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/native_library.h"
#include "flutter/fml/paths.h"
#include "flutter/fml/platform/ohos/napi_util.h"
#include "flutter/fml/platform/ohos/paths_ohos.h"
#include "flutter/fml/size.h"
#include "flutter/lib/ui/plugins/callback_cache.h"
#include "flutter/runtime/dart_vm.h"
#include "flutter/runtime/ptrace_check.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/switches.h"
#include "third_party/dart/runtime/include/dart_tools_api.h"
#include "third_party/skia/include/core/SkFontMgr.h"

namespace flutter {

std::vector<std::string> StringArrayToVector(napi_env env,
                                             napi_value arrayValue) {
  napi_status status;
  uint32_t arrayLength = 0;
  status = napi_get_array_length(env, arrayValue, &arrayLength);
  LOGD("StringArrayToVector get array length  %{pubilc}d", arrayLength);
  if (status != napi_ok) {
    LOGE("StringArrayToVector napi_get_array_length error ");
  }
  std::vector<std::string> stringArray;
  for (uint32_t i = 0; i < arrayLength; i++) {
    napi_value elementValue;
    status = napi_get_element(env, arrayValue, i, &elementValue);
    if (status != napi_ok) {
      LOGE("StringArrayToVector napi_get_element error");
    }
    size_t stringLength;
    status = napi_get_value_string_utf8(env, elementValue, nullptr, 0,
                                        &stringLength);
    if (status != napi_ok) {
      LOGE("StringArrayToVector napi_get_value_string_utf8 error");
    }

    std::string stringValue(stringLength, '\0');
    status = napi_get_value_string_utf8(env, elementValue, &stringValue[0],
                                        stringLength + 1, nullptr);
    if (status != napi_ok) {
      LOGE("StringArrayToVector napi_get_value_string_utf8 error");
    }
    stringArray.push_back(stringValue);
  }
  return stringArray;
}

OhosMain::OhosMain(const flutter::Settings& settings)
    : settings_(settings), observatory_uri_callback_() {}

OhosMain::~OhosMain() = default;

static std::unique_ptr<OhosMain> g_flutter_main;
static std::string productModel_;

OhosMain& OhosMain::Get() {
  FML_CHECK(g_flutter_main) << "ensureInitializationComplete must have already "
                               "been called.";
  return *g_flutter_main;
}

const flutter::Settings& OhosMain::GetSettings() const {
  return settings_;
}

/**
 * @brief
 * @note
 * @param  context: common.Context, args: Array<string>, bundlePath: string,
 * appStoragePath: string, engineCachesPath: string, initTimeMillis: number
 * @return napi_value
 */
napi_value OhosMain::Init(napi_env env, napi_callback_info info) {
  size_t argc = 7;
  napi_value param[7];
  std::string kernelPath, appStoragePath, engineCachesPath;
  int64_t initTimeMillis;
  std::string productModel;
  napi_get_cb_info(env, info, &argc, param, nullptr, nullptr);
  napi_get_value_int64(env, param[5], &initTimeMillis);
  fml::napi::GetString(env, param[2], kernelPath);
  fml::napi::GetString(env, param[3], appStoragePath);
  fml::napi::GetString(env, param[4], engineCachesPath);
  fml::napi::GetString(env, param[6], productModel);
  productModel_ = productModel;

  FML_DLOG(INFO) << "INIT kernelPath:" << kernelPath;
  FML_DLOG(INFO) << "appStoragePath:" << appStoragePath;
  FML_DLOG(INFO) << "engineCachesPath:" << engineCachesPath;
  FML_DLOG(INFO) << "initTimeMillis:" << initTimeMillis;
  FML_DLOG(INFO) << "productModel:" << productModel;

  std::vector<std::string> args;
  args.push_back("flutter");
  fml::napi::GetArrayString(env, param[1], args);
  for (std::vector<std::string>::iterator it = args.begin(); it != args.end();
       ++it) {
    FML_DLOG(INFO) << *it;
  }
  auto command_line = fml::CommandLineFromIterators(args.begin(), args.end());
  auto settings = SettingsFromCommandLine(command_line);
  flutter::DartCallbackCache::SetCachePath(appStoragePath);
  fml::paths::InitializeOhosCachesPath(std::string(engineCachesPath));
  flutter::DartCallbackCache::LoadCacheFromDisk();

  if (!flutter::DartVM::IsRunningPrecompiledCode() && kernelPath[0]) {
    auto application_kernel_path = kernelPath;
    if (fml::IsFile(application_kernel_path)) {
      FML_DLOG(INFO) << "application_kernel_path exist";
      settings.application_kernel_asset = application_kernel_path;
    }
  }

  settings.task_observer_add = [](intptr_t key, const fml::closure& callback) {
    FML_DLOG(INFO) << "task_observer_add:" << (int64_t)key;
    fml::MessageLoop::GetCurrent().AddTaskObserver(key, callback);
  };
  settings.task_observer_remove = [](intptr_t key) {
    FML_DLOG(INFO) << "task_observer_remove:" << (int64_t)key;
    fml::MessageLoop::GetCurrent().RemoveTaskObserver(key);
  };
  settings.log_message_callback = [](const std::string& tag,
                                     const std::string& message) {
    LOGI("%{public}s %{public}s", tag.c_str(), message.c_str());
  };

  if (settings.enable_software_rendering) {
    FML_CHECK(!settings.enable_impeller)
        << "Impeller does not support software rendering. Either disable "
           "software rendering or disable impeller.";
    settings.ohos_rendering_api = OHOSRenderingAPI::kSoftware;
  }
  if (settings.enable_impeller) {
    settings.ohos_rendering_api = OHOSRenderingAPI::kImpellerVulkan;
  } else {
    settings.ohos_rendering_api = OHOSRenderingAPI::kOpenGLES;
  }
  switch (settings.ohos_rendering_api) {
    case OHOSRenderingAPI::kSoftware:
    case OHOSRenderingAPI::kOpenGLES:
      settings.enable_impeller = false;
      break;
    case OHOSRenderingAPI::kImpellerVulkan:
      settings.enable_impeller = true;
      break;
  }

  if (!EnableTracingIfNecessary(settings)) {
    LOGE(
      "Cannot create a FlutterEngine instance in debug mode without Flutter tooling.\n\n"
      "To Launch in debug mode, run 'flutter run' from Flutter tools, run from an IDE with a"
      "Flutter IDE plugin.\nAlternatively profile and release mode apps canbe launched from "
      "the home screen.");
    return nullptr;
  }

  g_flutter_main.reset(new OhosMain(settings));
  // TODO : g_flutter_main->SetupObservatoryUriCallback(env);
  LOGI("OhosMain::Init finished.");
  napi_value result;
  napi_create_int64(env, 0, &result);
  return result;
}

napi_value OhosMain::NativeInit(napi_env env, napi_callback_info info) {
  napi_value result = OhosMain::Init(env, info);
  OHOSImageGenerator::ImageNativeInit(env, info);
  return result;
}

bool OhosMain::IsEmulator()
{
  return productModel_ == "emulator";
}

}  // namespace flutter
