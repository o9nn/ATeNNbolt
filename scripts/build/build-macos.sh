#!/bin/bash
# Cross-platform build script for macOS
# Supports both Intel and Apple Silicon architectures

set -e

echo ""
echo "===================================="
echo "  Bolt C++ - macOS Build Script"
echo "===================================="
echo ""

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found"
    echo "Please install CMake using: brew install cmake"
    exit 1
fi

# Detect architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

# Set default build type
BUILD_TYPE=${BUILD_TYPE:-Release}

# Detect Xcode
if command -v xcodebuild &> /dev/null; then
    GENERATOR="Xcode"
    echo "Using Xcode generator"
else
    GENERATOR="Unix Makefiles"
    echo "Using Unix Makefiles generator"
fi

# Set vcpkg toolchain if available
TOOLCHAIN_FILE=""
if [ -n "$VCPKG_ROOT" ] && [ -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
    TOOLCHAIN_FILE="-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    echo "Using vcpkg toolchain: $VCPKG_ROOT"
elif [ -f "vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    TOOLCHAIN_FILE="-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake"
    echo "Using local vcpkg toolchain"
fi

# Detect package manager
if command -v brew &> /dev/null; then
    echo "Homebrew detected - using Homebrew paths"
    CMAKE_PREFIX_PATH="/opt/homebrew;/usr/local"
elif command -v port &> /dev/null; then
    echo "MacPorts detected - using MacPorts paths"
    CMAKE_PREFIX_PATH="/opt/local"
fi

# Check for universal binary build
UNIVERSAL_BUILD=${BOLT_BUILD_UNIVERSAL:-OFF}
if [ "$UNIVERSAL_BUILD" = "ON" ] || [ "$1" = "universal" ]; then
    OSX_ARCHITECTURES="arm64;x86_64"
    echo "Building universal binary for Intel and Apple Silicon"
else
    case "$ARCH" in
        arm64)
            OSX_ARCHITECTURES="arm64"
            echo "Building for Apple Silicon (arm64)"
            ;;
        x86_64)
            OSX_ARCHITECTURES="x86_64"
            echo "Building for Intel (x86_64)"
            ;;
        *)
            OSX_ARCHITECTURES="$ARCH"
            echo "Building for native architecture: $ARCH"
            ;;
    esac
fi

# Create build directory
mkdir -p build
cd build

echo ""
echo "Configuring build..."
echo "Generator: $GENERATOR"
echo "Build Type: $BUILD_TYPE"
echo "Architecture: $OSX_ARCHITECTURES"
echo "macOS Target: ${MACOSX_DEPLOYMENT_TARGET:-10.15}"
if [ -n "$TOOLCHAIN_FILE" ]; then
    echo "Toolchain: $TOOLCHAIN_FILE"
fi
echo ""

# Configure with CMake
cmake .. -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    ${TOOLCHAIN_FILE} \
    -DBOLT_PLATFORM_MACOS=ON \
    -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-10.15}" \
    ${CMAKE_PREFIX_PATH:+-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"} \
    -DCMAKE_INSTALL_PREFIX=install

echo ""
echo "Building project..."
echo ""

# Build the project
if [ "$GENERATOR" = "Xcode" ]; then
    xcodebuild -configuration "$BUILD_TYPE" -jobs $(sysctl -n hw.ncpu)
else
    make -j$(sysctl -n hw.ncpu)
fi

echo ""
echo "================================="
echo "  Build completed successfully!"
echo "================================="
echo ""
echo "Build artifacts are in: $(pwd)"
echo ""
echo "To run tests:"
echo "  ctest --output-on-failure"
echo ""
echo "To install:"
echo "  cmake --install . --config $BUILD_TYPE"
echo ""
echo "To create package:"
echo "  cpack"
echo ""

# Show bundle info for GUI applications
if [ -d "*.app" ]; then
    echo "macOS app bundles created:"
    ls -la *.app
    echo ""
fi