#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# upload dart-sdk + artifacts to OBS (upload_dart_sdk.py,
# auto_compile.py per engine/upload_template.json). Needs OBS credentials
# (AccessKeyID/SecretAccessKey).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"
source "$SCRIPT_DIR/mac_x86_common.sh"

# Publishing is opt-in: only `ohos_ci_runner.sh mac_x86_build --publish`
# (MAC_X86_PUBLISH=1) uploads; default runs build & package only.
if [[ "${MAC_X86_PUBLISH:-0}" != "1" ]]; then
    log_info "publish skipped (pass --publish to ohos_ci_runner.sh to upload)"
    exit 0
fi

# The runner is a non-interactive sh (no shell aliases), so a bare python3
# may resolve to an interpreter without the obs package - probe first.
OBS_PYTHON_CANDIDATES=(python3 /usr/bin/python3 /usr/local/bin/python3)
OBS_PYTHON=""
for candidate in "${OBS_PYTHON_CANDIDATES[@]}"; do
    if command -v "$candidate" >/dev/null 2>&1 \
        && "$candidate" -c "from obs import ObsClient" >/dev/null 2>&1; then
        OBS_PYTHON="$candidate"
        break
    fi
done
if [[ -z "$OBS_PYTHON" ]]; then
    log_error "no python interpreter with the esdk-obs-python package"
    exit 1
fi
log_info "publish python: $OBS_PYTHON"

cd "$ENGINE_DIR" || exit 1
init_log_dir

# which artifacts flavor to upload for which target (auto_compile.py);
# untracked, created here and removed by the EXIT trap on every exit
# path, including the fail_with_log failures below.
trap 'rm -f "$ENGINE_DIR/upload_template.json"' EXIT
cat > upload_template.json <<'EOF'
{
    "tag": "",
    "files": {
        "darwin-x64": "artifacts.zip"
    }
}
EOF

run_logged "$LOG_DIR/upload_dart_sdk.log" "$OBS_PYTHON" src/flutter/attachment/scripts/upload_dart_sdk.py --arch x64 \
    || fail_with_log "$LOG_DIR/upload_dart_sdk.log" "upload_dart_sdk failed"
# upload_dart_sdk.py exits 0 even when OBS rejects the upload (the util only
# prints 'Put Content Failed' / 'errorCode:'), so re-check the log to make
# such failures visible to the pipeline.
if grep -qE 'Put Content Failed|errorCode:' "$LOG_DIR/upload_dart_sdk.log"; then
    fail_with_log "$LOG_DIR/upload_dart_sdk.log" "upload_dart_sdk upload rejected by OBS"
fi
run_logged "$LOG_DIR/auto_compile.log" "$OBS_PYTHON" src/flutter/attachment/scripts/auto_compile.py \
    || fail_with_log "$LOG_DIR/auto_compile.log" "auto_compile failed"
# auto_compile.py swallows upload exceptions (bare except: log only) and also
# exits 0, so re-check its log for the same OBS failure markers.
if grep -qE 'Put Content Failed|errorCode:' "$LOG_DIR/auto_compile.log"; then
    fail_with_log "$LOG_DIR/auto_compile.log" "auto_compile upload rejected by OBS"
fi
log_info "publish done (logs: $LOG_DIR)"
