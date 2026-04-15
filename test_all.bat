@echo off
setlocal enabledelayedexpansion

REM ---------------------------------------------------------------
REM  Build and run Anafestica tests for all three Embarcadero
REM  compilers using MSBuild.
REM
REM  Usage:  test_all.bat [--no-build] [--stop-on-error]
REM
REM    --no-build       Skip the build step; run existing executables
REM    --stop-on-error  Abort immediately on first failure
REM ---------------------------------------------------------------

set "ROOT=%~dp0."
set BUILD=1
set STOP_ON_ERROR=0

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--no-build"      ( set BUILD=0& shift & goto :parse_args )
if /i "%~1"=="--stop-on-error" ( set STOP_ON_ERROR=1& shift & goto :parse_args )
echo Unknown option: %~1
exit /b 1
:args_done

REM --- Set up the Embarcadero environment -------------------------
if defined BDS (
    set "RSVARS=%BDS%\bin\rsvars.bat"
) else (
    set "RSVARS=%ProgramFiles(x86)%\Embarcadero\Studio\37.0\bin\rsvars.bat"
)
if not exist "%RSVARS%" (
    echo [FATAL] rsvars.bat not found: %RSVARS%
    exit /b 1
)
call "%RSVARS%"
if errorlevel 1 (
    echo [FATAL] rsvars.bat failed
    exit /b 1
)

set PASSED=0
set FAILED=0
set SKIPPED=0

REM  Each entry:  label  cbproj-relative-path  platform  exe-relative-path
call :run_one "bcc32c - Win32"  "Test\TestBcc32c\anafestica_test_bcc32c.cbproj" Win32  "Test\TestBcc32c\Win32\Release\anafestica_test_bcc32c.exe"
call :run_one "bcc64  - Win64"  "Test\TestBcc64\anafestica_test_bcc64.cbproj"   Win64  "Test\TestBcc64\Win64\Release\anafestica_test_bcc64.exe"
call :run_one "bcc64x - Win64x" "Test\TestBcc64x\anafestica_test_bcc64x.cbproj" Win64x "Test\TestBcc64x\Win64x\Release\anafestica_test_bcc64x.exe"

REM --- Summary ----------------------------------------------------
echo.
echo ===============================================================
set /a TOTAL=PASSED+FAILED+SKIPPED
echo  Results: !PASSED! passed, !FAILED! failed, !SKIPPED! skipped  (total !TOTAL!)
if !FAILED! equ 0 (
    echo  ALL TESTS PASSED
) else (
    echo  THERE WERE FAILURES
)
echo ===============================================================
exit /b !FAILED!

REM ================================================================
:run_one
REM   %~1 = label   %~2 = cbproj   %~3 = platform   %~4 = exe
REM ================================================================
set "LABEL=%~1"
set "CBPROJ=%~2"
set "PLAT=%~3"
set "EXE=%~4"

echo.
echo ===============================================================
echo  %LABEL%
echo ===============================================================

REM --- Build (optional) -------------------------------------------
if !BUILD! equ 1 (
    echo --- MSBuild: !CBPROJ! [!PLAT!^|Release] ---
    msbuild "!ROOT!\!CBPROJ!" /p:Config=Release /p:Platform=!PLAT! /t:Build /v:minimal /nologo
    if errorlevel 1 (
        echo [FAIL] Build failed for !LABEL!
        set /a FAILED+=1
        if !STOP_ON_ERROR! equ 1 (
            echo Aborting due to --stop-on-error
            goto :summary_and_exit
        )
        exit /b 0
    )
)

REM --- Run --------------------------------------------------------
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
        goto :summary_and_exit
    )
) else (
    echo [PASS] !LABEL!
    set /a PASSED+=1
)
exit /b 0

:summary_and_exit
echo.
echo ===============================================================
set /a TOTAL=PASSED+FAILED+SKIPPED
echo  Results: !PASSED! passed, !FAILED! failed, !SKIPPED! skipped  (total !TOTAL!)
echo  ABORTED
echo ===============================================================
exit /b !FAILED!
