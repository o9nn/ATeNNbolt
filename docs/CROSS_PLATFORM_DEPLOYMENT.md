# Cross-Platform Deployment - Quick Reference

## 🚀 Quick Start

### One-Command Build
```bash
./scripts/build/build.sh
```
Auto-detects your platform and builds accordingly.

### Platform-Specific
```bash
# Windows
scripts\build\build-windows.bat

# macOS
scripts/build/build-macos.sh

# Linux
scripts/build/build-linux.sh
```

## 🎯 Supported Platforms

| Platform | Architectures | Compilers | Package Formats |
|----------|--------------|-----------|-----------------|
| **Windows** | x64 | MSVC 2019+, MinGW-w64 | .exe, .msi, .zip |
| **macOS** | x64, ARM64, Universal | Clang/Xcode | .app, .dmg, .tar.gz |
| **Linux** | x64, ARM64 | GCC 9+, Clang 10+ | .deb, .rpm, .tar.gz |

## 📦 CMake Presets

```bash
# List available presets
cmake --list-presets

# Build examples
cmake --preset windows-msvc-x64
cmake --preset macos-universal
cmake --preset linux-x64

cmake --build --preset [preset-name]
```

## 🔧 Environment Variables

```bash
export BUILD_TYPE=Release         # Debug, Release, RelWithDebInfo
export BOLT_BUILD_UNIVERSAL=ON    # macOS universal binary
export VCPKG_ROOT=/path/to/vcpkg   # vcpkg location
export JOBS=8                     # Parallel jobs (Linux/macOS)
```

## 📖 Documentation

See [`docs/deployment/cross-platform.md`](docs/deployment/cross-platform.md) for complete documentation.

## ✅ Features Implemented

- ✅ Modern CMake 3.15+ with cross-platform presets
- ✅ Platform-specific build configurations
- ✅ Universal build scripts with auto-detection
- ✅ vcpkg integration for dependency management
- ✅ Cross-platform feature detection
- ✅ GitHub Actions CI/CD pipeline
- ✅ Package generation for all platforms
- ✅ Code signing and notarization support structure
- ✅ Cross-compilation support
- ✅ Comprehensive documentation

---

*This cross-platform deployment system was implemented to fulfill the Long-term (Month 3+) roadmap item in DEVO-GENESIS.md*