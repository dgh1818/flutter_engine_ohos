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
#include "ohos_utils.h"
namespace flutter {

OHOSUtils::OHOSUtils() {};
OHOSUtils::~OHOSUtils() {};

/**
 * 将string序列化为uint8字节数组
 */
void OHOSUtils::SerializeString(const std::string& str, std::vector<uint8_t>& buffer)
{
    // store the length of the string as a uint8_t
    uint32_t length = str.size();
    buffer.insert(buffer.end(),
                  reinterpret_cast<uint8_t*>(&length),
                  reinterpret_cast<uint8_t*>(&length) + sizeof(length));
    // store the actual string data
    buffer.insert(buffer.end(), str.begin(), str.end());
}

/**
 * 将map<string, int>序列化为uint8字节数组
 */
std::vector<uint8_t> OHOSUtils::SerializeStringIntMap(const std::map<std::string, int32_t>& mp)
{
    std::vector<uint8_t> buffer;

    // Store the number of elements in the map
    uint32_t mapSize = mp.size();
    buffer.insert(buffer.end(),
                  reinterpret_cast<uint8_t*>(&mapSize),
                  reinterpret_cast<uint8_t*>(&mapSize) + sizeof(mapSize));

    // Iterate over the map and serialize each key-value pair
    for (const auto& it : mp) {
        SerializeString(it.first, buffer);
        buffer.insert(buffer.end(),
                      reinterpret_cast<const uint8_t*>(&it.second),
                      reinterpret_cast<const uint8_t*>(&it.second) + sizeof(it.second));
    }

    return buffer;
}

/**
 * convert char* type to int32_t type with a safe way
 */
void OHOSUtils::CharArrayToInt32(const char* str, int32_t& target)
{
    char* end;
    long int num = std::strtol(str, &end, 10);

    // Check if the entire string was converted
    if (*end == '\0') {
        if (INT_MIN <= num && num <= INT_MAX) {
            target = static_cast<int32_t>(num);
            LOGD("The int32_t value is: %{public}d", target);
        } else {
            LOGE("The value of num is out of range of int32_t");
        }
    } else {
        target = 0;
        LOGE("Conversion error, non-convertible part: %{public}s", end);
    }
}

}