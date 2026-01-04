# Bolt C++ Unit Test Framework

## Overview

The Bolt C++ project now includes a comprehensive unit test framework that provides:

- **Custom test framework** - Self-contained, no external dependencies required
- **Rich assertion macros** - Multiple assertion types with detailed error messages
- **Test suite organization** - Group related tests into named suites
- **Flexible test execution** - Run all tests, specific suites, or individual tests
- **CMake/CTest integration** - Seamless integration with CMake's testing infrastructure
- **Performance metrics** - Execution time tracking for each test

## Features

### Assertion Macros

```cpp
BOLT_ASSERT(condition)                 // Basic assertion
BOLT_ASSERT_EQ(expected, actual)       // Equality assertion with detailed output
BOLT_ASSERT_NE(not_expected, actual)   // Not-equal assertion
BOLT_ASSERT_TRUE(condition)            // True assertion
BOLT_ASSERT_FALSE(condition)           // False assertion
BOLT_ASSERT_NULL(ptr)                  // Null pointer assertion
BOLT_ASSERT_NOT_NULL(ptr)              // Non-null pointer assertion
BOLT_ASSERT_THROWS(exception_type, code) // Exception throwing assertion
```

### Test Definition

```cpp
#include "bolt/test_framework.hpp"

BOLT_TEST(SuiteName, TestName) {
    // Test implementation
    BOLT_ASSERT_EQ(42, some_function());
}
```

### Test Runner Options

```bash
# Run all tests
./bolt_unit_tests

# Run tests in specific suite
./bolt_unit_tests Chat

# List all available tests
./bolt_unit_tests --list

# Run in quiet mode
./bolt_unit_tests --quiet

# Show help
./bolt_unit_tests --help
```

## Current Test Coverage

The framework currently includes comprehensive tests for:

### Unit Tests (45 tests total)

1. **Chat System** (7 tests)
   - Chat class functionality
   - ChatMessage creation and handling
   - Message role management

2. **Memory Management** (4 tests)
   - Memory allocation and deallocation
   - Peak usage tracking
   - Error handling

3. **Store Systems** (15 tests)
   - ChatStore state management
   - EditorStore document handling
   - WorkbenchStore view management

4. **Message Handling** (5 tests)
   - Message creation and processing
   - MessageHandler initialization
   - Message queue operations

5. **Utility Functions** (3 tests)
   - String manipulation utilities
   - Algorithm correctness

6. **Code Folding System** (11 tests)
   - Code folding range creation and management
   - Folding detection algorithms
   - Integration with editor components

### Integration Tests (9 tests total)

The integration test suite focuses on component interactions and end-to-end workflows:

1. **BoltApp Integration** (1 test)
   - Application initialization and basic functionality
   - Core store accessibility after startup

2. **Multi-Store Interactions** (1 test)
   - Chat, Editor, and Workbench store coordination
   - State consistency across components

3. **Code Folding Integration** (1 test)
   - Integration between code folding and editor systems
   - Folding manager and integrated editor coordination

4. **Message Handler Workflow** (1 test)
   - End-to-end message processing workflow
   - Integration with chat store state management

5. **Memory Management Under Load** (1 test)
   - Memory allocation during complex operations
   - Integration with store systems under heavy usage

6. **Editor Document Lifecycle** (1 test)
   - Complete document lifecycle with multiple components
   - Editor, workbench, and folding system integration

7. **Chat-Editor Workflow** (1 test)
   - Realistic user workflow simulation
   - Chat assistance with code editing scenarios

8. **Multithreaded Operations** (1 test)
   - Component behavior under simulated concurrent access
   - Thread safety validation

9. **System Shutdown Cleanup** (1 test)
   - Proper state management during shutdown simulation
   - Resource cleanup verification

## CMake Integration

The test framework integrates with CMake's CTest system:

```cmake
# Run all tests via CTest
make test

# Or directly
ctest --verbose
```

Individual test suites are registered as separate CTest targets:

### Unit Test Targets
- `basic_bolt_test` - Original basic test
- `bolt_all_tests` - All unit tests (45 tests)
- `bolt_chat_tests` - Chat-specific tests
- `bolt_memory_tests` - Memory management tests
- `bolt_store_tests` - Store system tests
- `bolt_string_tests` - String utility tests

### Integration Test Targets
- `bolt_integration_all` - All integration tests (9 tests)
- `bolt_integration_basic` - Integration test suite

## Adding New Tests

### 1. Create a test file (e.g., `test_new_feature.cpp`)

```cpp
#include "bolt/test_framework.hpp"
#include "bolt/your_component.hpp"

BOLT_TEST(YourComponent, BasicFunctionality) {
    YourComponent component;
    BOLT_ASSERT_NOT_NULL(&component);
    BOLT_ASSERT_EQ(42, component.getValue());
}

BOLT_TEST(YourComponent, ErrorHandling) {
    YourComponent component;
    BOLT_ASSERT_THROWS(std::invalid_argument, component.setInvalidValue(-1));
}
```

### 2. Add to CMakeLists.txt

```cmake
# For unit tests - add your test file to the executable
add_executable(bolt_unit_tests
    test_runner.cpp
    test_bolt_core.cpp
    test_new_feature.cpp  # Add this line
)

# For integration tests - add your test file to the integration executable
add_executable(bolt_integration_tests
    test_runner.cpp
    test_integration.cpp
    test_new_integration.cpp  # Add this line for integration tests
)

# Add suite-specific CTest target
add_test(NAME bolt_yourcomponent_tests COMMAND bolt_unit_tests YourComponent)
add_test(NAME bolt_integration_yourfeature COMMAND bolt_integration_tests YourIntegration)
```

### 3. Build and run

```bash
cd build
make
# Run unit tests
./test/bolt_unit_tests YourComponent
# Run integration tests
./test/bolt_integration_tests YourIntegration
```

## Test Categories

### Unit Tests
Unit tests focus on individual components and their isolated functionality:
- Test single classes or functions
- Mock external dependencies
- Fast execution
- No component interaction

Example unit test:
```cpp
BOLT_TEST(StringUtils, ReverseString) {
    BOLT_ASSERT_EQ("olleh", StringUtils::reverseString("hello"));
}
```

### Integration Tests
Integration tests focus on component interactions and workflows:
- Test multiple components working together
- Real component dependencies
- End-to-end workflows
- Cross-component state verification

Example integration test:
```cpp
BOLT_TEST(Integration, ChatEditorWorkflow) {
    auto& chatStore = bolt::ChatStore::getInstance();
    auto& editorStore = bolt::EditorStore::getInstance();
    
    // Test complete workflow of chat + editor interaction
    chatStore.setChatStarted(true);
    // ... test workflow
}
```

## Best Practices

### Unit Tests
1. **Test Organization**: Group related tests into logical suites
2. **Test Naming**: Use descriptive names that explain what is being tested
3. **Assertions**: Use the most specific assertion macro available
4. **Error Messages**: Assertion failures provide file, line, and context information
5. **Independence**: Each test should be independent and not rely on other tests
6. **Setup/Teardown**: Use constructor/destructor or setUp/tearDown methods for test preparation

### Integration Tests
1. **State Management**: Reset component states between tests using helper functions
2. **Realistic Workflows**: Test actual user scenarios and component interactions
3. **Component Coordination**: Verify that multiple components work together correctly
4. **Error Handling**: Test how components handle errors when working together
5. **Resource Management**: Verify proper resource allocation and cleanup
6. **Thread Safety**: Test component interactions under simulated concurrent access

## Example Test Output

### Unit Tests
```
Bolt C++ Unit Test Runner
=========================
Running 45 tests...

[Chat] BasicInstantiation ... PASS
[Chat] AddSingleMessage ... PASS
[MemoryManager] AllocateAndDeallocate ... PASS
[StringUtils] ReverseString ... PASS

Test Results:
  Passed: 45
  Failed: 0
  Total:  45

All tests passed!
```

### Integration Tests
```
Bolt C++ Unit Test Runner
=========================
Running 9 tests...

[Integration] BoltAppInitialization ... PASS
[Integration] MultiStoreInteractions ... PASS
[Integration] ChatEditorWorkflow ... PASS
[Integration] MemoryManagerUnderLoad ... PASS

Test Results:
  Passed: 9
  Failed: 0
  Total:  9

All tests passed!
```

## Future Enhancements

Potential improvements to the test framework:

1. **Test Fixtures** - Support for shared test setup and teardown
2. **Parameterized Tests** - Run the same test with different input values
3. **Mocking Support** - Integration with mocking frameworks
4. **Performance Benchmarking** - Built-in performance measurement tools
5. **Parallel Execution** - Run tests in parallel for faster execution
6. **Test Discovery** - Automatic test discovery without manual registration
7. **XML/JSON Output** - Generate test reports in standard formats

The current framework provides a solid foundation for comprehensive testing of the Bolt C++ codebase with both unit and integration test capabilities. The integration tests ensure that components work together correctly and simulate realistic user workflows, while unit tests provide focused validation of individual component functionality.