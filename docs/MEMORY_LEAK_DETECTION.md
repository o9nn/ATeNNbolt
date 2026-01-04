# Memory Leak Detection

Bolt C++ ML provides comprehensive memory leak detection tools to help identify and diagnose memory management issues in your code.

## Overview

The memory leak detection system consists of multiple components:

1. **MemoryLeakDetector** - Built-in leak detection with source location tracking
2. **MemoryManager** - Basic allocation tracking and leak detection
3. **AddressSanitizer (ASan)** - Compiler-based memory error detection
4. **LeakSanitizer (LSan)** - Compiler-based memory leak detection
5. **Valgrind** - External tool for comprehensive memory analysis

## Quick Start

### Using Built-in Leak Detector

```bash
# Build and run the example
mkdir build && cd build
cmake ..
make memory_leak_detection_example
./memory_leak_detection_example
```

### Using AddressSanitizer

```bash
# Configure with ASan
cmake --preset asan
cmake --build --preset asan

# Run tests
ctest --preset asan-tests

# Or run a specific program
./memory_leak_detection_example
```

### Using LeakSanitizer

```bash
# Configure with LSan
cmake --preset lsan
cmake --build --preset lsan

# Run with leak detection
LSAN_OPTIONS=verbosity=1:log_threads=1 ./memory_leak_detection_example
```

### Using Comprehensive Leak Detection

```bash
# Configure with both ASan and LSan
cmake --preset leak-detection
cmake --build --preset leak-detection
ctest --preset leak-detection-tests
```

### Using Valgrind

```bash
# Install Valgrind
sudo apt-get install valgrind  # Ubuntu/Debian
# or
sudo yum install valgrind      # RHEL/CentOS

# Configure and build
cmake --preset valgrind
cmake --build --preset valgrind

# Run with Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./memory_leak_detection_example

# Or use CTest integration
ctest --preset valgrind-tests
```

## Built-in Memory Leak Detector

The `MemoryManager` class provides fundamental memory tracking capabilities:

### Features

- Track all allocations and deallocations
- Monitor current and peak memory usage
- Detect memory leaks
- Thread-safe operations
- Memory usage limits

### Usage

```cpp
#include "bolt/core/memory_manager.hpp"

auto& memMgr = bolt::MemoryManager::getInstance();

// Allocate memory
void* ptr = memMgr.allocate(1024);

// Use the memory
// ...

// Deallocate memory
memMgr.deallocate(ptr);

// Check for leaks
if (memMgr.hasMemoryLeaks()) {
    auto leaks = memMgr.getLeakedAllocations();
    std::cout << "Detected " << leaks.size() << " memory leaks\n";
}

// Get memory statistics
std::cout << "Current usage: " << memMgr.getCurrentUsage() << " bytes\n";
std::cout << "Peak usage: " << memMgr.getPeakUsage() << " bytes\n";
```

### Setting Memory Limits

```cpp
// Set maximum allowed memory usage (e.g., 100 MB)
memMgr.setMaxAllowedUsage(100 * 1024 * 1024);

try {
    void* ptr = memMgr.allocate(1024 * 1024 * 1024); // Try to allocate 1 GB
} catch (const bolt::MemoryException& e) {
    std::cerr << "Allocation failed: " << e.what() << "\n";
}
```

## Enhanced Memory Leak Detector

The `MemoryLeakDetector` class provides advanced leak detection with source location tracking and detailed reporting.

### Features

- **Source Location Tracking**: Track where allocations were made (file, line, function)
- **Categorization**: Organize allocations by category
- **Detailed Reporting**: Generate comprehensive leak reports
- **Statistics**: Get leak statistics by category, file, and more
- **Allocation Age**: Track how long allocations have been alive
- **Thread-Safe**: Safe for multi-threaded applications

### Basic Usage

```cpp
#include "bolt/core/memory_leak_detector.hpp"

auto& detector = bolt::MemoryLeakDetector::getInstance();

// Enable leak detection (enabled by default)
detector.setEnabled(true);

// Track an allocation
void* ptr = malloc(1024);
TRACK_ALLOCATION(ptr, 1024, "buffers");

// Use the memory
// ...

// Untrack when deallocating
UNTRACK_ALLOCATION(ptr);
free(ptr);

// Check for leaks
if (detector.hasLeaks()) {
    std::cout << detector.generateReport() << std::endl;
}
```

### Manual Tracking

```cpp
void* ptr = malloc(2048);
detector.trackAllocation(
    ptr,                    // Pointer
    2048,                   // Size in bytes
    __FILE__,              // Source file
    __LINE__,              // Line number
    __FUNCTION__,          // Function name
    "network_buffers"      // Category
);

// Later...
detector.untrackAllocation(ptr);
free(ptr);
```

### Generating Reports

#### Detailed Report

```cpp
std::string report = detector.generateReport();
std::cout << report << std::endl;
```

Example output:
```
=== Memory Leak Detection Report ===

⚠ Memory leaks detected!

Summary:
  Total leaks: 3
  Total leaked: 12.50 KB
  Peak usage: 45.25 MB
  Current usage: 12.50 KB

Leaks by category:
  buffers: 8.00 KB
  network_buffers: 4.50 KB

Leaks by file:
  src/network/buffer.cpp: 4.50 KB
  src/core/memory.cpp: 8.00 KB

Detailed leak information:

Leak #1:
  Address: 0x7f1234567890
  Size: 4.00 KB
  Location: src/network/buffer.cpp:42
  Function: allocateNetworkBuffer
  Category: network_buffers
  Age: 125 seconds

Leak #2:
  Address: 0x7f1234567abc
  Size: 8.00 KB
  Location: src/core/memory.cpp:156
  Function: allocateTempBuffer
  Category: buffers
  Age: 87 seconds

Leak #3:
  Address: 0x7f1234567def
  Size: 512.00 B
  Location: src/network/buffer.cpp:67
  Function: createSmallBuffer
  Category: network_buffers
  Age: 45 seconds
```

#### Summary Report

```cpp
std::string summary = detector.generateSummary();
std::cout << summary << std::endl;
// Output: "3 leaks, 12.50 KB leaked"
```

### Getting Statistics

```cpp
auto stats = detector.getStats();

std::cout << "Total leaks: " << stats.totalLeaks << "\n";
std::cout << "Total leaked bytes: " << stats.totalLeakedBytes << "\n";
std::cout << "Peak memory usage: " << stats.peakMemoryUsage << "\n";

// Leaks by category
for (const auto& [category, bytes] : stats.leaksByCategory) {
    std::cout << category << ": " << bytes << " bytes\n";
}

// Leaks by file
for (const auto& [file, bytes] : stats.leaksByFile) {
    std::cout << file << ": " << bytes << " bytes\n";
}
```

### Categorizing Allocations

Organize allocations by category for better leak analysis:

```cpp
// Network buffers
void* netBuf = malloc(4096);
TRACK_ALLOCATION(netBuf, 4096, "network");

// Image data
void* imgData = malloc(1024 * 1024);
TRACK_ALLOCATION(imgData, 1024 * 1024, "images");

// Temporary buffers
void* tempBuf = malloc(512);
TRACK_ALLOCATION(tempBuf, 512, "temporary");
```

### Integration with RAII

For RAII-style automatic tracking:

```cpp
template<typename T>
class TrackedPtr {
public:
    TrackedPtr(size_t size, const std::string& category = "general") 
        : size_(size), category_(category) {
        ptr_ = static_cast<T*>(malloc(size));
        TRACK_ALLOCATION(ptr_, size, category);
    }
    
    ~TrackedPtr() {
        if (ptr_) {
            UNTRACK_ALLOCATION(ptr_);
            free(ptr_);
        }
    }
    
    T* get() { return ptr_; }
    
private:
    T* ptr_;
    size_t size_;
    std::string category_;
};

// Usage
{
    TrackedPtr<char> buffer(1024, "buffers");
    // Use buffer.get()...
} // Automatically tracked and untracked
```

## Best Practices

### 1. Use Categories Wisely

Organize allocations by logical categories:

```cpp
// Good categories
TRACK_ALLOCATION(ptr, size, "network_buffers");
TRACK_ALLOCATION(ptr, size, "image_cache");
TRACK_ALLOCATION(ptr, size, "string_pool");

// Avoid generic categories
TRACK_ALLOCATION(ptr, size, "general");  // Less useful
```

### 2. Always Pair Track/Untrack

```cpp
void processData() {
    void* buffer = malloc(1024);
    TRACK_ALLOCATION(buffer, 1024, "processing");
    
    try {
        // Process data...
    } catch (...) {
        UNTRACK_ALLOCATION(buffer);
        free(buffer);
        throw;
    }
    
    UNTRACK_ALLOCATION(buffer);
    free(buffer);
}
```

### 3. Use RAII When Possible

Prefer RAII wrappers to ensure proper cleanup:

```cpp
class ScopedBuffer {
public:
    ScopedBuffer(size_t size, const std::string& category) {
        ptr_ = malloc(size);
        TRACK_ALLOCATION(ptr_, size, category);
    }
    
    ~ScopedBuffer() {
        UNTRACK_ALLOCATION(ptr_);
        free(ptr_);
    }
    
    void* get() { return ptr_; }
    
private:
    void* ptr_;
};
```

### 4. Generate Reports Regularly

In development builds, generate leak reports:

```cpp
#ifdef DEBUG
void checkForLeaks() {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    if (detector.hasLeaks()) {
        std::cerr << detector.generateReport() << std::endl;
        // Consider asserting or logging
    }
}

// Call at application shutdown
atexit(checkForLeaks);
#endif
```

### 5. Clear Tracking Between Tests

In unit tests, clear tracking between test cases:

```cpp
TEST_CASE("Memory test 1") {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    
    // Test code...
    
    REQUIRE(detector.hasLeaks() == false);
}

TEST_CASE("Memory test 2") {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.resetStats(); // Reset peak usage stats
    
    // Test code...
}
```

## Integration with Build Systems

### CMake Integration

Enable leak detection in debug builds:

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(bolt_lib PRIVATE ENABLE_LEAK_DETECTION)
endif()
```

### CMake Presets

Use predefined presets for different leak detection tools:

```bash
# AddressSanitizer
cmake --preset asan
cmake --build --preset asan

# LeakSanitizer
cmake --preset lsan
cmake --build --preset lsan

# UndefinedBehaviorSanitizer
cmake --preset ubsan

# ThreadSanitizer (for data race detection)
cmake --preset tsan

# Comprehensive leak detection (ASan + LSan)
cmake --preset leak-detection

# Valgrind
cmake --preset valgrind
```

### Conditional Compilation

```cpp
#ifdef ENABLE_LEAK_DETECTION
    #define TRACK_ALLOC(ptr, size, cat) TRACK_ALLOCATION(ptr, size, cat)
    #define UNTRACK_ALLOC(ptr) UNTRACK_ALLOCATION(ptr)
#else
    #define TRACK_ALLOC(ptr, size, cat)
    #define UNTRACK_ALLOC(ptr)
#endif
```

## Sanitizer Integration

### AddressSanitizer (ASan)

AddressSanitizer detects:
- Heap buffer overflow
- Stack buffer overflow
- Global buffer overflow
- Use after free
- Use after return
- Use after scope
- Memory leaks (with leak detection enabled)

**Configuration:**

```bash
# Build with ASan
cmake -DENABLE_SANITIZER_ADDRESS=ON ..
make

# Run with options
ASAN_OPTIONS=detect_leaks=1:halt_on_error=0 ./your_program
```

**Environment Variables:**

```bash
# Comprehensive ASan options
export ASAN_OPTIONS="
    detect_leaks=1
    halt_on_error=0
    verbosity=1
    log_path=asan.log
    detect_stack_use_after_return=1
    check_initialization_order=1
    detect_invalid_pointer_pairs=2
"
```

### LeakSanitizer (LSan)

LeakSanitizer is a specialized tool for detecting memory leaks.

**Configuration:**

```bash
# Build with LSan
cmake -DENABLE_SANITIZER_LEAK=ON ..
make

# Run with options
LSAN_OPTIONS=verbosity=1:log_threads=1 ./your_program
```

**Environment Variables:**

```bash
# Comprehensive LSan options
export LSAN_OPTIONS="
    verbosity=1
    log_threads=1
    log_pointers=1
    report_objects=1
    use_registers=0
    use_stacks=1
"
```

**Suppression File:**

Create `lsan.supp` to suppress known false positives:

```
# Suppress leaks in third-party library
leak:libthirdparty.so

# Suppress specific function
leak:known_leaky_function
```

Use with: `LSAN_OPTIONS=suppressions=lsan.supp ./your_program`

### UndefinedBehaviorSanitizer (UBSan)

Detects various forms of undefined behavior:
- Integer overflow
- Null pointer dereference
- Misaligned pointer use
- Signed integer overflow
- Division by zero

**Configuration:**

```bash
cmake -DENABLE_SANITIZER_UNDEFINED_BEHAVIOR=ON ..
make
```

### ThreadSanitizer (TSan)

Detects data races and synchronization issues.

**Note:** TSan is incompatible with ASan and LSan.

**Configuration:**

```bash
cmake -DENABLE_SANITIZER_THREAD=ON ..
make

# Run with options
TSAN_OPTIONS=verbosity=1:history_size=7 ./your_program
```

### MemorySanitizer (MSan)

Detects uninitialized memory reads (Clang only).

**Configuration:**

```bash
# Requires Clang compiler
cmake -DCMAKE_CXX_COMPILER=clang++ -DENABLE_SANITIZER_MEMORY=ON ..
make
```

## Valgrind Integration

### Basic Usage

```bash
# Memory leak check
valgrind --leak-check=full --show-leak-kinds=all ./your_program

# With origin tracking
valgrind --leak-check=full --track-origins=yes ./your_program

# Generate suppression file
valgrind --gen-suppressions=all ./your_program 2>&1 | grep -A 20 "insert_a_suppression_name_here"
```

### CMake Integration

The build system includes automatic Valgrind test integration:

```bash
# Enable Valgrind tests
cmake -DENABLE_VALGRIND=ON ..
make

# Run Valgrind tests via CTest
ctest -R valgrind
```

### Suppression Files

Valgrind suppressions are stored in `cmake/valgrind.supp`. Add custom suppressions:

```
{
   my_suppression_name
   Memcheck:Leak
   match-leak-kinds: possible
   ...
   fun:my_function
}
```

### Valgrind Options

Common useful options:

```bash
valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --verbose \
  --log-file=valgrind.log \
  --suppressions=cmake/valgrind.supp \
  ./your_program
```

## Conditional Compilation

## Troubleshooting

### Common Issues

#### False Positives

**Issue**: Legitimate allocations reported as leaks

**Solution**: Ensure proper untracking before deallocation

```cpp
// Wrong
free(ptr);
UNTRACK_ALLOCATION(ptr); // Too late!

// Correct
UNTRACK_ALLOCATION(ptr);
free(ptr);
```

#### Missing Leaks

**Issue**: Known leaks not being reported

**Solution**: Ensure tracking is enabled and allocation was tracked

```cpp
// Check if enabled
if (!detector.isEnabled()) {
    detector.setEnabled(true);
}

// Ensure allocation is tracked
TRACK_ALLOCATION(ptr, size, category);
```

#### Performance Impact

**Issue**: Leak detection slowing down the application

**Solution**: Disable in release builds or use sampling

```cpp
#ifdef NDEBUG
    detector.setEnabled(false);
#endif
```

## Performance Considerations

- **MemoryLeakDetector overhead**: ~20-50ns per allocation/deallocation in debug builds
- **Memory**: ~40 bytes per tracked allocation
- **Thread Safety**: All operations are thread-safe but may contend on mutex
- **AddressSanitizer overhead**: ~2x slowdown, 2-3x memory usage
- **LeakSanitizer overhead**: Minimal runtime impact, runs at process exit
- **Valgrind overhead**: ~10-50x slowdown
- **Recommendation**: Enable in debug/testing builds, disable in production

## Comparison of Tools

| Tool | Type | Pros | Cons | Best For |
|------|------|------|------|----------|
| **MemoryLeakDetector** | Built-in | Fast, customizable, detailed reports | Manual tracking required | Quick debugging, specific categories |
| **AddressSanitizer** | Compiler | Comprehensive, automatic | Runtime overhead | Development testing |
| **LeakSanitizer** | Compiler | Focused on leaks, automatic | Runs at exit only | Continuous integration |
| **Valgrind** | External | Very comprehensive, no recompilation | Very slow | Deep analysis |
| **UBSan** | Compiler | Catches undefined behavior | Not leak-specific | Code correctness |
| **TSan** | Compiler | Detects race conditions | Incompatible with ASan | Thread safety |

## Choosing the Right Tool

### For Development
- Use **MemoryLeakDetector** for quick iteration and category-based analysis
- Enable **AddressSanitizer** for comprehensive memory error detection

### For Continuous Integration
- Use **LeakSanitizer** in CI pipelines for fast leak detection
- Run **Valgrind** nightly for comprehensive analysis

### For Debugging Specific Issues
- Use **MemoryLeakDetector** with categories for targeted leak hunting
- Use **Valgrind** for deep analysis of complex memory issues

### For Thread Safety
- Use **ThreadSanitizer** to detect data races
- Note: Cannot be combined with ASan/LSan

## Workflow Examples

### Development Workflow

```bash
# 1. Quick iteration with built-in detector
./your_program  # Uses MemoryLeakDetector

# 2. Local comprehensive check
cmake --preset leak-detection
make
./your_program

# 3. Before committing
ctest --preset leak-detection-tests
```

### CI/CD Workflow

```yaml
# .github/workflows/leak-detection.yml
name: Leak Detection

on: [push, pull_request]

jobs:
  leak-detection:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install dependencies
        run: sudo apt-get install -y cmake ninja-build
      
      - name: Configure with LeakSanitizer
        run: cmake --preset lsan
      
      - name: Build
        run: cmake --build --preset lsan
      
      - name: Run tests
        run: ctest --preset lsan-tests --output-on-failure
        env:
          LSAN_OPTIONS: verbosity=1:log_threads=1
```

### Nightly Analysis Workflow

```bash
#!/bin/bash
# nightly-leak-check.sh

# Build with Valgrind support
cmake --preset valgrind
cmake --build --preset valgrind

# Run Valgrind tests
ctest --preset valgrind-tests --output-on-failure

# Generate report
valgrind --leak-check=full --xml=yes --xml-file=leak-report.xml ./bolt_unit_tests

# Parse and report
python3 scripts/parse_valgrind_report.py leak-report.xml
```

## API Reference

See the [API Documentation](api/html/index.html) for complete class and function reference.

## Examples

Complete examples are available in:
- `examples/memory_leak_detection_example.cpp` - Comprehensive demonstration
- `test/test_memory_manager.cpp` - MemoryManager tests
- `test/test_memory_leak_detector.cpp` - MemoryLeakDetector tests

## Related Documentation

- [Memory Manager](api/html/classMemoryManager.html)
- [Error Handling](api/html/error__handling_8hpp.html)
- [Testing Guide](../TESTING.md)
- [Build System](../README.md#build-system)

## Continuous Integration

Example GitHub Actions workflow for automated leak detection:

```yaml
name: Memory Leak Detection

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  asan:
    name: AddressSanitizer
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build
      - name: Configure
        run: cmake --preset asan
      - name: Build
        run: cmake --build --preset asan
      - name: Test
        run: ctest --preset asan-tests --output-on-failure
        env:
          ASAN_OPTIONS: detect_leaks=1:halt_on_error=0
  
  lsan:
    name: LeakSanitizer
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build
      - name: Configure
        run: cmake --preset lsan
      - name: Build
        run: cmake --build --preset lsan
      - name: Test
        run: ctest --preset lsan-tests --output-on-failure
        env:
          LSAN_OPTIONS: verbosity=1:log_threads=1
  
  valgrind:
    name: Valgrind
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build valgrind
      - name: Configure
        run: cmake --preset valgrind
      - name: Build
        run: cmake --build --preset valgrind
      - name: Test
        run: ctest --preset valgrind-tests --output-on-failure
```

---

**Last Updated**: December 2024

**See Also**: [Performance Profiler](PERFORMANCE_PROFILER.md), [Logging System](LOGGING_SYSTEM.md), [Testing Guide](../TESTING.md)
