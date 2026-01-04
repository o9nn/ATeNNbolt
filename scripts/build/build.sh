#!/bin/bash
# Universal cross-platform build script for Bolt C++
# Automatically detects the platform and runs appropriate build script

set -e

echo ""
echo "========================================"
echo "  Bolt C++ - Universal Build Script"
echo "========================================"
echo ""

# Detect platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    PLATFORM="windows"
else
    echo "ERROR: Unsupported platform: $OSTYPE"
    exit 1
fi

echo "Detected platform: $PLATFORM"
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run platform-specific build script
case "$PLATFORM" in
    linux)
        exec "$SCRIPT_DIR/build-linux.sh" "$@"
        ;;
    macos)
        exec "$SCRIPT_DIR/build-macos.sh" "$@"
        ;;
    windows)
        if command -v bash &> /dev/null; then
            # On Windows with Git Bash or similar
            exec "$SCRIPT_DIR/build-windows.bat" "$@"
        else
            echo "ERROR: Bash not available on Windows"
            echo "Please run build-windows.bat directly"
            exit 1
        fi
        ;;
    *)
        echo "ERROR: No build script available for platform: $PLATFORM"
        exit 1
        ;;
esac