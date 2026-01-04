# Language Server Protocol (LSP) Support

## Overview

Bolt C++ IDE now includes comprehensive Language Server Protocol (LSP) support, enabling integration with external language servers and providing a foundation for advanced language features.

## Architecture

The LSP implementation consists of several key components:

### Core Components

1. **LSP Protocol Types** (`include/bolt/editor/lsp_protocol.hpp`)
   - Standard LSP data structures (Position, Range, CompletionItem, etc.)
   - Support for completion, hover, diagnostics, formatting, and navigation
   - Compatible with LSP specification

2. **JSON-RPC Communication** (`include/bolt/editor/lsp_json_rpc.hpp`)
   - JSON-RPC 2.0 message handling
   - Request/response and notification patterns
   - Basic JSON serialization/deserialization

3. **LSP Server** (`include/bolt/editor/lsp_server.hpp`)
   - Implements LSP server functionality
   - Handles standard LSP requests (completion, hover, etc.)
   - Integrates with Bolt's plugin system

4. **LSP Client** (`include/bolt/editor/lsp_client.hpp`)
   - Communicates with external language servers
   - Process-based connection management
   - Asynchronous message handling

5. **Plugin Integration** (`include/bolt/editor/lsp_plugin_adapter.hpp`)
   - Bridges existing language plugins with LSP protocol
   - Adapter pattern for plugin compatibility
   - Unified language feature interface

## Features

### Supported LSP Features

- **Text Document Synchronization**: Keep documents in sync between client and server
- **Code Completion**: Context-aware code suggestions
- **Hover Information**: Show documentation and type information
- **Diagnostics**: Real-time error and warning detection
- **Document Formatting**: Automatic code formatting
- **Go to Definition**: Navigate to symbol definitions
- **Find References**: Locate symbol usages

### Language Plugin Integration

The LSP system seamlessly integrates with Bolt's existing language plugin architecture:

```cpp
// Register a language plugin with LSP support
auto cppPlugin = std::make_shared<CppLanguagePlugin>();
lspIntegration->registerLanguagePlugin("cpp", cppPlugin);

// External language server configuration
LspClientConfig config;
config.serverCommand = "clangd";
config.serverArgs = {"--log=verbose"};
config.languageId = "cpp";
config.fileExtensions = {".cpp", ".hpp", ".c", ".h"};
lspIntegration->registerExternalLanguageServer("cpp", config);
```

## Usage Examples

### Basic LSP Server Usage

```cpp
#include "bolt/editor/lsp_server.hpp"

// Create and configure LSP server
bolt::lsp::LanguageServer server;

// Initialize server capabilities
bolt::lsp::InitializeParams params;
params.rootPath = "/project/path";
params.capabilities.textDocumentSync = true;
params.capabilities.completionProvider = true;
server.initialize(params);

// Register language plugins
server.registerLanguagePlugin("cpp", cppPlugin);

// Handle document operations
bolt::lsp::TextDocumentItem document;
document.uri = "file:///path/to/file.cpp";
document.languageId = "cpp";
document.text = "int main() { return 0; }";
server.didOpenTextDocument(document);

// Get completions
bolt::lsp::TextDocumentPositionParams posParams;
posParams.textDocument.uri = document.uri;
posParams.position = bolt::lsp::Position(0, 10);
auto completions = server.completion(posParams);
```

### LSP Client for External Servers

```cpp
#include "bolt/editor/lsp_client.hpp"

// Create LSP client
auto connection = std::make_unique<bolt::lsp::ProcessLspConnection>();
bolt::lsp::LspClient client(std::move(connection));

// Configure and connect
bolt::lsp::LspClientConfig config;
config.serverCommand = "clangd";
config.rootPath = "/project/path";
client.connect(config);

// Initialize client
client.initialize([](bool success) {
    if (success) {
        std::cout << "LSP client initialized successfully" << std::endl;
    }
});

// Send completion request
bolt::lsp::TextDocumentPositionParams params;
params.textDocument.uri = "file:///path/to/file.cpp";
params.position = bolt::lsp::Position(10, 5);

client.completion(params, [](const bolt::lsp::CompletionList& completions) {
    std::cout << "Received " << completions.items.size() << " completions" << std::endl;
});
```

### Integration Plugin

```cpp
#include "bolt/editor/lsp_plugin_adapter.hpp"

// Create LSP integration plugin
auto lspPlugin = bolt::lsp::createLspIntegrationPlugin();

// Initialize and activate
lspPlugin->initialize(nullptr);
lspPlugin->activate();

// Register language plugins
lspPlugin->registerLanguagePlugin("cpp", cppPlugin);

// Use through standard plugin interface
auto completions = lspPlugin->getCodeCompletions("/path/to/file.cpp", 10, 5);
auto diagnostics = lspPlugin->getDiagnostics("/path/to/file.cpp", content);
auto hover = lspPlugin->getHoverInfo("/path/to/file.cpp", 10, 5);
```

## Configuration

### Server Configuration

Configure LSP server capabilities:

```cpp
bolt::lsp::ServerCapabilities capabilities;
capabilities.textDocumentSync = true;
capabilities.hoverProvider = true;
capabilities.completionProvider = true;
capabilities.definitionProvider = true;
capabilities.referencesProvider = true;
capabilities.documentFormattingProvider = true;
server.setCapabilities(capabilities);
```

### Client Configuration

Configure external language server connection:

```cpp
bolt::lsp::LspClientConfig config;
config.serverCommand = "language-server-executable";
config.serverArgs = {"--stdio", "--log-level=info"};
config.languageId = "cpp";
config.fileExtensions = {".cpp", ".hpp"};
config.rootPath = "/workspace/path";
config.timeoutMs = 5000;
```

## Extension Points

### Custom Language Plugins

Implement the `ILanguagePlugin` interface to add language support:

```cpp
class MyLanguagePlugin : public bolt::ILanguagePlugin {
public:
    std::vector<std::string> getCodeCompletions(const std::string& filePath, 
                                               size_t line, size_t column) override {
        // Implement completion logic
        return {"completion1", "completion2"};
    }
    
    std::string getHoverInfo(const std::string& filePath, 
                           size_t line, size_t column) override {
        // Implement hover logic
        return "Hover information";
    }
    
    // Implement other required methods...
};
```

### Custom LSP Connections

Implement `ILspConnection` for custom communication methods:

```cpp
class CustomLspConnection : public bolt::lsp::ILspConnection {
public:
    bool start(const std::string& command, 
              const std::vector<std::string>& args) override {
        // Implement custom connection logic
        return true;
    }
    
    void sendMessage(const std::string& message) override {
        // Implement message sending
    }
    
    std::string receiveMessage() override {
        // Implement message receiving
        return "";
    }
    
    // Implement other required methods...
};
```

## Testing

Run the LSP integration demo:

```bash
cd build
make demo_lsp_integration
./demo_lsp_integration
```

This demonstrates:
- Language plugin registration
- Code completion requests
- Hover information
- Diagnostic analysis
- Code formatting
- File type detection

## Integration with Existing Features

The LSP support integrates seamlessly with existing Bolt features:

- **Plugin System**: LSP plugins work alongside existing plugins
- **Editor Integration**: Direct integration with IntegratedEditor
- **Theme System**: LSP diagnostic highlighting respects themes
- **Multi-cursor**: LSP features work with multi-cursor editing
- **Split View**: LSP support across split editor panes

## Future Enhancements

Planned improvements include:

- **Hot Reloading**: Dynamic language server restart/reload
- **Advanced Diagnostics**: Code actions and quick fixes
- **Symbol Navigation**: Workspace-wide symbol search
- **Semantic Highlighting**: Advanced syntax highlighting
- **Refactoring Support**: Automated code refactoring
- **Debug Integration**: LSP debug adapter protocol support

## Status

✅ **Implemented**:
- Core LSP protocol types and structures
- JSON-RPC communication framework
- Basic LSP server implementation
- LSP client for external servers
- Plugin system integration
- Working demonstration

🚧 **In Progress**:
- Full LSP feature implementation
- External language server integration
- Advanced diagnostic features

📋 **Planned**:
- Performance optimizations
- Additional language server support
- Advanced LSP features
- Production-ready deployment

The LSP support provides a solid foundation for modern language features and can be extended to support additional languages and advanced IDE functionality.