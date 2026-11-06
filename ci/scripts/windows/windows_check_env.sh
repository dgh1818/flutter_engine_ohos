#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# Verify a Windows host running under Git Bash (a WSL bash cannot see the
# native toolchain).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"
source "$SCRIPT_DIR/windows_common.sh"

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) ;;
    *)
        log_error "windows build requires Git Bash on a Windows host (uname: $(uname -s))"
        exit 1
        ;;
esac
log_info "host: $(uname -s) $(uname -m)"
