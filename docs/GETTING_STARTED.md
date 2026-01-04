# Getting Started with Bolt C++ ML

Welcome to Bolt C++ ML! This guide will help you set up and start using Bolt C++ ML, a modern C++ IDE with comprehensive AI/ML integration.

## Quick Start

### Prerequisites

Before you begin, ensure you have:

- **C++17 or later compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: Version 3.15 or later
- **Git**: For cloning the repository
- **vcpkg**: Recommended for dependency management (alternative: Conan or system packages)

### Installation

#### 1. Clone the Repository

```bash
git clone https://github.com/cogpy/bolt-cppml.git
cd bolt-cppml
```

#### 2. Install Dependencies (Automated)

The easiest way to set up dependencies is using the automated setup script:

```bash
./scripts/setup-deps.sh
```

This script will:
- Detect your package manager (vcpkg, Conan, or system)
- Install required dependencies
- Configure the build system
- Prepare the project for compilation

#### 3. Build the Project

Using CMake presets (recommended):

```bash
# Configure with vcpkg
cmake --preset vcpkg

# Build in release mode
cmake --build --preset vcpkg-release
```

Or using traditional CMake:

```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)
```

#### 4. Run Tests

Verify your installation by running the test suite:

```bash
./build/test/bolt_unit_tests
```

Expected output: `All tests passed!`

### First Run

Launch Bolt C++ ML for the first time:

```bash
./build/bolt
```

On first launch:
1. The application initializes core components
2. Default configurations are loaded
3. AI models are initialized (if available)
4. The file tree displays the current directory

## Project Structure

Understanding the project layout:

```
bolt-cppml/
├── include/bolt/        # Public header files
│   ├── ai/             # AI/ML integration
│   ├── core/           # Core engine components
│   ├── editor/         # Editor features
│   ├── network/        # Network components
│   └── utils/          # Utility functions
├── src/bolt/           # Implementation files
├── test/               # Unit and integration tests
├── docs/               # Documentation
│   ├── api/            # Generated API docs (auto-generated)
│   ├── USER_GUIDE.md   # Comprehensive user guide
│   └── *.md            # Feature-specific documentation
├── examples/           # Example code and demos
└── scripts/            # Build and setup scripts
```

## Basic Usage

### Opening Files

1. **Via File Tree**: Click on any file in the left sidebar
2. **Via Keyboard**: `Ctrl+O` to open file dialog
3. **Programmatically**: 
   ```cpp
   auto& editor = bolt::IntegratedEditor::getInstance();
   editor.openDocument("myfile.cpp");
   ```

### Editor Features

#### Multi-Cursor Editing

Place multiple cursors for simultaneous editing:

- `Ctrl+Alt+Down`: Add cursor below
- `Ctrl+Alt+Up`: Add cursor above
- `Ctrl+D`: Select next occurrence of current word

#### Code Folding

Collapse and expand code blocks:

- Click the fold icon in the gutter
- `Ctrl+Shift+[`: Fold current block
- `Ctrl+Shift+]`: Unfold current block

#### Split View

View multiple files side by side:

- `Ctrl+\`: Split view horizontally
- `Ctrl+Shift+\`: Split view vertically
- `Ctrl+W`: Close current pane

### AI-Powered Features

#### Code Completion

Intelligent code suggestions powered by AI:

1. Start typing in the editor
2. Press `Ctrl+Space` to trigger completion
3. Use arrow keys to navigate suggestions
4. Press `Enter` to accept a suggestion

#### Code Generation

Generate code from natural language descriptions:

```cpp
// Type a comment describing what you want:
// TODO: Create a function that sorts a vector of integers

// Press Ctrl+Shift+G to generate the code
```

#### AI Refactoring

Improve code quality with AI suggestions:

1. Select a code block
2. Press `Ctrl+Shift+R`
3. Review AI suggestions
4. Accept or reject changes

### Development Tools

#### Integrated Debugger

Debug your applications:

```cpp
// Set breakpoint
debugger.setBreakpoint(lineNumber);

// Start debugging
debugger.start_debug_session(program);

// Step through code
debugger.step_over();
debugger.step_into();
debugger.continue_execution();
```

#### Performance Profiler

Profile your code:

```cpp
auto& profiler = bolt::PerformanceProfiler::getInstance();

// Start profiling a section
profiler.startSection("my_function");
myFunction();
profiler.endSection("my_function");

// Get results
auto report = profiler.generateReport();
```

#### Logging System

Comprehensive logging:

```cpp
#include "bolt/core/logging.hpp"

// Initialize logger
auto& logger = bolt::Logger::getInstance();

// Log messages
LOG_INFO("Application started");
LOG_DEBUG("Processing file: {}", filename);
LOG_ERROR("Failed to open file: {}", error);
```

## Configuration

### Keyboard Shortcuts

Customize keyboard shortcuts in `default_shortcuts.conf`:

```ini
[Editor]
save=Ctrl+S
open=Ctrl+O
find=Ctrl+F

[AI]
complete=Ctrl+Space
generate=Ctrl+Shift+G
refactor=Ctrl+Shift+R
```

### Theme System

Switch between themes:

```cpp
auto& theme = bolt::ThemeSystem::getInstance();
theme.loadTheme("dark");  // or "light"
```

### AI Configuration

Configure AI features in `bolt_ai_config.json`:

```json
{
  "model": "tinyllama",
  "context_length": 2048,
  "temperature": 0.7,
  "top_p": 0.9
}
```

## Next Steps

Now that you have Bolt C++ ML running:

1. **Explore Features**: Try the AI code completion and generation
2. **Read Documentation**: Check out the [User Guide](USER_GUIDE.md)
3. **Run Examples**: Browse the `examples/` directory
4. **View API Docs**: Open `docs/api/html/index.html` in your browser
5. **Join Community**: Contribute to the project on GitHub

## Troubleshooting

### Common Issues

#### Build Fails

**Issue**: Missing dependencies

**Solution**: Run the setup script again:
```bash
./scripts/setup-deps.sh
```

#### Tests Fail

**Issue**: Some tests are failing

**Solution**: Ensure you're on Linux and all dependencies are installed:
```bash
./build/test/bolt_unit_tests --verbose
```

#### AI Features Not Working

**Issue**: AI completion not responding

**Solution**: Check AI model is loaded:
```bash
# Verify model files exist
ls -la test/models/
```

### Getting Help

- **Documentation**: Browse `docs/` directory
- **API Reference**: Open `docs/api/html/index.html`
- **Issues**: Report bugs on [GitHub Issues](https://github.com/cogpy/bolt-cppml/issues)
- **Examples**: Check `examples/` for usage patterns

## Resources

- [User Guide](USER_GUIDE.md) - Comprehensive feature documentation
- [API Documentation](api/html/index.html) - Auto-generated API reference
- [Testing Guide](../TESTING.md) - How to write and run tests
- [Contributing Guide](../CONTRIBUTING.md) - How to contribute
- [Architecture Overview](../DEVO-GENESIS.md) - Project architecture

## What's Next?

Continue your journey with Bolt C++ ML:

- **[User Guide](USER_GUIDE.md)**: Deep dive into all features
- **[AI Features](../AI_IMPLEMENTATION_COMPLETE.md)**: Learn about AI integration
- **[LSP Support](../LSP_IMPLEMENTATION_COMPLETE.md)**: Language Server Protocol
- **[Git Integration](GIT_INTEGRATION.md)**: Version control features
- **[Performance](PERFORMANCE_PROFILER.md)**: Profiling and optimization

---

**Need help?** Visit our [GitHub repository](https://github.com/cogpy/bolt-cppml) or check the documentation in the `docs/` directory.
