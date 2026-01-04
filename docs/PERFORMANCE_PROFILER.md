# Bolt Performance Profiler Documentation

## Overview

The Bolt Performance Profiler provides comprehensive, thread-safe performance monitoring capabilities for the Bolt C++ IDE. It enables developers to profile function execution times, monitor system resources, analyze performance bottlenecks, and export detailed performance reports.

## Features

- **Thread-Safe Profiling**: Full support for concurrent profiling across multiple threads
- **Session-Based Organization**: Group related metrics into logical sessions
- **Multiple Profiling Methods**: Function-level, scope-based, and instant metrics
- **System Monitoring**: Memory usage and CPU utilization tracking
- **Data Export**: JSON and CSV export formats for analysis
- **Integration with Logging**: Seamless integration with the existing Bolt logging system
- **Minimal Overhead**: Optimized for production use with configurable enable/disable

## Quick Start

### Basic Usage

```cpp
#include "bolt/core/performance_profiler.hpp"

// Enable the profiler
auto& profiler = bolt::PerformanceProfiler::getInstance();
profiler.enable();

// Profile a function
{
    BOLT_PROFILE_FUNCTION();
    // Your function code here
}

// Profile a specific scope
{
    BOLT_PROFILE_SCOPE("my_operation");
    // Code to profile
}

// Category-specific profiling
{
    BOLT_PROFILE_AI("model_inference");
    // AI-related code
}
```

### Session-Based Profiling

```cpp
// Create and use a session
{
    BOLT_PROFILE_SESSION("data_processing");
    
    {
        BOLT_PROFILE_SCOPE("file_loading");
        // Load files
    }
    
    {
        BOLT_PROFILE_SCOPE("data_transformation");
        // Transform data
    }
    
    // Session automatically ends when scope exits
}

// Access session data
auto session = profiler.getSession("data_processing");
if (session) {
    std::cout << "Session completed in " << session->getTotalDurationMs() << " ms" << std::endl;
    std::cout << "Total metrics: " << session->getMetricsCount() << std::endl;
}
```

## API Reference

### Core Classes

#### PerformanceProfiler

The main profiler singleton class that manages all profiling operations.

**Key Methods:**
- `getInstance()` - Get the singleton instance
- `enable() / disable()` - Control profiler state
- `createSession(name)` - Create a new profiling session
- `startMetric(name, category)` - Start a manual metric
- `endMetric(metric)` - End a manual metric
- `exportToJson(filename)` - Export data to JSON
- `exportToCsv(filename)` - Export data to CSV
- `printSummary()` - Print performance summary
- `reset()` - Clear all collected data

#### ProfilerSession

Represents a collection of related performance metrics.

**Key Methods:**
- `getName()` - Get session name
- `isActive()` - Check if session is active
- `getMetricsCount()` - Get number of metrics
- `getTotalDurationMs()` - Get total session duration
- `getAverageDurationMs()` - Get average metric duration

#### ScopedProfiler

RAII class for automatic profiling of scopes.

**Key Methods:**
- `ScopedProfiler(name, category)` - Constructor starts profiling
- `~ScopedProfiler()` - Destructor ends profiling
- `addMetadata(key, value)` - Add custom metadata
- `getCurrentDurationMs()` - Get current elapsed time

### Convenience Macros

#### Basic Profiling
```cpp
BOLT_PROFILE_FUNCTION()         // Profile current function
BOLT_PROFILE_SCOPE(name)        // Profile named scope
BOLT_PROFILE_CATEGORY(name, cat) // Profile with specific category
```

#### Category-Specific Macros
```cpp
BOLT_PROFILE_CORE(name)         // Core system profiling
BOLT_PROFILE_MEMORY(name)       // Memory operations
BOLT_PROFILE_EDITOR(name)       // Editor functionality
BOLT_PROFILE_AI(name)           // AI/ML operations
BOLT_PROFILE_GUI(name)          // User interface
BOLT_PROFILE_NETWORK(name)      // Network operations
```

#### Session Management
```cpp
BOLT_PROFILE_SESSION(name)      // Create and activate session
```

## Performance Categories

The profiler supports the following predefined categories:

| Category | Description | Usage |
|----------|-------------|--------|
| `CORE` | Core system components | Initialization, main application flow |
| `MEMORY` | Memory management | Allocations, deallocations, memory pools |
| `EDITOR` | Editor functionality | File operations, text editing, syntax highlighting |
| `AI` | AI/ML components | Model loading, inference, training |
| `GUI` | User interface | Window events, user interactions, rendering |
| `NETWORK` | Network operations | HTTP requests, WebSocket connections |
| `PLUGIN` | Plugin system | Plugin loading, events, communication |
| `COLLABORATION` | Collaborative editing | Document synchronization, operational transforms |
| `DEBUGGER` | Debugger functionality | Breakpoints, variable inspection |
| `TESTING` | Test framework | Test execution, assertions |

## Advanced Features

### System Monitoring

Enable system resource monitoring to track memory and CPU usage:

```cpp
profiler.enableSystemMonitoring();

{
    BOLT_PROFILE_SCOPE("memory_intensive_operation");
    // Code that allocates memory
}
// Metric will include memory usage data
```

### Manual Profiling

For fine-grained control over profiling:

```cpp
auto metric = profiler.startMetric("custom_operation", "CUSTOM");
// Perform operation
profiler.endMetric(metric);

// Instant metrics for known durations
profiler.recordInstantMetric("cached_result", 5.5, "CACHE");
```

### Data Export and Analysis

Export profiling data for external analysis:

```cpp
// Export to JSON for detailed analysis
profiler.exportToJson("/tmp/performance_data.json");

// Export to CSV for spreadsheet analysis
profiler.exportToCsv("/tmp/performance_data.csv");

// Print reports to console
profiler.printSummary();        // Brief summary
profiler.printDetailedReport(); // Detailed breakdown
```

### Thread Safety

The profiler is fully thread-safe and supports concurrent profiling:

```cpp
std::vector<std::thread> workers;

for (int i = 0; i < 4; ++i) {
    workers.emplace_back([i]() {
        // Each thread can have its own session
        auto session = profiler.createSession("worker_" + std::to_string(i));
        profiler.setCurrentSession("worker_" + std::to_string(i));
        
        {
            BOLT_PROFILE_SCOPE("thread_work");
            // Thread-specific work
        }
    });
}

for (auto& worker : workers) {
    worker.join();
}
```

### Integration with Logging System

The profiler integrates seamlessly with the Bolt logging system:

```cpp
// Profiler automatically logs metrics when they complete
{
    BOLT_PROFILE_SCOPE("logged_operation");
    // Work here
}
// Automatically logs: "PERF [SCOPE] logged_operation completed in X ms"

// Manual session logging
auto session = profiler.getSession("my_session");
profiler.logSessionSummary(*session, bolt::LogLevel::INFO);
```

## Configuration

### Enable/Disable Profiling

```cpp
// Enable profiling (default)
profiler.enable();

// Disable profiling (zero overhead when disabled)
profiler.disable();

// Check state
if (profiler.isEnabled()) {
    // Profiling is active
}
```

### System Monitoring

```cpp
// Enable system resource monitoring
profiler.enableSystemMonitoring();

// Disable system resource monitoring (default)
profiler.disableSystemMonitoring();
```

## Best Practices

### 1. Use Appropriate Categories
```cpp
// Good - specific category
BOLT_PROFILE_AI("model_inference");

// Avoid - generic category
BOLT_PROFILE_SCOPE("work");
```

### 2. Use Sessions for Related Operations
```cpp
// Good - group related metrics
{
    BOLT_PROFILE_SESSION("file_processing");
    BOLT_PROFILE_SCOPE("load_file");
    BOLT_PROFILE_SCOPE("parse_content");
    BOLT_PROFILE_SCOPE("save_result");
}
```

### 3. Profile at Appropriate Granularity
```cpp
// Good - meaningful operations
BOLT_PROFILE_SCOPE("database_query");

// Avoid - too fine-grained
BOLT_PROFILE_SCOPE("add_two_numbers");
```

### 4. Clean Up Old Data
```cpp
// Periodically clean up old metrics
profiler.clearOldMetrics(std::chrono::hours(1));

// Reset all data when needed
profiler.reset();
```

## Performance Considerations

- **Overhead**: Profiling adds minimal overhead (~2-5μs per operation)
- **Memory**: Each metric uses approximately 200-300 bytes
- **Thread Safety**: All operations are thread-safe with minimal contention
- **Disable in Production**: Can be completely disabled for zero overhead

## Example Output

### JSON Export
```json
{
  "sessions": [
    {
      "name": "ai_inference_session",
      "totalDuration": 250.5,
      "metricsCount": 3,
      "metrics": [
        {
          "name": "model_loading",
          "category": "AI",
          "duration": 100.2,
          "memoryUsage": 16777216
        }
      ]
    }
  ]
}
```

### CSV Export
```csv
Session,Name,Category,Duration(ms),MemoryUsage(bytes)
ai_inference_session,model_loading,AI,100.2,16777216
ai_inference_session,inference,AI,45.3,16777216
Global,cache_lookup,MEMORY,2.1,0
```

## Testing

Run the comprehensive test suite:
```bash
./test_performance_profiler_standalone
```

Try the demonstration program:
```bash
./demo_performance_profiler_standalone
```

## Integration Examples

### With AI Components
```cpp
void processWithAI(const std::string& input) {
    BOLT_PROFILE_AI("ai_processing");
    
    {
        BOLT_PROFILE_SCOPE("model_loading");
        // Load AI model
    }
    
    {
        BOLT_PROFILE_SCOPE("inference");
        // Run inference
    }
}
```

### With Editor Operations
```cpp
void openFile(const std::string& filename) {
    BOLT_PROFILE_EDITOR("file_open");
    
    {
        BOLT_PROFILE_SCOPE("read_file");
        // Read file content
    }
    
    {
        BOLT_PROFILE_SCOPE("syntax_highlighting");
        // Apply syntax highlighting
    }
}
```

### With Memory Management
```cpp
void allocateBuffer(size_t size) {
    BOLT_PROFILE_MEMORY("buffer_allocation");
    
    // Memory allocation code
    auto* buffer = new char[size];
    
    // Add metadata about allocation
    auto metric = profiler.getCurrentSession();
    if (metric) {
        // Could add size information
    }
    
    delete[] buffer;
}
```

## Troubleshooting

### High Overhead
- Disable system monitoring if not needed
- Profile larger operations rather than micro-operations
- Consider disabling in performance-critical sections

### Memory Usage
- Use `clearOldMetrics()` to clean up old data
- Reset profiler periodically with `reset()`
- Limit session lifetime for long-running applications

### Thread Safety Issues
- Each thread maintains its own current session
- Sessions are shared but metrics are thread-safe
- Use unique session names for different threads

## File Structure

```
include/bolt/core/performance_profiler.hpp    # Main profiler interface
src/bolt/core/performance_profiler.cpp        # Implementation
test_performance_profiler.cpp                 # Comprehensive test suite
demo_performance_profiler.cpp                 # Demonstration program
docs/PERFORMANCE_PROFILER.md                  # This documentation
```

## Future Enhancements

- GPU profiling support
- Network latency tracking
- Automatic bottleneck detection
- Performance regression testing
- Web-based performance visualization
- Integration with external profiling tools