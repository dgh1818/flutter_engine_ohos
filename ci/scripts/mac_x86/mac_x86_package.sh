#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# zip dart-sdk (darwin-x64) and artifacts.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"
source "$SCRIPT_DIR/mac_x86_common.sh"

cd "$ENGINE_DIR" || exit 1
init_log_dir

run_logged "$LOG_DIR/zip_dart_sdk.log" python3 src/flutter/attachment/scripts/zip_dart_sdk.py --arch x64 \
    || fail_with_log "$LOG_DIR/zip_dart_sdk.log" "zip_dart_sdk failed"
run_logged "$LOG_DIR/zip_artifacts.log" python3 src/flutter/attachment/scripts/zip_artifacts.py \
    || fail_with_log "$LOG_DIR/zip_artifacts.log" "zip_artifacts failed"
log_info "package done (logs: $LOG_DIR)"
