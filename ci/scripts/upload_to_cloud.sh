#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

upload_to_cloud() {
    WORK_DIR=$(pwd)
    ARCHIVE_DIR="$WORK_DIR/Archive/out"

    if [[ -n "${PR_URL:-}" ]]; then
        log_info "PR detected, skipping upload to cloud"
        exit 0
    fi

    local target_branch="$1"
    local artifacts_url="https://cidownload.openharmony.cn/version/$version_type/$component/$VERSION_DATE/version-$version_type-$component-$VERSION_DATE-$component.tar.gz"

    run_cmd "echo $artifacts_url > artifacts.txt"

    run_cmd "archive cp artifacts.txt cloud://$target_branch/artifacts.txt"
    run_cmd "archive cp $ARCHIVE_DIR/engine.ohos.har.version cloud://$target_branch/engine.ohos.har.version"
}

upload_to_cloud "$@"
