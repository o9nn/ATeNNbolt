# Cross-Platform Deployment Guide for Bolt C++

This guide covers building and deploying Bolt C++ across Windows, macOS, and Linux platforms.

## Overview

Bolt C++ now supports full cross-platform deployment with:

- ✅ **Windows** (7/8/10/11) - MSVC 2019+, MinGW-w64
- ✅ **macOS** (10.15+) - Intel x64 and Apple Silicon (ARM64)
- ✅ **Linux** - Various distributions (Ubuntu, Fedora, Arch, etc.)

## Quick Start

### Universal Build Script

Use the universal build script that auto-detects your platform:

```bash
./scripts/build/build.sh
```

### Platform-Specific Scripts

Or use platform-specific scripts directly:

```bash
# Windows
scripts/build/build-windows.bat

# macOS
scripts/build/build-macos.sh

# Linux
scripts/build/build-linux.sh
```

## Prerequisites

### All Platforms

- **CMake 3.15+**
- **C++17 compatible compiler**
- **vcpkg** (recommended for dependency management)

### Windows

**Option 1: Visual Studio**
- Visual Studio 2019 or 2022 with C++ workload
- Windows SDK 10.0.18362.0 or later

**Option 2: MinGW**
- MinGW-w64 with GCC 9.0+
- MSYS2 (recommended)

### macOS

- Xcode 12.0+ or Xcode Command Line Tools
- macOS 10.15 (Catalina) or later
- Homebrew or MacPorts (for dependencies)

### Linux

**Ubuntu/Debian:**
```bash
sudo apt install cmake build-essential libcurl4-openssl-dev \
    libjsoncpp-dev libglfw3-dev libgl1-mesa-dev
```

**Fedora/RHEL:**
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake libcurl-devel jsoncpp-devel glfw-devel mesa-libGL-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake curl jsoncpp glfw mesa
```

## Build Configurations

### CMake Presets

Use modern CMake presets for consistent builds:

```bash
# List available presets
cmake --list-presets

# Configure for your platform
cmake --preset windows-msvc-x64    # Windows with MSVC
cmake --preset macos-universal     # macOS universal binary
cmake --preset linux-x64           # Linux x64

# Build
cmake --build --preset windows-msvc-x64
```

### Environment Variables

Control build behavior with environment variables:

```bash
export BUILD_TYPE=Release          # Debug, Release, RelWithDebInfo
export BOLT_BUILD_UNIVERSAL=ON     # macOS universal binary
export VCPKG_ROOT=/path/to/vcpkg   # vcpkg installation
export JOBS=8                      # Parallel build jobs (Linux/macOS)
```

## Dependencies

### vcpkg (Recommended)

Install dependencies using vcpkg:

```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh    # Linux/macOS
bootstrap-vcpkg.bat     # Windows

# Install dependencies
./vcpkg install curl jsoncpp glfw3 imgui opengl
```

### System Package Managers

Dependencies can also be installed via system package managers (Linux/macOS) or manually (Windows).

## Platform-Specific Notes

### Windows

**MSVC vs MinGW:**
- MSVC: Better Windows integration, official Microsoft support
- MinGW: Open source, better compatibility with Unix-like systems

**Static vs Dynamic Linking:**
- Static linking creates standalone executables
- Dynamic linking requires runtime libraries

**Windows Defender:**
- May flag freshly built executables as suspicious
- Add build directory to exclusions if needed

### macOS

**Architecture Support:**
- Intel x64: Compatible with all Intel Macs
- Apple Silicon (ARM64): Best performance on M1/M2/M3 Macs
- Universal Binary: Single executable for both architectures

**Code Signing:**
- Required for distribution outside the Mac App Store
- Set environment variables:
  ```bash
  export APPLE_CERTIFICATE_ID="Developer ID Application: Your Name"
  export APPLE_DEVELOPMENT_TEAM="TEAM_ID"
  ```

**Notarization:**
- Required for macOS 10.15+ distribution
- Use `xcrun notarytool` or `xcrun altool`

### Linux

**Distribution Compatibility:**
- Built binaries are generally compatible within the same distribution family
- Consider using AppImage for maximum compatibility
- Static linking can improve portability

**Desktop Integration:**
- `.desktop` files for application launchers
- Icon installation in standard locations
- MIME type associations for file formats

## Packaging and Distribution

### Windows

**NSIS Installer:**
```bash
cmake --build build --target package
# Creates: Bolt-CPP-1.0.0-win64.exe
```

**ZIP Archive:**
```bash
cpack -G ZIP
# Creates: Bolt-CPP-1.0.0-win64.zip
```

### macOS

**DMG Disk Image:**
```bash
cpack -G DragNDrop
# Creates: Bolt-CPP-1.0.0-Darwin.dmg
```

**Archive:**
```bash
cpack -G TGZ
# Creates: Bolt-CPP-1.0.0-Darwin.tar.gz
```

### Linux

**DEB Package (Ubuntu/Debian):**
```bash
cpack -G DEB
# Creates: bolt-cpp_1.0.0_amd64.deb
```

**RPM Package (Fedora/RHEL):**
```bash
cpack -G RPM
# Creates: bolt-cpp-1.0.0.x86_64.rpm
```

**TAR Archive:**
```bash
cpack -G TGZ
# Creates: bolt-cpp-1.0.0-Linux.tar.gz
```

## Testing Deployment

### Local Testing

1. Build the project
2. Run basic functionality tests
3. Test on clean system (VM or container)
4. Verify all dependencies are included

### Automated Testing

Set up CI/CD pipelines for automated cross-platform testing:

- **GitHub Actions**: `.github/workflows/`
- **GitLab CI**: `.gitlab-ci.yml`
- **Azure DevOps**: `azure-pipelines.yml`

### Platform-Specific Testing

**Windows:**
- Test on Windows 10 and 11
- Test both x64 and x86 if supported
- Test on systems without Visual Studio

**macOS:**
- Test on Intel and Apple Silicon Macs
- Test minimum macOS version support
- Verify code signing and notarization

**Linux:**
- Test on multiple distributions
- Test with different desktop environments
- Verify package dependencies

## Troubleshooting

### Common Issues

**Missing Dependencies:**
- Ensure all required libraries are installed
- Check vcpkg configuration
- Verify pkg-config paths (Linux)

**Permission Errors:**
- Run with appropriate permissions
- Check file system permissions
- Verify code signing certificates (macOS)

**Build Failures:**
- Check compiler version compatibility
- Verify CMake version requirements
- Clear build cache and retry

### Platform-Specific Issues

**Windows:**
- Path length limitations (enable long paths)
- Antivirus interference
- Missing Windows SDK components

**macOS:**
- Xcode command line tools not installed
- Certificate/keychain access issues
- Architecture mismatch warnings

**Linux:**
- Missing development headers
- Incorrect library paths
- Desktop environment integration issues

## Performance Optimization

### Build Optimizations

```bash
# Enable link-time optimization
cmake -DBOLT_ENABLE_LTO=ON

# Native architecture optimizations
cmake -DCMAKE_CXX_FLAGS="-march=native"

# Parallel builds
make -j$(nproc)                    # Linux
make -j$(sysctl -n hw.ncpu)        # macOS
```

### Runtime Optimizations

- Profile-guided optimization (PGO)
- Link-time optimization (LTO)
- Architecture-specific optimizations

## Distribution Channels

### Official Channels

- GitHub Releases
- Package repositories
- Official website downloads

### Third-Party Channels

**Windows:**
- Chocolatey
- Scoop
- WinGet

**macOS:**
- Homebrew
- MacPorts

**Linux:**
- Distribution repositories
- Snap Store
- Flatpak
- AppImage

## Security Considerations

- Code signing for all platforms
- Dependency vulnerability scanning
- Secure build environment
- Reproducible builds
- Supply chain security

---

For more detailed platform-specific information, see:
- [Windows Deployment](windows.md)
- [macOS Deployment](macos.md) 
- [Linux Deployment](linux.md)