# Plugin System Architecture

## Overview

The Bolt C++ IDE includes a comprehensive plugin system architecture that supports both modern interface-based plugins and legacy function-based plugins. The system provides dynamic loading capabilities, event handling, and type-specific plugin management.

## Architecture Components

### Core Interfaces

#### `IPlugin` - Base Plugin Interface
All plugins must implement this interface, providing:
- Metadata (name, version, description, author, API version)
- Lifecycle management (initialize, activate, deactivate, cleanup)
- Configuration support
- Event handling

#### Specialized Plugin Types

1. **`IEditorPlugin`** - For plugins that modify editor behavior
   - Document lifecycle hooks (open, close, modify)
   - Text processing capabilities
   - File type handling

2. **`ILanguagePlugin`** - For language-specific features
   - Code completions
   - Diagnostics
   - Code formatting
   - Symbol navigation
   - File extension support

3. **`IUIPlugin`** - For UI extensions
   - Custom UI components
   - Menu and toolbar items
   - UI event handling

4. **`IThemePlugin`** - For theme support
   - Color schemes
   - Font configurations
   - Theme application

### Plugin System Components

#### `PluginSystem` - Main Plugin Manager
- Singleton pattern for global access
- Thread-safe plugin operations
- Dynamic plugin loading/unloading
- Plugin discovery and registration
- Event system coordination
- Error handling and recovery

#### `PluginContext` - Plugin API Access
Provides plugins with access to:
- Editor components (EditorStore, IntegratedEditor)
- Event subscription/publishing
- Configuration management
- Logging facilities
- Inter-plugin communication

#### `PluginLoader` - Dynamic Loading
- Loads plugins from .so/.dll/.dylib files
- Plugin validation and compatibility checking
- Plugin discovery in directories
- Safe plugin unloading

## Plugin Development

### Creating a Plugin

1. **Implement Plugin Interface**
   ```cpp
   class MyLanguagePlugin : public bolt::ILanguagePlugin {
       // Implementation
   };
   ```

2. **Export Functions (for dynamic loading)**
   ```cpp
   extern "C" {
       bolt::IPlugin* createPlugin() {
           return new MyLanguagePlugin();
       }
       
       void destroyPlugin(bolt::IPlugin* plugin) {
           delete plugin;
       }
   }
   ```

3. **Static Registration (for built-in plugins)**
   ```cpp
   std::shared_ptr<bolt::IPlugin> createMyPlugin() {
       return std::make_shared<MyLanguagePlugin>();
   }
   ```

### Plugin Lifecycle

1. **Loading** - Plugin is loaded but not initialized
2. **Initialization** - Plugin receives context and sets up
3. **Activation** - Plugin becomes active and operational
4. **Deactivation** - Plugin stops operations but remains loaded
5. **Cleanup** - Plugin performs final cleanup
6. **Unloading** - Plugin is removed from memory

### Event System

Plugins can subscribe to and publish events:

```cpp
// Subscribe to events
context->subscribeToEvent(PluginEventType::DocumentOpened, 
    [](const PluginEvent& event) {
        // Handle event
    });

// Publish events
PluginEvent event;
event.type = PluginEventType::Custom;
event.data["key"] = value;
context->publishEvent(event);
```

## Usage Examples

### Loading a Plugin
```cpp
auto& pluginSystem = PluginSystem::getInstance();

// Load from file
pluginSystem.loadPlugin("./plugins/cpp_language.so");

// Load programmatically
auto plugin = std::make_shared<CppLanguagePlugin>();
pluginSystem.loadPlugin(plugin);
```

### Querying Plugins
```cpp
// Get all loaded plugins
auto plugins = pluginSystem.getPlugins();

// Get language plugins for a file
auto langPlugins = pluginSystem.getLanguagePluginsForFile("test.cpp");

// Get specific plugin
auto plugin = pluginSystem.getPlugin("CppLanguagePlugin");
```

### Plugin Configuration
```cpp
std::unordered_map<std::string, std::any> config;
config["maxCompletions"] = 20;
config["enableAdvanced"] = true;

pluginSystem.configurePlugin("MyPlugin", config);
```

## Built-in Plugins

### C++ Language Plugin
- Code completions for STL and C++ keywords
- Basic syntax validation
- Simple code formatting
- File type support: .cpp, .hpp, .c, .h

## Plugin Directories

The system automatically searches for plugins in:
- `./plugins/`
- `/usr/local/lib/bolt/plugins/`
- `~/.bolt/plugins/`

## API Compatibility

The plugin system uses semantic versioning for API compatibility:
- Major version changes break compatibility
- Minor version increases maintain backward compatibility
- Patch versions are fully compatible

Current API Version: 1.0.0

## Thread Safety

All plugin operations are thread-safe:
- Plugin loading/unloading
- Event publishing/handling
- Configuration changes
- Plugin queries

## Error Handling

The system provides comprehensive error handling:
- Plugin validation during loading
- Runtime error recovery
- Error logging and reporting
- Graceful degradation when plugins fail

## Testing

The plugin system includes comprehensive tests covering:
- Basic plugin lifecycle
- Type-specific plugin functionality
- Event system
- Configuration management
- Error handling
- Thread safety
- Legacy plugin support

## Future Enhancements

- Plugin dependency management
- Hot reloading capabilities
- Plugin marketplace integration
- Advanced security sandboxing
- Performance monitoring
- Plugin analytics

## Example Implementation

See `examples/plugins/cpp_language_plugin.cpp` for a complete plugin implementation and `demo_plugin_system.cpp` for usage examples.