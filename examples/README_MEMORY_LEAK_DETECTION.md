# Memory Leak Detection Tools

This directory contains comprehensive memory leak detection tools and examples for Bolt C++ ML.

## Quick Start

### Run the Example

```bash
# Build the example
mkdir build && cd build
cmake ..
make memory_leak_detection_example

# Run it
./memory_leak_detection_example
```

### Run with Sanitizers

```bash
# AddressSanitizer
cmake --preset asan
cmake --build --preset asan
ASAN_OPTIONS=detect_leaks=1 ./memory_leak_detection_example

# LeakSanitizer
cmake --preset lsan
cmake --build --preset lsan
LSAN_OPTIONS=verbosity=1 ./memory_leak_detection_example

# Comprehensive (ASan + LSan)
cmake --preset leak-detection
cmake --build --preset leak-detection
./memory_leak_detection_example
```

### Run Tests

```bash
# Unit tests
cd build
export LD_LIBRARY_PATH=$PWD:$PWD/ggml/ggml.cpp/src:$LD_LIBRARY_PATH
./test/bolt_unit_tests MemoryLeakDetector
./test/bolt_sanitizer_tests

# Or use CTest
ctest -R memory_leak
ctest -R sanitizer
```

## Available Tools

### 1. Built-in MemoryLeakDetector
- **Location**: `include/bolt/core/memory_leak_detector.hpp`
- **Features**: Source tracking, categorization, detailed reports
- **Overhead**: ~20-50ns per allocation
- **Use case**: Development, quick debugging

### 2. AddressSanitizer (ASan)
- **Build flag**: `-DENABLE_SANITIZER_ADDRESS=ON`
- **Features**: Comprehensive memory error detection
- **Overhead**: ~2x slowdown, 2-3x memory
- **Use case**: Development, CI testing

### 3. LeakSanitizer (LSan)
- **Build flag**: `-DENABLE_SANITIZER_LEAK=ON`
- **Features**: Focused leak detection at exit
- **Overhead**: Minimal runtime impact
- **Use case**: CI pipelines, automated testing

### 4. Valgrind
- **Build flag**: `-DENABLE_VALGRIND=ON`
- **Features**: Most comprehensive analysis
- **Overhead**: ~10-50x slowdown
- **Use case**: Deep debugging, nightly analysis

## Files

```
examples/
  memory_leak_detection_example.cpp  - Comprehensive example
test/
  test_memory_leak_detector.cpp     - Unit tests
  test_sanitizer_integration.cpp    - Sanitizer integration tests
cmake/
  Sanitizers.cmake                  - CMake sanitizer support
  valgrind.supp                     - Valgrind suppressions
docs/
  MEMORY_LEAK_DETECTION.md          - Full documentation
.github/workflows/
  memory-leak-detection.yml         - CI workflow
```

## CMake Presets

Use predefined presets for convenience:

```bash
# See available presets
cmake --list-presets

# Use a preset
cmake --preset asan          # AddressSanitizer
cmake --preset lsan          # LeakSanitizer
cmake --preset leak-detection # Both ASan + LSan
cmake --preset valgrind      # Valgrind support
```

## Environment Variables

### AddressSanitizer
```bash
export ASAN_OPTIONS="
    detect_leaks=1
    halt_on_error=0
    verbosity=1
    log_path=asan.log
"
```

### LeakSanitizer
```bash
export LSAN_OPTIONS="
    verbosity=1
    log_threads=1
    suppressions=lsan.supp
"
```

### Valgrind
```bash
valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --verbose \
  --suppressions=cmake/valgrind.supp \
  ./your_program
```

## Common Workflows

### Development
```bash
# Quick iteration
./memory_leak_detection_example

# Comprehensive local check
cmake --preset leak-detection
make
./test/bolt_sanitizer_tests
```

### Before Commit
```bash
# Run all leak detection tests
ctest -R "memory_leak|sanitizer"
```

### CI/CD
The GitHub Actions workflow automatically runs:
- AddressSanitizer tests
- LeakSanitizer tests
- Comprehensive leak detection
- Valgrind analysis (optional)

### Deep Analysis
```bash
# Build with Valgrind support
cmake --preset valgrind
make

# Run with Valgrind
valgrind --leak-check=full ./test/bolt_unit_tests
```

## Documentation

For complete documentation, see:
- [MEMORY_LEAK_DETECTION.md](../docs/MEMORY_LEAK_DETECTION.md) - Full guide
- [examples/memory_leak_detection_example.cpp](memory_leak_detection_example.cpp) - Example code
- [CMake Integration](../cmake/Sanitizers.cmake) - Build system

## Troubleshooting

### "Sanitizer not enabled"
Make sure to configure with the appropriate flag:
```bash
cmake -DENABLE_SANITIZER_ADDRESS=ON ..
```

### "Valgrind not found"
Install Valgrind:
```bash
sudo apt-get install valgrind  # Ubuntu/Debian
sudo yum install valgrind      # RHEL/CentOS
```

### False positives
Add suppressions to `cmake/valgrind.supp` or create a custom suppression file.

### Performance issues
Use sanitizers only in debug/testing builds:
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Disable sanitizers in release
endif()
```

## Contributing

When adding new features:
1. Add tests to `test/test_sanitizer_integration.cpp`
2. Update documentation in `docs/MEMORY_LEAK_DETECTION.md`
3. Run all leak detection tests before committing
4. Ensure CI passes

## Resources

- [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [LeakSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer)
- [Valgrind](https://valgrind.org/docs/manual/manual.html)
- [CMake Testing](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
