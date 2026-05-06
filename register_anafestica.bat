@echo off
setlocal

if /i "%~1"=="--help" goto :help
if /i "%~1"=="/?" goto :help

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "SCRIPT_DIR=%%~fI"
for %%I in ("%SCRIPT_DIR%") do set "SCRIPT_NAME=%%~nxI"

set "DRY_RUN="
if /i "%~1"=="--dry-run" (
    set "DRY_RUN=-DryRun"
    shift
)

set "TARGET_PATH=%~1"
if not defined TARGET_PATH (
    if /i "%SCRIPT_NAME%"=="Anafestica" (
        set "TARGET_PATH=%SCRIPT_DIR%"
    ) else if defined BDSCOMMONDIR (
        set "TARGET_PATH=%BDSCOMMONDIR%\Anafestica"
    ) else (
        set "TARGET_PATH=%SCRIPT_DIR%"
    )
)

set "BDSCOMMONDIR_DETECTED=%BDSCOMMONDIR%"
if not defined BDSCOMMONDIR_DETECTED (
    for %%I in ("%TARGET_PATH%\..") do set "BDSCOMMONDIR_DETECTED=%%~fI"
)

for %%I in ("%BDSCOMMONDIR_DETECTED%") do set "BDS_VERSION=%%~nxI"
if not defined BDS_VERSION (
    echo [FATAL] Unable to determine RAD Studio version from BDSCOMMONDIR.
    exit /b 1
)

if not exist "%TARGET_PATH%" (
    echo [FATAL] Anafestica path not found: %TARGET_PATH%
    exit /b 1
)

echo Registering Anafestica include path for RAD Studio %BDS_VERSION%
echo Path: %TARGET_PATH%

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\register_bds_include_path.ps1" -PathToAdd "%TARGET_PATH%" -BdsVersion "%BDS_VERSION%" %DRY_RUN%
exit /b %ERRORLEVEL%

:help
echo Usage:
echo   register_anafestica.bat [--dry-run] [AnafesticaPath]
echo.
echo If AnafesticaPath is omitted, the script uses:
echo   1. the current Anafestica folder if this script is inside it
echo   2. %%BDSCOMMONDIR%%\Anafestica if BDSCOMMONDIR is defined
echo   3. the script folder as a fallback
exit /b 0
