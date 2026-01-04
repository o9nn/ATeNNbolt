@echo off
REM Cross-platform build script for Windows
REM Supports both MSVC and MinGW compilers

setlocal enabledelayedexpansion

echo.
echo ====================================
echo  Bolt C++ - Windows Build Script
echo ====================================
echo.

REM Check for required tools
where cmake >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    exit /b 1
)

REM Detect compiler
if defined VCINSTALLDIR (
    echo Using Visual Studio compiler
    set GENERATOR=Visual Studio 17 2022
    set TOOLCHAIN_FILE=
) else if defined MINGW_PREFIX (
    echo Using MinGW compiler
    set GENERATOR=MinGW Makefiles
    set TOOLCHAIN_FILE=
) else (
    echo Detecting available compiler...
    where cl >nul 2>&1
    if !errorlevel! equ 0 (
        echo Using Visual Studio compiler
        set GENERATOR=Visual Studio 17 2022
        set TOOLCHAIN_FILE=
    ) else (
        where gcc >nul 2>&1
        if !errorlevel! equ 0 (
            echo Using MinGW compiler
            set GENERATOR=MinGW Makefiles
            set TOOLCHAIN_FILE=
        ) else (
            echo ERROR: No suitable compiler found
            echo Please install either Visual Studio or MinGW
            exit /b 1
        )
    )
)

REM Set default build type
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

REM Set vcpkg toolchain if available
if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    set TOOLCHAIN_FILE=-DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
    echo Using vcpkg toolchain: %VCPKG_ROOT%
) else if exist "vcpkg_installed" (
    if exist "vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set TOOLCHAIN_FILE=-DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake
        echo Using local vcpkg toolchain
    )
)

REM Create build directory
if not exist "build" mkdir build
cd build

echo.
echo Configuring build...
echo Generator: %GENERATOR%
echo Build Type: %BUILD_TYPE%
if defined TOOLCHAIN_FILE echo Toolchain: %TOOLCHAIN_FILE%
echo.

REM Configure with CMake
cmake .. -G "%GENERATOR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    %TOOLCHAIN_FILE% ^
    -DBOLT_PLATFORM_WINDOWS=ON ^
    -DCMAKE_INSTALL_PREFIX=install

if !errorlevel! neq 0 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo.
echo Building project...
echo.

REM Build the project
if "%GENERATOR%"=="MinGW Makefiles" (
    mingw32-make -j%NUMBER_OF_PROCESSORS%
) else (
    cmake --build . --config %BUILD_TYPE% --parallel %NUMBER_OF_PROCESSORS%
)

if !errorlevel! neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo =================================
echo  Build completed successfully!
echo =================================
echo.
echo Build artifacts are in: %CD%
echo.
echo To run tests:
echo   ctest --output-on-failure
echo.
echo To install:
echo   cmake --install . --config %BUILD_TYPE%
echo.
echo To create package:
echo   cpack
echo.

endlocal