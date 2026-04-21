@echo off
setlocal

rem Usage:
rem   Setup.bat            -> default vs2026
rem   Setup.bat 2022       -> generate VS2022 project
rem   Setup.bat 2026       -> generate VS2026 project
rem   Setup.bat vs2022
rem   Setup.bat vs2026

set "ACTION=vs2026"

if /I "%~1"=="2022"  set "ACTION=vs2022"
if /I "%~1"=="2026"  set "ACTION=vs2026"
if /I "%~1"=="vs2022" set "ACTION=vs2022"
if /I "%~1"=="vs2026" set "ACTION=vs2026"

if /I not "%~1"=="" if /I not "%~1"=="2022" if /I not "%~1"=="2026" if /I not "%~1"=="vs2022" if /I not "%~1"=="vs2026" (
    echo Invalid argument: %~1
    echo Usage: Setup.bat [2022^|2026^|vs2022^|vs2026]
    exit /b 1
)

echo Generating project files with action: %ACTION%
pushd "%~dp0"
premake\premake5.exe --file=Build.lua %ACTION%
set "ERR=%ERRORLEVEL%"
popd

if not "%ERR%"=="0" (
    echo Premake failed with code %ERR%.
    exit /b %ERR%
)

echo Done.
pause