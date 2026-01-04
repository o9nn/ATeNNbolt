# Dependency Management

This project uses modern C++ dependency management tools (primarily **vcpkg**, with **conan** as an alternative) to provide consistent, cross-platform builds and avoid git submodules in this monorepo.

## Quick Start

### Automated Setup (Recommended)

Use our automated setup script for easy dependency installation:

```bash
# Automated setup with package manager detection
./scripts/setup-deps.sh

# Force specific package manager
./scripts/setup-deps.sh --vcpkg
./scripts/setup-deps.sh --conan

# Setup only (no build)
./scripts/setup-deps.sh --no-build
```

### Manual Setup

#### Using CMake Presets (Recommended)
```bash
# vcpkg (recommended)
cmake --preset vcpkg
cmake --build --preset vcpkg-release

# Debug build
cmake --preset vcpkg-debug  
cmake --build --preset vcpkg-debug

# Conan alternative
cmake --preset conan
cmake --build --preset conan-release
```

## Package Manager Integration Features

### 🔧 Multi-Manager Support
- **Primary**: vcpkg with manifest-based dependency management
- **Alternative**: Conan with conanfile.txt configuration  
- **Fallback**: System package manager for base dependencies
- **Auto-detection**: Automatically chooses the best available package manager

### 🎯 Smart Dependency Resolution
- Intelligent fallback between package managers
- Cross-platform dependency mapping
- Feature-based dependency grouping (AI, GUI, networking)
- Version constraint management

### 🛠️ Developer Tools
- **Package Manager Status**: `python3 scripts/package-manager.py --status`
- **Automated Installation**: `python3 scripts/package-manager.py --install`
- **Integration Testing**: `python3 scripts/test-package-management.py`
- **Setup Automation**: `./scripts/setup-deps.sh`

### 📋 CMake Integration
- Enhanced package finding with fallback mechanisms
- Modern CMake target support with legacy compatibility
- Conditional feature compilation based on available dependencies
- Comprehensive package status reporting

## Supported Package Managers

### Primary: vcpkg

vcpkg is the primary package manager for this project.

#### Prerequisites
- CMake 3.15 or later
- C++17 compatible compiler  
- vcpkg (installed globally or system-wide)

#### Dependencies
The following dependencies are managed through vcpkg (declared in `vcpkg.json`):
- **curl**: HTTP client library with SSL/TLS support
- **jsoncpp**: JSON parsing and generation library  
- **glfw3**: Multi-platform library for OpenGL, window and input
- **imgui**: Immediate mode GUI library with GLFW and OpenGL3 bindings
- **opengl**: OpenGL graphics library

#### Setup and Build

```bash
# Using CMake Presets (Recommended)
cmake --preset vcpkg
cmake --build build --parallel

# Or traditional approach
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/usr/local/share/vcpkg/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)
```

### Alternative: Conan

Conan support is provided as an alternative dependency management option.

#### Setup  
1. Install conan: `pip install conan`
2. Dependencies are declared in `conanfile.txt` 
3. Install dependencies: `conan install . --build=missing`

#### Usage
```bash
# Install dependencies  
conan install . --build=missing -s build_type=Release

# Configure and build (when conan preset exists)
cmake --preset conan
cmake --build build
```

## CMake Integration

The project uses modern CMake practices:
- Modern `find_package()` calls instead of `pkg_check_modules()`  
- Target-based linking (e.g., `CURL::libcurl`, `JsonCpp::JsonCpp`, `imgui::imgui`)
- No hardcoded system include paths
- Proper dependency target propagation

## Dependencies

Current dependencies managed by package managers:

### Core Libraries
- **curl**: HTTP client library with SSL/TLS support and tools
- **jsoncpp**: JSON parsing and generation library
- **glfw3**: Multi-platform library for OpenGL, window and input  
- **imgui**: Immediate mode GUI library with GLFW and OpenGL3 bindings
- **opengl**: OpenGL graphics library

### Legacy Dependencies  
- **ggml**: AI/ML tensor library (manually managed in `ggml/` directory)
- Future goal: migrate to proper package management when available

## Configuration Files

- `vcpkg.json`: Package manifest with comprehensive dependency list
- `conanfile.txt`: Conan dependencies as alternative package manager
- `CMakePresets.json`: CMake configuration presets for vcpkg integration  
- `.copilot-rules.md`: Rules preventing submodule usage in monorepo
- `.gitignore`: Excludes vcpkg/conan installation directories

## Adding New Dependencies

### For vcpkg:
1. Find the package:
   ```bash
   vcpkg search <package-name>
   ```

2. Add to `vcpkg.json`:
   ```json
   {
     "dependencies": [
       {
         "name": "new-package",
         "features": ["feature1", "feature2"]
       }
     ]
   }
   ```

3. Update CMakeLists.txt with modern CMake:
   ```cmake
   find_package(new-package CONFIG REQUIRED)
   target_link_libraries(bolt_lib PRIVATE new-package::new-package)
   ```

### For conan:
1. Add to `conanfile.txt`:
   ```ini
   [requires]
   new-package/version
   ```

2. Install dependencies:
   ```bash
   conan install . --build=missing
   ```

## Dependency Management Rules

### DO
- Use vcpkg.json for dependency declarations
- Use modern CMake targets for linking (e.g., `imgui::imgui`)
- Update both vcpkg.json and conanfile.txt when adding dependencies
- Follow semantic versioning for dependency versions

### DON'T  
- Use git submodules in this monorepo (see `.copilot-rules.md`)
- Hardcode system include paths (e.g., `/usr/include/imgui`)
- Use pkg-config modules when CMake targets are available
- Copy external libraries directly into the repository

## Troubleshooting

### Build Issues
- Ensure `VCPKG_ROOT` is set correctly
- Try cleaning the build directory: `rm -rf build/`
- Update vcpkg: `git pull` in vcpkg directory

### Missing Dependencies
- Run `vcpkg install` to manually install dependencies
- Check `vcpkg.json` for correct package names and baseline

### Version Issues
- Update the `builtin-baseline` in `vcpkg.json` to a newer commit from vcpkg repository
- Use `vcpkg update` to get latest package versions

## Benefits

Using modern dependency management provides:
- **No git submodules complexity** in monorepo architecture
- **Reproducible builds** across different machines and platforms  
- **Easy dependency management** with automatic version resolution
- **Cross-platform support** for Windows, Linux, and macOS
- **Modern CMake integration** with automatic find_package support
- **Version control** with baseline commits for consistency
- **Multiple package manager options** (vcpkg primary, conan alternative)

## Migration Notes

The project has been successfully migrated from:
- Manual dependency management → vcpkg/conan package managers
- pkg-config calls → Modern CMake find_package() with targets  
- Hardcoded system paths → Proper package manager integration
- Git submodules potential → Explicit rules preventing their use

This ensures cleaner, more maintainable dependency management suitable for monorepo architecture.