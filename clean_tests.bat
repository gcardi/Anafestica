@echo off
setlocal enabledelayedexpansion

REM ---------------------------------------------------------------
REM  Clean build artifacts for Anafestica test suites.
REM
REM  Usage:  clean_tests.bat [toolchain ...]
REM
REM  Toolchains: bcc32c  bcc64  bcc64x   (default: all three)
REM
REM  Examples:
REM    clean_tests.bat                 Clean all three
REM    clean_tests.bat bcc64x          Clean bcc64x only
REM    clean_tests.bat bcc32c bcc64    Clean bcc32c and bcc64
REM ---------------------------------------------------------------

set "ROOT=%~dp0."
set DO_BCC32C=0
set DO_BCC64=0
set DO_BCC64X=0
set TOOLCHAIN_SELECTED=0

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="bcc32c"  ( set DO_BCC32C=1& set TOOLCHAIN_SELECTED=1& shift & goto :parse_args )
if /i "%~1"=="bcc64"   ( set DO_BCC64=1& set TOOLCHAIN_SELECTED=1& shift & goto :parse_args )
if /i "%~1"=="bcc64x"  ( set DO_BCC64X=1& set TOOLCHAIN_SELECTED=1& shift & goto :parse_args )
echo Unknown option: %~1
exit /b 1
:args_done

REM  No toolchain specified => clean all
if !TOOLCHAIN_SELECTED! equ 0 (
    set DO_BCC32C=1
    set DO_BCC64=1
    set DO_BCC64X=1
)

set CLEANED=0

if !DO_BCC32C! equ 1  call :clean_one "bcc32c - Win32"  "Test\TestBcc32c\Win32"
if !DO_BCC64! equ 1   call :clean_one "bcc64  - Win64"  "Test\TestBcc64\Win64"
if !DO_BCC64X! equ 1  call :clean_one "bcc64x - Win64x" "Test\TestBcc64x\Win64x"

echo.
echo Done. !CLEANED! toolchain(s) cleaned.
exit /b 0

REM ================================================================
:clean_one
REM   %~1 = label   %~2 = output directory (relative to ROOT)
REM ================================================================
set "LABEL=%~1"
set "OUTDIR=!ROOT!\%~2"

echo.
echo --- Cleaning: !LABEL! ---

if not exist "!OUTDIR!" (
    echo   Nothing to clean.
    exit /b 0
)

rd /s /q "!OUTDIR!"
if errorlevel 1 (
    echo   [WARN] Could not fully remove !OUTDIR!
) else (
    echo   Removed !OUTDIR!
)
set /a CLEANED+=1
exit /b 0
