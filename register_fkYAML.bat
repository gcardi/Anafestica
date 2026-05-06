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

set "BDSCOMMONDIR_DETECTED=%BDSCOMMONDIR%"
if not defined BDSCOMMONDIR_DETECTED (
    if /i "%SCRIPT_NAME%"=="Anafestica" (
        for %%I in ("%SCRIPT_DIR%\..") do set "BDSCOMMONDIR_DETECTED=%%~fI"
    )
)

if not defined TARGET_PATH (
    if defined BDSCOMMONDIR_DETECTED (
        if exist "%BDSCOMMONDIR_DETECTED%\fkYAML\include\fkYAML\node.hpp" (
            set "TARGET_PATH=%BDSCOMMONDIR_DETECTED%\fkYAML\include"
        ) else if exist "%BDSCOMMONDIR_DETECTED%\fkYAML\fkYAML\node.hpp" (
            set "TARGET_PATH=%BDSCOMMONDIR_DETECTED%\fkYAML"
        )
    )
)

if not defined TARGET_PATH (
    echo [FATAL] Unable to locate fkYAML automatically.
    echo         Pass the include root explicitly, for example:
    echo         register_fkYAML.bat "C:\Path\To\fkYAML\include"
    exit /b 1
)

if not defined BDSCOMMONDIR_DETECTED (
    for %%I in ("%TARGET_PATH%\..") do set "BDSCOMMONDIR_DETECTED=%%~fI"
)

for %%I in ("%BDSCOMMONDIR_DETECTED%") do set "BDS_VERSION=%%~nxI"
if not defined BDS_VERSION (
    echo [FATAL] Unable to determine RAD Studio version from BDSCOMMONDIR.
    exit /b 1
)

if not exist "%TARGET_PATH%" (
    echo [FATAL] fkYAML path not found: %TARGET_PATH%
    exit /b 1
)

if not exist "%TARGET_PATH%\fkYAML\node.hpp" (
    echo [FATAL] fkYAML header not found under: %TARGET_PATH%
    echo         Expected: %TARGET_PATH%\fkYAML\node.hpp
    exit /b 1
)

echo Registering fkYAML include path for RAD Studio %BDS_VERSION%
echo Path: %TARGET_PATH%

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%\register_bds_include_path.ps1" -PathToAdd "%TARGET_PATH%" -BdsVersion "%BDS_VERSION%" %DRY_RUN%
exit /b %ERRORLEVEL%

:help
echo Usage:
echo   register_fkYAML.bat [--dry-run] [fkYAMLIncludePath]
echo.
echo If fkYAMLIncludePath is omitted, the script looks for:
echo   %%BDSCOMMONDIR%%\fkYAML\include\fkYAML\node.hpp
echo and falls back to:
echo   %%BDSCOMMONDIR%%\fkYAML\fkYAML\node.hpp
exit /b 0
