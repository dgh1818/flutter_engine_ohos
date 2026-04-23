/*
 * Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */
#ifndef OHOS_WATCHDOG_H
#define OHOS_WATCHDOG_H

#include <memory>
#include "flutter/fml/task_runner.h"

namespace fml {

namespace OhosWatchdog {

std::pair<size_t, std::function<void(size_t)>> MakeWatchdog(
    const fml::RefPtr<fml::TaskRunner> &ui);

}  // namespace OhosWatchdog
}  // namespace fml
#endif //OHOS_WATCHDOG_H
