# Integrated Debugger Interface

The Bolt C++ ML integrated debugger provides comprehensive debugging capabilities for DIS VM programs, enabling developers to inspect code execution, manage breakpoints, step through code, and monitor variables in real-time.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Core Features](#core-features)
- [Getting Started](#getting-started)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Integration with Editor](#integration-with-editor)
- [Advanced Features](#advanced-features)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

## Overview

The integrated debugger interface provides a seamless debugging experience for DIS VM programs, integrating directly with the Bolt editor and providing both programmatic and UI-based debugging capabilities.

### Key Capabilities

- **Breakpoint Management**: Set, remove, enable/disable breakpoints at specific program counter (PC) locations or source lines
- **Execution Control**: Step over, step into, step out, continue, and pause execution
- **State Inspection**: Examine call stack, variables, program counter, and instruction information
- **Watch Expressions**: Monitor specific expressions or variables during debugging
- **Event Notifications**: Receive callbacks for debug events (breakpoint hit, step completed, etc.)
- **UI Integration**: Visual debugging interface with panels for controls, breakpoints, call stack, variables, and output
- **Editor Integration**: Highlight current execution line, display breakpoint markers, and sync with editor state

## Architecture

The debugger system consists of three main components:

### 1. DebuggerInterface

Core debugging functionality that manages the debugging session and interacts with the DIS VM.

```
┌─────────────────────────────────────┐
│     DebuggerInterface               │
│                                     │
│  ┌──────────────────────────────┐  │
│  │  Session Management          │  │
│  │  - Start/Stop                │  │
│  │  - State tracking            │  │
│  └──────────────────────────────┘  │
│                                     │
│  ┌──────────────────────────────┐  │
│  │  Execution Control           │  │
│  │  - Continue/Step/Pause       │  │
│  └──────────────────────────────┘  │
│                                     │
│  ┌──────────────────────────────┐  │
│  │  Breakpoint Management       │  │
│  │  - Set/Remove/Toggle         │  │
│  │  - Enable/Disable            │  │
│  └──────────────────────────────┘  │
│                                     │
│  ┌──────────────────────────────┐  │
│  │  State Inspection            │  │
│  │  - PC, Stack, Variables      │  │
│  └──────────────────────────────┘  │
└─────────────────────────────────────┘
           │
           ▼
    ┌────────────┐
    │   DIS VM   │
    └────────────┘
```

### 2. DebuggerUI

Visual interface for debugging, providing panels for controls, breakpoints, call stack, variables, watch expressions, and debug output.

### 3. IntegratedEditor Integration

Seamless integration with the editor for visual debugging feedback and source-level debugging.

## Core Features

### Debug States

The debugger operates in one of five states:

| State | Description |
|-------|-------------|
| `STOPPED` | No debugging session active |
| `RUNNING` | Code is executing |
| `PAUSED` | Execution paused (at breakpoint or after step) |
| `FINISHED` | Program execution completed |
| `ERROR` | Error occurred during debugging |

### Debug Events

The debugger fires events for important debugging actions:

| Event | Description |
|-------|-------------|
| `SESSION_STARTED` | Debug session initialized |
| `SESSION_STOPPED` | Debug session ended |
| `BREAKPOINT_HIT` | Execution paused at breakpoint |
| `STEP_COMPLETED` | Step operation completed |
| `EXECUTION_FINISHED` | Program finished execution |
| `ERROR_OCCURRED` | Error during debugging |

## Getting Started

### Basic Setup

```cpp
#include "bolt/editor/debugger_interface.hpp"
#include "bolt/editor/debugger_ui.hpp"

using namespace bolt;

// Create debugger instance
auto debugger = std::make_unique<DebuggerInterface>();

// Set up event callback (optional)
debugger->set_event_callback([](DebugEvent event, const std::string& message) {
    std::cout << "Debug Event: " << message << std::endl;
});

// Start debugging a Limbo program
std::string program = R"(
    print "Hello, World!"
    ai_chat "Test message"
    halt
)";

if (debugger->start_debug_session_from_source(program)) {
    std::cout << "Debug session started" << std::endl;
    
    // Set a breakpoint at PC 1
    debugger->set_breakpoint(1);
    
    // Step through execution
    debugger->step_over();
    
    // Continue execution
    debugger->continue_execution();
    
    // Stop session
    debugger->stop_debug_session();
}
```

### With UI

```cpp
#include "bolt/editor/debugger_interface.hpp"
#include "bolt/editor/debugger_ui.hpp"

using namespace bolt;

// Create debugger and UI
auto debugger = std::make_shared<DebuggerInterface>();
auto ui = std::make_unique<DebuggerUI>();

// Connect UI to debugger
ui->set_debugger(debugger);
ui->set_visible(true);

// Start debugging
std::string program = "print \"UI Demo\" halt";
if (debugger->start_debug_session_from_source(program)) {
    // UI will display debug state
    ui->render();
    
    // Handle user interactions
    ui->handle_step_over_clicked();
    ui->handle_continue_clicked();
}

ui->set_visible(false);
```

## API Reference

### Session Management

#### `bool start_debug_session(const DISProgram& program)`
Start a debug session with a pre-compiled DIS program.

**Parameters:**
- `program`: The DIS program to debug

**Returns:** `true` if session started successfully, `false` otherwise

**Example:**
```cpp
auto program = DISProgramFactory::create_echo_server();
if (debugger->start_debug_session(program)) {
    // Debugging session active
}
```

#### `bool start_debug_session_from_source(const std::string& limbo_source)`
Start a debug session by compiling Limbo source code.

**Parameters:**
- `limbo_source`: Limbo source code to compile and debug

**Returns:** `true` if session started successfully, `false` otherwise

**Example:**
```cpp
std::string source = "print \"Hello\" halt";
if (debugger->start_debug_session_from_source(source)) {
    // Debugging session active
}
```

#### `void stop_debug_session()`
Stop the current debugging session.

**Example:**
```cpp
debugger->stop_debug_session();
```

#### `bool is_debugging() const`
Check if a debugging session is active.

**Returns:** `true` if debugging is active, `false` otherwise

#### `DebugState get_debug_state() const`
Get the current debug state.

**Returns:** Current `DebugState` (STOPPED, RUNNING, PAUSED, FINISHED, ERROR)

### Execution Control

#### `void continue_execution()`
Continue execution until next breakpoint or program completion.

**Example:**
```cpp
debugger->continue_execution();
```

#### `void step_over()`
Execute the current instruction and pause at the next instruction (does not step into function calls).

**Example:**
```cpp
debugger->step_over();
```

#### `void step_into()`
Execute the current instruction, stepping into function calls.

**Example:**
```cpp
debugger->step_into();
```

#### `void step_out()`
Execute until the current function returns.

**Example:**
```cpp
debugger->step_out();
```

#### `void pause_execution()`
Pause execution if currently running.

**Example:**
```cpp
debugger->pause_execution();
```

### Breakpoint Management

#### `bool set_breakpoint(size_t pc)`
Set a breakpoint at the specified program counter.

**Parameters:**
- `pc`: Program counter location

**Returns:** `true` if breakpoint was set successfully, `false` otherwise

**Example:**
```cpp
debugger->set_breakpoint(5);  // Set breakpoint at PC 5
```

#### `bool set_breakpoint_at_line(const std::string& file_path, size_t line)`
Set a breakpoint at a specific line in a source file.

**Parameters:**
- `file_path`: Path to the source file
- `line`: Line number (0-based)

**Returns:** `true` if breakpoint was set successfully, `false` otherwise

**Example:**
```cpp
debugger->set_breakpoint_at_line("main.limbo", 10);
```

#### `bool remove_breakpoint(size_t pc)`
Remove a breakpoint at the specified program counter.

**Parameters:**
- `pc`: Program counter location

**Returns:** `true` if breakpoint was removed successfully, `false` otherwise

#### `bool remove_breakpoint_at_line(const std::string& file_path, size_t line)`
Remove a breakpoint at a specific line in a source file.

**Parameters:**
- `file_path`: Path to the source file
- `line`: Line number (0-based)

**Returns:** `true` if breakpoint was removed successfully, `false` otherwise

#### `void clear_all_breakpoints()`
Remove all breakpoints.

**Example:**
```cpp
debugger->clear_all_breakpoints();
```

#### `bool toggle_breakpoint(size_t pc)`
Toggle a breakpoint at the specified program counter (add if absent, remove if present).

**Parameters:**
- `pc`: Program counter location

**Returns:** `true` if operation succeeded, `false` otherwise

**Example:**
```cpp
debugger->toggle_breakpoint(3);  // Toggle breakpoint at PC 3
```

#### `bool enable_breakpoint(size_t pc, bool enabled)`
Enable or disable a breakpoint without removing it.

**Parameters:**
- `pc`: Program counter location
- `enabled`: `true` to enable, `false` to disable

**Returns:** `true` if operation succeeded, `false` otherwise

**Example:**
```cpp
debugger->enable_breakpoint(5, false);  // Disable breakpoint at PC 5
```

#### `std::vector<BreakpointInfo> get_all_breakpoints() const`
Get all breakpoints.

**Returns:** Vector of `BreakpointInfo` structures

**Example:**
```cpp
auto breakpoints = debugger->get_all_breakpoints();
for (const auto& bp : breakpoints) {
    std::cout << "Breakpoint at PC " << bp.pc 
              << (bp.enabled ? " (enabled)" : " (disabled)") << std::endl;
}
```

#### `bool has_breakpoint(size_t pc) const`
Check if a breakpoint exists at the specified program counter.

**Parameters:**
- `pc`: Program counter location

**Returns:** `true` if breakpoint exists, `false` otherwise

### State Inspection

#### `size_t get_current_pc() const`
Get the current program counter.

**Returns:** Current PC value

**Example:**
```cpp
size_t pc = debugger->get_current_pc();
std::cout << "Current PC: " << pc << std::endl;
```

#### `std::string get_current_instruction() const`
Get the instruction at the current program counter.

**Returns:** String representation of the current instruction

**Example:**
```cpp
std::string instruction = debugger->get_current_instruction();
std::cout << "Current instruction: " << instruction << std::endl;
```

#### `std::vector<size_t> get_call_stack() const`
Get the call stack.

**Returns:** Vector of program counters representing the call stack

**Example:**
```cpp
auto stack = debugger->get_call_stack();
std::cout << "Call stack depth: " << stack.size() << std::endl;
for (size_t i = 0; i < stack.size(); ++i) {
    std::cout << "  Frame " << i << ": PC " << stack[i] << std::endl;
}
```

#### `size_t get_call_stack_depth() const`
Get the call stack depth.

**Returns:** Number of call frames on the stack

#### `std::vector<std::string> get_stack_contents() const`
Get the contents of the data stack.

**Returns:** Vector of strings representing stack values

#### `std::map<std::string, std::string> get_global_variables() const`
Get global variables and their values.

**Returns:** Map of variable names to values

### Watch Expressions

#### `bool add_watch_expression(const std::string& expression)`
Add a watch expression to monitor during debugging.

**Parameters:**
- `expression`: Expression to watch

**Returns:** `true` if added successfully, `false` if already exists

**Example:**
```cpp
debugger->add_watch_expression("counter");
debugger->add_watch_expression("message.length");
```

#### `bool remove_watch_expression(const std::string& expression)`
Remove a watch expression.

**Parameters:**
- `expression`: Expression to remove

**Returns:** `true` if removed successfully, `false` if not found

#### `void clear_watch_expressions()`
Remove all watch expressions.

**Example:**
```cpp
debugger->clear_watch_expressions();
```

#### `std::vector<WatchExpression> get_watch_expressions() const`
Get all watch expressions.

**Returns:** Vector of `WatchExpression` structures

**Example:**
```cpp
auto watches = debugger->get_watch_expressions();
for (const auto& watch : watches) {
    std::cout << watch.expression << " = " << watch.value 
              << " (" << watch.type << ")" << std::endl;
}
```

#### `void update_watch_expressions()`
Update all watch expression values.

**Example:**
```cpp
debugger->update_watch_expressions();
```

### Event Handling

#### `void set_event_callback(DebugEventCallback callback)`
Set a callback function to receive debug events.

**Parameters:**
- `callback`: Function to call on debug events

**Example:**
```cpp
debugger->set_event_callback([](DebugEvent event, const std::string& message) {
    switch (event) {
        case DebugEvent::SESSION_STARTED:
            std::cout << "Session started: " << message << std::endl;
            break;
        case DebugEvent::BREAKPOINT_HIT:
            std::cout << "Breakpoint hit: " << message << std::endl;
            break;
        case DebugEvent::EXECUTION_FINISHED:
            std::cout << "Execution finished: " << message << std::endl;
            break;
        default:
            std::cout << "Event: " << message << std::endl;
    }
});
```

### Editor Integration

#### `void set_editor(IntegratedEditor* editor)`
Connect the debugger to an integrated editor instance.

**Parameters:**
- `editor`: Pointer to the editor instance

**Example:**
```cpp
auto& editor = IntegratedEditor::getInstance();
debugger->set_editor(&editor);
```

#### `void highlight_current_line()`
Highlight the current execution line in the editor.

#### `void clear_current_line_highlight()`
Clear the current execution line highlight.

#### `void refresh_breakpoint_markers()`
Update breakpoint markers in the editor.

### Program Information

#### `size_t get_program_size() const`
Get the total number of instructions in the program.

**Returns:** Program size in instructions

#### `std::string get_instruction_at(size_t pc) const`
Get the instruction at a specific program counter.

**Parameters:**
- `pc`: Program counter location

**Returns:** String representation of the instruction

#### `bool is_valid_pc(size_t pc) const`
Check if a program counter is valid.

**Parameters:**
- `pc`: Program counter to check

**Returns:** `true` if valid, `false` otherwise

### Debug Output and Logging

#### `void enable_debug_output(bool enabled)`
Enable or disable debug output logging.

**Parameters:**
- `enabled`: `true` to enable, `false` to disable

**Example:**
```cpp
debugger->enable_debug_output(true);
```

#### `std::vector<std::string> get_debug_log() const`
Get the debug output log.

**Returns:** Vector of log messages

**Example:**
```cpp
auto log = debugger->get_debug_log();
for (const auto& message : log) {
    std::cout << message << std::endl;
}
```

#### `void clear_debug_log()`
Clear the debug output log.

**Example:**
```cpp
debugger->clear_debug_log();
```

## Usage Examples

### Example 1: Basic Debugging Session

```cpp
#include "bolt/editor/debugger_interface.hpp"
#include <iostream>

using namespace bolt;

int main() {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    // Set up event callback
    debugger->set_event_callback([](DebugEvent event, const std::string& message) {
        std::cout << "Event: " << message << std::endl;
    });
    
    // Create a simple program
    std::string program = R"(
        print "Starting..."
        ai_chat "Hello AI"
        print "Done"
        halt
    )";
    
    // Start debugging
    if (debugger->start_debug_session_from_source(program)) {
        std::cout << "Debug session started" << std::endl;
        
        // Set breakpoint at second instruction
        debugger->set_breakpoint(1);
        
        // Continue until breakpoint
        debugger->continue_execution();
        
        // Check current location
        std::cout << "Current PC: " << debugger->get_current_pc() << std::endl;
        std::cout << "Current instruction: " << debugger->get_current_instruction() << std::endl;
        
        // Step over next instruction
        debugger->step_over();
        
        // Continue to completion
        debugger->continue_execution();
        
        // Stop debugging
        debugger->stop_debug_session();
        std::cout << "Debug session ended" << std::endl;
    }
    
    return 0;
}
```

### Example 2: Watch Expressions

```cpp
#include "bolt/editor/debugger_interface.hpp"
#include <iostream>

using namespace bolt;

int main() {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    std::string program = "print \"test\" halt";
    
    if (debugger->start_debug_session_from_source(program)) {
        // Add watch expressions
        debugger->add_watch_expression("counter");
        debugger->add_watch_expression("total_items");
        debugger->add_watch_expression("status.message");
        
        // Step through and monitor watches
        while (debugger->get_debug_state() == DebugState::PAUSED) {
            // Update watch values
            debugger->update_watch_expressions();
            
            // Display watch values
            auto watches = debugger->get_watch_expressions();
            std::cout << "Watch Expressions:" << std::endl;
            for (const auto& watch : watches) {
                std::cout << "  " << watch.expression << " = " << watch.value << std::endl;
            }
            
            // Step to next instruction
            debugger->step_over();
        }
        
        debugger->stop_debug_session();
    }
    
    return 0;
}
```

### Example 3: Breakpoint Management

```cpp
#include "bolt/editor/debugger_interface.hpp"
#include <iostream>

using namespace bolt;

int main() {
    auto debugger = std::make_unique<DebuggerInterface>();
    
    std::string program = R"(
        print "Line 1"
        print "Line 2"
        print "Line 3"
        print "Line 4"
        print "Line 5"
        halt
    )";
    
    if (debugger->start_debug_session_from_source(program)) {
        // Set multiple breakpoints
        debugger->set_breakpoint(1);
        debugger->set_breakpoint(3);
        debugger->set_breakpoint(5);
        
        // List all breakpoints
        auto breakpoints = debugger->get_all_breakpoints();
        std::cout << "Active breakpoints: " << breakpoints.size() << std::endl;
        for (const auto& bp : breakpoints) {
            std::cout << "  PC " << bp.pc 
                      << (bp.enabled ? " (enabled)" : " (disabled)") << std::endl;
        }
        
        // Disable a breakpoint
        debugger->enable_breakpoint(3, false);
        
        // Continue execution (will hit PC 1, skip PC 3, hit PC 5)
        debugger->continue_execution();
        std::cout << "Hit breakpoint at PC " << debugger->get_current_pc() << std::endl;
        
        debugger->continue_execution();
        std::cout << "Hit breakpoint at PC " << debugger->get_current_pc() << std::endl;
        
        // Clear all breakpoints
        debugger->clear_all_breakpoints();
        
        // Continue to completion
        debugger->continue_execution();
        
        debugger->stop_debug_session();
    }
    
    return 0;
}
```

### Example 4: Integration with Editor

```cpp
#include "bolt/editor/integrated_editor.hpp"
#include <iostream>

using namespace bolt;

int main() {
    auto& editor = IntegratedEditor::getInstance();
    
    // Create a test file
    std::string content = R"(print "Editor test"
ai_init "model"
ai_chat "Hello"
halt)";
    
    editor.openDocument("test.limbo", content);
    
    // Start debugging from editor
    if (editor.startDebugSessionFromSource(content)) {
        std::cout << "Debugging started" << std::endl;
        
        // Set breakpoints via editor
        editor.setBreakpointAtLine("test.limbo", 2);
        
        // Show debugger UI
        editor.showDebugger();
        
        // Step through
        editor.debugStepOver();
        std::cout << "PC: " << editor.getCurrentDebugPC() << std::endl;
        
        // Continue
        editor.debugContinue();
        
        // Stop debugging
        editor.stopDebugSession();
        editor.hideDebugger();
    }
    
    return 0;
}
```

## Integration with Editor

The debugger integrates seamlessly with the IntegratedEditor, providing visual feedback and source-level debugging.

### Editor Integration Features

1. **Source-Level Debugging**: Debug Limbo source files directly from the editor
2. **Current Line Highlighting**: Visual indication of the currently executing line
3. **Breakpoint Markers**: Visual breakpoint indicators in the editor gutter
4. **Synchronized State**: Editor and debugger state stay synchronized

### Using Debugger with IntegratedEditor

```cpp
auto& editor = IntegratedEditor::getInstance();

// Start debugging current file
editor.startDebugSession("myprogram.limbo");

// Set breakpoint at line 10
editor.setBreakpointAtLine("myprogram.limbo", 10);

// Show debugger UI
editor.showDebugger();

// Step through code
editor.debugStepOver();

// Continue execution
editor.debugContinue();

// Stop debugging
editor.stopDebugSession();
```

## Advanced Features

### Conditional Breakpoints (Future Enhancement)

The `BreakpointInfo` structure includes a `condition` field for future support of conditional breakpoints:

```cpp
struct BreakpointInfo {
    size_t pc;
    std::string file_path;
    size_t line_number;
    bool enabled;
    std::string condition;  // For conditional breakpoints (future)
};
```

### Custom Event Handlers

You can implement custom behavior for different debug events:

```cpp
debugger->set_event_callback([](DebugEvent event, const std::string& message) {
    switch (event) {
        case DebugEvent::SESSION_STARTED:
            // Initialize debugging UI
            break;
        case DebugEvent::BREAKPOINT_HIT:
            // Update UI, log state
            break;
        case DebugEvent::STEP_COMPLETED:
            // Refresh variable display
            break;
        case DebugEvent::EXECUTION_FINISHED:
            // Show completion message
            break;
        case DebugEvent::ERROR_OCCURRED:
            // Handle error
            break;
    }
});
```

### Multi-threaded Debugging Considerations

When debugging in a multi-threaded environment:

1. Use thread-safe access to the debugger
2. Ensure UI updates happen on the main thread
3. Consider using locks when accessing shared debugging state

## Best Practices

### 1. Always Check Session State

Before performing debug operations, check if a session is active:

```cpp
if (debugger->is_debugging()) {
    debugger->step_over();
}
```

### 2. Handle Events Appropriately

Use event callbacks to update UI and handle state changes:

```cpp
debugger->set_event_callback([&ui](DebugEvent event, const std::string& message) {
    ui->update_status(message);
    if (event == DebugEvent::BREAKPOINT_HIT) {
        ui->highlight_current_line();
    }
});
```

### 3. Clean Up Sessions

Always stop debugging sessions when done:

```cpp
// In destructor or cleanup code
if (debugger->is_debugging()) {
    debugger->stop_debug_session();
}
```

### 4. Validate Breakpoint Locations

Check if a PC is valid before setting breakpoints:

```cpp
if (debugger->is_valid_pc(pc)) {
    debugger->set_breakpoint(pc);
}
```

### 5. Use Watch Expressions Wisely

Limit the number of watch expressions to avoid performance impact:

```cpp
// Add only essential watches
debugger->add_watch_expression("critical_variable");
debugger->add_watch_expression("error_count");
```

## Troubleshooting

### Issue: Breakpoint Not Hit

**Possible Causes:**
1. Breakpoint set at invalid PC
2. Breakpoint disabled
3. Code never reaches that location

**Solutions:**
```cpp
// Verify breakpoint is valid
if (debugger->has_breakpoint(pc)) {
    auto breakpoints = debugger->get_all_breakpoints();
    for (const auto& bp : breakpoints) {
        if (bp.pc == pc && !bp.enabled) {
            debugger->enable_breakpoint(pc, true);
        }
    }
}
```

### Issue: Debug Session Won't Start

**Possible Causes:**
1. Invalid Limbo source
2. Previous session not stopped
3. Program compilation failed

**Solutions:**
```cpp
// Stop any existing session first
if (debugger->is_debugging()) {
    debugger->stop_debug_session();
}

// Enable debug output to see errors
debugger->enable_debug_output(true);

// Try starting session
if (!debugger->start_debug_session_from_source(source)) {
    auto log = debugger->get_debug_log();
    for (const auto& message : log) {
        std::cerr << message << std::endl;
    }
}
```

### Issue: Watch Expressions Not Updating

**Possible Causes:**
1. Watch expressions not being updated
2. Expression evaluation not implemented for variable type

**Solutions:**
```cpp
// Explicitly update watches after each step
debugger->step_over();
debugger->update_watch_expressions();

// Check watch validity
auto watches = debugger->get_watch_expressions();
for (const auto& watch : watches) {
    if (!watch.valid) {
        std::cerr << "Invalid watch: " << watch.expression << std::endl;
    }
}
```

### Issue: UI Not Updating

**Possible Causes:**
1. UI not connected to debugger
2. Auto-refresh disabled
3. UI not visible

**Solutions:**
```cpp
// Ensure UI is properly connected
ui->set_debugger(debugger);
ui->set_visible(true);

// Enable auto-refresh
ui->set_auto_refresh(true, 100.0f);  // Refresh every 100ms

// Manually render UI
ui->render();
```

## Performance Considerations

### Debug Output Overhead

Debug output logging can impact performance. Disable it in production:

```cpp
#ifdef NDEBUG
    debugger->enable_debug_output(false);
#else
    debugger->enable_debug_output(true);
#endif
```

### Watch Expression Updates

Update watch expressions only when needed:

```cpp
// Update only after stepping or hitting breakpoint
if (state == DebugState::PAUSED) {
    debugger->update_watch_expressions();
}
```

### UI Refresh Rate

Adjust UI refresh rate based on needs:

```cpp
// Lower refresh rate for better performance
ui->set_auto_refresh(true, 500.0f);  // 500ms instead of 100ms
```

## Related Documentation

- [User Guide](./USER_GUIDE.md)
- [Getting Started](./GETTING_STARTED.md)
- [Performance Profiler](./PERFORMANCE_PROFILER.md)
- [Logging System](./LOGGING_SYSTEM.md)

## Version History

- **v1.0.0** (December 2024): Initial integrated debugger implementation
  - Basic debugging functionality
  - Breakpoint management
  - Step debugging
  - Watch expressions
  - UI integration
  - Editor integration

## License

Part of the Bolt C++ ML project. See [LICENSE](../LICENSE) for details.
