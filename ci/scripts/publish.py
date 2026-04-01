#!/usr/bin/env python3
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.
"""
Publish script based on .ci_ohos.yaml configuration
"""

import argparse
import os
import sys
import subprocess
from pathlib import Path
from typing import Dict, Optional
import yaml

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
from logger import Logger


# OHOS engine type -> output directory mapping
OHOS_ENGINE_TYPE_OUT = {
    'ohos-arm64': 'ohos_debug_arm64',
    'ohos-arm64-profile': 'ohos_profile_arm64',
    'ohos-arm64-release': 'ohos_release_arm64',
    'ohos-x64': 'ohos_debug_x64',
    'ohos-x64-profile': 'ohos_profile_x64',
    'ohos-x64-release': 'ohos_release_x64',
}


def find_file_in_dir(directory: str, filename: str, maxdepth: int = 3) -> Optional[str]:
    """Find file in directory with max depth"""
    dir_path = Path(directory)
    if not dir_path.exists():
        return None

    for depth in range(1, maxdepth + 1):
        for match in dir_path.rglob(filename):
            depth_from_root = len(match.relative_to(dir_path).parts)
            if depth_from_root <= maxdepth:
                return str(match)

    return None


def upload_file_via_archive(file_path: str, target_path: str) -> bool:
    """Upload file via archive cp command"""
    file_name = os.path.basename(file_path)
    Logger.info(f"Uploading: {file_name} -> {target_path}")

    cmd = f'archive cp "{file_path}" "{target_path}"'
    print(f"$ {cmd}")

    try:
        result = subprocess.run(cmd, shell=True, check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        Logger.error(f"Failed to upload {file_name}: {e}")
        return False


def parse_pub_path(pub_path: str, version: str, build_type: str, filename: str) -> str:
    """Parse pub_path pattern and replace placeholders"""
    # Replace placeholders: version, build_type, file
    result = pub_path
    result = result.replace('version', version)
    result = result.replace('build_type', build_type)
    result = result.replace('file', filename)
    return f"pub://{result}"


def load_config(config_file: str) -> Dict:
    """Load YAML configuration file"""
    try:
        with open(config_file, 'r') as f:
            config = yaml.safe_load(f)
        return config
    except Exception as e:
        Logger.error(f"Failed to load config file {config_file}: {e}")
        sys.exit(1)


def get_search_directory(base_engine_dir: str, search_dir: str) -> str:
    """Get the full search directory path"""
    if search_dir == "out":
        return os.path.join(base_engine_dir, "src/out")
    elif search_dir == "engine":
        return base_engine_dir
    else:
        return os.path.join(base_engine_dir, search_dir)


def publish_files(config: Dict, mode: str, engine_dir: str, version: str) -> bool:
    """Publish files based on configuration"""
    if mode not in config.get('publish_files', {}):
        Logger.error(f"Mode '{mode}' not found in publish_files configuration")
        return False

    files_config = config['publish_files'][mode]
    if not files_config:
        Logger.warn(f"No files configured for mode '{mode}'")
        return False

    Logger.step(f"Starting publish for mode: {mode}")
    Logger.info(f"Engine directory: {engine_dir}")
    Logger.info(f"Version: {version}")

    success_count = 0
    total_files = 0

    # Process each file configuration
    for file_info in files_config:
        filename = file_info.get('file', '')
        search_directory = file_info.get('search_directory', 'out')
        search_depth = file_info.get('search_depth', 3)
        pub_path_pattern = file_info.get('pub_path', 'version/build_type/file')

        if not filename:
            Logger.warn("Skipping file config: no filename specified")
            continue

        # Get search directory
        search_dir_path = get_search_directory(engine_dir, search_directory)

        Logger.info(f"Searching for: {filename}")
        Logger.info(f"  Search dir: {search_dir_path}")
        Logger.info(f"  Max depth: {search_depth}")

        # Determine which build types to process
        if search_directory == "out":
            # For out directory, iterate through all build types
            build_types = list(OHOS_ENGINE_TYPE_OUT.keys())
        else:
            # For engine directory, no build type needed
            build_types = [""]

        # Publish for each build type
        for build_type in build_types:
            total_files += 1

            if build_type:
                # out directory scenario
                out_path = OHOS_ENGINE_TYPE_OUT[build_type]
                # Build output directory for this build type
                build_output_dir = os.path.join(engine_dir, "src/out", out_path)
                if not os.path.exists(build_output_dir):
                    Logger.warn(f"Build output directory not found: {build_output_dir}, skipped for {build_type}")
                    continue

                # Search file in this build type directory
                file_path_for_buildtype = find_file_in_dir(build_output_dir, filename, search_depth)
                if not file_path_for_buildtype:
                    Logger.warn(f"File not found for {build_type}: {filename}")
                    continue
                actual_file_path = file_path_for_buildtype
            else:
                # engine directory scenario
                actual_file_path = find_file_in_dir(search_dir_path, filename, search_depth)

                if not actual_file_path:
                    Logger.warn(f"File not found: {filename} in {search_dir_path}")
                    continue

            Logger.info(f"Found: {actual_file_path}")

            # Parse pub path pattern
            target_path = parse_pub_path(pub_path_pattern, version, build_type, filename)

            # Upload the file
            if upload_file_via_archive(actual_file_path, target_path):
                success_count += 1

    Logger.step(f"Publish completed: {success_count}/{total_files} files uploaded successfully")
    return success_count > 0


def main():
    """Main function"""
    parser = argparse.ArgumentParser(
        description='Publish files based on .ci_ohos.yaml configuration'
    )
    parser.add_argument(
        '--mode',
        choices=['incremental', 'full'],
        required=True,
        help='Publish mode (incremental or full)'
    )
    parser.add_argument(
        '--engine-dir',
        required=True,
        help='Path to engine directory (e.g., flutter_flutter/engine)'
    )
    parser.add_argument(
        '--version',
        required=True,
        help='Version string for publishing'
    )

    args = parser.parse_args()

    # Resolve absolute paths
    engine_dir = os.path.abspath(args.engine_dir)

    # Validate engine directory exists
    if not os.path.exists(engine_dir):
        Logger.error(f"Engine directory does not exist: {engine_dir}")
        sys.exit(1)

    script_dir = Path(__file__).parent.parent.parent
    config_file = script_dir / '.ci_ohos.yaml'

    # Validate config file exists
    if not os.path.exists(config_file):
        Logger.error(f"Config file does not exist: {config_file}")
        sys.exit(1)

    # Load configuration
    config = load_config(config_file)

    # Perform publish
    success = publish_files(config, args.mode, engine_dir, args.version)

    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
