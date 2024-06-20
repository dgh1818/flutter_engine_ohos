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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <dlfcn.h>
#include <time.h>
#include <mutex>

#include "flutter/fml/logging.h"

namespace fml {

#define MAX_LENGTH_OF_PARAM_NAME 49

/**
 * @brief Define the type of the param.
 */
enum HiSysEventParamType {
    HISYSEVENT_INVALID = 0,
    HISYSEVENT_BOOL = 1,
    HISYSEVENT_INT8 = 2,
    HISYSEVENT_UINT8 = 3,
    HISYSEVENT_INT16 = 4,
    HISYSEVENT_UINT16 = 5,
    HISYSEVENT_INT32 = 6,
    HISYSEVENT_UINT32 = 7,
    HISYSEVENT_INT64 = 8,
    HISYSEVENT_UINT64 = 9,
    HISYSEVENT_FLOAT = 10,
    HISYSEVENT_DOUBLE = 11,
    HISYSEVENT_STRING = 12,
    HISYSEVENT_BOOL_ARRAY = 13,
    HISYSEVENT_INT8_ARRAY = 14,
    HISYSEVENT_UINT8_ARRAY = 15,
    HISYSEVENT_INT16_ARRAY = 16,
    HISYSEVENT_UINT16_ARRAY = 17,
    HISYSEVENT_INT32_ARRAY = 18,
    HISYSEVENT_UINT32_ARRAY = 19,
    HISYSEVENT_INT64_ARRAY = 20,
    HISYSEVENT_UINT64_ARRAY = 21,
    HISYSEVENT_FLOAT_ARRAY = 22,
    HISYSEVENT_DOUBLE_ARRAY = 23,
    HISYSEVENT_STRING_ARRAY = 24
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
    HISYSEVENT_FAULT = 1,
    HISYSEVENT_STATISTIC = 2,
    HISYSEVENT_SECURITY = 3,
    HISYSEVENT_BEHAVIOR = 4
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

#if !FLUTTER_RELEASE && defined(FML_OS_OHOS)
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
    int HiSysEventWrite(const char* name, uint64_t time) {
        static void* handle = NULL;
        static int (*HiSysEvent_Write)(const char* func, int64_t line, const char* domain, const char* name,
                    HiSysEventEventType type, const HiSysEventParam params[], size_t size) = NULL;
        
        static const char* domain_ = "PERFORMANCE";
        static const char* event_ = "INTERACTION_HITCH_TIME_RATIO";
        static const HiSysEventEventType type_ = HISYSEVENT_BEHAVIOR;
        static HiSysEventParam params_[12] = {
            {
                .name = "SCENE_ID",
                .t = HISYSEVENT_STRING,
                .v = { .s = "" },
                .arraySize = 0,
            },
            {
                .name = "PROCESS_NAME",
                .t = HISYSEVENT_STRING,
                .v = { .s = "" },
                .arraySize = 0,
            },
            {
                .name = "MODULE_NAME",
                .t = HISYSEVENT_STRING,
                .v = { .s = "" },
                .arraySize = 0,
            },
            {
                .name = "ABILITY_NAME",
                .t = HISYSEVENT_STRING,
                .v = { .s = "" },
                .arraySize = 0,
            },
            {
                .name = "PAGE_URL",
                .t = HISYSEVENT_STRING,
                .v = { .s = "" },
                .arraySize = 0,
            },
            {
                .name = "UI_START_TIME",
                .t = HISYSEVENT_UINT64,
                .v = { .ui64 = 0 },
                .arraySize = 0,
            },
            {
                .name = "RS_START_TIME",
                .t = HISYSEVENT_UINT64,
                .v = { .ui64 = 0 },
                .arraySize = 0,
            },
            {
                .name = "DURATION",
                .t = HISYSEVENT_UINT64,
                .v = { .ui64 = 0 },
                .arraySize = 0,
            },
            {
                .name = "HITCH_TIME",
                .t = HISYSEVENT_UINT64,
                .v = { .ui64 = 0 },
                .arraySize = 0,
            },
            {
                .name = "HITCH_TIME_RATIO",
                .t = HISYSEVENT_FLOAT,
                .v = { .f = 0 },
                .arraySize = 0,
            },
            {
                .name = "IS_FOLD_DISP",
                .t = HISYSEVENT_BOOL,
                .v = { .b = false },
                .arraySize = 0,
            },
            {
                .name = "BUNDLE_NAME_EX",
                .t = HISYSEVENT_STRING,
                .v = { .s = "" },
                .arraySize = 0,
            },
        };
        static const size_t size_ = 12;
        
        if (handle == NULL && HiSysEvent_Write == NULL) {
            std::mutex mtx;
            mtx.lock();
            if (handle == NULL && HiSysEvent_Write == NULL) {
                handle = dlopen("/system/lib64/chipset-pub-sdk/libhisysevent.z.so", RTLD_LAZY);
                if (handle != NULL) {
                    HiSysEvent_Write = reinterpret_cast<int (*)(const char*, int64_t, const char*, const char*,
                        HiSysEventEventType, const HiSysEventParam[], size_t)>(dlsym(handle, "HiSysEvent_Write"));
                    if (HiSysEvent_Write == NULL) {
                        FML_DLOG(ERROR) << "dlsym HiSysEvent_Write return NULL\n";
                        dlclose(handle);
                        handle = NULL;
                    }
                } else {
                    FML_DLOG(ERROR) << "dlopen libhisysevent.z.so return NULL\n";
                }
            }
            mtx.unlock();
        }

        params_[3].v.s = name;
        params_[7].v.ui64 = time;
        int ret = HiSysEvent_Write(__FUNCTION__, __LINE__, domain_, event_, type_, params_, size_);
        return ret;
    }
    ~HiSysEventTrace() {
        clock_gettime(CLOCK_MONOTONIC, &end_time_);
        int ret = HiSysEventWrite(name_, (end_time_.tv_sec - begin_time_.tv_sec) * 1e6 +
                                  (end_time_.tv_nsec - begin_time_.tv_nsec) / 1000); // us
        FML_DLOG(INFO) << "HiSysEventWrite return " << ret;
    }
};
#else
class HiSysEventTrace {
public:
    explicit HiSysEventTrace(const char* name) {}
    ~HiSysEventTrace() {}
};
#endif

}  // namespace fml
#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_HISYSEVENT_INCLUDE_HISYSEVENT_C_H
