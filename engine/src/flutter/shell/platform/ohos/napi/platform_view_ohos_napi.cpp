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

#include "platform_view_ohos_napi.h"
#include <dlfcn.h>
#include <js_native_api.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <multimedia/image_framework/image_mdk.h>
#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <native_image/native_image.h>
#include <rawfile/raw_file.h>
#include <rawfile/raw_file_manager.h>
#include <string>

#include "flutter/common/constants.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/fml/platform/ohos/napi_util.h"
#include "flutter/lib/ui/plugins/callback_cache.h"
#include "flutter/shell/platform/ohos/accessibility/ohos_accessibility_features.h"
#include "flutter/shell/platform/ohos/ohos_logging.h"
#include "flutter/shell/platform/ohos/ohos_main.h"
#include "flutter/shell/platform/ohos/ohos_shell_holder.h"
#include "flutter/shell/platform/ohos/ohos_xcomponent_adapter.h"
#include "flutter/shell/platform/ohos/surface/ohos_native_window.h"
#include "flutter/shell/platform/ohos/types.h"
#include "unicode/uchar.h"

#define OHOS_SHELL_HOLDER (reinterpret_cast<OHOSShellHolder*>(shell_holder))
namespace flutter {

int64_t PlatformViewOHOSNapi::display_width = 0;
int64_t PlatformViewOHOSNapi::display_height = 0;
int64_t PlatformViewOHOSNapi::display_refresh_rate = 60;
double PlatformViewOHOSNapi::display_density_pixels = 1.0;

napi_env PlatformViewOHOSNapi::env_;
std::vector<std::string> PlatformViewOHOSNapi::system_languages;

/**
 * @brief send  empty PlatformMessage
 * @note
 * @param nativeShellHolderId: number,channel: string,responseId: number
 * @return void
 */
napi_value PlatformViewOHOSNapi::nativeDispatchEmptyPlatformMessage(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeDispatchEmptyPlatformMessage";
  napi_status ret;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int64_t shell_holder, responseId;
  std::string channel;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeDispatchEmptyPlatformMessage napi get shell_holder error");
    return nullptr;
  }
  fml::napi::GetString(env, args[1], channel);
  FML_DLOG(INFO) << "nativeDispatchEmptyPlatformMessage channel:" << channel;
  ret = napi_get_value_int64(env, args[2], &responseId);
  if (ret != napi_ok) {
    LOGE("nativeDispatchEmptyPlatformMessage napi get responseId error");
    return nullptr;
  }
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeDispatchEmptyPlatformMessage "
                    "DispatchEmptyPlatformMessage";
  OHOS_SHELL_HOLDER->GetPlatformView()->DispatchEmptyPlatformMessage(
      channel, responseId);
  return nullptr;
}

/**
 * @brief send PlatformMessage
 * @note
 * @param  nativeShellHolderId: number,channel: string,message:
 * ArrayBuffer,position: number,responseId: number
 * @return void
 */
napi_value PlatformViewOHOSNapi::nativeDispatchPlatformMessage(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeDispatchPlatformMessage";
  napi_status ret;
  napi_value thisArg;
  size_t argc = 5;
  napi_value args[5] = {nullptr};
  int64_t shell_holder, responseId, position;
  std::string channel;
  void* message = nullptr;
  size_t message_lenth = 0;

  int32_t status;

  ret = napi_get_cb_info(env, info, &argc, args, &thisArg, nullptr);
  if (argc < 5 || ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeDispatchPlatformMessage napi get argc ,argc="
                    << argc << "<5,error:" << ret;
    napi_throw_type_error(env, nullptr, "Wrong number of arguments");
    return nullptr;
  }
  napi_value napiShellHolder = args[0];
  napi_value napiChannel = args[1];
  napi_value napiMessage = args[2];
  napi_value napiPos = args[3];
  napi_value napiResponseId = args[4];

  ret = napi_get_value_int64(env, napiShellHolder, &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeDispatchPlatformMessage napi get shell_holder error");
    return nullptr;
  }
  FML_DLOG(INFO) << "nativeDispatchPlatformMessage:shell_holder:"
                 << shell_holder;

  if (0 != (status = fml::napi::GetString(env, napiChannel, channel))) {
    FML_DLOG(ERROR) << "nativeDispatchPlatformMessage napi get channel error:"
                    << status;
    return nullptr;
  }
  FML_DLOG(INFO) << "nativeDispatchEmptyPlatformMessage channel:" << channel;

  if (0 != (status = fml::napi::GetArrayBuffer(env, napiMessage, &message,
                                               &message_lenth))) {
    FML_DLOG(ERROR) << "nativeDispatchPlatformMessage napi get message error:"
                    << status;
    return nullptr;
  }
  if (message == nullptr) {
    FML_LOG(ERROR)
        << "nativeInvokePlatformMessageResponseCallback message null";
    return nullptr;
  }
  ret = napi_get_value_int64(env, napiPos, &position);
  if (ret != napi_ok) {
    LOGE("nativeDispatchPlatformMessage napi get position error");
    return nullptr;
  }
  ret = napi_get_value_int64(env, napiResponseId, &responseId);
  if (ret != napi_ok) {
    LOGE("nativeDispatchPlatformMessage napi get responseId error");
    return nullptr;
  }
  FML_DLOG(INFO) << "DispatchPlatformMessage,channel:" << channel
                 << ",message:" << message << ",position:" << position
                 << ",responseId:" << responseId;

  OHOS_SHELL_HOLDER->GetPlatformView()->DispatchPlatformMessage(
      channel, message, position, responseId);
  return nullptr;
}
/**
 * @brief
 * @note
 * @param  nativeShellHolderId: number,responseId: number
 * @return void
 */
napi_value
PlatformViewOHOSNapi::nativeInvokePlatformMessageEmptyResponseCallback(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "nativeInvokePlatformMessageEmptyResponseCallback";
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int64_t shell_holder, responseId;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE(
        "nativeInvokePlatformMessageEmptyResponseCallback napi get "
        "shell_holder error");
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[1], &responseId);
  if (ret != napi_ok) {
    LOGE(" napi get responseId error");
    return nullptr;
  }
  FML_DLOG(INFO) << "InvokePlatformMessageEmptyResponseCallback";
  OHOS_SHELL_HOLDER->GetPlatformMessageHandler()
      ->InvokePlatformMessageEmptyResponseCallback(responseId);
  return nullptr;
}
/**
 * @brief
 * @note
 * @param  nativeShellHolderId: number, responseId: number, message:
 * ArrayBuffer,position: number
 * @return void
 */
napi_value PlatformViewOHOSNapi::nativeInvokePlatformMessageResponseCallback(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "nativeInvokePlatformMessageResponseCallback";
  napi_status ret;
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  int64_t shell_holder, responseId, position;
  void* message = nullptr;
  size_t message_lenth = 0;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE(" napi get shell_holdererror");
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[1], &responseId);
  if (ret != napi_ok) {
    LOGE(" napi get responseId error");
    return nullptr;
  }
  int32_t result =
      fml::napi::GetArrayBuffer(env, args[2], &message, &message_lenth);
  if (result != 0) {
    FML_DLOG(ERROR)
        << "nativeInvokePlatformMessageResponseCallback GetArrayBuffer error "
        << result;
  }
  if (message == nullptr) {
    FML_LOG(ERROR)
        << "nativeInvokePlatformMessageResponseCallback message null";
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[3], &position);
  if (ret != napi_ok) {
    LOGE("nativeInvokePlatformMessageResponseCallback napi get position error");
    return nullptr;
  }

  uint8_t* response_data = static_cast<uint8_t*>(message);
  FML_DCHECK(response_data != nullptr);
  auto mapping = std::make_unique<fml::MallocMapping>(
      fml::MallocMapping::Copy(response_data, response_data + position));
  FML_DLOG(INFO) << "InvokePlatformMessageResponseCallback";
  OHOS_SHELL_HOLDER->GetPlatformMessageHandler()
      ->InvokePlatformMessageResponseCallback(responseId, std::move(mapping));
  return nullptr;
}

/* void PlatformViewOHOSNapi::FlutterViewHandlePlatformMessageResponse(
    int responseId,
    std::unique_ptr<fml::Mapping> data) {

}
 */
PlatformViewOHOSNapi::PlatformViewOHOSNapi(napi_env env) {}

void PlatformViewOHOSNapi::FlutterViewHandlePlatformMessageResponse(
    int reponse_id,
    std::unique_ptr<fml::Mapping> data) {
  FML_DLOG(INFO) << "FlutterViewHandlePlatformMessageResponse";
  napi_status status;
  napi_value callbackParam[2];
  status = napi_create_int64(env_, reponse_id, callbackParam);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "napi_create_int64 reponse_id fail";
  }

  if (data == nullptr) {
    callbackParam[1] = NULL;
  } else {
    callbackParam[1] = fml::napi::CreateArrayBuffer(
        env_, (void*)data->GetMapping(), data->GetSize());
  }

  status = fml::napi::InvokeJsMethod(
      env_, ref_napi_obj_, "handlePlatformMessageResponse", 2, callbackParam);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "InvokeJsMethod fail ";
  }
}

void PlatformViewOHOSNapi::FlutterViewHandlePlatformMessage(
    int reponse_id,
    std::unique_ptr<flutter::PlatformMessage> message) {
  FML_DLOG(INFO) << "FlutterViewHandlePlatformMessage message channal "
                 << message->channel().c_str();

  napi_value callbackParam[4];
  napi_status status;

  status = napi_create_string_utf8(env_, message->channel().c_str(),
                                   message->channel().size(), callbackParam);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "napi_create_string_utf8 err " << status;
    return;
  }

  callbackParam[1] = fml::napi::CreateArrayBuffer(
      env_, (void*)message->data().GetMapping(), message->data().GetSize());

  status = napi_create_int64(env_, reponse_id, &callbackParam[2]);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "napi_create_int64 err " << status;
    return;
  }
  if (message->hasData()) {
    fml::MallocMapping mapping = message->releaseData();
    char* mapData = (char*)mapping.Release();
    mapData[mapping.GetSize()] = '\0';
    status = napi_create_string_utf8(env_, mapData, strlen(mapData),
                                     &callbackParam[3]);
    if (status != napi_ok) {
      FML_DLOG(ERROR) << "napi_create_string_utf8 err " << status;
      return;
    }
    FML_DLOG(INFO) << "FlutterViewHandlePlatformMessage mapData= " << mapData;
    if (mapData) {
      delete mapData;
    }
  } else {
    callbackParam[3] = nullptr;
  }

  status = fml::napi::InvokeJsMethod(env_, ref_napi_obj_,
                                     "handlePlatformMessage", 4, callbackParam);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "InvokeJsMethod fail ";
  }
}

void PlatformViewOHOSNapi::FlutterViewOnFirstFrame(bool is_preload) {
  FML_DLOG(INFO) << "FlutterViewOnFirstFrame";
  napi_value callbackParam[1];
  napi_status status = napi_create_int64(env_, is_preload, callbackParam);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "napi_create_int64 firstframe fail ";
  }
  status = fml::napi::InvokeJsMethod(env_, ref_napi_obj_, "onFirstFrame", 1,
                                     callbackParam);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "InvokeJsMethod onFirstFrame fail ";
  }
}

void PlatformViewOHOSNapi::FlutterViewOnPreEngineRestart() {
  FML_DLOG(INFO) << "FlutterViewOnPreEngineRestart";
  napi_status status = fml::napi::InvokeJsMethod(
      env_, ref_napi_obj_, "onPreEngineRestart", 0, nullptr);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "InvokeJsMethod onPreEngineRestart fail ";
  }
}

std::vector<std::string> splitString(const std::string& input, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(input);
  std::string token;

  while (std::getline(ss, token, delimiter)) {
    result.push_back(token);
  }

  return result;
}

flutter::locale PlatformViewOHOSNapi::resolveNativeLocale(
    std::vector<flutter::locale> supportedLocales) {
  if (supportedLocales.empty()) {
    flutter::locale default_locale;
    default_locale.language = "zh";
    default_locale.script = "Hans";
    default_locale.region = "CN";
    return default_locale;
  }
  char delimiter = '-';
  if (PlatformViewOHOSNapi::system_languages.empty()) {
    PlatformViewOHOSNapi::system_languages.push_back("zh-Hans");
  }
  for (size_t i = 0; i < PlatformViewOHOSNapi::system_languages.size(); i++) {
    std::string language = PlatformViewOHOSNapi::system_languages
        [i];  // 格式language-script-region,例如en-Latn-US
    for (const locale& localeInfo : supportedLocales) {
      if (language == localeInfo.language + "-" + localeInfo.script + "-" +
                          localeInfo.region) {
        return localeInfo;
      }
      std::vector<std::string> element = splitString(language, delimiter);
      if (element[0] + "-" + element[1] ==
          localeInfo.language + "-" + localeInfo.region) {
        return localeInfo;
      }
      if (element[0] == localeInfo.language) {
        return localeInfo;
      }
    }
  }
  return supportedLocales[0];
}

std::unique_ptr<std::vector<std::string>>
PlatformViewOHOSNapi::FlutterViewComputePlatformResolvedLocales(
    const std::vector<std::string>& support_locale_data) {
  std::vector<flutter::locale> supportedLocales;
  std::vector<std::string> result;
  const int localeDataLength = 3;
  flutter::locale mlocale;
  for (size_t i = 0; i < support_locale_data.size(); i += localeDataLength) {
    mlocale.language = support_locale_data[i + kLanguageIndex];
    mlocale.region = support_locale_data[i + kRegionIndex];
    mlocale.script = support_locale_data[i + kScriptIndex];
    supportedLocales.push_back(mlocale);
  }
  mlocale = resolveNativeLocale(supportedLocales);
  result.push_back(mlocale.language);
  result.push_back(mlocale.region);
  result.push_back(mlocale.script);
  FML_DLOG(INFO) << "resolveNativeLocale result to flutter language: "
                 << result[kLanguageIndex]
                 << " region: " << result[kRegionIndex]
                 << " script: " << result[kScriptIndex];
  return std::make_unique<std::vector<std::string>>(std::move(result));
}

void PlatformViewOHOSNapi::FlutterViewOnTouchEvent(
    std::shared_ptr<std::string[]> touchPacketString,
    int size) {
  if (touchPacketString == nullptr) {
    FML_LOG(ERROR) << "Input parameter error";
    return;
  }
  napi_value arrayString;
  napi_create_array(env_, &arrayString);

  for (int i = 0; i < size; ++i) {
    napi_value stringItem;
    napi_create_string_utf8(env_, touchPacketString[i].c_str(), -1,
                            &stringItem);
    napi_set_element(env_, arrayString, i, stringItem);
  }

  napi_status status = fml::napi::InvokeJsMethod(
      env_, ref_napi_obj_, "onTouchEvent", 1, &arrayString);
  if (status != napi_ok) {
    FML_LOG(ERROR) << "InvokeJsMethod onTouchEvent fail";
  }
}

/**
 *   attach flutterNapi实例给到 native
 * engine，这个支持rkts到flutter平台的无关引擎之间的通信 attach只需要执行一次
 */
napi_value PlatformViewOHOSNapi::nativeAttach(napi_env env,
                                              napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeAttach";

  napi_status status;
  // 获取传入的参数
  size_t argc = 1;
  napi_value argv[1];
  status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "nativeAttach Failed to get napiObjec info";
  }

  std::shared_ptr<PlatformViewOHOSNapi> napi_facade =
      std::make_shared<PlatformViewOHOSNapi>(env);
  napi_create_reference(env, argv[0], 1, &(napi_facade->ref_napi_obj_));

  uv_loop_t* platform_loop = nullptr;
  status = napi_get_uv_event_loop(env, &platform_loop);
  if (status != napi_ok) {
    FML_DLOG(ERROR) << "nativeAttach napi_get_uv_event_loop  fail";
  }

  if (napi_facade == nullptr) {
    FML_DLOG(ERROR) << "napi_facade get nullptr";
  }

  auto shell_holder = std::make_unique<OHOSShellHolder>(
      OhosMain::Get().GetSettings(), napi_facade, platform_loop);
  if (shell_holder->IsValid()) {
    int64_t shell_holder_value = reinterpret_cast<int64_t>(shell_holder.get());
    FML_DLOG(INFO) << "PlatformViewOHOSNapi shell_holder:"
                   << shell_holder_value;
    napi_value id;
    napi_create_int64(env, reinterpret_cast<int64_t>(shell_holder.release()),
                      &id);
    return id;
  } else {
    FML_DLOG(ERROR) << "shell holder inValid";
    napi_value id;
    napi_create_int64(env, 0, &id);
    return id;
  }
}

/**
 *  加载dart工程构建产物
 */
napi_value PlatformViewOHOSNapi::nativeRunBundleAndSnapshotFromLibrary(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 6;
  napi_value args[6] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeRunBundleAndSnapshotFromLibrary napi_get_cb_info error");
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeRunBundleAndSnapshotFromLibrary napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeRunBundleAndSnapshotFromLibrary::shell_holder : %{public}ld",
       shell_holder);

  std::string bundlePath;
  if (fml::napi::kSuccess != fml::napi::GetString(env, args[1], bundlePath)) {
    LOGE(" napi_get_value_string_utf8 error");
    return nullptr;
  }
  LOGD("nativeRunBundleAndSnapshotFromLibrary: bundlePath: %{public}s",
       bundlePath.c_str());

  std::string entrypointFunctionName;
  if (fml::napi::kSuccess !=
      fml::napi::GetString(env, args[2], entrypointFunctionName)) {
    LOGE(" napi_get_value_string_utf8 error");
    return nullptr;
  }
  LOGD("entrypointFunctionName: %{public}s", entrypointFunctionName.c_str());

  std::string pathToEntrypointFunction;
  if (fml::napi::kSuccess !=
      fml::napi::GetString(env, args[3], pathToEntrypointFunction)) {
    LOGE(" napi_get_value_string_utf8 error");
    return nullptr;
  }
  LOGD(" pathToEntrypointFunction: %{public}s",
       pathToEntrypointFunction.c_str());

  NativeResourceManager* ResourceManager =
      OH_ResourceManager_InitNativeResourceManager(env, args[4]);

  std::vector<std::string> entrypointArgs;
  if (fml::napi::kSuccess !=
      fml::napi::GetArrayString(env, args[5], entrypointArgs)) {
    LOGE("nativeRunBundleAndSnapshotFromLibrary GetArrayString error");
    return nullptr;
  }

  auto ohos_asset_provider = std::make_unique<flutter::OHOSAssetProvider>(
      static_cast<void*>(ResourceManager));
  OHOS_SHELL_HOLDER->Launch(std::move(ohos_asset_provider),
                            entrypointFunctionName, pathToEntrypointFunction,
                            entrypointArgs);

  env_ = env;
  return nullptr;
}

/**
 *  设置ResourceManager和assetBundlePath到engine
 */
napi_value PlatformViewOHOSNapi::nativeUpdateOhosAssetManager(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeUpdateOhosAssetManager");

  return nullptr;
}

/**
 * 从engine获取当前绘制pixelMap
 */
napi_value PlatformViewOHOSNapi::nativeGetPixelMap(napi_env env,
                                                   napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeGetPixelMap");

  return nullptr;
}

/**
 * 从当前的flutterNapi复制一个新的实例
 */
napi_value PlatformViewOHOSNapi::nativeSpawn(napi_env env,
                                             napi_callback_info info) {
  napi_status ret;
  size_t argc = 6;
  napi_value args[6] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeSpawn napi_get_cb_info error");
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeSpawn napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSpawn::shell_holder : %{public}ld", shell_holder);

  std::string entrypoint;
  if (fml::napi::kSuccess != fml::napi::GetString(env, args[1], entrypoint)) {
    LOGE(" napi_get_value_string_utf8 error");
    return nullptr;
  }
  LOGD("entrypoint: %{public}s", entrypoint.c_str());

  std::string libraryUrl;
  if (fml::napi::kSuccess != fml::napi::GetString(env, args[2], libraryUrl)) {
    LOGE(" napi_get_value_string_utf8 error");
    return nullptr;
  }
  LOGD(" libraryUrl: %{public}s", libraryUrl.c_str());

  std::string initial_route;
  if (fml::napi::kSuccess !=
      fml::napi::GetString(env, args[3], initial_route)) {
    LOGE(" napi_get_value_string_utf8 error");
    return nullptr;
  }
  LOGD(" initialRoute: %{public}s", initial_route.c_str());

  std::vector<std::string> entrypoint_args;
  if (fml::napi::kSuccess !=
      fml::napi::GetArrayString(env, args[4], entrypoint_args)) {
    LOGE("nativeRunBundleAndSnapshotFromLibrary GetArrayString error");
    return nullptr;
  }

  std::shared_ptr<PlatformViewOHOSNapi> napi_facade =
      std::make_shared<PlatformViewOHOSNapi>(env);
  napi_create_reference(env, args[5], 1, &(napi_facade->ref_napi_obj_));

  auto spawned_shell_holder = OHOS_SHELL_HOLDER->Spawn(
      napi_facade, entrypoint, libraryUrl, initial_route, entrypoint_args);

  if (spawned_shell_holder == nullptr || !spawned_shell_holder->IsValid()) {
    FML_LOG(ERROR) << "Could not spawn Shell";
    return nullptr;
  }

  napi_value shell_holder_id;
  napi_create_int64(env,
                    reinterpret_cast<int64_t>(spawned_shell_holder.release()),
                    &shell_holder_id);
  return shell_holder_id;
}

static void LoadLoadingUnitFailure(intptr_t loading_unit_id,
                                   const std::string& message,
                                   bool transient) {
  // TODO(garyq): Implement
  LOGD("LoadLoadingUnitFailure: message  %s  transient %d", message.c_str(),
       transient);
}

/**
 * load一个合法的.so文件到dart vm
 */
napi_value PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeLoadDartDeferredLibrary");

  napi_status ret;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeLoadDartDeferredLibrary napi_get_cb_info error");
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeLoadDartDeferredLibrary napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeLoadDartDeferredLibrary::shell_holder : %{public}ld",
       shell_holder);

  int64_t loadingUnitId;
  ret = napi_get_value_int64(env, args[1], &loadingUnitId);
  if (ret != napi_ok) {
    LOGE("nativeLoadDartDeferredLibrary napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeLoadDartDeferredLibrary::loadingUnitId : %{public}ld",
       loadingUnitId);

  std::vector<std::string> search_paths;
  if (fml::napi::kSuccess !=
      fml::napi::GetArrayString(env, args[2], search_paths)) {
    LOGE("nativeLoadDartDeferredLibrary GetArrayString error");
    return nullptr;
  }

  LOGD("nativeLoadDartDeferredLibrary::search_paths");
  for (const std::string& path : search_paths) {
    LOGD("%{public}s", path.c_str());
  }

  intptr_t loading_unit_id = static_cast<intptr_t>(loadingUnitId);
  // Use dlopen here to directly check if handle is nullptr before creating a
  // NativeLibrary.
  void* handle = nullptr;
  while (handle == nullptr && !search_paths.empty()) {
    std::string path = search_paths.back();
    handle = ::dlopen(path.c_str(), RTLD_NOW);
    search_paths.pop_back();
  }
  if (handle == nullptr) {
    LoadLoadingUnitFailure(loading_unit_id,
                           "No lib .so found for provided search paths.", true);
    return nullptr;
  }
  fml::RefPtr<fml::NativeLibrary> native_lib =
      fml::NativeLibrary::CreateWithHandle(handle, false);

  // Resolve symbols.
  std::unique_ptr<const fml::SymbolMapping> data_mapping =
      std::make_unique<const fml::SymbolMapping>(
          native_lib, DartSnapshot::kIsolateDataSymbol);
  std::unique_ptr<const fml::SymbolMapping> instructions_mapping =
      std::make_unique<const fml::SymbolMapping>(
          native_lib, DartSnapshot::kIsolateInstructionsSymbol);

  OHOS_SHELL_HOLDER->GetPlatformView()->LoadDartDeferredLibrary(
      loading_unit_id, std::move(data_mapping),
      std::move(instructions_mapping));

  return nullptr;
}

/**
 *  把物理屏幕参数通知到native
 */
napi_value PlatformViewOHOSNapi::nativeSetViewportMetrics(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeSetViewportMetrics");

  napi_status ret;
  size_t argc = 20;
  napi_value args[20] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_cb_info error");
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::shell_holder : %{public}ld", shell_holder);

  double devicePixelRatio;
  ret = napi_get_value_double(env, args[1], &devicePixelRatio);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::devicePixelRatio : %{public}lf",
       devicePixelRatio);

  int64_t physicalWidth;
  ret = napi_get_value_int64(env, args[2], &physicalWidth);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalWidth : %{public}ld", physicalWidth);

  int64_t physicalHeight;
  ret = napi_get_value_int64(env, args[3], &physicalHeight);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalHeight : %{public}ld",
       physicalHeight);

  int64_t physicalPaddingTop;
  ret = napi_get_value_int64(env, args[4], &physicalPaddingTop);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalPaddingTop : %{public}ld",
       physicalPaddingTop);

  int64_t physicalPaddingRight;
  ret = napi_get_value_int64(env, args[5], &physicalPaddingRight);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalPaddingRight : %{public}ld",
       physicalPaddingRight);

  int64_t physicalPaddingBottom;
  ret = napi_get_value_int64(env, args[6], &physicalPaddingBottom);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalPaddingBottom : %{public}ld",
       physicalPaddingBottom);

  int64_t physicalPaddingLeft;
  ret = napi_get_value_int64(env, args[7], &physicalPaddingLeft);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalPaddingLeft : %{public}ld",
       physicalPaddingLeft);

  int64_t physicalViewInsetTop;
  ret = napi_get_value_int64(env, args[8], &physicalViewInsetTop);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalViewInsetTop : %{public}ld",
       physicalViewInsetTop);

  int64_t physicalViewInsetRight;
  ret = napi_get_value_int64(env, args[9], &physicalViewInsetRight);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalViewInsetRight : %{public}ld",
       physicalViewInsetRight);

  int64_t physicalViewInsetBottom;
  ret = napi_get_value_int64(env, args[10], &physicalViewInsetBottom);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalViewInsetBottom : %{public}ld",
       physicalViewInsetBottom);

  int64_t physicalViewInsetLeft;
  ret = napi_get_value_int64(env, args[11], &physicalViewInsetLeft);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalViewInsetLeft : %{public}ld",
       physicalViewInsetLeft);
  int64_t systemGestureInsetTop;
  ret = napi_get_value_int64(env, args[12], &systemGestureInsetTop);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::systemGestureInsetTop : %{public}ld",
       systemGestureInsetTop);
  int64_t systemGestureInsetRight;
  ret = napi_get_value_int64(env, args[13], &systemGestureInsetRight);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::systemGestureInsetRight : %{public}ld",
       systemGestureInsetRight);

  int64_t systemGestureInsetBottom;
  ret = napi_get_value_int64(env, args[14], &systemGestureInsetBottom);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::systemGestureInsetBottom : %{public}ld",
       systemGestureInsetBottom);
  int64_t systemGestureInsetLeft;
  ret = napi_get_value_int64(env, args[15], &systemGestureInsetLeft);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::systemGestureInsetLeft : %{public}ld",
       systemGestureInsetLeft);

  double physicalTouchSlop;
  ret = napi_get_value_double(env, args[16], &physicalTouchSlop);
  if (ret != napi_ok) {
    LOGE("nativeSetViewportMetrics napi_get_value_double error");
    return nullptr;
  }
  LOGD("nativeSetViewportMetrics::physicalTouchSlop : %{public}lf",
       physicalTouchSlop);

  std::vector<double> displayFeaturesBounds;
  napi_value array = args[17];
  uint32_t length;
  napi_get_array_length(env, array, &length);
  displayFeaturesBounds.resize(length);
  for (uint32_t i = 0; i < length; ++i) {
    napi_value element;
    napi_get_element(env, array, i, &element);
    napi_get_value_double(env, element, &(displayFeaturesBounds[i]));
  }

  LOGD("nativeSetViewportMetrics::displayFeaturesBounds");
  for (const uint64_t& bounds : displayFeaturesBounds) {
    LOGD(" %{public}ld", bounds);
  }

  std::vector<int64_t> displayFeaturesType;
  array = args[18];
  napi_get_array_length(env, array, &length);
  displayFeaturesType.resize(length);
  for (uint32_t i = 0; i < length; ++i) {
    napi_value element;
    napi_get_element(env, array, i, &element);
    napi_get_value_int64(env, element, &(displayFeaturesType[i]));
  }

  LOGD("nativeSetViewportMetrics::displayFeaturesType");
  for (const uint64_t& featuresType : displayFeaturesType) {
    LOGD(" %{public}ld", featuresType);
  }

  std::vector<int64_t> displayFeaturesState;
  array = args[19];
  napi_get_array_length(env, array, &length);
  displayFeaturesState.resize(length);
  for (uint32_t i = 0; i < length; ++i) {
    napi_value element;
    napi_get_element(env, array, i, &element);
    napi_get_value_int64(env, element, &(displayFeaturesState[i]));
  }

  LOGD("nativeSetViewportMetrics::displayFeaturesState");
  for (const uint64_t& featurestate : displayFeaturesState) {
    LOGD(" %{public}ld", featurestate);
  }

  flutter::ViewportMetrics metrics{
      static_cast<double>(devicePixelRatio),
      static_cast<double>(physicalWidth),
      static_cast<double>(physicalHeight),
      static_cast<double>(physicalPaddingTop),
      static_cast<double>(physicalPaddingRight),
      static_cast<double>(physicalPaddingBottom),
      static_cast<double>(physicalPaddingLeft),
      static_cast<double>(physicalViewInsetTop),
      static_cast<double>(physicalViewInsetRight),
      static_cast<double>(physicalViewInsetBottom),
      static_cast<double>(physicalViewInsetLeft),
      static_cast<double>(systemGestureInsetTop),
      static_cast<double>(systemGestureInsetRight),
      static_cast<double>(systemGestureInsetBottom),
      static_cast<double>(systemGestureInsetLeft),
      static_cast<double>(physicalTouchSlop),
      displayFeaturesBounds,
      std::vector<int>(displayFeaturesType.begin(), displayFeaturesType.end()),
      std::vector<int>(displayFeaturesState.begin(),
                       displayFeaturesState.end()),
      0,  // Display ID
  };

  OHOS_SHELL_HOLDER->GetPlatformView()->SetViewportMetrics(
      kFlutterImplicitViewId, metrics);

  return nullptr;
}

/**
 *  清除某个messageData
 */
napi_value PlatformViewOHOSNapi::nativeCleanupMessageData(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeCleanupMessageData");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeCleanupMessageData napi_get_cb_info error");
    return nullptr;
  }

  int64_t messageData;
  ret = napi_get_value_int64(env, args[0], &messageData);
  if (ret != napi_ok) {
    LOGE("nativeCleanupMessageData napi_get_value_int64 error");
    return nullptr;
  }

  LOGD("nativeCleanupMessageData  messageData: %{public}ld", messageData);
  free(reinterpret_cast<void*>(messageData));
  return nullptr;
}

/**
 *   设置刷新率
 */
napi_value PlatformViewOHOSNapi::nativeUpdateRefreshRate(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeUpdateRefreshRate");

  int64_t refreshRate;
  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeUpdateRefreshRate napi_get_cb_info error");
    return nullptr;
  }

  ret = napi_get_value_int64(env, args[0], &refreshRate);
  if (ret != napi_ok) {
    LOGE("nativeUpdateRefreshRate napi_get_value_int64 error");
    return nullptr;
  }

  LOGD("PlatformViewOHOSNapi::nativeUpdateRefreshRate: %{public}ld",
       refreshRate);
  FML_DCHECK(refreshRate > 0);
  display_refresh_rate = refreshRate;
  return nullptr;
}

/**
 *   设置屏幕尺寸
 */
napi_value PlatformViewOHOSNapi::nativeUpdateSize(napi_env env,
                                                  napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeUpdateSize");

  int64_t width;
  int64_t height;
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeUpdateSize napi_get_cb_info error");
    return nullptr;
  }

  ret = napi_get_value_int64(env, args[0], &width);
  if (ret != napi_ok) {
    LOGE("nativeUpdateSize napi_get_value_int64 error");
    return nullptr;
  }

  ret = napi_get_value_int64(env, args[1], &height);
  if (ret != napi_ok) {
    LOGE("nativeUpdateSize napi_get_value_int64 error");
    return nullptr;
  }

  LOGD("PlatformViewOHOSNapi::nativeUpdateSize: %{public}ld %{public}ld", width,
       height);
  FML_DCHECK(width > 0);
  FML_DCHECK(height > 0);
  display_width = width;
  display_height = height;
  return nullptr;
}

/**
 *   设置屏幕像素密度（也就是缩放系数）
 */
napi_value PlatformViewOHOSNapi::nativeUpdateDensity(napi_env env,
                                                     napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeUpdateDensity");

  double densityPixels;
  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeUpdateDensity napi_get_cb_info error");
    return nullptr;
  }

  ret = napi_get_value_double(env, args[0], &densityPixels);
  if (ret != napi_ok) {
    LOGE("nativeUpdateDensity napi_get_value_double error");
    return nullptr;
  }

  LOGD("PlatformViewOHOSNapi::nativeUpdateDensity: %{public}lf", densityPixels);
  FML_DCHECK(densityPixels > 0);
  display_density_pixels = densityPixels;
  return nullptr;
}

/**
 *  初始化SkFontMgr::RefDefault()，skia引擎文字管理初始化
 */
napi_value PlatformViewOHOSNapi::nativePrefetchDefaultFontManager(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativePrefetchDefaultFontManager");

  OHOSShellHolder::InitializeSystemFont();
  return nullptr;
}

/**
 *  hot reload font
 */
napi_value PlatformViewOHOSNapi::nativeCheckAndReloadFont(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeCheckAndReloadFont");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeCheckAndReloadFont napi_get_cb_info error");
    return nullptr;
  }
  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeCheckAndReloadFont napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeCheckAndReloadFont shell_holder: %{public}ld", shell_holder);
  OHOS_SHELL_HOLDER->ReloadSystemFonts();
  return nullptr;
}

/**
 *  返回是否支持软件绘制
 */
napi_value PlatformViewOHOSNapi::nativeGetIsSoftwareRenderingEnabled(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeGetIsSoftwareRenderingEnabled");
  napi_value result = nullptr;
  // TODO:  需要 FlutterMain 初始化
  napi_status ret = napi_get_boolean(
      env, OhosMain::Get().GetSettings().enable_software_rendering, &result);
  if (ret != napi_ok) {
    LOGE("nativeGetIsSoftwareRenderingEnabled napi_get_boolean error");
    return nullptr;
  }

  return result;
}

/**
 *  Detaches flutterNapi和engine之间的关联
 */
napi_value PlatformViewOHOSNapi::nativeDestroy(napi_env env,
                                               napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeDestroy");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeDestroy napi_get_cb_info error");
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeDestroy napi_get_value_int64 error");
    return nullptr;
  }

  LOGD("nativeDestroy shell_holder: %{public}ld", shell_holder);

  delete OHOS_SHELL_HOLDER;
  return nullptr;
}

/**
 *  设置能力参数
 */
napi_value PlatformViewOHOSNapi::nativeSetAccessibilityFeatures(
    napi_env env,
    napi_callback_info info) {
  LOGD("nativeSetAccessibilityFeatures");

  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeSetAccessibilityFeatures napi_get_cb_info error");
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeDestroy napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeSetAccessibilityFeatures shell_holder: %{public}ld",
       shell_holder);
  int64_t flags;
  ret = napi_get_value_int64(env, args[1], &flags);
  if (ret != napi_ok) {
    LOGE("nativeSetAccessibilityFeatures napi_get_value_int64 error");
    return nullptr;
  }
  LOGD(
      "PlatformViewOHOSNapi::nativeSetAccessibilityFeatures flags: %{public}ld",
      flags);
  OHOS_SHELL_HOLDER->GetPlatformView()->SetAccessibilityFeatures(flags);
  return nullptr;
}

/**
 * 加载动态库，或者dart库失败时的通知
 */
napi_value PlatformViewOHOSNapi::nativeDeferredComponentInstallFailure(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeDeferredComponentInstallFailure");

  napi_status ret;
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeDeferredComponentInstallFailure napi_get_cb_info error");
    return nullptr;
  }

  int64_t loadingUnitId;
  ret = napi_get_value_int64(env, args[0], &loadingUnitId);
  if (ret != napi_ok) {
    LOGE("nativeDeferredComponentInstallFailure napi_get_value_int64 error");
    return nullptr;
  }
  LOGD(
      "PlatformViewOHOSNapi::nativeSetAccessibilityFeatures loadingUnitId: "
      "%{public}ld",
      loadingUnitId);
  std::string error;
  if (fml::napi::kSuccess != fml::napi::GetString(env, args[1], error)) {
    LOGE(
        "nativeDeferredComponentInstallFailure napi_get_value_string_utf8 "
        "error");
    return nullptr;
  }
  LOGD("nativeSetAccessibilityFeatures loadingUnitId: %s", error.c_str());

  bool isTransient;
  ret = napi_get_value_bool(env, args[2], &isTransient);
  if (ret != napi_ok) {
    LOGE("nativeDeferredComponentInstallFailure napi_get_value_bool error");
    return nullptr;
  }
  LOGD("nativeSetAccessibilityFeatures loadingUnitId: %{public}d", isTransient);

  LoadLoadingUnitFailure(static_cast<intptr_t>(loadingUnitId),
                         std::string(error), static_cast<bool>(isTransient));

  return nullptr;
}

/**
 * 应用低内存警告
 */
napi_value PlatformViewOHOSNapi::nativeNotifyLowMemoryWarning(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeNotifyLowMemoryWarning");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeNotifyLowMemoryWarning napi_get_cb_info error");
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    LOGE("nativeNotifyLowMemoryWarning napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeNotifyLowMemoryWarning shell_holder: %{public}ld", shell_holder);
  OHOS_SHELL_HOLDER->NotifyLowMemoryWarning();

  return nullptr;
}

// 下面的方法，从键盘输入中判断当前字符是否是emoji

/**
 *
 */
napi_value PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmoji(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmoji");
  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmoji napi_get_cb_info error");
    return nullptr;
  }

  int64_t codePoint;
  ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmoji napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeFlutterTextUtilsIsEmoji codePoint: %{public}ld ", codePoint);

  bool value = u_hasBinaryProperty(codePoint, UProperty::UCHAR_EMOJI);
  napi_value result = nullptr;
  ret = napi_get_boolean(env, value, &result);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmoji napi_get_boolean error");
    return nullptr;
  }

  return result;
}

/**
 *
 */
napi_value PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifier(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifier");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmojiModifier napi_get_cb_info error");
    return nullptr;
  }

  int64_t codePoint;
  ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmojiModifier napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeFlutterTextUtilsIsEmojiModifier codePoint: %{public}ld ",
       codePoint);

  bool value = u_hasBinaryProperty(codePoint, UProperty::UCHAR_EMOJI_MODIFIER);
  napi_value result = nullptr;
  ret = napi_get_boolean(env, value, &result);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmojiModifier napi_get_boolean error");
    return nullptr;
  }

  return result;
}

/**
 *
 */
napi_value PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifierBase(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeFlutterTextUtilsIsEmojiModifierBase");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmojiModifierBase napi_get_cb_info error");
    return nullptr;
  }

  int64_t codePoint;
  ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    LOGE(
        "nativeFlutterTextUtilsIsEmojiModifierBase napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeFlutterTextUtilsIsEmojiModifierBase codePoint: %{public}ld ",
       codePoint);

  bool value =
      u_hasBinaryProperty(codePoint, UProperty::UCHAR_EMOJI_MODIFIER_BASE);
  napi_value result = nullptr;
  ret = napi_get_boolean(env, value, &result);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsEmojiModifierBase napi_get_boolean error");
    return nullptr;
  }

  return result;
}

/**
 *
 */
napi_value PlatformViewOHOSNapi::nativeFlutterTextUtilsIsVariationSelector(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeFlutterTextUtilsIsVariationSelector");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsVariationSelector napi_get_cb_info error");
    return nullptr;
  }

  int64_t codePoint;
  ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    LOGE(
        "nativeFlutterTextUtilsIsVariationSelector napi_get_value_int64 error");
    return nullptr;
  }
  LOGD("nativeFlutterTextUtilsIsVariationSelector codePoint: %{public}ld ",
       codePoint);

  bool value =
      u_hasBinaryProperty(codePoint, UProperty::UCHAR_VARIATION_SELECTOR);
  napi_value result = nullptr;
  ret = napi_get_boolean(env, value, &result);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsVariationSelector napi_get_boolean error");
    return nullptr;
  }

  return result;
}

/**
 *
 */
napi_value PlatformViewOHOSNapi::nativeFlutterTextUtilsIsRegionalIndicator(
    napi_env env,
    napi_callback_info info) {
  LOGD("PlatformViewOHOSNapi::nativeFlutterTextUtilsIsRegionalIndicator");

  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsRegionalIndicator napi_get_cb_info error");
    return nullptr;
  }

  int64_t codePoint;
  ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsRegionalIndicator napi_get_value_int64 fail");
    return nullptr;
  }
  LOGD("nativeFlutterTextUtilsIsRegionalIndicator codePoint: %{public}ld ",
       codePoint);

  bool value =
      u_hasBinaryProperty(codePoint, UProperty::UCHAR_REGIONAL_INDICATOR);
  napi_value result = nullptr;
  ret = napi_get_boolean(env, value, &result);
  if (ret != napi_ok) {
    LOGE("nativeFlutterTextUtilsIsRegionalIndicator napi_get_boolean error");
    return nullptr;
  }

  return result;
}

/**
 * @brief   ArkTS下发系统语言设置列表
 * @note
 * @param  nativeShellHolderId: number
 * @param  systemLanguages: Array<string>
 * @return void
 */
napi_value PlatformViewOHOSNapi::nativeGetSystemLanguages(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  int64_t shell_holder;
  std::vector<std::string> local_languages;
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeGetSystemLanguages napi_get_cb_info error:"
                    << ret;
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeGetSystemLanguages napi_get_value_int64 error";
    return nullptr;
  }
  if (fml::napi::kSuccess !=
      fml::napi::GetArrayString(env, args[1], local_languages)) {
    FML_DLOG(ERROR) << "nativeGetSystemLanguages GetArrayString error";
    return nullptr;
  }
  system_languages = local_languages;
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeRegisterTexture(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeRegisterTexture";
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  int64_t surfaceId =
      OHOS_SHELL_HOLDER->GetPlatformView()->RegisterExternalTexture(textureId);
  napi_value res;
  napi_create_int64(env, surfaceId, &res);
  return res;
}

napi_value PlatformViewOHOSNapi::nativeUnregisterTexture(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeUnregisterTexture";
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  OHOS_SHELL_HOLDER->GetPlatformView()->UnRegisterExternalTexture(textureId);
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeGetTextureWindowId(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeGetTextureWindowId";
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  uint64_t windowId =
      OHOS_SHELL_HOLDER->GetPlatformView()->GetExternalTextureWindowId(
          textureId);
  napi_value res;
  napi_create_int64(env, windowId, &res);
  return res;
}

napi_value PlatformViewOHOSNapi::nativeSetTextureBufferSize(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeSetTextureBufferSize";
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  int32_t width;
  int32_t height;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  NAPI_CALL(env, napi_get_value_int32(env, args[2], &width));
  NAPI_CALL(env, napi_get_value_int32(env, args[3], &height));
  OHOS_SHELL_HOLDER->GetPlatformView()->SetTextureBufferSize(textureId, width,
                                                             height);
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeSetExternalNativeImage(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeSetExternalNativeImage";
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  int64_t native_image_ptr;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  NAPI_CALL(env, napi_get_value_int64(env, args[2], &native_image_ptr));

  OH_NativeImage* native_image =
      (reinterpret_cast<OH_NativeImage*>(native_image_ptr));

  bool ret = OHOS_SHELL_HOLDER->GetPlatformView()->SetExternalNativeImage(
      textureId, native_image);
  napi_value res;
  napi_create_int64(env, (int64_t)ret, &res);
  return res;
}

napi_value PlatformViewOHOSNapi::nativeResetExternalTexture(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeResetExternalTexture";
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  bool need_surfaceId;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  NAPI_CALL(env, napi_get_value_bool(env, args[2], &need_surfaceId));

  uint64_t surface_id =
      OHOS_SHELL_HOLDER->GetPlatformView()->ResetExternalTexture(
          textureId, need_surfaceId);
  napi_value res;
  napi_create_int64(env, surface_id, &res);
  return res;
}

napi_value PlatformViewOHOSNapi::nativeMarkTextureFrameAvailable(
    napi_env env,
    napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  OHOS_SHELL_HOLDER->GetPlatformView()->MarkTextureFrameAvailable(textureId);
  return nullptr;
}

static OH_NativeBuffer* GetNativeBufferFromPixelMap(napi_env env,
                                                    napi_value pixel_map) {
  OH_PixelmapNative* pixelMap_native = nullptr;
  OH_NativeBuffer* native_buffer = nullptr;
  OH_PixelmapNative_ConvertPixelmapNativeFromNapi(env, pixel_map,
                                                  &pixelMap_native);
  if (pixelMap_native) {
    // Once a NativeBuffer is obtained, Reference is automatically called, so it
    // needs to be Unreferenced before it can be released.
    OH_PixelmapNative_GetNativeBuffer(pixelMap_native, &native_buffer);
    OH_PixelmapNative_Release(pixelMap_native);
  }
  return native_buffer;
}

napi_value PlatformViewOHOSNapi::nativeRegisterPixelMap(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeRegisterPixelMap";
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  NativePixelMap* nativePixelMap = OH_PixelMap_InitNativePixelMap(env, args[2]);
  OH_NativeBuffer* native_buffer = GetNativeBufferFromPixelMap(env, args[2]);

  OHOS_SHELL_HOLDER->GetPlatformView()->RegisterExternalTextureByPixelMap(
      textureId, nativePixelMap, native_buffer);
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeSetTextureBackGroundPixelMap(
    napi_env env,
    napi_callback_info info) {
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeSetTextureBackGroundPixelMap";
  size_t argc = 3;
  napi_value args[3] = {nullptr};
  int64_t shell_holder;
  int64_t textureId;
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
  NAPI_CALL(env, napi_get_value_int64(env, args[0], &shell_holder));
  NAPI_CALL(env, napi_get_value_int64(env, args[1], &textureId));
  NativePixelMap* nativePixelMap = OH_PixelMap_InitNativePixelMap(env, args[2]);
  OH_NativeBuffer* native_buffer = GetNativeBufferFromPixelMap(env, args[2]);

  OHOS_SHELL_HOLDER->GetPlatformView()->SetExternalTextureBackGroundPixelMap(
      textureId, nativePixelMap, native_buffer);
  return nullptr;
}

void PlatformViewOHOSNapi::SurfaceCreated(int64_t shell_holder,
                                          void* window,
                                          int width,
                                          int height) {
  auto native_window = fml::MakeRefCounted<OHOSNativeWindow>(
      static_cast<OHNativeWindow*>(window));
  OHOS_SHELL_HOLDER->GetPlatformView()->UpdateDisplaySize(width, height);
  OHOS_SHELL_HOLDER->GetPlatformView()->NotifyCreate(std::move(native_window));
}

void PlatformViewOHOSNapi::SurfacePreload(int64_t shell_holder,
                                          int width,
                                          int height) {
  OHOS_SHELL_HOLDER->GetPlatformView()->UpdateDisplaySize(width, height);
  OHOS_SHELL_HOLDER->GetPlatformView()->Preload(width, height);
}

void PlatformViewOHOSNapi::SurfaceChanged(int64_t shell_holder,
                                          void* window,
                                          int width,
                                          int height) {
  FML_LOG(INFO) << "impeller" << "SurfaceChanged:";
  auto native_window = fml::MakeRefCounted<OHOSNativeWindow>(
      static_cast<OHNativeWindow*>(window));
  OHOS_SHELL_HOLDER->GetPlatformView()->UpdateDisplaySize(width, height);
  OHOS_SHELL_HOLDER->GetPlatformView()->NotifySurfaceWindowChanged(
      native_window);
}

void PlatformViewOHOSNapi::SurfaceDestroyed(int64_t shell_holder) {
  OHOS_SHELL_HOLDER->GetPlatformView()->NotifyDestroyed();
}

void PlatformViewOHOSNapi::SetPlatformTaskRunner(
    fml::RefPtr<fml::TaskRunner> platform_task_runner) {
  platform_task_runner_ = platform_task_runner;
}

/**
 * @brief   xcomponent与flutter引擎绑定
 * @note
 * @param  nativeShellHolderId: number
 * @param  xcomponentId: number
 * @return void
 */
napi_value PlatformViewOHOSNapi::nativeXComponentAttachFlutterEngine(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  std::string xcomponent_id;
  int64_t shell_holder;
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    FML_DLOG(ERROR)
        << "nativeXComponentAttachFlutterEngine napi_get_cb_info error:" << ret;
    return nullptr;
  }

  if (fml::napi::GetString(env, args[0], xcomponent_id) != 0) {
    FML_DLOG(ERROR)
        << "nativeXComponentAttachFlutterEngine xcomponent_id GetString error";
    return nullptr;
  }

  ret = napi_get_value_int64(env, args[1], &shell_holder);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentAttachFlutterEngine shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }
  std::string shell_holder_str = std::to_string(shell_holder);

  LOGD(
      "nativeXComponentAttachFlutterEngine xcomponent_id: %{public}s, "
      "shell_holder: %{public}ld ",
      xcomponent_id.c_str(), shell_holder);

  XComponentAdapter::GetInstance()->AttachFlutterEngine(xcomponent_id,
                                                        shell_holder_str);
  return nullptr;
}

/**
 * @brief   提前绘制xcomponent的内容
 * @note
 * @param  nativeShellHolderId: number
 * @param  xcomponentId: number
 * @return void
 */
napi_value PlatformViewOHOSNapi::nativeXComponentPreDraw(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  std::string xcomponent_id;
  int64_t shell_holder;
  int width = 0;
  int height = 0;
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentPreDraw napi_get_cb_info error:" << ret;
    return nullptr;
  }

  if (fml::napi::GetString(env, args[0], xcomponent_id) != 0) {
    FML_DLOG(ERROR) << "nativeXComponentPreDraw xcomponent_id GetString error";
    return nullptr;
  }

  ret = napi_get_value_int64(env, args[1], &shell_holder);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentPreDraw shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }
  std::string shell_holder_str = std::to_string(shell_holder);

  ret = napi_get_value_int32(env, args[2], &width);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentPreDraw width "
                       "napi_get_value_int32 error";
    return nullptr;
  }

  ret = napi_get_value_int32(env, args[3], &height);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentPreDraw height "
                       "napi_get_value_int32 error";
    return nullptr;
  }

  LOGD(
      "nativeXComponentPreDraw xcomponent_id: %{public}s, "
      "shell_holder: %{public}ld ",
      xcomponent_id.c_str(), shell_holder);

  XComponentAdapter::GetInstance()->PreDraw(xcomponent_id, shell_holder_str,
                                            width, height);
  return nullptr;
}

/**
 * @brief xcomponent解除flutter引擎绑定
 * @note
 * @param  nativeShellHolderId: number
 * @param  xcomponentId: number
 * @return napi_value
 */
napi_value PlatformViewOHOSNapi::nativeXComponentDetachFlutterEngine(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  std::string xcomponent_id;
  int64_t shell_holder;
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    FML_DLOG(ERROR)
        << "nativeXComponentAttachFlutterEngine napi_get_cb_info error:" << ret;
    return nullptr;
  }
  if (fml::napi::GetString(env, args[0], xcomponent_id) != 0) {
    FML_DLOG(ERROR)
        << "nativeXComponentAttachFlutterEngine xcomponent_id GetString error";
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[1], &shell_holder);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentAttachFlutterEngine shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }

  LOGD("nativeXComponentDetachFlutterEngine xcomponent_id: %{public}s",
       xcomponent_id.c_str());
  XComponentAdapter::GetInstance()->DetachFlutterEngine(xcomponent_id);
  return nullptr;
}

/**
 * @brief flutterEngine get mouseWheel event from ets
 * @note
 * @param  nativeShellHolderId: number
 * @param  xcomponentId: number
 * @param  eventType: string
 * @param  fingerId: number
 * @param  globalX: number
 * @param  globalY: number
 * @param  offsetY: number
 * @param  timestamp: number
 * @return napi_value
 */
napi_value PlatformViewOHOSNapi::nativeXComponentDispatchMouseWheel(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 8;
  napi_value args[8] = {nullptr};
  int64_t shellHolder;
  std::string xcomponentId;
  std::string eventType;
  int64_t fingerId;
  double globalX;
  double globalY;
  double offsetY;
  int64_t timestamp;
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    FML_DLOG(ERROR)
        << "nativeXComponentDispatchMouseWheel napi_get_cb_info error:" << ret;
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[0], &shellHolder);
  if (ret != napi_ok) {
    LOGE(
        "nativeXComponentDispatchMouseWheel shellHolder napi_get_value_int64 "
        "error");
    return nullptr;
  }
  if (fml::napi::GetString(env, args[1], xcomponentId) != 0) {
    FML_DLOG(ERROR)
        << "nativeXComponentDispatchMouseWheel xcomponentId GetString error";
    return nullptr;
  }
  if (fml::napi::GetString(env, args[2], eventType) != 0) {
    FML_DLOG(ERROR)
        << "nativeXComponentDispatchMouseWheel eventType GetString error";
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[3], &fingerId);
  if (ret != napi_ok) {
    LOGE(
        "nativeXComponentDispatchMouseWheel fingerId napi_get_value_int64 "
        "error");
    return nullptr;
  }
  ret = napi_get_value_double(env, args[4], &globalX);
  if (ret != napi_ok) {
    LOGE(
        "nativeXComponentDispatchMouseWheel globalX napi_get_value_double "
        "error");
    return nullptr;
  }
  ret = napi_get_value_double(env, args[5], &globalY);
  if (ret != napi_ok) {
    LOGE(
        "nativeXComponentDispatchMouseWheel globalY napi_get_value_double "
        "error");
    return nullptr;
  }
  ret = napi_get_value_double(env, args[6], &offsetY);
  if (ret != napi_ok) {
    LOGE(
        "nativeXComponentDispatchMouseWheel offsetY napi_get_value_double "
        "error");
    return nullptr;
  }
  ret = napi_get_value_int64(env, args[7], &timestamp);
  if (ret != napi_ok) {
    LOGE(
        "nativeXComponentDispatchMouseWheel timestamp napi_get_value_int64 "
        "error");
    return nullptr;
  }
  flutter::mouseWheelEvent event{eventType, shellHolder, fingerId, globalX,
                                 globalY,   offsetY,     timestamp};
  XComponentAdapter::GetInstance()->OnMouseWheel(xcomponentId, event);
  return nullptr;
}

/**
 * @brief flutterEngine convert string to Uint8Array
 * @note
 * @param  str: string
 * @return napi_value
 */
napi_value PlatformViewOHOSNapi::nativeEncodeUtf8(napi_env env,
                                                  napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  size_t length = 0;
  napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);

  auto null_terminated_length = length + 1;
  auto char_array = std::make_unique<char[]>(null_terminated_length);
  napi_get_value_string_utf8(env, args[0], char_array.get(),
                             null_terminated_length, nullptr);

  void* data;
  napi_value arraybuffer;
  napi_create_arraybuffer(env, length, &data, &arraybuffer);
  std::memcpy(data, char_array.get(), length);

  napi_value uint8_array;
  napi_create_typedarray(env, napi_uint8_array, length, arraybuffer, 0,
                         &uint8_array);
  return uint8_array;
}

/**
 * @brief flutterEngine convert Uint8Array to string
 * @note
 * @param  array: Uint8Array
 * @return napi_value
 */
napi_value PlatformViewOHOSNapi::nativeDecodeUtf8(napi_env env,
                                                  napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  size_t size = 0;
  void* data = nullptr;
  napi_get_typedarray_info(env, args[0], nullptr, &size, &data, nullptr,
                           nullptr);

  napi_value result;
  napi_create_string_utf8(env, static_cast<char*>(data), size, &result);
  return result;
}

napi_value PlatformViewOHOSNapi::nativeUpdateSemantics(
    napi_env env,
    napi_callback_info info) {
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeUpdateCustomAccessibilityActions(
    napi_env env,
    napi_callback_info info) {
  return nullptr;
}

/**
 * 无障碍特征之字体加粗功能，获取ets侧系统字体粗细系数
 */
napi_value PlatformViewOHOSNapi::nativeSetFontWeightScale(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // get param nativeShellHolderId
  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeSetFontWeightScale "
                       "napi_get_value_int64 error:"
                    << ret;
    return nullptr;
  }
  // get param fontWeightScale
  double fontWeightScale = 1.0;
  ret = napi_get_value_double(env, args[1], &fontWeightScale);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeSetFontWeightScale "
                       "napi_get_value_double error:"
                    << ret;
    return nullptr;
  }
  auto accessibilityFeatures = std::make_shared<OhosAccessibilityFeatures>();
  accessibilityFeatures->SetBoldText(fontWeightScale, shell_holder);
  FML_DLOG(INFO)
      << "PlatformViewOHOSNapi::nativeSetFontWeightScale -> shell_holder: "
      << shell_holder << " fontWeightScale: " << fontWeightScale;
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeLookupCallbackInformation(
    napi_env env,
    napi_callback_info info) {
  napi_value result;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_status ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    LOGE("nativeLookupCallbackInformation napi_get_cb_info error");
    napi_create_int32(env, -1, &result);
    return result;
  }

  int64_t handle;
  bool lossless;
  ret = napi_get_value_bigint_int64(env, args[1], &handle, &lossless);
  if (ret != napi_ok) {
    LOGE("nativeLookupCallbackInformation napi_get_value_int64 error");
    napi_create_int32(env, -1, &result);
    return result;
  }

  LOGD("nativeLookupCallbackInformation::handle : %{public}ld", handle);
  auto cbInfo = flutter::DartCallbackCache::GetCallbackInformation(handle);
  if (cbInfo == nullptr) {
    LOGE(
        "nativeLookupCallbackInformation DartCallbackCache "
        "GetCallbackInformation nullptr");
    napi_create_int32(env, -1, &result);
    return result;
  }

  napi_ref callbck_napi_obj;
  ret = napi_create_reference(env, args[0], 1, &callbck_napi_obj);
  if (ret != napi_ok) {
    LOGE("nativeLookupCallbackInformation napi_create_reference error");
    napi_create_int32(env, -1, &result);
    return result;
  }

  napi_value callbackParam[3];
  napi_create_string_utf8(env, cbInfo->name.c_str(), NAPI_AUTO_LENGTH,
                          &callbackParam[0]);
  napi_create_string_utf8(env, cbInfo->class_name.c_str(), NAPI_AUTO_LENGTH,
                          &callbackParam[1]);
  napi_create_string_utf8(env, cbInfo->library_path.c_str(), NAPI_AUTO_LENGTH,
                          &callbackParam[2]);

  ret = fml::napi::InvokeJsMethod(env, callbck_napi_obj, "init", 3,
                                  callbackParam);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeLookupCallbackInformation init fail ";
    napi_create_int32(env, -1, &result);
    return result;
  }
  napi_delete_reference(env, callbck_napi_obj);
  napi_create_int32(env, 0, &result);
  return result;
}

napi_value PlatformViewOHOSNapi::nativeUnicodeIsEmoji(napi_env env,
                                                      napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  bool is_emoji = false;
  int64_t codePoint = 0;
  bool ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentAttachFlutterEngine shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }

  is_emoji = u_hasBinaryProperty(codePoint, UProperty::UCHAR_EMOJI);

  napi_value result;
  napi_create_int32(env, (int)is_emoji, &result);
  return result;
}

napi_value PlatformViewOHOSNapi::nativeUnicodeIsEmojiModifier(
    napi_env env,
    napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  bool is_emoji = false;
  int64_t codePoint = 0;
  bool ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentAttachFlutterEngine shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }

  is_emoji = u_hasBinaryProperty(codePoint, UProperty::UCHAR_EMOJI_MODIFIER);

  napi_value result;
  napi_create_int32(env, (int)is_emoji, &result);
  return result;
}

napi_value PlatformViewOHOSNapi::nativeUnicodeIsEmojiModifierBase(
    napi_env env,
    napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  bool is_emoji = false;
  int64_t codePoint = 0;
  bool ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentAttachFlutterEngine shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }

  is_emoji =
      u_hasBinaryProperty(codePoint, UProperty::UCHAR_EMOJI_MODIFIER_BASE);

  napi_value result;
  napi_create_int32(env, (int)is_emoji, &result);
  return result;
}

napi_value PlatformViewOHOSNapi::nativeUnicodeIsVariationSelector(
    napi_env env,
    napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  bool is_emoji = false;
  int64_t codePoint = 0;
  bool ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentAttachFlutterEngine shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }

  is_emoji =
      u_hasBinaryProperty(codePoint, UProperty::UCHAR_VARIATION_SELECTOR);

  napi_value result;
  napi_create_int32(env, (int)is_emoji, &result);
  return result;
}

napi_value PlatformViewOHOSNapi::nativeUnicodeIsRegionalIndicatorSymbol(
    napi_env env,
    napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  bool is_emoji = false;
  int64_t codePoint = 0;
  bool ret = napi_get_value_int64(env, args[0], &codePoint);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "nativeXComponentAttachFlutterEngine shell_holder "
                       "napi_get_value_int64 error";
    return nullptr;
  }

  is_emoji =
      u_hasBinaryProperty(codePoint, UProperty::UCHAR_REGIONAL_INDICATOR);

  napi_value result;
  napi_create_int32(env, (int)is_emoji, &result);
  return result;
}

/**
 * 监听获取系统的无障碍服务是否开启
 */
napi_value PlatformViewOHOSNapi::nativeAccessibilityStateChange(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeAccessibilityStateChange "
                       "napi_get_cb_info error:"
                    << ret;
    return nullptr;
  }
  int64_t shell_holder_id;
  ret = napi_get_value_int64(env, args[0], &shell_holder_id);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeAccessibilityStateChange "
                       "napi_get_value_int64 error:"
                    << ret;
    return nullptr;
  }
  bool state = false;
  ret = napi_get_value_bool(env, args[1], &state);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeAccessibilityStateChange "
                       "napi_get_value_bool error:"
                    << ret;
    return nullptr;
  }
  LOGD(
      "PlatformViewOHOSNapi::nativeAccessibilityStateChange state is: "
      "%{public}s",
      (state ? "true" : "false"));

  //send to accessibility bridge
  auto a11y_bridge = OhosAccessibilityBridge::GetInstance();
  a11y_bridge->OnOhosAccessibilityStateChange(shell_holder_id, state);
  FML_DLOG(INFO) << "nativeAccessibilityStateChange: state=" << state
                 << " shell_holder_id=" << shell_holder_id;
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeAnnounce(napi_env env,
                                                napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  size_t length = 0;
  napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);

  auto null_terminated_length = length + 1;
  auto char_array = std::make_unique<char[]>(null_terminated_length);
  napi_get_value_string_utf8(env, args[0], char_array.get(),
                             null_terminated_length, nullptr);
  LOGD("PlatformViewOHOSNapi::nativeAnnounce message: %{public}s", char_array.get());
  auto handler = std::make_shared<NativeAccessibilityChannel::AccessibilityMessageHandler>();
  handler->Announce(char_array);
  return nullptr;
}

napi_value PlatformViewOHOSNapi::nativeSetSemanticsEnabled(
    napi_env env,
    napi_callback_info info) {
  napi_status ret;
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  ret = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeSetSemanticsEnabled "
                       "napi_get_cb_info error:"
                    << ret;
    return nullptr;
  }

  int64_t shell_holder;
  ret = napi_get_value_int64(env, args[0], &shell_holder);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeSetSemanticsEnabled "
                       "napi_get_value_int64 error:"
                    << ret;
    return nullptr;
  }
  bool enabled = false;
  ret = napi_get_value_bool(env, args[1], &enabled);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeSetSemanticsEnabled "
                       "napi_get_value_bool error:"
                    << ret;
    return nullptr;
  }
  OHOS_SHELL_HOLDER->GetPlatformView()->SetSemanticsEnabled(enabled);
  FML_DLOG(INFO)
      << "PlatformViewOHOSNapi::nativeSetSemanticsEnabled "
         "OHOS_SHELL_HOLDER->GetPlatformView()->SetSemanticsEnabled= "
      << enabled;

  // when the system accessibility service is off
  if (!enabled) {
    auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
    ohosAccessibilityBridge->ClearFlutterSemanticsCaches();
    FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeSetSemanticsEnabled -> "
                      "ClearFlutterSemanticsCaches()";
  }

  // 给无障碍bridge传递nativeShellHolderId
  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->native_shell_holder_id_ = shell_holder;
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeSetSemanticsEnabled -> shell_holder:"<<shell_holder;
  return nullptr;
}

/**
 * accessibility-relevant interfaces
 */
void PlatformViewOHOSNapi::SetSemanticsEnabled(int64_t shell_holder, 
                                               bool enabled)
{
  OHOS_SHELL_HOLDER->GetPlatformView()->SetSemanticsEnabled(enabled);
}

void PlatformViewOHOSNapi::DispatchSemanticsAction(
    int64_t shell_holder,
    int32_t id, 
    flutter::SemanticsAction action, 
    fml::MallocMapping args)
{
  OHOS_SHELL_HOLDER->GetPlatformView()->PlatformView::DispatchSemanticsAction(id, action, fml::MallocMapping());
}

void PlatformViewOHOSNapi::SetAccessibilityFeatures(int64_t shell_holder,
                                                    int32_t flags)
{
  OHOS_SHELL_HOLDER->GetPlatformView()->SetAccessibilityFeatures(flags);
}

napi_value PlatformViewOHOSNapi::nativeSetFlutterNavigationAction(napi_env env, napi_callback_info info) {
  napi_status ret;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  bool isNavigate;
  ret = napi_get_value_bool(env, args[0], &isNavigate);
  if (ret != napi_ok) {
    FML_DLOG(ERROR) << "PlatformViewOHOSNapi::nativeSetFlutterNavigationAction "
                       "napi_get_value_bool error:"
                    << ret;
    return nullptr;
  }

  auto ohosAccessibilityBridge = OhosAccessibilityBridge::GetInstance();
  ohosAccessibilityBridge->IS_FLUTTER_NAVIGATE = isNavigate;
  FML_DLOG(INFO) << "PlatformViewOHOSNapi::nativeSetFlutterNavigationAction -> "
                 << isNavigate;
  return nullptr;
}
}  // namespace flutter
