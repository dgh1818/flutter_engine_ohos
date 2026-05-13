#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

check_network() {
    urls=(
        "$PUB_HOSTED_URL"
        "$FLUTTER_STORAGE_BASE_URL"
        "https://chrome-infra-packages.appspot.com/prpc/cipd.Repository/GetInstanceURL"
    )

    for url in "${urls[@]}"; do
        status=$(curl -s -o /dev/null -w "%{http_code}" -m 5 "$url" 2>/dev/null || echo " failed")
        if [[ "$status" =~ ^[23] ]]; then
            log_info "✓ $url: $status"
        else
            log_warn "✗ $url: $status"
        fi
    done
}

check_network
