#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

# Single-machine staggered parallel shard scheduler for OHOS CI.
#
# Strategy: launch heavy shards first, then stagger lightweight shards
# after a delay. This avoids the initial resource contention that occurs
# when 14+ flutter test processes all start compiling simultaneously.
#
# Wave 1 (t=0):  Heavy framework subshards (libraries x3, widgets x3)
# Wave 2 (t=2m): Medium shards (misc, misc2, widget_extras, tool_tests_commands)
# Wave 3 (t=4m): Light shards (slow, general, snippets, host_cross_arch)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FLUTTER_ROOT="$SCRIPT_DIR/../.."

export OHOS_CI=true
export PUB_CACHE="${PUB_CACHE:-$WORK_DIR/pub_cache_shared}"

LOG_DIR="$WORK_DIR/Archive/out/unit_tests_logs"
mkdir -p "$LOG_DIR"

# --- Helper: launch a shard ---

declare -A SHARD_PIDS
declare -A SHARD_LABELS

launch_shard() {
    local shard="$1"
    local subshard="${2:-}"
    local extra_env="${3:-}"

    env_label=""
    env_export=""
    if [ -n "$extra_env" ]; then
        env_export="$extra_env"
        env_label="_${extra_env#*=}"
    fi

    if [ -n "$subshard" ]; then
        label="${shard}_${subshard}${env_label}"
    else
        label="${shard}_all"
    fi

    if [ -n "$subshard" ]; then
        env $env_export bash "$SCRIPT_DIR/run_shard_test.sh" "$shard" "$subshard" &
    else
        env $env_export bash "$SCRIPT_DIR/run_shard_test.sh" "$shard" &
    fi
    pid=$!
    SHARD_PIDS[$label]=$pid
    SHARD_LABELS[$pid]=$label
    echo "[$(date +%H:%M:%S)] START: $label (pid=$pid)"
}

# --- Wave 1: Heavy shards (t=0) ---
# These are the longest-running shards. Start them first to maximize
# parallel overlap with shorter shards that start later.

echo "[$(date +%H:%M:%S)] Wave 1: Launching heavy framework shards"

launch_shard framework_tests libraries "OHOS_LIBRARIES_SUBSHARD=1_3"
launch_shard framework_tests libraries "OHOS_LIBRARIES_SUBSHARD=2_3"
launch_shard framework_tests libraries "OHOS_LIBRARIES_SUBSHARD=3_3"
launch_shard framework_tests libraries "OHOS_LIBRARIES_SUBSHARD=non_material OHOS_NON_MATERIAL_SUBSHARD=1_2"
launch_shard framework_tests libraries "OHOS_LIBRARIES_SUBSHARD=non_material OHOS_NON_MATERIAL_SUBSHARD=2_2"
launch_shard framework_tests widgets   "OHOS_WIDGETS_SUBSHARD=1_3"
launch_shard framework_tests widgets   "OHOS_WIDGETS_SUBSHARD=2_3"
launch_shard framework_tests widgets   "OHOS_WIDGETS_SUBSHARD=3_3"

# --- Wave 2: Medium shards (t=2min) ---
# Wait 2 minutes for the heavy shards to finish their initial compilation
# and pub resolution, then start the medium-weight shards.

sleep 120

echo "[$(date +%H:%M:%S)] Wave 2: Launching medium shards"

launch_shard analyze
launch_shard framework_tests misc
launch_shard framework_tests misc2
launch_shard framework_tests misc3
launch_shard framework_tests widget_extras
launch_shard tool_tests_commands

# --- Wave 3: Light shards (t=4min) ---
# Another 2-minute delay, then start the lightest shards that finish
# in seconds to minutes. By this time the heavy shards are deep into
# test execution and no longer competing for compilation resources.

sleep 120

echo "[$(date +%H:%M:%S)] Wave 3: Launching light shards"

launch_shard framework_tests slow
launch_shard tool_tests general
launch_shard snippets
launch_shard tool_host_cross_arch_tests

# --- Wait for all shards ---

FAILED_SHARDS=()
PASSED_SHARDS=()

for label in "${!SHARD_PIDS[@]}"; do
    pid=${SHARD_PIDS[$label]}
    if wait $pid; then
        PASSED_SHARDS+=("$label")
    else
        FAILED_SHARDS+=("$label")
    fi
done

# --- Summary ---

TOTAL_SHARDS=$((${#PASSED_SHARDS[@]} + ${#FAILED_SHARDS[@]}))

echo ""
echo "=========================================="
echo "  Parallel Shard Test Summary"
echo "=========================================="
echo "  Passed: ${#PASSED_SHARDS[@]} / $TOTAL_SHARDS"
for label in "${PASSED_SHARDS[@]}"; do
    echo "    PASS: $label"
done
if [ ${#FAILED_SHARDS[@]} -gt 0 ]; then
    echo "  Failed: ${#FAILED_SHARDS[@]} / $TOTAL_SHARDS"
    for label in "${FAILED_SHARDS[@]}"; do
        echo "    FAIL: $label"
    done
fi
echo ""

if [ ${#FAILED_SHARDS[@]} -gt 0 ]; then
    echo "  RESULT: FAIL (${#FAILED_SHARDS[@]} shard(s) failed)"
    echo "  Logs: $LOG_DIR"
    echo "  Reports: $LOG_DIR/reports/"
    exit 1
fi

echo "  RESULT: ALL PASSED"
echo "  Reports: $LOG_DIR/reports/"
exit 0
