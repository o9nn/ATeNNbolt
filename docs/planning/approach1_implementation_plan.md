# Implementation Plan: Unify to Single GGML Source (Approach 1)

## Overview

This document provides a step-by-step implementation plan for unifying the GGML source to use only the version embedded in `llama.cpp`, eliminating the standalone `ggml.cpp`.

## Prerequisites

- Backup of current `ggml/` directory structure
- Understanding of current dependencies on standalone GGML
- Test environment for validation

## Phase 1: Analysis and Preparation

### 1.1. Identify All Dependencies

**Current Dependencies on `ggml/ggml.cpp`**:
- `CMakeLists.txt` line 92: `add_subdirectory(ggml/ggml.cpp)`
- `CMakeLists.txt` line 289: Include path `${PROJECT_SOURCE_DIR}/ggml/ggml.cpp/include`
- `CMakeLists.txt` line 292: Link target `ggml`
- `src/bolt/ai/ggml.cpp` - Wrapper implementation
- `src/bolt/ai/rwkv_wrapper.cpp` - RWKV model implementation
- `src/bolt/ai/ggml_loader.cpp` - GGML model loader

### 1.2. Verify llama.cpp GGML Availability

The `llama.cpp` repository includes GGML at:
- Include path: `ggml/llama.cpp/include/`
- Library target: `ggml` (provided by llama.cpp CMake)

## Phase 2: CMakeLists.txt Modifications

### 2.1. Remove Standalone GGML Subdirectory

**Before**:
```cmake
# Add GGML as subdirectory (conditional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ggml/ggml.cpp")
    add_subdirectory(ggml/ggml.cpp)
    set(GGML_AVAILABLE TRUE)
else()
    set(GGML_AVAILABLE FALSE)
    message(WARNING "GGML not found - AI functionality will be simulated")
endif()
```

**After**:
```cmake
# GGML is now provided by llama.cpp
# Check if llama.cpp is available and provides GGML
if(ENABLE_LLAMA_CPP AND LLAMA_AVAILABLE)
    set(GGML_AVAILABLE TRUE)
    message(STATUS "GGML provided by llama.cpp")
else()
    set(GGML_AVAILABLE FALSE)
    message(WARNING "GGML not available - AI functionality will be simulated")
endif()
```

### 2.2. Update Include Paths

**Before**:
```cmake
if(GGML_AVAILABLE)
    target_include_directories(bolt_lib PUBLIC
        ${PROJECT_SOURCE_DIR}/ggml/ggml.cpp/include
        ${PROJECT_SOURCE_DIR}/ggml/rwkv.cpp
    )
    target_link_libraries(bolt_lib PUBLIC ggml)
endif()
```

**After**:
```cmake
if(GGML_AVAILABLE)
    target_include_directories(bolt_lib PUBLIC
        ${PROJECT_SOURCE_DIR}/ggml/llama.cpp/include
        ${PROJECT_SOURCE_DIR}/ggml/rwkv.cpp
    )
    target_link_libraries(bolt_lib PUBLIC ggml)
endif()
```

### 2.3. Enable llama.cpp by Default

**Before**:
```cmake
option(ENABLE_LLAMA_CPP "Enable llama.cpp integration" OFF)
```

**After**:
```cmake
option(ENABLE_LLAMA_CPP "Enable llama.cpp integration" ON)
```

## Phase 3: Source Code Verification

### 3.1. Check GGML API Usage

Verify that all GGML API calls in the following files are compatible with llama.cpp's GGML:
- `src/bolt/ai/ggml.cpp`
- `src/bolt/ai/rwkv_wrapper.cpp`
- `src/bolt/ai/ggml_loader.cpp`
- `src/bolt/ai/model_loader.cpp`

### 3.2. Update Include Statements (if needed)

Most files should already use `#include "ggml.h"` which will resolve correctly with the updated include paths.

## Phase 4: Build and Test

### 4.1. Clean Build

```bash
cd /home/ubuntu/bolt-cppml
rm -rf build
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DENABLE_LLAMA_CPP=ON ..
make -j$(nproc)
```

### 4.2. Verify Build Artifacts

Check that the following are built successfully:
- `bolt` executable
- `libbolt_lib.so` shared library
- `libggml.so` from llama.cpp
- `libllama.so` from llama.cpp

### 4.3. Run Tests

Execute any existing tests to verify functionality:
```bash
cd /home/ubuntu/bolt-cppml/build
ctest --output-on-failure
```

## Phase 5: Cleanup

### 5.1. Remove Standalone GGML Directory

Once build and tests pass:
```bash
cd /home/ubuntu/bolt-cppml
rm -rf ggml/ggml.cpp
```

### 5.2. Update Documentation

Update the following documentation files:
- `README.md` - Note that GGML is provided by llama.cpp
- `BUILD.md` - Update build instructions
- `docs/planning/implementation_plan.md` - Mark llama.cpp integration as complete

## Phase 6: Git Commit

```bash
git add CMakeLists.txt
git commit -m "Unify GGML source to llama.cpp embedded version

- Removed standalone ggml.cpp dependency
- Updated include paths to use llama.cpp/include
- Enabled llama.cpp by default
- GGML now provided exclusively by llama.cpp
- Resolves API incompatibility issues

This change simplifies dependency management and ensures
compatibility between llama.cpp and GGML."
```

## Rollback Plan

If issues arise during implementation:

```bash
cd /home/ubuntu/bolt-cppml
tar -xzf /home/ubuntu/ggml_backup.tar.gz
git checkout CMakeLists.txt
```

## Success Criteria

- ✅ Project builds without errors
- ✅ All GGML-dependent components compile
- ✅ llama.cpp integration works
- ✅ No duplicate GGML symbols
- ✅ Tests pass (if available)
- ✅ Documentation updated

## Estimated Time

- Phase 1: 30 minutes
- Phase 2: 30 minutes
- Phase 3: 1 hour
- Phase 4: 1-2 hours
- Phase 5: 15 minutes
- Phase 6: 15 minutes

**Total: 3-4 hours**

## Risk Mitigation

1. **Backup Created**: Full backup of `ggml/` directory before changes
2. **Incremental Changes**: Make changes in small, testable increments
3. **Continuous Testing**: Build and test after each major change
4. **Version Control**: Commit working states frequently
5. **Rollback Ready**: Keep backup and know rollback procedure
