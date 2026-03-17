#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

check_system_info() {
    run_cmd "uname -a"
    run_cmd "cat /etc/os-release"
    run_cmd "id -un"
    run_cmd "set"
}

check_system_info
