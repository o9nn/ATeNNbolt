#!/bin/bash
# Cross-platform build script for Linux
# Supports multiple distributions and compilers

set -e

echo ""
echo "===================================="
echo "  Bolt C++ - Linux Build Script"
echo "===================================="
echo ""

# Detect Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    DISTRO_VERSION=$VERSION_ID
    echo "Detected distribution: $PRETTY_NAME"
else
    DISTRO="unknown"
    echo "WARNING: Could not detect Linux distribution"
fi

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found"
    case "$DISTRO" in
        ubuntu|debian)
            echo "Please install CMake using: sudo apt install cmake"
            ;;
        fedora|centos|rhel)
            echo "Please install CMake using: sudo dnf install cmake"
            ;;
        arch|manjaro)
            echo "Please install CMake using: sudo pacman -S cmake"
            ;;
        *)
            echo "Please install CMake using your distribution's package manager"
            ;;
    esac
    exit 1
fi

# Detect compiler
if command -v gcc &> /dev/null; then
    COMPILER="GCC $(gcc -dumpversion)"
    echo "Using GCC compiler: $COMPILER"
elif command -v clang &> /dev/null; then
    COMPILER="Clang $(clang --version | head -1 | cut -d' ' -f3)"
    echo "Using Clang compiler: $COMPILER"
else
    echo "ERROR: No suitable C++ compiler found"
    case "$DISTRO" in
        ubuntu|debian)
            echo "Please install GCC using: sudo apt install build-essential"
            ;;
        fedora|centos|rhel)
            echo "Please install GCC using: sudo dnf groupinstall 'Development Tools'"
            ;;
        arch|manjaro)
            echo "Please install GCC using: sudo pacman -S base-devel"
            ;;
        *)
            echo "Please install GCC or Clang using your distribution's package manager"
            ;;
    esac
    exit 1
fi

# Check for dependencies
check_dependency() {
    local lib=$1
    local pkg_ubuntu=$2
    local pkg_fedora=$3
    local pkg_arch=$4
    
    if ! pkg-config --exists "$lib" 2>/dev/null; then
        echo "WARNING: $lib development library not found"
        case "$DISTRO" in
            ubuntu|debian)
                [ -n "$pkg_ubuntu" ] && echo "  Install with: sudo apt install $pkg_ubuntu"
                ;;
            fedora|centos|rhel)
                [ -n "$pkg_fedora" ] && echo "  Install with: sudo dnf install $pkg_fedora"
                ;;
            arch|manjaro)
                [ -n "$pkg_arch" ] && echo "  Install with: sudo pacman -S $pkg_arch"
                ;;
        esac
    else
        echo "Found dependency: $lib"
    fi
}

echo ""
echo "Checking dependencies..."
check_dependency "libcurl" "libcurl4-openssl-dev" "libcurl-devel" "curl"
check_dependency "jsoncpp" "libjsoncpp-dev" "jsoncpp-devel" "jsoncpp"
check_dependency "glfw3" "libglfw3-dev" "glfw-devel" "glfw"
check_dependency "gl" "libgl1-mesa-dev" "mesa-libGL-devel" "mesa"

# Set default build type
BUILD_TYPE=${BUILD_TYPE:-Release}

# Set number of parallel jobs
JOBS=${JOBS:-$(nproc)}

# Set vcpkg toolchain if available
TOOLCHAIN_FILE=""
if [ -n "$VCPKG_ROOT" ] && [ -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
    TOOLCHAIN_FILE="-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    echo "Using vcpkg toolchain: $VCPKG_ROOT"
elif [ -f "vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    TOOLCHAIN_FILE="-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake"
    echo "Using local vcpkg toolchain"
fi

# Create build directory
mkdir -p build
cd build

echo ""
echo "Configuring build..."
echo "Distribution: $DISTRO"
echo "Compiler: $COMPILER"
echo "Build Type: $BUILD_TYPE"
echo "Parallel Jobs: $JOBS"
if [ -n "$TOOLCHAIN_FILE" ]; then
    echo "Toolchain: $TOOLCHAIN_FILE"
fi
echo ""

# Configure with CMake
cmake .. -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    ${TOOLCHAIN_FILE} \
    -DBOLT_PLATFORM_LINUX=ON \
    -DCMAKE_INSTALL_PREFIX=install

echo ""
echo "Building project..."
echo ""

# Build the project
make -j"$JOBS"

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
echo "  make install"
echo ""
echo "To create package:"
echo "  cpack"
echo ""

# Show executable info
if [ -f "bolt" ]; then
    echo "Main executable created: bolt"
    echo "File info: $(file bolt)"
    echo ""
fi

echo "Available executables:"
find . -name "bolt*" -type f -executable | head -10