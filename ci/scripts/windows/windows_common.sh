#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# Shared helpers for the windows build & publish steps.

WINDOWS_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Repo root (self-locating) and the vendored engine dir.
REPO_ROOT="$(cd "$WINDOWS_SCRIPT_DIR/../../.." && pwd)"
ENGINE_DIR="$REPO_ROOT/engine"

# Shared log dir for one pipeline run: engine/outputs/windows/<run-id>/
# (WINDOWS_RUN_ID is set by ohos_ci_runner.sh; standalone steps fall back
# to their own timestamp).
init_log_dir() {
    LOG_DIR="$ENGINE_DIR/outputs/windows/${WINDOWS_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
    mkdir -p "$LOG_DIR"
}

# Run a command with output redirected to a log file.
run_logged() {
    local logfile="$1"
    shift
    "$@" >"$logfile" 2>&1
}

# Fail a step, showing the tail of its log.
fail_with_log() {
    log_error "$2 (see $1)"
    tail -n 20 "$1" >&2 || true
    exit 1
}
