# Bolt C++ ML Documentation

Welcome to the Bolt C++ ML documentation! This directory contains comprehensive guides, API references, and technical documentation for the Bolt C++ ML IDE.

## 📚 Documentation Index

### Getting Started

- **[Getting Started Guide](GETTING_STARTED.md)** - Quick start guide for new users
  - Installation and setup
  - First run instructions
  - Basic usage patterns
  - Troubleshooting common issues

- **[User Guide](USER_GUIDE.md)** - Comprehensive feature documentation
  - Editor features
  - AI-powered capabilities
  - Development tools
  - Keyboard shortcuts
  - Configuration options

### API Documentation

- **[API Reference](api/html/index.html)** - Auto-generated API documentation
  - Generated using Doxygen
  - Complete class and function reference
  - Code examples and usage patterns
  - Updated automatically from source code

  To regenerate the API documentation:
  ```bash
  cd docs
  doxygen Doxyfile
  ```

### Core Features

#### AI/ML Integration

- **[AI Implementation Complete](../AI_IMPLEMENTATION_COMPLETE.md)** - Complete AI integration overview
- **[Advanced AI Features](../ADVANCED_AI_FEATURES_COMPLETE.md)** - Advanced AI capabilities
- **[AI Code Completion](../AI_CODE_COMPLETION_INTEGRATION_COMPLETE.md)** - Code completion system
- **[AI Chat Setup](../AI_CHAT_SETUP_GUIDE.md)** - Chat system integration
- **[GPU Acceleration](GPU_ACCELERATION.md)** - GPU-accelerated AI inference
- **[AI Testing Strategy](../AI_TESTING_STRATEGY.md)** - Testing AI components

#### Editor Features

- **[Code Folding](code_folding.md)** - Code folding implementation
- **[Multi-Cursor Support](../MULTI_CURSOR_IMPLEMENTATION.md)** - Multi-cursor editing
- **[Split View](../SPLIT_VIEW_IMPLEMENTATION.md)** - Split view editing
- **[Theme System](theme-system.md)** - Theme customization
- **[Widget Framework](WIDGET_FRAMEWORK.md)** - Custom widgets
- **[GUI Integration](../GUI_INTEGRATION_COMPLETE.md)** - GUI components

#### Development Tools

- **[Integrated Debugger](INTEGRATED_DEBUGGER.md)** - Debugging interface and tools
- **[Language Server Protocol](LSP_SUPPORT.md)** - LSP client/server
- **[Git Integration](GIT_INTEGRATION.md)** - Version control features
- **[Performance Profiler](PERFORMANCE_PROFILER.md)** - Code profiling tools
- **[Logging System](LOGGING_SYSTEM.md)** - Comprehensive logging
- **[Benchmark Suite](BENCHMARK_SUITE.md)** - Performance benchmarking
- **[Advanced Code Analysis](ADVANCED_CODE_ANALYSIS.md)** - Static analysis
- **[Plugin System](PLUGIN_SYSTEM.md)** - Plugin architecture
- **[Memory Leak Detection](MEMORY_LEAK_DETECTION.md)** - Memory debugging tools

#### Collaboration & Network

- **[Collaborative Editing](COLLABORATIVE_EDITING.md)** - Real-time collaboration
- **[Network Optimizations](../NETWORK_OPTIMIZATIONS.md)** - Network performance

### Project Information

- **[Main README](../README.md)** - Project overview
- **[DEVO-GENESIS](../DEVO-GENESIS.md)** - Development roadmap and architecture
- **[Contributing Guide](../CONTRIBUTING.md)** - How to contribute
- **[Testing Guide](../TESTING.md)** - Testing framework and practices
- **[TODO List](../bolt-cpp-todo.md)** - Current progress and next steps

### Technical Documentation

- **[Dependency Management](dependency-management.md)** - Package management
- **[Cross-Platform Deployment](CROSS_PLATFORM_DEPLOYMENT.md)** - Platform support
- **[Code Coverage](CODE_COVERAGE.md)** - Test coverage reporting

## 🚀 Quick Navigation

### For New Users
1. Start with [Getting Started Guide](GETTING_STARTED.md)
2. Read the [User Guide](USER_GUIDE.md)
3. Explore the [API Reference](api/html/index.html)

### For Developers
1. Review [DEVO-GENESIS](../DEVO-GENESIS.md) for architecture
2. Check [Contributing Guide](../CONTRIBUTING.md)
3. Read [Testing Guide](../TESTING.md)
4. Browse [API Documentation](api/html/index.html)

### For AI/ML Features
1. [AI Implementation Complete](../AI_IMPLEMENTATION_COMPLETE.md)
2. [Advanced AI Features](../ADVANCED_AI_FEATURES_COMPLETE.md)
3. [GPU Acceleration](GPU_ACCELERATION.md)
4. [AI Code Completion](../AI_CODE_COMPLETION_INTEGRATION_COMPLETE.md)

### For Editor Features
1. [User Guide - Editor Features](USER_GUIDE.md#editor-features)
2. [Code Folding](code_folding.md)
3. [Multi-Cursor](../MULTI_CURSOR_IMPLEMENTATION.md)
4. [Split View](../SPLIT_VIEW_IMPLEMENTATION.md)

## 📖 Documentation Categories

### By Topic

| Category | Documents |
|----------|-----------|
| **Getting Started** | [Getting Started](GETTING_STARTED.md), [User Guide](USER_GUIDE.md) |
| **AI/ML** | [AI Implementation](../AI_IMPLEMENTATION_COMPLETE.md), [GPU Acceleration](GPU_ACCELERATION.md), [AI Chat](../AI_CHAT_SETUP_GUIDE.md) |
| **Editor** | [Code Folding](code_folding.md), [Multi-Cursor](../MULTI_CURSOR_IMPLEMENTATION.md), [Split View](../SPLIT_VIEW_IMPLEMENTATION.md) |
| **Development** | [Debugger](INTEGRATED_DEBUGGER.md), [LSP](LSP_SUPPORT.md), [Git](GIT_INTEGRATION.md), [Profiler](PERFORMANCE_PROFILER.md), [Logging](LOGGING_SYSTEM.md) |
| **Collaboration** | [Collaborative Editing](COLLABORATIVE_EDITING.md), [Network](../NETWORK_OPTIMIZATIONS.md) |
| **System** | [Plugin System](PLUGIN_SYSTEM.md), [Widget Framework](WIDGET_FRAMEWORK.md), [Theme System](theme-system.md) |
| **Testing** | [Testing Guide](../TESTING.md), [AI Testing](../AI_TESTING_STRATEGY.md), [Benchmarks](BENCHMARK_SUITE.md) |

### By Audience

| Audience | Recommended Reading |
|----------|-------------------|
| **New Users** | Getting Started, User Guide, API Reference |
| **Contributors** | DEVO-GENESIS, Contributing, Testing Guide, API Docs |
| **AI Developers** | AI Implementation, GPU Acceleration, AI Testing |
| **Plugin Authors** | Plugin System, Widget Framework, API Reference |
| **System Integrators** | LSP Support, Git Integration, Dependency Management |

## 🔧 Building Documentation

### API Documentation (Doxygen)

Generate API documentation from source code:

```bash
cd docs
doxygen Doxyfile
```

Output will be in `docs/api/html/index.html`

### Viewing Documentation

1. **API Reference**: Open `docs/api/html/index.html` in a web browser
2. **Markdown Docs**: View `.md` files in any markdown viewer or GitHub

## 📝 Documentation Standards

### Writing Guidelines

- Use clear, concise language
- Include code examples where applicable
- Keep examples up-to-date with current API
- Add cross-references to related documentation
- Update the documentation index when adding new docs

### File Naming

- Use `UPPERCASE_WITH_UNDERSCORES.md` for major guides
- Use `lowercase-with-hyphens.md` for technical docs
- Use descriptive names that indicate content

### Structure

Each documentation file should include:
1. Title and brief description
2. Table of contents (for longer docs)
3. Main content with examples
4. Related documentation links
5. Last updated date (optional)

## 🤝 Contributing to Documentation

Help improve the documentation:

1. **Fix typos and errors**: Submit a pull request
2. **Add examples**: Show real-world usage patterns
3. **Improve clarity**: Rewrite confusing sections
4. **Update outdated info**: Keep docs current with code
5. **Add missing topics**: Document undocumented features

See [Contributing Guide](../CONTRIBUTING.md) for details.

## 📊 Documentation Status

### Completed Documentation ✅

- [x] Getting Started Guide
- [x] User Guide
- [x] API Reference (auto-generated)
- [x] AI Implementation docs
- [x] Editor feature docs
- [x] Development tools docs
- [x] Testing documentation
- [x] Collaboration features

### In Progress 🚧

- [ ] Video tutorials
- [ ] Interactive examples
- [ ] Advanced configuration guide
- [ ] Performance tuning guide

### Planned 📋

- [ ] Architecture deep-dives
- [ ] Plugin development guide
- [ ] Deployment guide
- [ ] Security best practices

## 🔗 External Resources

- **GGML**: [https://github.com/ggerganov/ggml](https://github.com/ggerganov/ggml)
- **llama.cpp**: [https://github.com/ggerganov/llama.cpp](https://github.com/ggerganov/llama.cpp)
- **KoboldCpp**: [https://github.com/LostRuins/koboldcpp](https://github.com/LostRuins/koboldcpp)
- **ImGui**: [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui)
- **CMake**: [https://cmake.org/documentation/](https://cmake.org/documentation/)

## 📞 Support

Need help?

- **GitHub Issues**: [Report bugs or request features](https://github.com/cogpy/bolt-cppml/issues)
- **Discussions**: Join community discussions
- **Documentation**: Browse this docs directory
- **Examples**: Check the `examples/` directory

---

**Last Updated**: November 2024

**Version**: 1.0.0

**Maintainers**: Bolt C++ ML Team
