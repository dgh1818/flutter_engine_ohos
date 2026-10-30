#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
#
# sync repo + engine deps (git fetch + reset --hard, gclient sync).
# reset --hard pins the local branch to the remote tip: local commits and
# uncommitted changes are discarded, so the tree always matches the remote
# branch exactly (unmerged local code, e.g. these CI scripts, is lost on
# sync - expected; re-apply from a patch backup if needed).

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../common.sh"
source "$SCRIPT_DIR/mac_arm64_common.sh"

init_log_dir
cd "$REPO_ROOT" || exit 1

branch="$(git rev-parse --abbrev-ref HEAD)"
run_logged "$LOG_DIR/sync_git_fetch.log" git fetch origin "$branch" \
    || fail_with_log "$LOG_DIR/sync_git_fetch.log" "git fetch failed"
run_logged "$LOG_DIR/sync_git_reset.log" git reset --hard FETCH_HEAD \
    || fail_with_log "$LOG_DIR/sync_git_reset.log" "git reset failed"
run_logged "$LOG_DIR/sync_gclient.log" gclient sync -f -D -v \
    || fail_with_log "$LOG_DIR/sync_gclient.log" "gclient sync failed"
log_info "sync done (logs: $LOG_DIR)"
