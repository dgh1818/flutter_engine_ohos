#!/usr/bin/env python3
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

import os
import sys
import subprocess
import time
import yaml
from pathlib import Path
from typing import Dict, Any
from logger import Logger


class Runner:
    """Prepare runner that reads YAML config and executes steps"""

    def __init__(self, config_file: str):
        self.config_file = Path(config_file)
        self.root_dir = Path(__file__).parent.parent.parent
        self.work_dir: Path = None  # Will be set after loading config
        self.config: Dict[str, Any] = {}
        self.success_count = 0
        self.fail_count = 0

    def load_config(self) -> bool:
        """Load and parse YAML configuration"""
        Logger.step(f"Parsing Configuration: {self.config_file}")

        if not self.config_file.exists():
            Logger.error(f"Configuration file not found: {self.config_file}")
            return False

        try:
            with open(self.config_file, 'r', encoding='utf-8') as f:
                self.config = yaml.safe_load(f)
        except Exception as e:
            Logger.error(f"Failed to parse YAML: {e}")
            return False

        Logger.info("Configuration loaded successfully")

        # Calculate work_dir based on config
        work_dir_levels = self.config.get('work_dir_levels', 2)
        self.work_dir = self.root_dir
        for _ in range(work_dir_levels):
            self.work_dir = self.work_dir.parent
        Logger.info(f"Set work_dir={self.work_dir} (levels={work_dir_levels})")

        return True

    def setup_env_vars(self) -> bool:
        """Set up environment variables from config"""
        Logger.step("Setting Up Environment Variables")

        # Set PROJECT_DIR first (not from config)
        os.environ['WORK_DIR'] = str(self.work_dir)
        Logger.info(f"Set WORK_DIR={self.work_dir}")

        env_vars = self.config.get('env_vars', {})

        # Set individual environment variables first
        for key, value in env_vars.items():
            if key == 'PATH':
                # PATH is handled specially below
                continue
            os.environ[key] = str(value)
            Logger.info(f"Set {key}={value}")

        # Handle PATH specially - it's a list in YAML
        path_components_config = env_vars.get('PATH', [])

        if path_components_config:
            # Expand environment variable placeholders using os.path.expandvars
            path_components = []
            for path in path_components_config:
                expanded = os.path.expandvars(path)
                path_components.append(expanded)

            # Filter out empty paths and join with colon
            path_components = [p for p in path_components if p and p != '']
            new_path = ':'.join(path_components)

            os.environ['PATH'] = new_path
            Logger.info(f"Set PATH")

        return True

    def execute_step(self, step: Dict[str, Any]) -> bool:
        """Execute a single step"""
        name = step.get('name', 'Unknown Step')
        script = step.get('script', '')
        description = step.get('description', '')
        args = step.get('args', [])
        required = step.get('required', True)

        Logger.step(f"Executing: {name}")

        script_path = self.root_dir / script

        if not script_path.exists():
            Logger.error(f"Script not found: {script_path}")
            if required:
                return False
            else:
                Logger.warn("Skipping optional step")
                return True

        Logger.info(description)

        try:
            cmd = ['bash', str(script_path)]
            if args:
                cmd.extend(args)
                Logger.info(f"Arguments: {args}")

            result = subprocess.run(
                cmd,
                cwd=str(self.work_dir),
                env=os.environ.copy(),
                stdout=None,
                stderr=None,
                text=True
            )

            if result.returncode == 0:
                Logger.info(f"✓ {name} completed")
                return True
            else:
                Logger.error(f"✗ {name} failed with exit code: {result.returncode}")
                return False

        except Exception as e:
            Logger.error(f"✗ {name} failed with exception: {e}")
            return False

    def execute_steps(self, stage: str) -> bool:
        """Execute all steps for a given stage from config"""
        Logger.step(f"Starting {stage} Steps")

        steps = self.config.get(f'{stage}_steps', [])

        if not steps:
            Logger.warn(f"No {stage} steps found in config")
            return True

        self.success_count = 0
        self.fail_count = 0

        for step in steps:
            if self.execute_step(step):
                self.success_count += 1
            else:
                self.fail_count += 1
                required = step.get('required', True)
                if required:
                    Logger.error("Required step failed, stopping execution")
                    return False

        Logger.step("Steps Summary")
        Logger.info(f"Succeeded: {self.success_count}, Failed: {self.fail_count}")

        return True

    def run(self, stage: str) -> int:
        """Main execution flow"""
        start_time = time.time()

        Logger.step(f"Starting {stage} Stage")
        Logger.info(f"Configuration: {self.config_file}")
        Logger.info(f"Date: {time.strftime('%Y-%m-%d %H:%M:%S')}")

        if not self.load_config():
            return 1

        if not self.setup_env_vars():
            return 1

        if not self.execute_steps(stage):
            return 1

        end_time = time.time()
        duration = int(end_time - start_time)

        Logger.step(f"{stage} Completed Successfully")
        Logger.info(f"Total duration: {duration}s")

        return 0


def main():
    """Main entry point"""
    if len(sys.argv) > 1:
        stage = sys.argv[1]
    else:
        Logger.error("Stage not provided")
        sys.exit(1)

    script_dir = Path(__file__).parent.parent.parent
    config_file = script_dir / '.ci_ohos.yaml'

    runner = Runner(str(config_file))
    sys.exit(runner.run(stage))


if __name__ == '__main__':
    main()
