@echo off
setlocal

cd /d "%~dp0"

set CMAKE_POLICY_VERSION_MINIMUM=3.5

if not exist "build\CMakeCache.txt" (
    echo [LoadingScreen] Configuring CMake with Visual Studio 2022 Win32...
    cmake -B build -G "Visual Studio 17 2022" -A Win32
    if errorlevel 1 (
        echo [LoadingScreen] CMake configure failed!
        exit /b %errorlevel%
    )
)

echo [LoadingScreen] Building Release...
cmake --build build --config Release
if errorlevel 1 (
    echo [LoadingScreen] Build failed!
    exit /b %errorlevel%
)

echo [LoadingScreen] Build successful! Binary is located at bin/LoadingScreen.asi
endlocal