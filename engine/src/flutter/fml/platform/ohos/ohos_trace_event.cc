/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "flutter/fml/trace_event.h"

#include <string>

#if defined(FML_OS_OHOS)
namespace fml {
namespace tracing {

static constexpr char OHOS_COLON[] = ":";
static constexpr char OHOS_SCOPE[] = "::";
static constexpr char OHOS_WHITESPACE[] = " ";
static constexpr char OHOS_FILTER_NAME_SCENE[] = "SceneDisplayLag";
static constexpr char OHOS_FILTER_NAME_POINTER[] = "PointerEvent";

void OHOSTraceTimelineEvent(TraceArg category_group,
                            TraceArg name,
                            TraceIDArg id,
                            Dart_Timeline_Event_Type type,
                            intptr_t argument_count,
                            const char** argument_names,
                            const char** argument_values) {
  if (type != Dart_Timeline_Event_Begin &&
      type != Dart_Timeline_Event_Async_Begin &&
      type != Dart_Timeline_Event_Async_End &&
      type != Dart_Timeline_Event_Flow_Begin &&
      type != Dart_Timeline_Event_Flow_End) {
    return;
  }

  if (type != Dart_Timeline_Event_Begin &&
      strcmp(name, OHOS_FILTER_NAME_POINTER) == 0) {
    // Trace 'PointerEvent' is not work in the scenario of extenal texture
    return;
  }

  int realNumber = argument_count;
  if (type != Dart_Timeline_Event_Begin &&
      strcmp(name, OHOS_FILTER_NAME_SCENE) == 0) {
    // Trace 'SceneDisplayLag' have inconsistent parameters. It's not good to
    // watch.
    realNumber = 0;
  }

  std::string TraceName(category_group);
  TraceName.append(OHOS_SCOPE);
  TraceName.append(name);

  if (argument_names != nullptr && argument_values != nullptr) {
    for (int i = 0; i < realNumber; i++) {
      std::string TraceParam(OHOS_WHITESPACE);
      TraceName +=
          TraceParam + argument_names[i] + OHOS_COLON + argument_values[i];
    }
  }

  switch (type) {
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

}  // namespace tracing
}  // namespace fml
#endif  // FML_OS_OHOS
