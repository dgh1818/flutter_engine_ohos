#!/usr/bin/env python3
# Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE_HW file.

import os
import re
import sys
import subprocess
import time
import yaml
from pathlib import Path
from typing import Dict, Any, List, Optional
from logger import Logger


class Runner:
    """Prepare runner that reads YAML config and executes steps"""

    def __init__(self, config_file: Optional[str] = None):
        self.root_dir = Path(__file__).parent.parent.parent
        self.config_file = Path(config_file) if config_file else self.root_dir / '.ci_ohos.yaml'
        self.work_dir: Path = None  # Will be set after loading config
        self.config: Dict[str, Any] = {}
        self.success_count = 0
        self.fail_count = 0
        self._changed_files_cache: Optional[List[str]] = None

    @staticmethod
    def _expand_braces(pattern: str) -> List[str]:
        """Expand brace patterns like {h,c,cc} into multiple patterns."""
        match = re.search(r'\{([^}]+)\}', pattern)
        if not match:
            return [pattern]
        options = match.group(1).split(',')
        pre, post = pattern[:match.start()], pattern[match.end():]
        result: List[str] = []
        for opt in options:
            result.extend(Runner._expand_braces(pre + opt + post))
        return result

    @staticmethod
    def _glob_to_regex(pattern: str) -> str:
        """Convert a glob pattern (with ** support) to a regex string."""
        parts: List[str] = []
        i = 0
        while i < len(pattern):
            if pattern[i:i + 2] == '**':
                i += 2
                if i < len(pattern) and pattern[i] == '/':
                    parts.append('(?:.*/)?')
                    i += 1
                else:
                    parts.append('.*')
            elif pattern[i] == '*':
                parts.append('[^/]*')
                i += 1
            elif pattern[i] == '?':
                parts.append('[^/]')
                i += 1
            else:
                parts.append(re.escape(pattern[i]))
                i += 1
        return '^' + ''.join(parts) + '$'

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

        # Set TARGET_BRANCH from config (used by runIf and as $TARGET_BRANCH in step args)
        target_branch = self._get_target_branch()
        if not target_branch:
            Logger.error("Missing 'target_branch' in .ci_ohos.yaml")
            return False
        os.environ['TARGET_BRANCH'] = target_branch
        Logger.info(f"Set TARGET_BRANCH={target_branch}")

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

        # Check runIf condition (incremental trigger)
        run_if = step.get('runIf')
        if run_if and not self._should_run_step(run_if):
            Logger.info(f"⏭ {name} skipped (runIf: no matching changed files)")
            return True

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
                expanded_args = [os.path.expandvars(str(arg)) for arg in args]
                cmd.extend(expanded_args)
                Logger.info(f"Arguments: {expanded_args}")

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

    # ------------------------------------------------------------------
    # runIf support: conditional step execution based on changed files
    # ------------------------------------------------------------------

    def _get_target_branch(self) -> Optional[str]:
        """Get the target branch for diff comparison from config, or None."""
        return self.config.get('target_branch')

    def _get_changed_files(self) -> Optional[List[str]]:
        """Get list of files changed relative to the target branch.

        Returns paths relative to the flutter_flutter repo root, or None if
        they could not be determined (callers should fail-open).
        Results are cached for the duration of a run.
        """
        if self._changed_files_cache is not None:
            return self._changed_files_cache

        target_branch = self._get_target_branch()
        repo_dir = self.work_dir / 'third_party' / 'flutter_flutter'
        diff_ref = f'gitcode/{target_branch}'

        try:
            result = subprocess.run(
                ['git', 'diff', '--name-only', '--diff-filter=d', f'{diff_ref}...HEAD'],
                cwd=str(repo_dir),
                capture_output=True,
                text=True,
                timeout=30,
            )
            if result.returncode == 0:
                files = [f.strip() for f in result.stdout.splitlines() if f.strip()]
                self._changed_files_cache = files
                Logger.info(f"Changed files ({len(files)}) vs {diff_ref}")
                return files
            else:
                Logger.warn(f"git diff failed (rc={result.returncode}): {result.stderr.strip()}")
        except Exception as e:
            Logger.warn(f"Failed to get changed files: {e}")

        self._changed_files_cache = None
        return self._changed_files_cache

    def _should_run_step(self, run_if) -> bool:
        """Check if a step should run based on the runIf glob condition.

        - In daily builds (no PR_URL), always returns True.
        - In PR builds, returns True only if at least one changed file
          matches any of the runIf glob patterns.
        """
        # Daily build: always run
        if not os.environ.get('PR_URL'):
            return True

        changed_files = self._get_changed_files()
        if changed_files is None:
            Logger.warn("Changed files unknown; running step (fail-open)")
            return True
        if not changed_files:
            return False

        patterns = run_if if isinstance(run_if, list) else [run_if]
        for file_path in changed_files:
            if self._match_any_pattern(file_path, patterns):
                return True

        return False

    def _match_any_pattern(self, file_path: str, patterns: List[str]) -> bool:
        """Check if file_path matches any glob pattern (with brace expansion)."""
        for pattern in patterns:
            for expanded in self._expand_braces(pattern):
                regex = self._glob_to_regex(expanded)
                if re.match(regex, file_path):
                    return True
        return False


def main():
    """Main entry point.

    Usage: runner.py <stage>
      stage - preparation | compilation | test | integration_gate |
              daily_integration | daily_memory | daily_perf | customer | mac_arm64_build
    """
    if len(sys.argv) > 1:
        stage = sys.argv[1]
    else:
        Logger.error("Stage not provided")
        sys.exit(1)

    runner = Runner()
    sys.exit(runner.run(stage))


if __name__ == '__main__':
    main()
