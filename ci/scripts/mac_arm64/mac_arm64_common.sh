#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# Shared helpers for the mac-arm64 build & publish steps.

MAC_ARM64_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Repo root (self-locating: works from any cwd, worktree or CI layout).
REPO_ROOT="$(cd "$MAC_ARM64_SCRIPT_DIR/../../.." && pwd)"
ENGINE_DIR="$REPO_ROOT/engine"

# One log directory per pipeline run, shared by every step:
# engine/outputs/mac_arm64/<run-id>/. ohos_ci_runner.sh generates the run id
# once per invocation (MAC_ARM64_RUN_ID); a step run standalone falls back to
# its own timestamp.
init_log_dir() {
    LOG_DIR="$ENGINE_DIR/outputs/mac_arm64/${MAC_ARM64_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
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
