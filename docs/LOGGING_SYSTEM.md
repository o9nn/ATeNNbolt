# Bolt Logging System Documentation

## Overview

The Bolt Logging System provides comprehensive, thread-safe logging capabilities for the Bolt C++ project. It supports multiple log levels, categories, output destinations, and formatting options, with integration into the existing error handling framework.

## Features

### 🎯 Core Features
- **Multiple Log Levels**: TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF
- **Category-Based Logging**: CORE, MEMORY, EDITOR, AI, GUI, NETWORK, PLUGIN, COLLABORATION, DEBUGGER, TESTING
- **Thread-Safe Operations**: All logging operations are thread-safe using existing thread safety patterns
- **Multiple Output Sinks**: Console, file, rotating file support
- **Flexible Formatting**: Simple, detailed, and JSON formatters
- **Performance Metrics**: Track total messages, dropped messages, and timing
- **Integration with Error Handling**: Seamless integration with the existing Bolt error framework

### 🚀 Advanced Features
- **Log Filtering**: Level-based and category-based filtering
- **Rotating Logs**: Automatic log rotation based on file size
- **Scoped Logging**: Automatic function entry/exit logging with timing
- **Global Logger Management**: Singleton pattern with easy configuration
- **Macro Support**: Convenient macros for easy logging with file/line information

## Quick Start

### Basic Usage

```cpp
#include "bolt/core/logging.hpp"

// Configure global logger
bolt::LogManager::configureConsoleLogging(bolt::LogLevel::INFO);

// Basic logging
BOLT_INFO("Application started");
BOLT_WARN("This is a warning");
BOLT_ERROR("An error occurred");

// Category-specific logging
BOLT_LOG_MEMORY(bolt::LogLevel::DEBUG, "Allocated 1024 bytes");
BOLT_LOG_EDITOR(bolt::LogLevel::INFO, "File opened: example.cpp");
```

### File Logging

```cpp
// Configure file logging
bolt::LogManager::configureFileLogging("app.log", bolt::LogLevel::DEBUG);

BOLT_INFO("This will be written to app.log");
```

### Custom Logger Configuration

```cpp
// Create custom logger
auto logger = std::make_unique<bolt::Logger>(bolt::LogLevel::DEBUG);

// Add multiple sinks
logger->addSink(std::make_unique<bolt::ConsoleSink>());
logger->addSink(std::make_unique<bolt::FileSink>("debug.log"));

// Add filters
logger->addFilter(std::make_unique<bolt::LevelFilter>(bolt::LogLevel::INFO));

// Set custom formatter
logger->setFormatter(std::make_unique<bolt::JsonFormatter>());

// Set as global logger
bolt::LogManager::setGlobalLogger(std::move(logger));
```

## Log Levels

| Level | Description | Usage |
|-------|-------------|-------|
| `TRACE` | Detailed debugging information | Function entry/exit, variable values |
| `DEBUG` | General debugging information | Algorithm steps, state changes |
| `INFO` | Informational messages | Application events, status updates |
| `WARN` | Warning messages | Recoverable errors, deprecated usage |
| `ERROR` | Error messages | Unrecoverable errors, exceptions |
| `FATAL` | Fatal error messages | Critical failures, application termination |
| `OFF` | Disable logging | Production mode |

## Log Categories

| Category | Description | Usage |
|----------|-------------|-------|
| `CORE` | Core system components | Initialization, main application flow |
| `MEMORY` | Memory management | Allocations, deallocations, memory pools |
| `EDITOR` | Editor functionality | File operations, text editing, UI |
| `AI` | AI/ML components | Model loading, inference, training |
| `GUI` | User interface | Window events, user interactions |
| `NETWORK` | Network operations | HTTP requests, WebSocket connections |
| `PLUGIN` | Plugin system | Plugin loading, events, communication |
| `COLLABORATION` | Collaborative editing | Document synchronization, operational transforms |
| `DEBUGGER` | Debugger functionality | Breakpoints, variable inspection |
| `TESTING` | Test framework | Test execution, assertions |

## Formatters

### SimpleFormatter
Basic formatting with level and category:
```
[INFO] [CORE] Application started
```

### DetailedFormatter
Comprehensive formatting with timestamp, thread ID, and location:
```
[2025-08-13 23:50:34.414] [INFO] [CORE] [T:140048293877568] [main.cpp:42 in main()] Application started
```

### JsonFormatter
JSON-structured output for log aggregation systems:
```json
{"timestamp":"2025-08-13 23:50:34.414","level":"INFO","category":"CORE","thread":"140048293877568","message":"Application started","file":"main.cpp","line":42,"function":"main"}
```

## Sinks

### ConsoleSink
Outputs to stdout/stderr with color coding:
```cpp
auto sink = std::make_unique<bolt::ConsoleSink>(false); // stdout
logger->addSink(std::move(sink));
```

### FileSink
Outputs to a specified file:
```cpp
auto sink = std::make_unique<bolt::FileSink>("application.log");
logger->addSink(std::move(sink));
```

### RotatingFileSink
Automatically rotates files when size limit is reached:
```cpp
auto sink = std::make_unique<bolt::RotatingFileSink>("app.log", 10*1024*1024, 5); // 10MB, 5 files
logger->addSink(std::move(sink));
```

## Filters

### LevelFilter
Filter messages based on minimum log level:
```cpp
auto filter = std::make_unique<bolt::LevelFilter>(bolt::LogLevel::WARN);
logger->addFilter(std::move(filter));
```

### CategoryFilter
Set different levels for different categories:
```cpp
auto filter = std::make_unique<bolt::CategoryFilter>(bolt::LogLevel::INFO); // Default
filter->setCategoryLevel(bolt::LogCategory::MEMORY, bolt::LogLevel::DEBUG); // Debug for memory
filter->setCategoryLevel(bolt::LogCategory::EDITOR, bolt::LogLevel::ERROR); // Errors only for editor
logger->addFilter(std::move(filter));
```

## Convenience Macros

### Basic Logging Macros
```cpp
BOLT_TRACE("Detailed trace information");
BOLT_DEBUG("Debug information");
BOLT_INFO("Informational message");
BOLT_WARN("Warning message");
BOLT_ERROR("Error message");
BOLT_FATAL("Fatal error message");
```

### Category-Specific Macros
```cpp
BOLT_LOG_CORE(bolt::LogLevel::INFO, "Core system message");
BOLT_LOG_MEMORY(bolt::LogLevel::DEBUG, "Memory operation");
BOLT_LOG_EDITOR(bolt::LogLevel::WARN, "Editor warning");
BOLT_LOG_AI(bolt::LogLevel::INFO, "AI processing status");
```

### Scoped Logging
Automatic function entry/exit logging with timing:
```cpp
void myFunction() {
    BOLT_SCOPED_LOG_CORE(); // Logs entry and exit with timing
    
    BOLT_INFO("Doing work...");
    
    {
        BOLT_SCOPED_LOG(bolt::LogCategory::MEMORY);
        BOLT_DEBUG("Memory operations in nested scope");
    }
}
```

## Performance Considerations

### Metrics
The logging system tracks performance metrics:
```cpp
bolt::Logger& logger = bolt::LogManager::getInstance();
std::cout << "Total messages: " << logger.getTotalMessages() << std::endl;
std::cout << "Dropped messages: " << logger.getDroppedMessages() << std::endl;
```

### Optimization Tips
1. **Use appropriate log levels**: Set higher thresholds in production
2. **Category filtering**: Enable debug only for specific components
3. **Async logging**: Consider using separate thread for heavy I/O operations
4. **Macro usage**: Macros include file/line info only when needed

## Integration with Error Handling

The logging system integrates seamlessly with Bolt's error handling:

```cpp
try {
    BOLT_INFO("Starting risky operation");
    throw bolt::MemoryException(bolt::ErrorCode::MEMORY_ALLOCATION_FAILED, "Out of memory");
} catch (const bolt::BoltException& e) {
    BOLT_ERROR("Exception caught: " + std::string(e.what()));
    BOLT_DEBUG("Error code: " + std::to_string(static_cast<int>(e.getErrorCode())));
}
```

## Configuration Examples

### Development Configuration
```cpp
// Enable detailed logging for development
bolt::LogManager::configureDualLogging("debug.log", 
    bolt::LogLevel::INFO,  // Console level
    bolt::LogLevel::DEBUG  // File level
);
```

### Production Configuration
```cpp
// Minimal logging for production
bolt::LogManager::configureRotatingFileLogging("production.log", 
    50*1024*1024, // 50MB per file
    10,           // Keep 10 files
    bolt::LogLevel::WARN // Only warnings and errors
);
```

### Testing Configuration
```cpp
// Comprehensive logging for testing
auto logger = std::make_unique<bolt::Logger>(bolt::LogLevel::TRACE);
logger->addSink(std::make_unique<bolt::FileSink>("test.log"));
logger->setFormatter(std::make_unique<bolt::DetailedFormatter>(true, true, true));

// Category filter for testing specific components
auto filter = std::make_unique<bolt::CategoryFilter>(bolt::LogLevel::INFO);
filter->setCategoryLevel(bolt::LogCategory::TESTING, bolt::LogLevel::TRACE);
logger->addFilter(std::move(filter));

bolt::LogManager::setGlobalLogger(std::move(logger));
```

## Thread Safety

All logging operations are thread-safe:
```cpp
// Multiple threads can log safely
std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i) {
    threads.emplace_back([i]() {
        for (int j = 0; j < 100; ++j) {
            BOLT_INFO("Thread " + std::to_string(i) + " message " + std::to_string(j));
        }
    });
}

for (auto& thread : threads) {
    thread.join();
}
```

## Best Practices

### 1. Use Appropriate Log Levels
```cpp
// Good
BOLT_DEBUG("Processing item " + std::to_string(i) + " of " + std::to_string(total));
BOLT_INFO("File processing completed successfully");
BOLT_WARN("Configuration file not found, using defaults");
BOLT_ERROR("Failed to connect to database");

// Avoid
BOLT_INFO("Processing item " + std::to_string(i)); // Too verbose for INFO
BOLT_ERROR("User clicked button"); // Not an error
```

### 2. Use Categories Appropriately
```cpp
// Good
BOLT_LOG_MEMORY(bolt::LogLevel::DEBUG, "Allocated " + std::to_string(size) + " bytes");
BOLT_LOG_NETWORK(bolt::LogLevel::ERROR, "Connection timeout after 30 seconds");
BOLT_LOG_EDITOR(bolt::LogLevel::INFO, "File saved: " + filename);

// Avoid mixing categories
BOLT_LOG_MEMORY(bolt::LogLevel::INFO, "File saved"); // Should be EDITOR category
```

### 3. Include Relevant Context
```cpp
// Good
BOLT_ERROR("Failed to open file '" + filename + "': " + strerror(errno));
BOLT_WARN("Cache miss for key '" + key + "', loading from disk");

// Avoid
BOLT_ERROR("Failed to open file"); // Which file?
BOLT_WARN("Cache miss"); // For what?
```

### 4. Use Scoped Logging for Performance Analysis
```cpp
void expensiveOperation() {
    BOLT_SCOPED_LOG_CORE(); // Automatically logs timing
    
    // ... expensive work ...
}
```

## Testing

The logging system includes comprehensive tests covering:
- Basic logging functionality
- Multiple formatters and sinks
- Filtering mechanisms
- Thread safety
- Performance metrics
- Error handling integration

Run tests with:
```bash
./test_logging_standalone
```

## File Structure

```
include/bolt/core/logging.hpp     # Main logging interface
src/bolt/core/logging.cpp         # Implementation
test/test_logging.cpp             # Comprehensive test suite
test_logging_simple.cpp           # Standalone test
demo_logging_simple.cpp           # Demonstration program
```

## API Reference

### Classes
- `LogEntry`: Represents a single log entry
- `Logger`: Main logging class
- `LogManager`: Global logger management
- `LogFormatter`: Base class for formatters
- `LogSink`: Base class for output destinations
- `LogFilter`: Base class for filtering
- `ScopedLogger`: RAII-style function logging

### Enums
- `LogLevel`: Log severity levels
- `LogCategory`: Component categories

### Functions
- `levelToString()` / `stringToLevel()`: Level conversion
- `categoryToString()` / `stringToCategory()`: Category conversion

This comprehensive logging system provides the foundation for debugging, monitoring, and maintaining the Bolt application across all its components.