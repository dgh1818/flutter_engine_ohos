/*
 * Copyright (c) 2023 Hunan OpenValley Digital Industry Development Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_KHZG file.
 */

#ifndef OHOS_FLUTTER_NAPI_UTIL_H
#define OHOS_FLUTTER_NAPI_UTIL_H
#include <string.h>
#include <uv.h>
#include <string>
#include <vector>
#include "flutter/fml/logging.h"
#include "napi/native_api.h"

namespace fml {
namespace napi {
enum {
  kSuccess = 0,
  kErrorType = -100,
  kErrorNull,
};

int32_t GetString(napi_env env, napi_value arg, std::string& strValue);
int32_t GetArrayString(napi_env env,
                       napi_value arg,
                       std::vector<std::string>& arrayString);
int32_t GetArrayBuffer(napi_env env,
                       napi_value arg,
                       void** message,
                       size_t* lenth);

/**
 *  打印napi_value @args 的类型信息
 */
void NapiPrintValueTypes(napi_env env, int argc, napi_value* argv);
/**
 *  打印napi_value @cur 的类型信息
 */
void NapiPrintValueType(napi_env env, napi_value cur);

/**
 * 判断napi value  类型
 */
bool NapiIsType(napi_env env, napi_value value, napi_valuetype type);
bool NapiIsNotType(napi_env env, napi_value value, napi_valuetype type);
/**
 * 判断napi value类型是否是其中的一种
 */
bool NapiIsAnyType(napi_env env, napi_value value, ...);
/**
 * 判断值是否为空
 */
bool NapiIsNull(napi_env env, napi_value value);

/**
 * 创建ArrayBuffer
 */
napi_value CreateArrayBuffer(napi_env env, void* inputData, size_t dataSize);

/**
 * 回调JS方法
 */
napi_status InvokeJsMethod(napi_env env_,
                           napi_ref ref_napi_obj_,
                           const char* methodName,
                           size_t argc,
                           const napi_value* argv);
}  // namespace napi
}  // namespace fml
#endif
