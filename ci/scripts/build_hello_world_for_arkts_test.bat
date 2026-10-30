@echo off
setlocal
rem Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
rem Use of this source code is governed by a BSD-style license that can be
rem found in the LICENSE_HW file.
rem
rem Windows port of build_hello_world_for_arkts_test.sh:
rem   Build hello_world and stage its products under the engine's
rem   flutter_embedding for ArkTS unit testing.
rem
rem Steps:
rem   1. Invokes bin\flutter.bat to package hello_world with main_ohos.dart
rem      entry in debug mode (matches the ohosTest HAP debug build).
rem   2. Copies native .so files into
rem      engine\src\flutter\shell\platform\ohos\flutter_embedding\flutter\libs\arm64-v8a
rem   3. Copies flutter_assets into
rem      engine\src\flutter\shell\platform\ohos\flutter_embedding\flutter\src\ohosTest\resources\rawfile\flutter_assets

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..\..") do set "WORK_DIR=%%~fI"
cd /d "%WORK_DIR%"

set "HELLO_WORLD_DIR=%WORK_DIR%\examples\hello_world"
if not exist "%HELLO_WORLD_DIR%" (
    echo [ERROR] hello_world directory not found: %HELLO_WORLD_DIR%
    goto :fail
)

set "FLUTTER_BIN=%WORK_DIR%\bin\flutter.bat"
if not exist "%FLUTTER_BIN%" (
    echo [ERROR] flutter binary not found: %FLUTTER_BIN%
    goto :fail
)

rem The ohosTest HAP is built in debug mode by DevEco, so engine so and
rem flutter_assets must be debug/JIT products (kernel_blob.bin + debug
rem libflutter.so with embedded core snapshot). Profile products crash with
rem SEGV: the DartVM cannot bootstrap and Shell::Create leaves platform_view_
rem null (see OHOSShellHolder ctor).
set "BUILD_MODE=debug"
echo [INFO] Building hello_world in %BUILD_MODE% mode
echo $ (cd examples\hello_world ^&^& flutter build hap --%BUILD_MODE% -t lib/main_ohos.dart)
pushd "%HELLO_WORLD_DIR%"
call "%FLUTTER_BIN%" build hap --%BUILD_MODE% -t lib/main_ohos.dart
set "BUILD_RC=%ERRORLEVEL%"
popd
if not "%BUILD_RC%"=="0" (
    echo [ERROR] flutter build hap failed with rc=%BUILD_RC%
    goto :fail
)

set "EMBEDDING_DIR=%WORK_DIR%\engine\src\flutter\shell\platform\ohos\flutter_embedding\flutter"
set "LIBS_SRC=%HELLO_WORLD_DIR%\ohos\entry\build\default\intermediates\libs\default\arm64-v8a"
set "LIBS_DST=%EMBEDDING_DIR%\libs\arm64-v8a"
set "ASSETS_SRC=%HELLO_WORLD_DIR%\ohos\entry\src\main\resources\rawfile\flutter_assets"
set "ASSETS_DST=%EMBEDDING_DIR%\src\ohosTest\resources\rawfile\flutter_assets"

if not exist "%LIBS_SRC%" (
    echo [ERROR] hello_world native libs not found: %LIBS_SRC%
    goto :fail
)
echo [INFO] Staging hello_world native libs into %LIBS_DST%
if not exist "%LIBS_DST%" mkdir "%LIBS_DST%"
copy /Y "%LIBS_SRC%\*.so" "%LIBS_DST%\" >nul
if errorlevel 1 (
    echo [ERROR] failed to copy native libs
    goto :fail
)

if not exist "%ASSETS_SRC%" (
    echo [ERROR] hello_world flutter_assets not found: %ASSETS_SRC%
    goto :fail
)
echo [INFO] Staging hello_world flutter_assets into %ASSETS_DST%
if not exist "%ASSETS_DST%" mkdir "%ASSETS_DST%"
xcopy "%ASSETS_SRC%" "%ASSETS_DST%" /E /I /Y >nul
if errorlevel 1 (
    echo [ERROR] failed to copy flutter_assets
    goto :fail
)

echo [INFO] Done.
echo.
echo ============ RESULT: SUCCESS ============
pause
endlocal
exit /b 0

:fail
echo.
echo ============ RESULT: FAILED ============
pause
endlocal
exit /b 1
