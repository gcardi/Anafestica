@echo off
setlocal

where dot >nul 2>nul
if errorlevel 1 (
    echo Graphviz dot.exe was not found on PATH.
    echo Install Graphviz or add its bin directory to PATH, then rerun this script.
    exit /b 1
)

dot -Tsvg "%~dp0class_diagram.dot" -o "%~dp0class_diagram.svg"
if errorlevel 1 (
    echo Failed to render class_diagram.svg.
    exit /b %errorlevel%
)

echo Rendered class_diagram.svg from class_diagram.dot.
exit /b 0
