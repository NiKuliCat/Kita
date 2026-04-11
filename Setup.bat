@echo off

pushd "%~dp0"
premake\premake5.exe --file=Build.lua vs2026
popd
pause
