#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

# Unified shard test entry point - drives dev/bots/test.dart via SHARD/SUBSHARD env vars.
# Usage: run_shard_test.sh <shard> [subshard]
# Example: run_shard_test.sh framework_tests libraries
#
# Automatically retries on segfault (exit code -6/139) caused by concurrent
# flutter test processes competing for shared cache under FLUTTER_ALREADY_LOCKED.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FLUTTER_ROOT="$SCRIPT_DIR/../.."

SHARD="${1:?Usage: run_shard_test.sh <shard> [subshard]}"
SUBSHARD="${2:-}"

export OHOS_CI=true
export PUB_CACHE="${PUB_CACHE:-$WORK_DIR/pub_cache_shared}"
export FLUTTER_ALREADY_LOCKED=true

MAX_RETRIES=2

LOG_DIR="$WORK_DIR/Archive/out/unit_tests_logs"
mkdir -p "$LOG_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

SUBSHARD_SUFFIX=""
for var in OHOS_LIBRARIES_SUBSHARD OHOS_WIDGETS_SUBSHARD OHOS_NON_MATERIAL_SUBSHARD; do
    if [ -n "${!var}" ]; then
        SUBSHARD_SUFFIX="_${!var}"
        break
    fi
done
if [ "$OHOS_LIBRARIES_SUBSHARD" = "non_material" ] && [ -n "$OHOS_NON_MATERIAL_SUBSHARD" ]; then
    SUBSHARD_SUFFIX="_non_material_${OHOS_NON_MATERIAL_SUBSHARD}"
fi
LOG_FILE="$LOG_DIR/${SHARD}_${SUBSHARD:-all}${SUBSHARD_SUFFIX}_$TIMESTAMP.log"

echo "[$(date +%H:%M:%S)] START: SHARD=$SHARD SUBSHARD=$SUBSHARD" | tee "$LOG_FILE"

run_test() {
    cd "$FLUTTER_ROOT"
    if [ -n "$SUBSHARD" ]; then
        SHARD="$SHARD" SUBSHARD="$SUBSHARD" dart dev/bots/test.dart 2>&1 | tee -a "$LOG_FILE"
    else
        SHARD="$SHARD" dart dev/bots/test.dart 2>&1 | tee -a "$LOG_FILE"
    fi
    return ${PIPESTATUS[0]}
}

attempt=0
exit_code=0

while [ $attempt -le $MAX_RETRIES ]; do
    run_test
    exit_code=$?

    # SIGABRT: exit code 134 (signal 6), SIGSEGV: exit code 139 (signal 11)
    if [ $exit_code -eq 134 ] || [ $exit_code -eq 139 ]; then
        attempt=$((attempt + 1))
        if [ $attempt -le $MAX_RETRIES ]; then
            echo "[$(date +%H:%M:%S)] RETRY ($attempt/$MAX_RETRIES): SHARD=$SHARD SUBSHARD=$SUBSHARD (segfault exit=$exit_code)" | tee -a "$LOG_FILE"
            sleep 10
            continue
        fi
    fi

    break
done

# Collect JSON reports from test_output/ to the shard's report directory
REPORT_DIR="$LOG_DIR/reports/${SHARD}_${SUBSHARD:-all}${SUBSHARD_SUFFIX}"
if [ -d "$FLUTTER_ROOT/test_output" ]; then
    mkdir -p "$REPORT_DIR"
    for f in "$FLUTTER_ROOT/test_output"/*; do
        [ -f "$f" ] && mv "$f" "$REPORT_DIR/"
    done
    rmdir "$FLUTTER_ROOT/test_output" 2>/dev/null
fi

if [ $exit_code -eq 0 ]; then
    echo "[$(date +%H:%M:%S)] PASS: SHARD=$SHARD SUBSHARD=$SUBSHARD"
else
    echo "[$(date +%H:%M:%S)] FAIL: SHARD=$SHARD SUBSHARD=$SUBSHARD (exit=$exit_code, log=$LOG_FILE)"
fi

exit $exit_code
