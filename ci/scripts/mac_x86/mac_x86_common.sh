#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# Shared helpers for the mac-x86 build & publish steps.

MAC_X86_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Repo root (self-locating: works from any cwd, worktree or CI layout).
REPO_ROOT="$(cd "$MAC_X86_SCRIPT_DIR/../../.." && pwd)"
ENGINE_DIR="$REPO_ROOT/engine"

# depot_tools on PATH for vpython3/gclient; append-only, never prepend.
for _dt in "$HOME/depot_tools" "$ENGINE_DIR/src/flutter/third_party/depot_tools"; do
    if [ -x "$_dt/vpython3" ]; then
        case ":$PATH:" in
            *":$_dt:"*) ;;
            *) PATH="$PATH:$_dt"; export PATH ;;
        esac
    fi
done
unset _dt

# Default DEVECO_SDK_HOME to the local SDK install if unset or invalid.
if [ ! -d "${DEVECO_SDK_HOME:-}" ] && [ -d "/Applications/DevEco-Studio.app/Contents/sdk" ]; then
    export DEVECO_SDK_HOME="/Applications/DevEco-Studio.app/Contents/sdk"
fi

# One log directory per pipeline run, shared by every step:
# engine/outputs/mac_x86/<run-id>/. ohos_ci_runner.sh generates the run id
# once per invocation (MAC_X86_RUN_ID); a step run standalone falls back to
# its own timestamp.
init_log_dir() {
    LOG_DIR="$ENGINE_DIR/outputs/mac_x86/${MAC_X86_RUN_ID:-$(date +%Y%m%d-%H%M%S)}"
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
