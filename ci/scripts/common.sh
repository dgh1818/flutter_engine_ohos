#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# Common functions for CI scripts

# Logging functions
log_info() {
    echo "[INFO] $*"
}

log_warn() {
    echo "[WARN] $*"
}

log_error() {
    echo "[ERROR] $*" >&2
}

log_step() {
    echo "========================================"
    echo "$*"
    echo "========================================"
}

# Command execution wrapper
run_cmd() {
    local cmd="$1"
    echo "$ $cmd"
    eval "$cmd"
}
