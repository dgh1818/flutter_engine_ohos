/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE_HW file.
 */

#include "deviceinfo.h"
#include "qos/qos.h"

extern "C" {

int OH_QoS_SetThreadQoS(QoS_Level /*level*/) {
  return 0;
}

}  // extern "C"
