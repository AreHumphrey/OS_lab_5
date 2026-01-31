@echo off
setlocal ENABLEEXTENSIONS

REM Usage: build_windows.cmd [BuildType]
REM Default: Debug. Requires git, cmake, MinGW-w64 g++ (or MSVC), sqlite3 dev libs.

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

set SCRIPT_DIR=%~dp0
for %%I in ("%SCRIPT_DIR%..") do set LAB5_ROOT=%%~fI
for %%I in ("%LAB5_ROOT%\..") do set REPO_ROOT=%%~fI

echo Updating repository...
git -C "%REPO_ROOT%" pull --rebase
if errorlevel 1 goto :error

echo Configuring CMake (%BUILD_TYPE%)...
cmake -S "%LAB5_ROOT%" -B "%LAB5_ROOT%\build" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 goto :error

echo Building project...
cmake --build "%LAB5_ROOT%\build" --config %BUILD_TYPE%
if errorlevel 1 goto :error

echo Completed. Binaries in %LAB5_ROOT%\build\bin
exit /b 0

:error
echo Build failed. See output above.
exit /b 1
