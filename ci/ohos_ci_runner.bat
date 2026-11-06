@echo off
rem Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved.
rem Use of this source code is governed by a BSD-style license that can be
rem found in the LICENSE_HW file.
rem
rem Windows entry for ci/ohos_ci_runner.sh: locates Git Bash (a WSL bash
rem cannot see the native toolchain), puts it first on PATH for the whole
rem pipeline, then forwards all arguments to the shell entry.
rem Usage: ci\ohos_ci_runner.bat <stage> [options]

setlocal EnableExtensions

set "GIT_BASH="
if exist "%ProgramFiles%\Git\bin\bash.exe" set "GIT_BASH=%ProgramFiles%\Git\bin\bash.exe"
if not defined GIT_BASH (
    echo [ERROR] Git Bash not found ^(looked in %%ProgramFiles%%\Git\bin^).
    exit /b 1
)

for %%A in ("%GIT_BASH%") do set "GIT_BASH_DIR=%%~dpA"
set "PATH=%GIT_BASH_DIR%;%PATH%"

"%GIT_BASH%" "%~dp0ohos_ci_runner.sh" %*
exit /b %ERRORLEVEL%
