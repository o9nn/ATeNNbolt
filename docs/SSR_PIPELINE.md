# SSR Pipeline - Server-Side Rendering Build Configuration

## Overview

This document describes the SSR (Server-Side Rendering) pipeline configuration for the Bolt C++ project. The pipeline has been updated to use a local vcpkg installation instead of git submodules.

## Architecture

### Dependency Management

The SSR pipeline uses a locally cloned vcpkg instance to manage C++ dependencies:

- **Location**: `./ssr_deps/vcpkg`
- **Version**: Pinned to commit `de117c9255ff3d68c1ba730b6c96dc3418c74ae3`
- **Setup**: Cloned directly in CI workflows, not as a git submodule

### Key Changes

1. **Removed Submodule References**: All `submodules: recursive` declarations have been removed from CI workflows
2. **Direct vcpkg Cloning**: vcpkg is now cloned directly into `./ssr_deps/vcpkg` during CI runs
3. **Git History Preserved**: The `.git` directory is retained in the cloned vcpkg (required for baseline resolution)
4. **CMake Presets**: Platform-specific presets added for SSR builds

## CMake Presets

The following presets are available for SSR builds:

- `vcpkg-ssr`: Base preset using local SSR vcpkg
- `linux-x64`: Linux x64 build
- `windows-msvc-x64`: Windows MSVC x64 build
- `windows-mingw-x64`: Windows MinGW x64 build
- `macos-x64`: macOS Intel x64 build
- `macos-arm64`: macOS Apple Silicon ARM64 build
- `macos-universal`: macOS Universal Binary build

## Usage

### Local Development

1. Clone vcpkg locally:
   ```bash
   mkdir -p ./ssr_deps
   git clone https://github.com/Microsoft/vcpkg.git ./ssr_deps/vcpkg
   cd ./ssr_deps/vcpkg
   git checkout de117c9255ff3d68c1ba730b6c96dc3418c74ae3
   ./bootstrap-vcpkg.sh -disableMetrics
   ```

2. Configure CMake:
   ```bash
   cmake --preset linux-x64
   ```

3. Build:
   ```bash
   cmake --build --preset linux-x64
   ```

### CI Workflows

The CI workflows automatically handle vcpkg setup:

1. Clone repository (without submodules)
2. Clone vcpkg to `./ssr_deps/vcpkg`
3. Checkout specific commit
4. Bootstrap vcpkg
5. Configure and build using platform-specific preset

## Dependencies

The project uses the following vcpkg packages:

- **curl**: HTTP client library
- **jsoncpp**: JSON parser
- **glfw3**: OpenGL windowing library
- **imgui**: Immediate mode GUI
- **opengl**: Graphics API

## Best Practices

1. **Git Ignore**: The `ssr_deps/` directory is ignored by git
2. **Baseline Pinning**: vcpkg baseline is pinned to ensure reproducible builds
3. **Manifest Mode**: Uses `vcpkg.json` manifest for dependency management
4. **Caching**: CI workflows cache vcpkg packages for faster builds

## Troubleshooting

### vcpkg baseline errors

If you see errors about baseline resolution:
- Ensure the `.git` directory exists in `./ssr_deps/vcpkg`
- Verify you're on the correct commit: `de117c9255ff3d68c1ba730b6c96dc3418c74ae3`

### Missing dependencies

If dependencies are not found:
- Check that `vcpkg.json` is in the project root
- Ensure vcpkg is bootstrapped
- Try clearing the build directory and reconfiguring

## Design Principles

This configuration follows proven SSR build practices:

1. **Reproducible Builds**: Pinned vcpkg version ensures consistency across environments
2. **Simplified Repository**: Direct cloning eliminates submodule complexity
3. **Build Reliability**: Local vcpkg prevents network-dependent builds and ensures offline capability
4. **Maintainability**: Clear separation of build dependencies from source code

## References

- vcpkg documentation: https://vcpkg.io/
- CMake Presets: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
