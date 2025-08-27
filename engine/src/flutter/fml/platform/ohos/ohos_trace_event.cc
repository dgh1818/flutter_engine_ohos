/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "flutter/fml/trace_event.h"

#include <string>
#include "flutter/fml/logging.h"
#include "flutter/fml/platform/ohos/hiappevent/ohos_hiappevent.h"

#if defined(FML_OS_OHOS)
namespace fml {
namespace tracing {

static constexpr char OHOS_COLON[] = ":";
static constexpr char OHOS_SCOPE[] = "::";
static constexpr char OHOS_WHITESPACE[] = " ";
static constexpr char OHOS_FILTER_NAME_SCENE[] = "SceneDisplayLag";
static constexpr char OHOS_FILTER_NAME_POINTER[] = "PointerEvent";
static const int Argument_Size = 3;
static const int vsync_transitions_missed_Size = 2;

void OHOSTraceTimelineEvent(TraceArg category_group,
                            TraceArg name,
                            int64_t timestamp_micros,
                            TraceIDArg id,
                            Dart_Timeline_Event_Type type,
                            intptr_t argument_count,
                            const char** argument_names,
                            const char** argument_values)
{
    if (type != Dart_Timeline_Event_Begin && type != Dart_Timeline_Event_Async_Begin &&
        type != Dart_Timeline_Event_Async_End && type != Dart_Timeline_Event_Flow_Begin &&
        type != Dart_Timeline_Event_Flow_End) {
        return;
    }

    if (type != Dart_Timeline_Event_Begin && strcmp(name, OHOS_FILTER_NAME_POINTER) == 0) {
        // Trace 'PointerEvent' is not work in the scenario of extenal texture
        return;
    }

    int realNumber = argument_count;
    if (type != Dart_Timeline_Event_Begin && strcmp(name, OHOS_FILTER_NAME_SCENE) == 0) {
        // Trace 'SceneDisplayLag' have inconsistent parameters. It's not good to watch.
        realNumber = 0;
        if ((type == Dart_Timeline_Event_Async_Begin) && (argument_count >= Argument_Size)) {
            int vsync_transitions_missed = std::stoi(argument_values[2]);
            if (vsync_transitions_missed >= vsync_transitions_missed_Size) {
                fml::hiappevent::OhosHiappEventDDL::GetInstance()->ReportJANKEvent(
                    timestamp_micros, argument_values, argument_count);
            }
        }
    }

    std::string TraceName(category_group);
    TraceName.append(OHOS_SCOPE);
    TraceName.append(name);

    if (argument_names != nullptr && argument_values != nullptr) {
        for (int i = 0; i < realNumber; i++) {
            std::string TraceParam(OHOS_WHITESPACE);
            TraceName += TraceParam + argument_names[i] + OHOS_COLON + argument_values[i];
        }
    }

    switch(type) {
        case Dart_Timeline_Event_Begin:
            OH_HiTrace_StartTrace(TraceName.c_str());
            break;
        case Dart_Timeline_Event_Async_Begin:
        case Dart_Timeline_Event_Flow_Begin:
            OH_HiTrace_StartAsyncTrace(TraceName.c_str(), id);
            break;
        case Dart_Timeline_Event_Async_End:
        case Dart_Timeline_Event_Flow_End:
            OH_HiTrace_FinishAsyncTrace(TraceName.c_str(), id);
            break;
        default:
            break;
    }
    return;
}

void OHOSTraceTimelineEvent(TraceArg category_group,
                            TraceArg name,
                            TraceIDArg id,
                            Dart_Timeline_Event_Type type,
                            intptr_t argument_count,
                            const char** argument_names,
                            const char** argument_values)
{
    if (type != Dart_Timeline_Event_Begin && type != Dart_Timeline_Event_Async_Begin &&
        type != Dart_Timeline_Event_Async_End && type != Dart_Timeline_Event_Flow_Begin &&
        type != Dart_Timeline_Event_Flow_End) {
        return;
    }

    if (type != Dart_Timeline_Event_Begin && strcmp(name, OHOS_FILTER_NAME_POINTER) == 0) {
        // Trace 'PointerEvent' is not work in the scenario of extenal texture
        return;
    }

    int realNumber = argument_count;
    if (type != Dart_Timeline_Event_Begin && strcmp(name, OHOS_FILTER_NAME_SCENE) == 0) {
        // Trace 'SceneDisplayLag' have inconsistent parameters. It's not good to watch.
        realNumber = 0;
    }

    std::string TraceName(category_group);
    TraceName.append(OHOS_SCOPE);
    TraceName.append(name);

    if (argument_names != nullptr && argument_values != nullptr) {
        for (int i = 0; i < realNumber; i++) {
            std::string TraceParam(OHOS_WHITESPACE);
            TraceName += TraceParam + argument_names[i] + OHOS_COLON + argument_values[i];
        }
    }

    switch(type) {
        case Dart_Timeline_Event_Begin:
            OH_HiTrace_StartTrace(TraceName.c_str());
            break;
        case Dart_Timeline_Event_Async_Begin:
        case Dart_Timeline_Event_Flow_Begin:
            OH_HiTrace_StartAsyncTrace(TraceName.c_str(), id);
            break;
        case Dart_Timeline_Event_Async_End:
        case Dart_Timeline_Event_Flow_End:
            OH_HiTrace_FinishAsyncTrace(TraceName.c_str(), id);
            break;
        default:
            break;
    }
    return;
}

void OHOSTraceEventEnd(void) {
    OH_HiTrace_FinishTrace();
}

} // namespace tracing
} // namespace fml
#endif // FML_OS_OHOS
