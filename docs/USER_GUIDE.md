# Bolt C++ ML - User Guide

Welcome to Bolt C++ ML, a modern C++ IDE with comprehensive AI integration and advanced development features.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Editor Features](#editor-features)
3. [AI-Powered Features](#ai-powered-features)
4. [Development Tools](#development-tools)
5. [Collaboration](#collaboration)
6. [Configuration](#configuration)
7. [Keyboard Shortcuts](#keyboard-shortcuts)
8. [Troubleshooting](#troubleshooting)

## Getting Started

### Installation

#### Prerequisites
- C++17 or later compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.15 or later
- vcpkg or Conan (recommended for dependencies)

#### Quick Install

```bash
# Clone the repository
git clone https://github.com/cogpy/bolt-cppml.git
cd bolt-cppml

# Automated setup (recommended)
./scripts/setup-deps.sh

# Build
cmake --preset vcpkg
cmake --build --preset vcpkg-release

# Run tests
./build/test/bolt_unit_tests
```

### First Run

When you first launch Bolt C++ ML:

1. The application will initialize core components
2. Default keyboard shortcuts will be registered
3. The file tree will display your project directory
4. AI features will initialize (may take a few seconds)

## Editor Features

### Multi-File Tabs

Bolt supports multiple file tabs for efficient navigation between open files.

**Opening Files:**
- Via file tree: Click on any file in the file tree
- Via keyboard: Use file open shortcuts
- Programmatically: `editor.openDocumentInTab(filePath, content)`

**Tab Navigation:**
- `Ctrl+PageDown`: Switch to next tab (wraps around)
- `Ctrl+PageUp`: Switch to previous tab
- `Ctrl+W`: Close current tab
- Click on tab: Switch to that file

**Tab Features:**
- Dirty indicator (*) for unsaved changes
- Pin tabs to protect from accidental closure
- Close other tabs (keep only current)
- Reorder tabs by drag and drop (if GUI enabled)

**Example:**
```cpp
auto& editor = IntegratedEditor::getInstance();

// Open multiple files in tabs
size_t tab1 = editor.openDocumentInTab("main.cpp", fileContent1);
size_t tab2 = editor.openDocumentInTab("utils.cpp", fileContent2);

// Switch between tabs
editor.switchToNextTab();      // Go to utils.cpp
editor.switchToPreviousTab();  // Back to main.cpp

// Pin a tab to prevent accidental closure
editor.setTabPinned(tab1, true);

// Mark tab as having unsaved changes
editor.setTabDirty(tab1, true);
```

### Code Folding

Collapse and expand code blocks for better navigation of large files.

**Automatic Detection:**
- Functions and methods
- Classes and structs
- Namespaces
- Control structures (if/for/while blocks)
- Multi-line comments

**Usage:**
- Click fold icon in gutter to toggle
- Keyboard shortcut to fold/unfold at cursor
- Fold all / unfold all commands

**Example:**
```cpp
// Automatically detects this as foldable
namespace bolt {
    class MyClass {
        void method() {
            if (condition) {
                // ...
            }
        }
    };
} // Can fold entire namespace
```

### Multi-Cursor Editing

Edit multiple locations simultaneously for efficient refactoring.

**Adding Cursors:**
- `Ctrl+D`: Add cursor at next occurrence of selected text
- `Ctrl+Alt+Up/Down`: Add cursor above/below current line
- `Ctrl+Shift+L`: Select all occurrences
- `Alt+Click`: Add cursor at click location

**Usage:**
```cpp
// Select "value" and press Ctrl+D multiple times
int value = 10;
double value = 3.14;  // Both instances will be selected
return value;         // Can edit all at once
```

### Split View

Work with multiple files side-by-side.

**Creating Splits:**
- `Ctrl+\`: Horizontal split
- `Ctrl+Shift+\`: Vertical split

**Navigation:**
- `Ctrl+Tab`: Next pane
- `Ctrl+Shift+Tab`: Previous pane
- `Ctrl+W`: Close current pane

**Use Cases:**
- Compare two files
- View header and implementation
- Edit test alongside code
- Reference documentation while coding

### Find and Replace

Powerful search and replace with regex support.

**Basic Find:**
- `Ctrl+F`: Find in current file
- `Ctrl+H`: Find and replace
- `F3` / `Shift+F3`: Find next/previous
- `Ctrl+Shift+F`: Find in all files

**Regex Examples:**
```regex
\bfunction\s+(\w+)\s*\(     # Find function definitions
TODO:.*                     # Find all TODO comments
class\s+(\w+)               # Find class declarations
```

### Minimap

Get a bird's-eye view of your code with the minimap.

**Features:**
- Scaled-down view of entire file
- Current viewport indicator
- Click to navigate
- Syntax-aware coloring
- Configurable size and position

## AI-Powered Features

### AI Code Completion

Get intelligent, context-aware code suggestions powered by AI.

**Triggering Completion:**
- `Ctrl+Space`: Manual trigger
- Automatic trigger while typing (configurable)

**Accepting Completions:**
- `Enter` or `Tab`: Accept selected suggestion
- `Escape`: Cancel
- `Up/Down`: Navigate suggestions

**Example:**
```cpp
std::v    // Triggers suggestions:
          // - vector
          // - variant
          // - visit
          
class MyC  // Suggests:
           // class MyClass { ... }  (full template)
```

**Completion Quality:**
- Language-aware (C++, Python, JavaScript, etc.)
- Context-sensitive (considers current scope)
- Learns from your codebase
- Fallback to static analysis

### AI Code Generation

Generate code from comments or descriptions.

**Usage:**
```cpp
// Generate a function to parse JSON
// AI can generate:
nlohmann::json parseJSON(const std::string& jsonStr) {
    nlohmann::json j = nlohmann::json::parse(jsonStr);
    return j;
}

// Generate test cases for this function
// AI generates comprehensive test suite
```

### AI Refactoring

Get AI-powered suggestions for code improvements.

**Refactoring Types:**
- Extract method/function
- Reduce complexity
- Remove code duplication
- Apply design patterns
- Performance optimizations
- Security improvements

**Example:**
```cpp
// Original code
if (user != nullptr && user->isActive() && user->hasPermission("read")) {
    // ...
}

// AI suggests:
// Extract to: bool canUserRead(const User* user)
// Priority: MEDIUM
// Impact: MODERATE
// Improves readability and reusability
```

## Development Tools

### Integrated Debugger

Debug your code without leaving the IDE.

**Starting Debug Session:**
```cpp
auto& editor = IntegratedEditor::getInstance();
editor.startDebugSession("myprogram.dis");
```

**Breakpoints:**
- Click gutter to set/remove breakpoint
- `F9`: Toggle breakpoint at current line
- Red dot indicator for active breakpoints
- Conditional breakpoints support

**Execution Control:**
- `F5`: Continue
- `F10`: Step over
- `F11`: Step into
- `Shift+F11`: Step out
- `Shift+F5`: Stop debugging

**Debug Views:**
- Call stack
- Local variables
- Watch expressions
- Breakpoint list

### Performance Profiler

Identify performance bottlenecks in your code.

**Features:**
- CPU usage per function
- Memory allocation tracking
- Hot path identification
- Flame graph visualization
- Timeline view

### Logging System

Comprehensive logging with multiple levels and sinks.

**Log Levels:**
- TRACE: Detailed diagnostic information
- DEBUG: Debug information
- INFO: Informational messages
- WARN: Warning messages
- ERROR: Error messages
- FATAL: Fatal errors

**Usage:**
```cpp
BOLT_LOG_INFO("Application started");
BOLT_LOG_DEBUG("Processing file: {}", filename);
BOLT_LOG_ERROR("Failed to open file: {}", errorMsg);
BOLT_LOG_CATEGORY(Core, INFO, "Core module initialized");
```

**Configuration:**
```cpp
auto& logManager = LogManager::getInstance();
logManager.setLevel(LogLevel::DEBUG);
logManager.addSink(std::make_shared<ConsoleSink>());
logManager.addSink(std::make_shared<FileSink>("bolt.log"));
```

### Code Analysis

Static analysis tools integrated into the editor.

**Features:**
- Linting (cppcheck, clang-tidy integration)
- Complexity metrics
- Code coverage visualization
- Dependency analysis
- Security vulnerability scanning

## Collaboration

### Real-Time Collaborative Editing

Work with team members on the same codebase simultaneously.

**Features:**
- Multi-user editing
- Operational transformation for conflict resolution
- User presence indicators
- Cursor position sharing
- Selection highlighting

**Starting a Session:**
```cpp
auto& integration = CollaborativeEditorIntegration::getInstance();
integration.enableCollaboration("document.cpp");
```

## Configuration

### Editor Settings

Configure the editor to your preferences:

```cpp
// Set tab size
editorStore.setTabSize(4);

// Enable/disable features
editor.setFoldingEnabled(true);
editor.setAICompletionEnabled(true);

// Theme selection
auto& themeSystem = ThemeSystem::getInstance();
themeSystem.setActiveTheme("dark");
```

### AI Settings

Configure AI behavior:

```cpp
auto& aiEngine = AICodeCompletionEngine::getInstance();

// Enable/disable AI
aiEngine.setAIEnabled(true);

// Set completion provider
aiEngine.setProvider(std::make_unique<CustomAIProvider>());
```

## Keyboard Shortcuts

### File Operations
- `Ctrl+N`: New file
- `Ctrl+O`: Open file
- `Ctrl+S`: Save file
- `Ctrl+Shift+S`: Save all files
- `Ctrl+W`: Close file/tab

### Editing
- `Ctrl+Z`: Undo
- `Ctrl+Y`: Redo
- `Ctrl+X`: Cut
- `Ctrl+C`: Copy
- `Ctrl+V`: Paste
- `Ctrl+A`: Select all

### Code Navigation
- `F12`: Go to definition
- `Shift+F12`: Find references
- `Ctrl+G`: Go to line
- `Ctrl+P`: Quick file open

### Multi-Cursor
- `Ctrl+D`: Add cursor at next occurrence
- `Ctrl+Alt+Up`: Add cursor above
- `Ctrl+Alt+Down`: Add cursor below
- `Ctrl+Shift+L`: Select all occurrences
- `Escape`: Clear extra cursors

### Code Completion
- `Ctrl+Space`: Trigger completion
- `Enter`: Accept completion
- `Escape`: Cancel completion
- `Up/Down`: Navigate completions

### Tabs
- `Ctrl+PageDown`: Next tab
- `Ctrl+PageUp`: Previous tab
- `Ctrl+W`: Close current tab

### Split View
- `Ctrl+\`: Create horizontal split
- `Ctrl+Shift+\`: Create vertical split
- `Ctrl+Tab`: Navigate to next pane
- `Ctrl+W`: Close current pane

### Debugging
- `F9`: Toggle breakpoint
- `F5`: Continue/Start debugging
- `F10`: Step over
- `F11`: Step into
- `Shift+F11`: Step out
- `Shift+F5`: Stop debugging

### Customizing Shortcuts

```cpp
auto& shortcuts = KeyboardShortcuts::getInstance();
shortcuts.registerShortcut("Ctrl+K", "myCommand", []() {
    // Your custom action
}, ShortcutContext::Global, "My custom command");
```

## Troubleshooting

### Common Issues

#### AI Completion Not Working

**Symptoms:** No suggestions appear when typing

**Solutions:**
1. Check AI is enabled: `aiEngine.isAIEnabled()`
2. Verify AI provider is ready: `aiEngine.isAIReady()`
3. Check logs for initialization errors
4. Ensure model files are available

#### High Memory Usage

**Symptoms:** Application using excessive RAM

**Solutions:**
1. Close unused tabs
2. Disable minimap for large files
3. Reduce AI cache size
4. Check for memory leaks with profiler

#### Slow Performance

**Symptoms:** Editor feels sluggish

**Solutions:**
1. Disable syntax highlighting for large files
2. Turn off AI features temporarily
3. Close collaboration sessions
4. Reduce number of open files

### Debug Mode

Enable debug logging for troubleshooting:

```cpp
BOLT_LOG_SET_LEVEL(LogLevel::DEBUG);
BOLT_LOG_DEBUG("Debugging issue: {}", issueDescription);
```

### Getting Help

- **Documentation**: Check docs/ directory
- **API Reference**: Run `make docs` for Doxygen documentation
- **Issues**: Report bugs on GitHub Issues
- **Community**: Join our Discord/Slack channel

## Advanced Topics

### Creating Plugins

Extend Bolt with custom plugins:

```cpp
class MyPlugin : public Plugin {
public:
    void initialize() override {
        // Plugin initialization
    }
    
    void activate() override {
        // Called when plugin is activated
    }
};

// Register plugin
PluginManager::getInstance().registerPlugin(std::make_unique<MyPlugin>());
```

### Custom AI Providers

Implement your own AI completion provider:

```cpp
class MyAIProvider : public AICompletionProvider {
public:
    std::vector<CompletionItem> generateCompletions(
        const CodeContext& context,
        const std::string& prefix,
        size_t maxSuggestions) override {
        // Your AI logic here
        return completions;
    }
    
    bool isReady() const override {
        return initialized_;
    }
};

// Use custom provider
aiEngine.setProvider(std::make_unique<MyAIProvider>());
```

### Performance Optimization

Tips for optimal performance:

1. **Use Code Folding**: Collapse large blocks
2. **Limit Open Files**: Close unused tabs
3. **Selective AI**: Disable AI for simple edits
4. **Indexing**: Let LSP index complete before editing
5. **Hardware**: SSD recommended, 16GB+ RAM ideal

## Best Practices

### Code Organization
- Use tabs to group related files
- Pin frequently used files
- Utilize split view for related code
- Keep file tree organized by module

### AI Usage
- Review AI suggestions before accepting
- Provide clear comments for code generation
- Use AI for boilerplate, verify for logic
- Learn from AI suggestions

### Collaboration
- Communicate before major refactoring
- Use meaningful commit messages
- Resolve conflicts promptly
- Share code navigation shortcuts

## Appendix

### File Formats
- Source files: `.cpp`, `.hpp`, `.h`, `.c`
- Project files: `CMakeLists.txt`, `vcpkg.json`
- Configuration: `.boltconfig` (custom settings)

### Environment Variables
- `BOLT_HOME`: Installation directory
- `BOLT_CONFIG`: Configuration directory
- `BOLT_PLUGINS`: Plugin directory
- `BOLT_LOG_LEVEL`: Default log level

### System Requirements

**Minimum:**
- CPU: Dual-core 2.0 GHz
- RAM: 4 GB
- Disk: 500 MB
- OS: Linux, Windows 10+, macOS 10.14+

**Recommended:**
- CPU: Quad-core 3.0 GHz+
- RAM: 16 GB
- Disk: SSD with 2 GB free
- GPU: Dedicated (for AI features)
- OS: Latest stable release

## Conclusion

Bolt C++ ML provides a powerful, AI-enhanced development environment. This guide covers the essentials - explore the features, experiment with AI tools, and customize to your workflow.

For more information:
- **API Documentation**: `make docs`
- **Developer Guide**: See CONTRIBUTING.md
- **Architecture**: See DEVO-GENESIS.md
- **Testing**: See TESTING.md

Happy coding with Bolt C++ ML!
