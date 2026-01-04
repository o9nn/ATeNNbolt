# CTest Custom Configuration for Bolt C++ ML
# This file configures CTest behavior for the test suite

# ============================================
# Test Timeout Configuration
# ============================================
# Default timeout for tests (in seconds)
set(CTEST_TEST_TIMEOUT 120)

# ============================================
# Memory Check Configuration (Valgrind/ASan)
# ============================================
set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --show-reachable=yes --track-origins=yes")
set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/valgrind.supp")

# ============================================
# Coverage Configuration
# ============================================
set(CTEST_COVERAGE_COMMAND "gcov")
set(CTEST_COVERAGE_EXTRA_FLAGS "-l -p")

# ============================================
# Test Output Customization
# ============================================
# Patterns to match in output that indicate errors/warnings
set(CTEST_CUSTOM_ERROR_MATCH
    "error:"
    "Error:"
    "ERROR:"
    "FAIL"
    "FAILED"
    "Assertion failed"
    "Segmentation fault"
    "SIGSEGV"
    "SIGABRT"
    "memory leak"
    "AddressSanitizer"
    "LeakSanitizer"
    "UndefinedBehaviorSanitizer"
    "ThreadSanitizer"
)

set(CTEST_CUSTOM_WARNING_MATCH
    "warning:"
    "Warning:"
    "WARNING:"
    "deprecated"
)

# Patterns to exclude from error matching (false positives)
set(CTEST_CUSTOM_ERROR_EXCEPTION
    "No errors detected"
    "0 errors"
    "error: 0"
)

set(CTEST_CUSTOM_WARNING_EXCEPTION
    "0 warnings"
    "warning: 0"
)

# ============================================
# Test Labels
# ============================================
# Available labels for filtering tests:
# - Unit: Unit tests
# - Integration: Integration tests
# - E2E: End-to-end tests
# - Core: Core functionality tests
# - Editor: Editor component tests
# - AI: AI/ML related tests
# - LSP: Language Server Protocol tests
# - ErrorHandling: Error handling tests
# - Performance: Performance benchmarks

# ============================================
# Test Groups
# ============================================
# Run specific test groups using:
#   ctest -L Unit        # Run all unit tests
#   ctest -L E2E         # Run all E2E tests
#   ctest -L "AI|LSP"    # Run AI or LSP tests
#   ctest -E "valgrind"  # Exclude valgrind tests

# ============================================
# Parallel Test Execution
# ============================================
# Tests can be run in parallel:
#   ctest -j$(nproc)     # Linux/macOS
#   ctest -j%NUMBER_OF_PROCESSORS%  # Windows

# ============================================
# Test Discovery (for GoogleTest)
# ============================================
set(CTEST_GIT_COMMAND "git")
set(CTEST_GIT_UPDATE_OPTIONS "-q")

# ============================================
# Custom Test Scripts
# ============================================
# Pre-test script (runs before all tests)
# set(CTEST_CUSTOM_PRE_TEST "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pre-test.sh")

# Post-test script (runs after all tests)
# set(CTEST_CUSTOM_POST_TEST "${CMAKE_CURRENT_SOURCE_DIR}/scripts/post-test.sh")

# ============================================
# Maximum Output Size
# ============================================
set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 102400)  # 100KB
set(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE 512000)  # 500KB

# ============================================
# Test Cost (for scheduling)
# ============================================
# Tests with higher costs run first to optimize parallel execution
# Costs are automatically calculated based on execution time

# ============================================
# Resource Locks
# ============================================
# Use resource locks to prevent concurrent access to shared resources
# Example: RESOURCE_LOCK "database" for tests that use a shared DB

# ============================================
# Dashboard Configuration
# ============================================
set(CTEST_PROJECT_NAME "Bolt")
set(CTEST_NIGHTLY_START_TIME "00:00:00 UTC")

# ============================================
# Build Configuration
# ============================================
set(CTEST_CMAKE_GENERATOR "Ninja")
set(CTEST_BUILD_CONFIGURATION "Release")

# ============================================
# Test Retry Configuration
# ============================================
# Number of times to retry failed tests
set(CTEST_REPEAT_UNTIL_FAIL 0)
set(CTEST_REPEAT_UNTIL_PASS 0)

# ============================================
# Test Exclusions (disabled tests)
# ============================================
set(CTEST_CUSTOM_TESTS_IGNORE
    # Add any tests to skip here
    # "test_name_to_ignore"
)

# ============================================
# Memory Check Suppressions
# ============================================
set(CTEST_CUSTOM_MEMCHECK_IGNORE
    # Tests that legitimately fail memcheck
)

# ============================================
# Environment Variables for Tests
# ============================================
# Set environment variables that apply to all tests
# set(ENV{BOLT_TEST_MODE} "1")
# set(ENV{BOLT_LOG_LEVEL} "debug")
