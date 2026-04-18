@echo off
setlocal enabledelayedexpansion

REM ---------------------------------------------------------------
REM  Build and run Anafestica tests for Embarcadero compilers
REM  using MSBuild.
REM
REM  Usage:  test_all.bat [options] [toolchain ...]
REM
REM  Options:
REM    --no-build       Skip the build step; run existing executables
REM    --rebuild        Force a clean rebuild (Clean + Build)
REM    --stop-on-error  Abort immediately on first failure
REM    --verbose-build  Use normal MSBuild verbosity and show compiler commands
REM
REM  Toolchains: bcc32c  bcc64  bcc64x   (default: all three)
REM
REM  Examples:
REM    test_all.bat                    Build + run all three
REM    test_all.bat bcc64x             Build + run bcc64x only
REM    test_all.bat --rebuild bcc32c bcc64
REM    test_all.bat --no-build bcc64x  Run bcc64x without building
REM ---------------------------------------------------------------

set "ROOT=%~dp0."
set BUILD=1
set "MSBUILD_TARGET=Make"
set "MSBUILD_VERBOSITY=minimal"
set STOP_ON_ERROR=0
set DO_BCC32C=0
set DO_BCC64=0
set DO_BCC64X=0
set TOOLCHAIN_SELECTED=0

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--no-build"      ( set BUILD=0& shift & goto :parse_args )
if /i "%~1"=="--rebuild"       ( set "MSBUILD_TARGET=Clean,Build"& shift & goto :parse_args )
if /i "%~1"=="--stop-on-error" ( set STOP_ON_ERROR=1& shift & goto :parse_args )
if /i "%~1"=="--verbose-build" ( set "MSBUILD_VERBOSITY=normal"& shift & goto :parse_args )
if /i "%~1"=="bcc32c"  ( set DO_BCC32C=1& set TOOLCHAIN_SELECTED=1& shift & goto :parse_args )
if /i "%~1"=="bcc64"   ( set DO_BCC64=1& set TOOLCHAIN_SELECTED=1& shift & goto :parse_args )
if /i "%~1"=="bcc64x"  ( set DO_BCC64X=1& set TOOLCHAIN_SELECTED=1& shift & goto :parse_args )
echo Unknown option: %~1
exit /b 1
:args_done

REM  No toolchain specified => enable all
if !TOOLCHAIN_SELECTED! equ 0 (
    set DO_BCC32C=1
    set DO_BCC64=1
    set DO_BCC64X=1
)

REM --- Set up the Embarcadero environment -------------------------
set "RSVARS="
if defined BDS set "RSVARS=!BDS!\bin\rsvars.bat"
if not defined RSVARS set "RSVARS=!ProgramFiles(x86)!\Embarcadero\Studio\37.0\bin\rsvars.bat"
if not exist "!RSVARS!" (
    echo [FATAL] rsvars.bat not found: !RSVARS!
    exit /b 1
)
call "!RSVARS!"
if errorlevel 1 (
    echo [FATAL] rsvars.bat failed
    exit /b 1
)

set BUILD_OK=1
set BUILD_FAILED=0
set PASSED=0
set FAILED=0
set SKIPPED=0

REM ================================================================
REM  Phase 1 — Build
REM ================================================================
if !BUILD! equ 1 (
    if !DO_BCC32C! equ 1  call :build_one "bcc32c - Win32"  "Test\TestBcc32c\anafestica_test_bcc32c.cbproj" Win32
    if !DO_BCC64! equ 1   call :build_one "bcc64  - Win64"  "Test\TestBcc64\anafestica_test_bcc64.cbproj"   Win64
    if !DO_BCC64X! equ 1  call :build_one "bcc64x - Win64x" "Test\TestBcc64x\anafestica_test_bcc64x.cbproj" Win64x
)

REM  If any build failed, skip the test phase
if !BUILD_OK! equ 0 goto :summary

REM ================================================================
REM  Phase 2 — Run tests
REM ================================================================
if !DO_BCC32C! equ 1  call :test_one "bcc32c - Win32"  "Test\TestBcc32c\Win32\Release\anafestica_test_bcc32c.exe"
if !DO_BCC64! equ 1   call :test_one "bcc64  - Win64"  "Test\TestBcc64\Win64\Release\anafestica_test_bcc64.exe"
if !DO_BCC64X! equ 1  call :test_one "bcc64x - Win64x" "Test\TestBcc64x\Win64x\Release\anafestica_test_bcc64x.exe"

REM ================================================================
REM  Summary
REM ================================================================
:summary
echo.
echo ===============================================================
set /a TOTAL=PASSED+FAILED+SKIPPED+BUILD_FAILED
echo  Results: !PASSED! passed, !FAILED! failed, !BUILD_FAILED! build errors, !SKIPPED! skipped  (total !TOTAL!)
if !FAILED! equ 0 if !BUILD_FAILED! equ 0 (
    echo  ALL TESTS PASSED
) else (
    echo  THERE WERE FAILURES
)
echo ===============================================================
set /a EXIT_CODE=FAILED+BUILD_FAILED
exit /b !EXIT_CODE!

REM ================================================================
:build_one
REM   %~1 = label   %~2 = cbproj   %~3 = platform
REM ================================================================
set "LABEL=%~1"
set "CBPROJ=%~2"
set "PLAT=%~3"

echo.
echo ===============================================================
echo  Building: !LABEL!
echo ===============================================================
echo --- MSBuild: !CBPROJ! [!PLAT!^|Release, /t:!MSBUILD_TARGET!, /v:!MSBUILD_VERBOSITY!] ---

set "MSBUILD_LOG=!TEMP!\anafestica_msbuild_!PLAT!.log"
msbuild "!ROOT!\!CBPROJ!" /p:Config=Release /p:Platform=!PLAT! /t:!MSBUILD_TARGET! /v:!MSBUILD_VERBOSITY! /nologo > "!MSBUILD_LOG!" 2>&1
set "MSBUILD_ERR=!ERRORLEVEL!"
findstr /v /c:"MSB4056" "!MSBUILD_LOG!"
del "!MSBUILD_LOG!" >nul 2>&1

if !MSBUILD_ERR! neq 0 (
    echo [FAIL] Build failed for !LABEL!
    set /a BUILD_FAILED+=1
    set BUILD_OK=0
    if !STOP_ON_ERROR! equ 1 (
        echo Aborting due to --stop-on-error
        goto :summary
    )
) else (
    echo [OK] Build succeeded for !LABEL!
)
exit /b 0

REM ================================================================
:test_one
REM   %~1 = label   %~2 = exe
REM ================================================================
set "LABEL=%~1"
set "EXE=%~2"

echo.
echo ===============================================================
echo  Testing: !LABEL!
echo ===============================================================

if not exist "!ROOT!\!EXE!" (
    echo [SKIP] Executable not found: !EXE!
    set /a SKIPPED+=1
    exit /b 0
)

echo --- Running: !EXE! ---
"!ROOT!\!EXE!"
if errorlevel 1 (
    echo [FAIL] Tests failed for !LABEL!
    set /a FAILED+=1
    if !STOP_ON_ERROR! equ 1 (
        echo Aborting due to --stop-on-error
        goto :summary
    )
) else (
    echo [PASS] !LABEL!
    set /a PASSED+=1
)
exit /b 0
