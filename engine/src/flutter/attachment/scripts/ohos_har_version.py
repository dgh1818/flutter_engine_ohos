#!/usr/bin/env python3
# Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

import argparse
import json
import os
import re
import subprocess
import sys


def run_git_short_commit(path):
    result = subprocess.run(
        ["git", "-C", path, "rev-parse", "--short", "HEAD"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "git rev-parse failed")
    return result.stdout.strip()


def get_base_version_from_package_file(package_file_path):
    with open(package_file_path, "r", encoding="utf-8") as f:
        config = json.load(f)

    version = config.get("version", "")
    if not isinstance(version, str):
        raise RuntimeError(f"Invalid version field in {package_file_path}")

    match = re.search(r"\d+(?:\.\d+)*", version)
    if not match:
        raise RuntimeError(
            f"Could not extract semantic version from '{version}' in {package_file_path}"
        )
    return match.group(0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--package-file",
        required=True,
        help="Path to the OHOS package manifest (oh-package.json5) to read the base version from.",
    )
    parser.add_argument(
        "--git-dir",
        default=None,
        help="Repository directory to use for git rev-parse. Defaults to engine/src (GN source root).",
    )
    args = parser.parse_args()

    package_file_path = os.path.abspath(args.package_file)
    git_dir = args.git_dir
    if git_dir is None:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        git_dir = os.path.dirname(os.path.dirname(os.path.dirname(script_dir)))

    base_version = get_base_version_from_package_file(package_file_path)
    commit = run_git_short_commit(git_dir)

    print(json.dumps(f"{base_version}-{commit}"))


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(str(e), file=sys.stderr)
        sys.exit(1)
