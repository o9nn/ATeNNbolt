
#ifndef PLUGIN_SYSTEM_HPP
#define PLUGIN_SYSTEM_HPP

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

// Platform-specific dynamic library loading headers
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "bolt/core/thread_safety.hpp"
#include "bolt/core/plugin_interface.hpp"
#include "bolt/core/error_handling.hpp"
#include "bolt/core/editor_store.hpp"
#include "bolt/editor/integrated_editor.hpp"

namespace bolt {

/**
 * Legacy plugin structure for backward compatibility
 */
struct Plugin {
    std::string name;
    std::string version;
    std::function<void()> initialize;
    std::function<void()> cleanup;
};

/**
 * Dynamic plugin loader for .so/.dll files
 */
class PluginLoader {
public:
    struct LoadedPlugin {
        void* handle;
        std::shared_ptr<IPlugin> plugin;
        PluginMetadata metadata;
        std::string filePath;
        
        ~LoadedPlugin() {
            if (handle) {
#ifdef _WIN32
                FreeLibrary(static_cast<HMODULE>(handle));
#else
                dlclose(handle);
#endif
            }
        }
    };

    static std::unique_ptr<LoadedPlugin> loadPlugin(const std::string& filePath);
    static bool unloadPlugin(LoadedPlugin* plugin);
    static std::vector<std::string> discoverPlugins(const std::string& directory);
    
private:
    static bool validatePlugin(IPlugin* plugin);
};

/**
 * Plugin context implementation
 */
class PluginContextImpl : public PluginContext {
private:
    EditorStore* editorStore_;
    IntegratedEditor* integratedEditor_;
    std::unordered_map<PluginEventType, std::vector<std::function<void(const PluginEvent&)>>> eventHandlers_;
    std::unordered_map<std::string, std::any> configValues_;
    std::function<std::shared_ptr<IPlugin>(const std::string&)> pluginGetter_;
    std::function<std::vector<std::shared_ptr<IPlugin>>()> allPluginsGetter_;

public:
    PluginContextImpl(EditorStore* editorStore, IntegratedEditor* integratedEditor);
    
    // PluginContext interface
    EditorStore* getEditorStore() override { return editorStore_; }
    IntegratedEditor* getIntegratedEditor() override { return integratedEditor_; }
    
    void subscribeToEvent(PluginEventType eventType, std::function<void(const PluginEvent&)> handler) override;
    void unsubscribeFromEvent(PluginEventType eventType) override;
    void publishEvent(const PluginEvent& event) override;
    
    std::any getConfigValue(const std::string& key) override;
    void setConfigValue(const std::string& key, const std::any& value) override;
    
    void logInfo(const std::string& message) override;
    void logWarning(const std::string& message) override;
    void logError(const std::string& message) override;
    
    std::shared_ptr<IPlugin> getPlugin(const std::string& name) override;
    std::vector<std::shared_ptr<IPlugin>> getPlugins() override;
    
    // Internal methods
    void setPluginGetter(std::function<std::shared_ptr<IPlugin>(const std::string&)> getter) { pluginGetter_ = getter; }
    void setAllPluginsGetter(std::function<std::vector<std::shared_ptr<IPlugin>>()> getter) { allPluginsGetter_ = getter; }
};

/**
 * Enhanced plugin system with dynamic loading and modern interface support
 */
class PluginSystem {
private:
    // Legacy plugins for backward compatibility
    ThreadSafe<std::vector<Plugin>> legacyPlugins_;
    
    // Modern plugin management
    ThreadSafe<std::unordered_map<std::string, std::unique_ptr<PluginLoader::LoadedPlugin>>> loadedPlugins_;
    ThreadSafe<std::unordered_map<std::string, std::shared_ptr<IPlugin>>> activePlugins_;
    
    // Plugin registry by type
    ThreadSafe<std::unordered_map<std::string, std::vector<std::shared_ptr<IEditorPlugin>>>> editorPlugins_;
    ThreadSafe<std::unordered_map<std::string, std::vector<std::shared_ptr<ILanguagePlugin>>>> languagePlugins_;
    ThreadSafe<std::unordered_map<std::string, std::vector<std::shared_ptr<IUIPlugin>>>> uiPlugins_;
    ThreadSafe<std::unordered_map<std::string, std::shared_ptr<IThemePlugin>>> themePlugins_;
    
    // Plugin directories
    std::vector<std::string> pluginDirectories_;
    
    // Plugin context
    std::unique_ptr<PluginContextImpl> context_;
    
    // Event system
    void publishEventToPlugins(const PluginEvent& event);

public:
    static PluginSystem& getInstance() {
        static PluginSystem instance;
        return instance;
    }
    
    // Initialization
    void initialize(EditorStore* editorStore, IntegratedEditor* integratedEditor);
    void shutdown();
    
    // Legacy plugin support
    void registerPlugin(Plugin plugin);
    void initializeAll();
    void cleanupAll();
    
    // Modern plugin management
    bool loadPlugin(const std::string& filePath);
    bool loadPlugin(std::shared_ptr<IPlugin> plugin);
    bool unloadPlugin(const std::string& name);
    void reloadPlugin(const std::string& name);
    
    // Plugin discovery
    void addPluginDirectory(const std::string& directory);
    void removePluginDirectory(const std::string& directory);
    void discoverAndLoadPlugins();
    
    // Plugin queries
    std::vector<std::string> getLoadedPluginNames() const;
    std::shared_ptr<IPlugin> getPlugin(const std::string& name) const;
    std::vector<std::shared_ptr<IPlugin>> getPlugins() const;
    std::vector<PluginMetadata> getPluginMetadata() const;
    
    // Type-specific plugin access
    std::vector<std::shared_ptr<IEditorPlugin>> getEditorPlugins() const;
    std::vector<std::shared_ptr<ILanguagePlugin>> getLanguagePluginsForFile(const std::string& filePath) const;
    std::vector<std::shared_ptr<IUIPlugin>> getUIPlugins() const;
    std::shared_ptr<IThemePlugin> getActiveThemePlugin() const;
    
    // Plugin state management
    bool isPluginLoaded(const std::string& name) const;
    bool isPluginActive(const std::string& name) const;
    PluginState getPluginState(const std::string& name) const;
    bool activatePlugin(const std::string& name);
    bool deactivatePlugin(const std::string& name);
    
    // Configuration
    bool configurePlugin(const std::string& name, const std::unordered_map<std::string, std::any>& config);
    std::unordered_map<std::string, std::any> getPluginConfiguration(const std::string& name) const;
    
    // Event system
    void publishEvent(const PluginEvent& event);
    PluginContext* getPluginContext() { return context_.get(); }
    
    // Error handling
    void handlePluginError(const std::string& pluginName, const std::string& error);
    std::vector<std::string> getPluginErrors() const;
    void clearPluginErrors();
    
private:
    PluginSystem() = default;
    ~PluginSystem() = default;
    
    // Internal helpers
    std::string getFileExtension(const std::string& filePath) const;
    void registerPluginByType(std::shared_ptr<IPlugin> plugin);
    void unregisterPluginByType(const std::string& name);
    bool validatePluginCompatibility(const PluginMetadata& metadata) const;
    
    // Error tracking
    ThreadSafe<std::vector<std::string>> pluginErrors_;
};

} // namespace bolt

#endif
