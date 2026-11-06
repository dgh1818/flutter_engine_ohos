#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# verify an x86_64 host.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"

os="$(uname)"
if [[ "$os" != "Darwin" ]]; then
    log_error "mac-x86 build requires macOS, detected: $os"
    exit 1
fi

arch="$(uname -m)"
if [[ "$arch" != "x86_64" ]]; then
    log_error "mac-x86 build requires an x86_64 host, detected: $arch"
    exit 1
fi
log_info "host arch: $arch"
