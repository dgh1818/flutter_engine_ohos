#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# Generic in-place file replacement helper.
# Args: $1=file path relative to repo root, $2=sed replacement expression
replace_in_file() {
    local file_rel_path="$1"
    local sed_expr="$2"

    # repo root, self-locating: works under the CI work_dir/third_party
    # layout and a plain local checkout alike
    local repo_root="$(cd "$SCRIPT_DIR/../.." && pwd)"
    local target_file="$repo_root/$file_rel_path"

    if [[ -f "$target_file" ]]; then
        # BSD sed (macOS) needs an explicit empty -i suffix
        if [[ "$(uname)" == "Darwin" ]]; then
            run_cmd "sed -i '' '$sed_expr' $target_file"
        else
            run_cmd "sed -i '$sed_expr' $target_file"
        fi
    fi
}

replace_in_file "$@"
