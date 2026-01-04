#ifndef PLUGIN_INTERFACE_HPP
#define PLUGIN_INTERFACE_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>
#include <any>

#include <unordered_map>

namespace bolt {

// Forward declarations
class EditorStore;
class IntegratedEditor;
class PluginContext;

}

namespace bolt {

// Forward declarations
class EditorStore;
class IntegratedEditor;
class PluginContext;

/**
 * Plugin API version for compatibility checking
 */
struct PluginAPIVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
    
    bool isCompatible(const PluginAPIVersion& other) const {
        return major == other.major && minor >= other.minor;
    }
    
    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
};

/**
 * Plugin metadata structure
 */
struct PluginMetadata {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string license;
    std::vector<std::string> dependencies;
    PluginAPIVersion apiVersion;
    std::unordered_map<std::string, std::string> customProperties;
    
    bool isValid() const {
        return !name.empty() && !version.empty();
    }
};

/**
 * Plugin lifecycle states
 */
enum class PluginState {
    Unloaded,
    Loaded,
    Initialized,
    Active,
    Error
};

/**
 * Plugin event types for the event system
 */
enum class PluginEventType {
    DocumentOpened,
    DocumentClosed,
    DocumentModified,
    CursorMoved,
    SelectionChanged,
    FileTreeChanged,
    ThemeChanged,
    EditorFocused,
    EditorBlurred,
    Custom
};

/**
 * Plugin event data
 */
struct PluginEvent {
    PluginEventType type;
    std::string source;
    std::unordered_map<std::string, std::any> data;
    
    template<typename T>
    T getData(const std::string& key) const {
        auto it = data.find(key);
        if (it != data.end()) {
            return std::any_cast<T>(it->second);
        }
        throw std::runtime_error("Plugin event data key not found: " + key);
    }
    
    template<typename T>
    T getData(const std::string& key, const T& defaultValue) const {
        auto it = data.find(key);
        if (it != data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
};

/**
 * Base interface for all plugin types
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Plugin metadata
    virtual PluginMetadata getMetadata() const = 0;
    
    // Lifecycle methods
    virtual bool initialize(PluginContext* context) = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual void cleanup() = 0;
    
    // Configuration
    virtual bool configure(const std::unordered_map<std::string, std::any>& config) { return true; }
    virtual std::unordered_map<std::string, std::any> getConfiguration() const { return {}; }
    
    // Event handling
    virtual void onEvent(const PluginEvent& event) {}
    
    // State management
    virtual PluginState getState() const { return state_; }
    
protected:
    PluginState state_ = PluginState::Unloaded;
};

/**
 * Editor plugin interface for plugins that modify editor behavior
 */
class IEditorPlugin : public IPlugin {
public:
    // Editor integration hooks
    virtual void onDocumentOpened(const std::string& filePath, const std::string& content) {}
    virtual void onDocumentClosed(const std::string& filePath) {}
    virtual void onDocumentModified(const std::string& filePath, const std::string& content) {}
    virtual void onCursorChanged(const std::string& filePath, size_t line, size_t column) {}
    virtual void onSelectionChanged(const std::string& filePath, size_t startLine, size_t startCol, size_t endLine, size_t endCol) {}
    
    // Text processing hooks
    virtual std::string processText(const std::string& text) { return text; }
    virtual bool canHandleFileType(const std::string& fileExtension) { return false; }
};

/**
 * Language server plugin interface
 */
class ILanguagePlugin : public IEditorPlugin {
public:
    // Language-specific features
    virtual std::vector<std::string> getCodeCompletions(const std::string& filePath, size_t line, size_t column) { return {}; }
    virtual std::vector<std::string> getDiagnostics(const std::string& filePath, const std::string& content) { return {}; }
    virtual std::string formatCode(const std::string& code) { return code; }
    virtual std::vector<std::pair<size_t, size_t>> findReferences(const std::string& filePath, size_t line, size_t column) { return {}; }
    virtual std::string getHoverInfo(const std::string& filePath, size_t line, size_t column) { return ""; }
    
    // Supported file extensions
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

/**
 * UI plugin interface for plugins that add UI components
 */
class IUIPlugin : public IPlugin {
public:
    // UI component management
    virtual void renderUI() {}
    virtual void onUIEvent(const std::string& componentId, const std::string& eventType, const std::any& eventData) {}
    
    // Menu and toolbar integration
    virtual std::vector<std::string> getMenuItems() { return {}; }
    virtual std::vector<std::string> getToolbarItems() { return {}; }
    virtual void onMenuItemClicked(const std::string& itemId) {}
    virtual void onToolbarItemClicked(const std::string& itemId) {}
};

/**
 * Theme plugin interface
 */
class IThemePlugin : public IPlugin {
public:
    virtual std::string getThemeName() const = 0;
    virtual std::unordered_map<std::string, std::string> getThemeColors() const = 0;
    virtual std::unordered_map<std::string, std::string> getThemeFonts() const { return {}; }
    virtual void applyTheme() = 0;
};

/**
 * Plugin context provides access to editor APIs
 */
class PluginContext {
public:
    virtual ~PluginContext() = default;
    
    // Editor access
    virtual EditorStore* getEditorStore() = 0;
    virtual IntegratedEditor* getIntegratedEditor() = 0;
    
    // Event system
    virtual void subscribeToEvent(PluginEventType eventType, std::function<void(const PluginEvent&)> handler) = 0;
    virtual void unsubscribeFromEvent(PluginEventType eventType) = 0;
    virtual void publishEvent(const PluginEvent& event) = 0;
    
    // Configuration access
    virtual std::any getConfigValue(const std::string& key) = 0;
    virtual void setConfigValue(const std::string& key, const std::any& value) = 0;
    
    // Logging
    virtual void logInfo(const std::string& message) = 0;
    virtual void logWarning(const std::string& message) = 0;
    virtual void logError(const std::string& message) = 0;
    
    // Plugin communication
    virtual std::shared_ptr<IPlugin> getPlugin(const std::string& name) = 0;
    virtual std::vector<std::shared_ptr<IPlugin>> getPlugins() = 0;
};

} // namespace bolt

#endif