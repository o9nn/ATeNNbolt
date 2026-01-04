# llama.cpp Research Findings

## llama.cpp Overview

**Repository**: https://github.com/ggml-org/llama.cpp  
**Purpose**: LLM inference in C/C++ with minimal setup and state-of-the-art performance

### Key Features

- CPU-first design with GPU acceleration support
- Multiple backend support: CUDA, Vulkan, OpenCL, HIP, Metal
- Minimal dependencies
- C-style interface (`llama.h`)
- Extensive example programs and tools

## Building llama.cpp

### Standard Build

```bash
cmake -B build
cmake --build build --config Release
```

### Static Library Build

```bash
cmake -B build -DBUILD_SHARED_LIBS=OFF
cmake --build build --config Release
```

### Shared Library Build

```bash
cmake -B build -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release
```

or

```bash
cmake -B build -DGGML_SHARED=ON
cmake --build build --config Release
```

## Integration as Subdirectory

### CMake Integration

Add to parent `CMakeLists.txt`:

```cmake
add_subdirectory(path/to/llama.cpp)
target_link_libraries(your_target PRIVATE llama)
```

### Required Headers

- `llama.h` - Main C-style interface
- `common.h` - Common utilities (optional but recommended)

### Required Libraries

**For Static Build**:
- `libllama.a` (or `llama.lib` on Windows)
- `libcommon.a` (or `common.lib` on Windows)
- `libggml.a` (or `ggml.lib` on Windows)

**For Shared Build**:
- `libllama.so` / `llama.dll` (includes ggml and common statically linked)
- Link against `llama.lib` (import library on Windows)

### Linking in CMake

```cmake
# Static linking
target_link_libraries(your_target PRIVATE llama common ggml)

# Shared linking
target_link_libraries(your_target PRIVATE llama)
```

### Linking in Visual Studio (Windows)

```cpp
#pragma comment(lib, "common.lib")
#pragma comment(lib, "llama.lib")
```

## Common Build Issues and Solutions

### Issue 1: C++ Standard Version Conflicts

**Problem**: llama.cpp may require C++11 minimum, but some features need C++17+  
**Solution**: Use C++17 or C++20 for compatibility
```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Issue 2: CUDA Architecture Detection

**Problem**: nvcc cannot auto-detect GPU compute capability  
**Solution**: Manually specify architecture
```bash
cmake -B build -DGGML_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES="86;89"
```

### Issue 3: Missing Dependencies

**Problem**: libcurl not found  
**Solution**: Install development libraries or disable curl
```bash
# Debian/Ubuntu
sudo apt-get install libcurl4-openssl-dev

# Or disable
cmake -B build -DLLAMA_CURL=OFF
```

### Issue 4: Multiple Backend Conflicts

**Problem**: Building with multiple GPU backends causes conflicts  
**Solution**: Build backends as dynamic libraries
```bash
cmake -B build -DGGML_BACKEND_DL=ON
```

### Issue 5: Subdirectory Integration Fails

**Problem**: CMake cannot find llama.cpp targets  
**Solution**: Ensure llama.cpp CMakeLists.txt exports targets properly
```cmake
# In parent CMakeLists.txt
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(ggml/llama.cpp EXCLUDE_FROM_ALL)
```

## Best Practices for Integration

1. **Use Shared Library for Deployment**
   - Smaller executable size
   - Easier updates
   - Only need to ship `llama.dll` / `libllama.so`

2. **Use Static Library for Development**
   - Single executable
   - No DLL/SO dependencies
   - Easier debugging

3. **Disable Unnecessary Features**
   ```bash
   cmake -B build \
     -DLLAMA_CURL=OFF \
     -DGGML_CUDA=OFF \
     -DGGML_VULKAN=OFF
   ```

4. **Set Build Type**
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```

5. **Handle Platform Differences**
   - Windows: Use `.lib` and `.dll`
   - Linux: Use `.a` and `.so`
   - macOS: Use `.a` and `.dylib`

## Integration Checklist

- [ ] Clone llama.cpp into project subdirectory
- [ ] Add `add_subdirectory()` to CMakeLists.txt
- [ ] Set `BUILD_SHARED_LIBS` option
- [ ] Link required libraries (`llama`, `common`, `ggml`)
- [ ] Include `llama.h` in source files
- [ ] Test build with minimal example
- [ ] Handle runtime library paths (LD_LIBRARY_PATH, PATH)
- [ ] Package shared libraries with executable

## Recommended Build Configuration for bolt-cppml

```cmake
# In bolt-cppml CMakeLists.txt
option(ENABLE_LLAMA_CPP "Enable llama.cpp integration" ON)

if(ENABLE_LLAMA_CPP)
    # Configure llama.cpp build options
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(LLAMA_CURL OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    
    # Add llama.cpp subdirectory
    add_subdirectory(ggml/llama.cpp EXCLUDE_FROM_ALL)
    
    # Link to bolt_lib
    target_link_libraries(bolt_lib PRIVATE llama)
    target_compile_definitions(bolt_lib PUBLIC BOLT_HAVE_LLAMA_CPP=1)
    
    set(LLAMA_AVAILABLE TRUE)
else()
    set(LLAMA_AVAILABLE FALSE)
endif()
```

## Runtime Considerations

### Model Loading

```cpp
#include "llama.h"

// Initialize backend
llama_backend_init();

// Load model
llama_model_params model_params = llama_model_default_params();
llama_model* model = llama_load_model_from_file("model.gguf", model_params);

// Create context
llama_context_params ctx_params = llama_context_default_params();
llama_context* ctx = llama_new_context_with_model(model, ctx_params);

// Use model...

// Cleanup
llama_free(ctx);
llama_free_model(model);
llama_backend_free();
```

### Memory Requirements

- Small models (7B): ~4-8 GB RAM
- Medium models (13B): ~8-16 GB RAM
- Large models (70B): ~40-80 GB RAM
- GPU VRAM requirements vary by quantization

### Performance Optimization

- Use GPU acceleration when available
- Enable OpenMP for CPU parallelization
- Use quantized models (Q4_0, Q5_K_M, etc.)
- Adjust context size based on available memory
