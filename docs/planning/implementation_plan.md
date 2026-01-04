# Development Roadmap: Re-integration of LSP and llama.cpp

**Author**: Manus AI  
**Date**: December 12, 2025  
**Version**: 1.0

---

## 1. Introduction

This document outlines a detailed development plan for re-integrating the Language Server Protocol (LSP) components and llama.cpp support into the bolt-cppml repository. The successful completion of this plan will restore full IDE functionality and enable local AI model inference, significantly enhancing the capabilities of the Bolt C++ IDE.

### 1.1. Current Status

- **LSP Components**: Disabled due to compilation errors and missing protocol types.
- **llama.cpp Support**: Disabled due to build issues and integration complexity.

### 1.2. Objectives

1. **Re-integrate LSP**: Restore full Language Server Protocol functionality for features like code completion, diagnostics, and hover information.
2. **Re-integrate llama.cpp**: Enable local AI model inference using llama.cpp for enhanced AI capabilities.
3. **Ensure Stability**: Maintain build stability and prevent regressions.
4. **Provide Documentation**: Update documentation to reflect changes and guide future development.

---

## 2. LSP Re-integration Plan

### 2.1. Investigation Findings

**Problem**: Compilation errors in `lsp_client.cpp` due to missing LSP protocol types in `lsp_protocol.hpp`.

**Missing Types**:
- `TextDocumentItem`
- `VersionedTextDocumentIdentifier`
- `TextDocumentContentChangeEvent`
- `TextDocumentIdentifier`
- `TextDocumentPositionParams`

### 2.2. Implementation Strategy

#### Milestone 1: Implement Missing LSP Types

**Task 1.1: Define Missing Types in `lsp_protocol.hpp`**

Add the following struct definitions to `include/bolt/editor/lsp_protocol.hpp` based on the LSP 3.17 specification [1]:

```cpp
// In namespace bolt::lsp

struct TextDocumentIdentifier {
    std::string uri;
};

struct VersionedTextDocumentIdentifier : public TextDocumentIdentifier {
    int version;
};

struct TextDocumentItem {
    std::string uri;
    std::string languageId;
    int version;
    std::string text;
};

struct TextDocumentContentChangeEvent {
    std::optional<Range> range;
    std::optional<size_t> rangeLength;
    std::string text;
};

struct TextDocumentPositionParams {
    TextDocumentIdentifier textDocument;
    Position position;
};
```

**Task 1.2: Verify Compilation**

Attempt to compile `lsp_client.cpp` and `lsp_server.cpp` to ensure all type-related errors are resolved.

```bash
g++ -std=c++20 -I./include -c src/bolt/editor/lsp_client.cpp -o /tmp/lsp_client_test.o
```

#### Milestone 2: Re-enable LSP Components in Build System

**Task 2.1: Update `CMakeLists.txt`**

Uncomment the LSP source files in the main `CMakeLists.txt`:

```cmake
# In target_sources(bolt_lib PRIVATE ...)
src/bolt/editor/lsp_json_rpc.cpp
src/bolt/editor/lsp_server.cpp
src/bolt/editor/lsp_client.cpp
src/bolt/editor/lsp_plugin_adapter.cpp
```

**Task 2.2: Full Project Rebuild**

Perform a full rebuild of the project to ensure all LSP components are correctly integrated.

```bash
cd build
make -j$(nproc)
```

#### Milestone 3: Testing and Validation

**Task 3.1: Unit Tests**

- Create a new test file `test/test_lsp.cpp`.
- Add unit tests for:
  - JSON-RPC message serialization/deserialization.
  - LSP client connection to a mock server.
  - `didOpen`, `didChange`, `didClose` notifications.

**Task 3.2: Integration Tests**

- Use the `demo_lsp_editor_integration.cpp` and `demo_lsp_integration.cpp` demos.
- Connect to a real language server (e.g., `clangd`).
- Verify that code completion, hover, and diagnostics are working in the Bolt GUI.

### 2.3. Dependencies

- **jsoncpp**: Already a dependency, used for JSON parsing.
- **Language Server**: A compatible language server (e.g., `clangd`) is required for testing.

### 2.4. Timeline

| Milestone | Task | Estimated Time |
|---|---|---|
| 1 | Implement Missing LSP Types | 2-3 hours |
| 2 | Re-enable LSP Components | 1-2 hours |
| 3 | Testing and Validation | 4-6 hours |
| **Total** | | **7-11 hours** |

---

## 3. llama.cpp Re-integration Plan

### 3.1. Investigation Findings

**Problem**: llama.cpp was disabled in `CMakeLists.txt` due to build issues.

**Root Cause**: Likely due to incorrect subdirectory integration, missing dependencies, or C++ standard conflicts.

### 3.2. Implementation Strategy

#### Milestone 1: Update llama.cpp Submodule

**Task 1.1: Update Git Submodule**

Ensure the llama.cpp submodule is up to date with the latest version from the official repository [2].

```bash
git submodule update --remote --merge
```

#### Milestone 2: Re-enable llama.cpp in Build System

**Task 2.1: Update `CMakeLists.txt`**

Modify the main `CMakeLists.txt` to correctly integrate llama.cpp as a subdirectory [3]:

```cmake
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

**Task 2.2: Full Project Rebuild**

Perform a full rebuild to ensure llama.cpp is correctly compiled and linked.

```bash
cd build
cmake -DENABLE_LLAMA_CPP=ON ..
make -j$(nproc)
```

#### Milestone 3: Integration with AI Components

**Task 3.1: Update `DirectGGUFInference`**

Modify `src/bolt/ai/direct_gguf_inference.cpp` to use the `llama.h` API for model loading and inference.

**Task 3.2: Implement Model Loading**

```cpp
// In DirectGGUFInference class

#include "llama.h"

void load_model(const std::string& model_path) {
    llama_backend_init();
    llama_model_params model_params = llama_model_default_params();
    model_ = llama_load_model_from_file(model_path.c_str(), model_params);
    // ... error handling ...
}
```

**Task 3.3: Implement Inference**

```cpp
// In DirectGGUFInference class

std::string infer(const std::string& prompt) {
    // ... create context, tokenize prompt, run inference ...
    // ... detokenize result, return string ...
}
```

#### Milestone 4: Testing and Validation

**Task 4.1: Unit Tests**

- Add unit tests for `DirectGGUFInference` class.
- Test model loading with a small GGUF model.
- Test inference with a simple prompt.

**Task 4.2: Integration Tests**

- Use the AI chat interface in the Bolt GUI.
- Load a small GGUF model (e.g., TinyLlama).
- Verify that the AI chat responds with model-generated text.

### 3.3. Dependencies

- **GGUF Model**: A small GGUF model is required for testing (e.g., TinyLlama).
- **Hardware**: Sufficient RAM to load the model (4-8 GB for a 7B model).

### 3.4. Timeline

| Milestone | Task | Estimated Time |
|---|---|---|
| 1 | Update llama.cpp Submodule | 1 hour |
| 2 | Re-enable llama.cpp in Build System | 2-3 hours |
| 3 | Integration with AI Components | 4-6 hours |
| 4 | Testing and Validation | 4-6 hours |
| **Total** | | **11-16 hours** |

---

## 4. Documentation and Finalization

### 4.1. Update Documentation

- **README.md**: Add instructions for building with LSP and llama.cpp support.
- **BUILD.md**: Document the new `ENABLE_LLAMA_CPP` CMake option.
- **CONTRIBUTING.md**: Update contribution guidelines.

### 4.2. Code Cleanup

- Remove any remaining mock implementations related to LSP and llama.cpp.
- Ensure consistent coding style and formatting.

### 4.3. Final Testing

- Perform a full regression test of the entire application.
- Test on all supported platforms (Linux, Windows, macOS).

---

## 5. References

[1] Microsoft. (n.d.). *Language Server Protocol Specification - 3.17*. Retrieved from https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/

[2] ggml-org. (n.d.). *llama.cpp*. Retrieved from https://github.com/ggml-org/llama.cpp

[3] ggml-org. (n.d.). *llama.cpp/docs/build.md*. Retrieved from https://github.com/ggml-org/llama.cpp/blob/master/docs/build.md
