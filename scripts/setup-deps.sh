#!/bin/bash

# Bolt C++ Dependency Setup Script
# Automates package manager setup and dependency installation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Package manager preference order
PREFERRED_PKG_MANAGER=""
VCPKG_ROOT="${VCPKG_ROOT:-/usr/local/share/vcpkg}"

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE} Bolt C++ Dependency Setup${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

detect_os() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        CYGWIN*|MINGW*) echo "windows";;
        *)          echo "unknown";;
    esac
}

install_system_deps() {
    local os="$1"
    print_status "Installing system dependencies for $os..."
    
    case "$os" in
        linux)
            if command -v apt-get &> /dev/null; then
                sudo apt-get update
                sudo apt-get install -y \
                    build-essential cmake ninja-build pkg-config \
                    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
                    libgl1-mesa-dev libglu1-mesa-dev xorg-dev \
                    git curl wget
            elif command -v yum &> /dev/null; then
                sudo yum install -y \
                    gcc-c++ cmake ninja-build pkgconfig \
                    libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel \
                    mesa-libGL-devel mesa-libGLU-devel \
                    git curl wget
            elif command -v pacman &> /dev/null; then
                sudo pacman -S --needed \
                    base-devel cmake ninja pkgconf \
                    libx11 libxrandr libxinerama libxcursor libxi \
                    mesa glu \
                    git curl wget
            else
                print_warning "Unknown package manager. Please install dependencies manually."
            fi
            ;;
        macos)
            if command -v brew &> /dev/null; then
                brew update
                brew install cmake ninja pkg-config git curl wget
            else
                print_error "Homebrew not found. Please install Homebrew first."
                exit 1
            fi
            ;;
        *)
            print_warning "Unsupported OS: $os. Manual setup required."
            ;;
    esac
}

check_vcpkg() {
    if [ -x "$VCPKG_ROOT/vcpkg" ]; then
        print_status "Found vcpkg at $VCPKG_ROOT"
        return 0
    elif command -v vcpkg &> /dev/null; then
        VCPKG_ROOT="$(dirname "$(which vcpkg)")"
        print_status "Found vcpkg in PATH at $VCPKG_ROOT"
        return 0
    else
        return 1
    fi
}

install_vcpkg() {
    print_status "Installing vcpkg..."
    
    local install_dir="/usr/local/share/vcpkg"
    if [ ! -d "$install_dir" ]; then
        sudo mkdir -p "$(dirname "$install_dir")"
        sudo git clone https://github.com/Microsoft/vcpkg.git "$install_dir"
        cd "$install_dir"
        sudo ./bootstrap-vcpkg.sh
    fi
    
    VCPKG_ROOT="$install_dir"
    export VCPKG_ROOT
}

check_conan() {
    command -v conan &> /dev/null
}

install_conan() {
    print_status "Installing Conan..."
    
    if command -v pip3 &> /dev/null; then
        pip3 install --user conan
    elif command -v pip &> /dev/null; then
        pip install --user conan
    else
        print_error "pip not found. Cannot install Conan."
        return 1
    fi
    
    # Add to PATH if not already there
    if ! command -v conan &> /dev/null; then
        export PATH="$HOME/.local/bin:$PATH"
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    fi
}

setup_package_manager() {
    print_status "Setting up package managers..."
    
    # Try to use existing package managers first
    if check_vcpkg; then
        PREFERRED_PKG_MANAGER="vcpkg"
    elif check_conan; then
        PREFERRED_PKG_MANAGER="conan"
    else
        # Install vcpkg as default
        install_vcpkg
        PREFERRED_PKG_MANAGER="vcpkg"
    fi
    
    # Install conan as backup if not present
    if [ "$PREFERRED_PKG_MANAGER" = "vcpkg" ] && ! check_conan; then
        print_status "Installing Conan as backup package manager..."
        install_conan
    fi
}

configure_cmake() {
    print_status "Configuring CMake build..."
    
    cd "$PROJECT_ROOT"
    rm -rf "$BUILD_DIR"
    
    case "$PREFERRED_PKG_MANAGER" in
        vcpkg)
            print_status "Using vcpkg preset..."
            cmake --preset vcpkg
            ;;
        conan)
            print_status "Using Conan for dependencies..."
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            conan install .. --build=missing -s build_type=Release
            cmake .. -DCMAKE_BUILD_TYPE=Release
            ;;
        *)
            print_warning "No package manager available. Using system dependencies only."
            mkdir -p "$BUILD_DIR"
            cd "$BUILD_DIR"
            cmake ..
            ;;
    esac
}

build_project() {
    print_status "Building project..."
    
    cd "$PROJECT_ROOT"
    if [ -f "$BUILD_DIR/Makefile" ]; then
        cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || echo 4)"
    else
        print_error "Build configuration failed."
        exit 1
    fi
}

verify_build() {
    print_status "Verifying build..."
    
    local executables=(
        "$BUILD_DIR/bolt"
        "$BUILD_DIR/demo_drawkern"
        "$BUILD_DIR/demo_drawkern_full"
    )
    
    local success_count=0
    for exe in "${executables[@]}"; do
        if [ -x "$exe" ]; then
            print_status "✓ Built: $(basename "$exe")"
            ((success_count++))
        else
            print_warning "✗ Missing: $(basename "$exe")"
        fi
    done
    
    if [ "$success_count" -gt 0 ]; then
        print_status "Build verification passed ($success_count executables found)"
        return 0
    else
        print_error "Build verification failed (no executables found)"
        return 1
    fi
}

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
  --help, -h          Show this help message
  --vcpkg             Force use of vcpkg package manager
  --conan             Force use of Conan package manager  
  --no-build          Setup dependencies but don't build
  --clean             Clean build directory before setup

Environment Variables:
  VCPKG_ROOT          Path to vcpkg installation (default: /usr/local/share/vcpkg)

Examples:
  $0                  # Auto-detect and setup
  $0 --vcpkg          # Force vcpkg usage
  $0 --no-build       # Setup only, no build
EOF
}

main() {
    local no_build=false
    local clean_build=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help|-h)
                show_usage
                exit 0
                ;;
            --vcpkg)
                PREFERRED_PKG_MANAGER="vcpkg"
                shift
                ;;
            --conan)
                PREFERRED_PKG_MANAGER="conan"
                shift
                ;;
            --no-build)
                no_build=true
                shift
                ;;
            --clean)
                clean_build=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    print_header
    
    local os=$(detect_os)
    print_status "Detected OS: $os"
    
    # Clean build if requested
    if [ "$clean_build" = true ]; then
        print_status "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    # Install system dependencies
    install_system_deps "$os"
    
    # Setup package managers
    setup_package_manager
    
    print_status "Using package manager: $PREFERRED_PKG_MANAGER"
    
    # Configure CMake
    configure_cmake
    
    # Build project unless --no-build specified
    if [ "$no_build" = false ]; then
        build_project
        verify_build
    fi
    
    print_status "Setup completed successfully!"
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN} Next steps:${NC}"
    echo -e "${GREEN}   cd $(realpath "$PROJECT_ROOT")${NC}"
    if [ "$no_build" = true ]; then
        echo -e "${GREEN}   cmake --build build --parallel${NC}"
    fi
    echo -e "${GREEN}   ./build/bolt${NC}"
    echo -e "${GREEN}========================================${NC}"
}

# Run main function
main "$@"