#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# verify an arm64 (Apple Silicon) host.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"

arch="$(uname -m)"
if [[ "$arch" != "arm64" ]]; then
    log_error "mac-arm64 build requires an arm64 host, detected: $arch"
    exit 1
fi
log_info "host arch: $arch"
