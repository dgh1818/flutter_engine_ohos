#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# build the 8 artifact configs in parallel.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"
source "$SCRIPT_DIR/mac_x86_common.sh"

# load-bearing: runner executes steps with cwd=work_dir, ./ohos must run
# from the engine root
cd "$ENGINE_DIR" || exit 1
init_log_dir

PARALLEL="${MAC_X86_PARALLEL:-4}"

# ccache is on by default (keeps the runner's warm cache across runs).
CCACHE_FLAG=""
HOST_CCACHE=""
if command -v ccache >/dev/null 2>&1; then
    CCACHE_FLAG="--use-ccache"
    HOST_CCACHE="--ccache"
fi

GN="src/flutter/tools/gn"
GS_TARGET="flutter/shell/platform/ohos:gen_snapshot flutter/shell/platform/ohos:new_gen_snapshot"
declare -a BUILDS=(
    "host_release|./ohos -t release -n host ${CCACHE_FLAG}"
    "host_debug|${GN} --runtime-mode debug --no-lto --no-goma --no-prebuilt-dart-sdk --full-dart-sdk --disable-desktop-embeddings --no-build-embedder-examples --verbose --mac-cpu x64 ${HOST_CCACHE} && ninja -C src/out/host_debug flutter/lib/snapshot:generate_snapshot_bin"
    "ohos_release_arm64|./ohos -t release -n config ${CCACHE_FLAG} && ninja -C src/out/ohos_release_arm64 ${GS_TARGET}"
    "ohos_profile_arm64|./ohos -t profile -n config ${CCACHE_FLAG} && ninja -C src/out/ohos_profile_arm64 ${GS_TARGET}"
    "ohos_debug_arm64|./ohos -t debug -n config ${CCACHE_FLAG} && ninja -C src/out/ohos_debug_arm64 ${GS_TARGET}"
    "ohos_release_x64|./ohos -t release -n config --ohos-cpu x64 ${CCACHE_FLAG} && ninja -C src/out/ohos_release_x64 ${GS_TARGET}"
    "ohos_profile_x64|./ohos -t profile -n config --ohos-cpu x64 ${CCACHE_FLAG} && ninja -C src/out/ohos_profile_x64 ${GS_TARGET}"
    "ohos_debug_x64|./ohos -t debug -n config --ohos-cpu x64 ${CCACHE_FLAG} && ninja -C src/out/ohos_debug_x64 ${GS_TARGET}"
)

TOTAL=${#BUILDS[@]}
log_info "building $TOTAL configs with parallelism $PARALLEL (logs under $LOG_DIR)"
for spec in "${BUILDS[@]}"; do
    log_info "  - ${spec%%|*}"
done

pids=()
names=()
idx=0
rc=0
while [ "$idx" -lt "$TOTAL" ] || [ "${#pids[@]}" -gt 0 ]; do
    # keep up to PARALLEL builds running
    while [ "${#pids[@]}" -lt "$PARALLEL" ] && [ "$idx" -lt "$TOTAL" ]; do
        spec="${BUILDS[$idx]}"
        name="${spec%%|*}"
        cmd="${spec#*|}"
        log_info "start: $name (log: $LOG_DIR/${name}.log)"
        bash -c "$cmd" >"$LOG_DIR/${name}.log" 2>&1 &
        pids+=("$!")
        names+=("$name")
        idx=$((idx + 1))
    done
    # reap finished builds
    new_pids=()
    new_names=()
    for i in "${!pids[@]}"; do
        if kill -0 "${pids[$i]}" 2>/dev/null; then
            new_pids+=("${pids[$i]}")
            new_names+=("${names[$i]}")
        else
            if wait "${pids[$i]}"; then
                log_info "[ OK ] ${names[$i]}"
            else
                log_error "[FAIL] ${names[$i]} (see $LOG_DIR/${names[$i]}.log)"
                rc=1
            fi
        fi
    done
    pids=("${new_pids[@]}")
    names=("${new_names[@]}")
    if [ "${#pids[@]}" -gt 0 ]; then
        sleep 5
    fi
done

if [[ $rc -ne 0 ]]; then
    log_error "build failed (logs under $LOG_DIR)"
fi
exit $rc
