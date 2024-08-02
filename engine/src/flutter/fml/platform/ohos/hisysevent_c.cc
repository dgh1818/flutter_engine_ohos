#if !defined(_WIN32) && !defined(_WIN64)
#include "flutter/fml/platform/ohos/hisysevent_c.h"

namespace fml {

static void* handle = NULL;
static HiSysEvent_Write_Def HiSysEvent_Write  = NULL;
static const char* domain_ = "PERFORMANCE";
static const char* event_ = "INTERACTION_HITCH_TIME_RATIO";
static const HiSysEventEventType kType = kHisyseventBehavior;
static HiSysEventParam params_[12] = {
    {
        .name = "SCENE_ID",
        .t = kHisyseventString,
        .v = { .s = "" },
        .arraySize = 0,
    },
    {
        .name = "PROCESS_NAME",
        .t = kHisyseventString,
        .v = { .s = "" },
        .arraySize = 0,
    },
    {
        .name = "MODULE_NAME",
        .t = kHisyseventString,
        .v = { .s = "" },
        .arraySize = 0,
    },
    {
        .name = "ABILITY_NAME",
        .t = kHisyseventString,
        .v = { .s = "" },
        .arraySize = 0,
    },
    {
        .name = "PAGE_URL",
        .t = kHisyseventString,
        .v = { .s = "" },
        .arraySize = 0,
    },
    {
        .name = "UI_START_TIME",
        .t = kHisyseventUint64,
        .v = { .ui64 = 0 },
        .arraySize = 0,
    },
    {
        .name = "RS_START_TIME",
        .t = kHisyseventUint64,
        .v = { .ui64 = 0 },
        .arraySize = 0,
    },
    {
        .name = "DURATION",
        .t = kHisyseventUint64,
        .v = { .ui64 = 0 },
        .arraySize = 0,
    },
    {
        .name = "HITCH_TIME",
        .t = kHisyseventUint64,
        .v = { .ui64 = 0 },
        .arraySize = 0,
    },
    {
        .name = "HITCH_TIME_RATIO",
        .t = kHisyseventFloat,
        .v = { .f = 0 },
        .arraySize = 0,
    },
    {
        .name = "IS_FOLD_DISP",
        .t = kHisyseventBool,
        .v = { .b = false },
        .arraySize = 0,
    },
    {
        .name = "BUNDLE_NAME_EX",
        .t = kHisyseventString,
        .v = { .s = "" },
        .arraySize = 0,
    },
};

static const size_t kSize = 12;
int HiSysEventWrite(const char* name, uint64_t time) {
    if (handle == NULL && HiSysEvent_Write == NULL) {
        handle = dlopen("/system/lib64/chipset-pub-sdk/libhisysevent.z.so", RTLD_LAZY);
        if (handle != NULL) {
            HiSysEvent_Write = reinterpret_cast<HiSysEvent_Write_Def>(dlsym(handle, "HiSysEvent_Write"));
            if (HiSysEvent_Write == NULL) {
                FML_DLOG(ERROR) << "dlsym HiSysEvent_Write return NULL\n";
                dlclose(handle);
                handle = NULL;
            }
        } else {
            FML_DLOG(ERROR) << "dlopen libhisysevent.z.so return NULL\n";
        }
    }

    if (HiSysEvent_Write != NULL) {
        params_[3].v.s = name;
        params_[7].v.ui64 = time;
        int ret = HiSysEvent_Write(__FUNCTION__, __LINE__, domain_, event_, kType, params_, kSize);
        return ret;
    } else {
        FML_DLOG(ERROR) << "HiSysEvent_Write is NULL\n";
        return -1;
    }
}
}  // namespace fml
#endif // #if !defined(_WIN32) && !defined(_WIN64)
