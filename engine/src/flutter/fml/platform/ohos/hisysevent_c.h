/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_INCLUDE_HISYSEVENT_C_H
#define HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_INCLUDE_HISYSEVENT_C_H

#if !defined(_WIN32) && !defined(_WIN64)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <dlfcn.h>
#include <time.h>
#include "flutter/fml/logging.h"

#if !FLUTTER_RELEASE && defined(FML_OS_OHOS)
#define HISYSEVENT_WRITE_SINGLE(name)  \
    ::fml::HiSysEventWrite(name, 0);
#define HISYSEVENT_WRITE_DURATION(name)   \
    ::fml::HiSysEventTrace __FML__TOKEN_CAT__2(hisysevent, __LINE__)(name);
#else
#define HISYSEVENT_WRITE_SINGLE(name)
#define HISYSEVENT_WRITE_DURATION(name)
#endif // !FLUTTER_RELEASE && defined(FML_OS_OHOS)

namespace fml {

#define MAX_LENGTH_OF_PARAM_NAME 49

/**
 * @brief Define the type of the param.
 */
enum HiSysEventParamType {
    kHisyseventInvalid = 0,
    kHisyseventBool = 1,
    kHisyseventInt8 = 2,
    kHisyseventUint8 = 3,
    kHisyseventInt16 = 4,
    kHisyseventUint16 = 5,
    kHisyseventInt32 = 6,
    kHisyseventUint32 = 7,
    kHisyseventInt64 = 8,
    kHisyseventUint64 = 9,
    kHisyseventFloat = 10,
    kHisyseventDouble = 11,
    kHisyseventString = 12,
    kHisyseventBoolArray = 13,
    kHisyseventInt8Array = 14,
    kHisyseventUint8Array = 15,
    kHisyseventInt16Array = 16,
    kHisyseventUint16Array = 17,
    kHisyseventInt32Array = 18,
    kHisyseventUint32Array = 19,
    kHisyseventInt64Array = 20,
    kHisyseventUint64Array = 21,
    kHisyseventFloatArray = 22,
    kHisyseventDoubleArray = 23,
    kHisyseventStringArray = 24
};
typedef enum HiSysEventParamType HiSysEventParamType;

/**
 * @brief Define the value of the param.
 */
union HiSysEventParamValue {
    bool b;
    int8_t i8;
    uint8_t ui8;
    int16_t i16;
    uint16_t ui16;
    int32_t i32;
    uint32_t ui32;
    int64_t i64;
    uint64_t ui64;
    float f;
    double d;
    const char *s;
    void *array;
};
typedef union HiSysEventParamValue HiSysEventParamValue;

/**
 * @brief Define param struct.
 */
struct HiSysEventParam {
    char name[MAX_LENGTH_OF_PARAM_NAME];
    HiSysEventParamType t;
    HiSysEventParamValue v;
    size_t arraySize;
};
typedef struct HiSysEventParam HiSysEventParam;

/**
 * @brief Event type.
 */
enum HiSysEventEventType {
    kHisyseventFault = 1,
    kHisyseventStatistic = 2,
    kHisyseventSecurity = 3,
    kHisyseventBehavior = 4
};
typedef enum HiSysEventEventType HiSysEventEventType;

/**
 * @brief Write system event.
 * @param domain event domain.
 * @param name   event name.
 * @param type   event type.
 * @param params event params.
 * @param size   the size of param list.
 * @return 0 means success, less than 0 means failure, greater than 0 means invalid params.
 */
// #define OH_HiSysEvent_Write(domain, name, type, params, size) \
//     HiSysEvent_Write(__FUNCTION__, __LINE__, domain, name, type, params, size)

// int HiSysEvent_Write(const char* func, int64_t line, const char* domain, const char* name,
//     HiSysEventEventType type, const HiSysEventParam params[], size_t size);

typedef int (*HiSysEvent_Write_Def)(const char* func, int64_t line, const char* domain, const char* name,
    HiSysEventEventType type, const HiSysEventParam params[], size_t size);

int HiSysEventWrite(const char* name, uint64_t time);

class HiSysEventTrace {
private:
    const char* name_;
    struct timespec begin_time_;
    struct timespec end_time_;

public:
    explicit HiSysEventTrace(const char* name) {
        if (name != NULL) {
            name_ = name;
        } else {
            name_ = "flutter default trace name";
        }
        clock_gettime(CLOCK_MONOTONIC, &begin_time_);
    }
    ~HiSysEventTrace() {
        clock_gettime(CLOCK_MONOTONIC, &end_time_);
        int ret = HiSysEventWrite(name_, (end_time_.tv_sec - begin_time_.tv_sec) * 1e6 +
                                  (end_time_.tv_nsec - begin_time_.tv_nsec) / 1000); // us
        FML_DLOG(INFO) << "HiSysEventWrite return " << ret;
    }
};

}  // namespace fml

#else
#define HISYSEVENT_WRITE_SINGLE(name)
#define HISYSEVENT_WRITE_DURATION(name)
#endif // #if !defined(_WIN32) && !defined(_WIN64)
#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_INCLUDE_HISYSEVENT_C_H
