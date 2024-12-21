/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef FLUTTER_SHELL_PLATFORM_OHOS_UTILS_OHOS_UTILS_H_
#define FLUTTER_SHELL_PLATFORM_OHOS_UTILS_OHOS_UTILS_H_
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "flutter/shell/platform/ohos/ohos_logging.h"
namespace flutter {

class OHOSUtils {
 public:
  OHOSUtils();
  ~OHOSUtils();

  static void SerializeString(const std::string& str,
                              std::vector<uint8_t>& buffer);
  static std::vector<uint8_t> SerializeStringIntMap(
      const std::map<std::string, int32_t>& mp);
  static void CharArrayToInt32(const char* str, int32_t& target);
};

}  // namespace flutter
#endif  // FLUTTER_SHELL_PLATFORM_OHOS_UTILS_OHOS_UTILS_H_
