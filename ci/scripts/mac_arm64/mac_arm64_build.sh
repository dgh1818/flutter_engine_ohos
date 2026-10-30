#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# build host_debug_arm64 + host_release_arm64 in parallel.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"
source "$SCRIPT_DIR/mac_arm64_common.sh"

# load-bearing: runner executes steps with cwd=work_dir, ./ohos must run
# from the engine root
cd "$ENGINE_DIR" || exit 1
init_log_dir

pids=()
for t in debug release; do
    cmd="./ohos -t $t -n host --host-cpu arm64"
    log_info "start: $cmd (log: $LOG_DIR/build.host_${t}_arm64.log)"
    bash -c "$cmd" >"$LOG_DIR/build.host_${t}_arm64.log" 2>&1 &
    pids+=("$!")
done

rc=0
for pid in "${pids[@]}"; do
    wait "$pid" || rc=1
done
if [[ $rc -ne 0 ]]; then
    log_error "build failed (logs under $LOG_DIR)"
fi
exit $rc
