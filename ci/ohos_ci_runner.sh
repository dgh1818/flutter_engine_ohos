#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# OHOS CI Runner
#
# Unified entry point for CI stages. Delegates to runner.py with the
# appropriate stage name based on the sub-command provided.
#
# Usage: sh ./third_party/flutter_flutter/ci/ohos_ci_runner.sh <stage>
#   stage: prepare           -> preparation
#          compile           -> compilation
#          test              -> test
#          integration_gate  -> integration_gate
#          daily_integration -> daily_integration
#          daily_memory      -> daily_memory
#          daily_perf        -> daily_perf
#          customer          -> customer
#          mac_arm64_build   -> mac arm64 build & package; add --publish to
#                               also upload to OBS (default: no upload)
#          mac_x86_build     -> mac x86 build & package; add --publish to
#                               also upload to OBS (default: no upload)
#          windows_build     -> windows x64 build & package; add --publish to
#                               also upload to OBS (default: no upload);
#                               --no-sync skips the sync step

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="$SCRIPT_DIR/scripts/runner.py"

STAGE="$1"

if [[ -z "$STAGE" ]]; then
    echo "[ERROR] Stage not provided"
    echo "Usage: $0 <prepare|compile|test|integration_gate|daily_integration|daily_memory|daily_perf|customer|mac_arm64_build|mac_x86_build|windows_build>"
    exit 1
fi

case "$STAGE" in
    prepare|preparation)
        STAGE="preparation"
        ;;
    compile|compilation)
        STAGE="compilation"
        ;;
    test)
        STAGE="test"
        ;;
    integration_gate)
        STAGE="integration_gate"
        ;;
    daily_integration)
        STAGE="daily_integration"
        ;;
    daily_memory)
        STAGE="daily_memory"
        ;;
    daily_perf)
        STAGE="daily_perf"
        ;;
    customer)
        STAGE="customer"
        ;;
    mac_arm64_build|mac-arm64-build)
        STAGE="mac_arm64_build"
        # One shared log dir per pipeline run: every mac_arm64 step writes
        # its logs into engine/outputs/mac_arm64/<run-id>/
        MAC_ARM64_RUN_ID="$(date +%Y%m%d-%H%M%S)"
        export MAC_ARM64_RUN_ID
        # Publishing is opt-in: only --publish uploads to OBS. Always set the
        # flag explicitly so a stale value in the environment cannot leak in.
        MAC_ARM64_PUBLISH=0
        for arg in "${@:2}"; do
            case "$arg" in
                --publish)
                    MAC_ARM64_PUBLISH=1
                    ;;
                *)
                    echo "[ERROR] Unknown option '$arg' (only --publish is supported)"
                    exit 1
                    ;;
            esac
        done
        export MAC_ARM64_PUBLISH
        ;;
    mac_x86_build|mac-x86-build)
        STAGE="mac_x86_build"
        # One shared log dir per pipeline run: every mac_x86 step writes
        # its logs into engine/outputs/mac_x86/<run-id>/
        MAC_X86_RUN_ID="$(date +%Y%m%d-%H%M%S)"
        export MAC_X86_RUN_ID
        # Publishing is opt-in: only --publish uploads to OBS. Always set the
        # flag explicitly so a stale value in the environment cannot leak in.
        MAC_X86_PUBLISH=0
        for arg in "${@:2}"; do
            case "$arg" in
                --publish)
                    MAC_X86_PUBLISH=1
                    ;;
                *)
                    echo "[ERROR] Unknown option '$arg' (only --publish is supported)"
                    exit 1
                    ;;
            esac
        done
        export MAC_X86_PUBLISH
        ;;
    windows_build|windows-build)
        STAGE="windows_build"
        # One shared log dir per pipeline run: every windows step writes
        # its logs into engine/outputs/windows/<run-id>/
        WINDOWS_RUN_ID="$(date +%Y%m%d-%H%M%S)"
        export WINDOWS_RUN_ID
        # Publishing is opt-in: only --publish uploads to OBS. Always set
        # the flags explicitly so a stale value cannot leak in.
        WINDOWS_PUBLISH=0
        WINDOWS_NO_SYNC=0
        for arg in "${@:2}"; do
            case "$arg" in
                --publish)
                    WINDOWS_PUBLISH=1
                    ;;
                --no-sync)
                    WINDOWS_NO_SYNC=1
                    ;;
                *)
                    echo "[ERROR] Unknown option '$arg' (only --publish and --no-sync are supported)"
                    exit 1
                    ;;
            esac
        done
        export WINDOWS_PUBLISH WINDOWS_NO_SYNC
        ;;
    *)
        echo "[ERROR] Unknown stage: $STAGE"
        echo "Usage: $0 <prepare|compile|test|integration_gate|daily_integration|daily_memory|daily_perf|customer|mac_arm64_build|mac_x86_build|windows_build>"
        exit 1
        ;;
esac

if ! command -v python3 &> /dev/null; then
    echo "[ERROR] Python 3 is required but not installed."
    exit 1
fi

python3 -c "import yaml" 2>/dev/null || {
    echo "[ERROR] PyYAML is not installed. Run: pip install pyyaml"
    exit 1
}

python3 "$RUNNER" "$STAGE"
