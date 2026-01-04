#include "bolt/core/plugin_system.hpp"
#include "bolt/core/editor_store.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>

namespace bolt {

// PluginLoader implementation
std::unique_ptr<PluginLoader::LoadedPlugin> PluginLoader::loadPlugin(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("Plugin file not found: " + filePath);
    }
    
#ifdef _WIN32
    // Windows-specific dynamic library loading
    HMODULE handle = LoadLibraryA(filePath.c_str());
    if (!handle) {
        DWORD error = GetLastError();
        throw std::runtime_error("Failed to load plugin: Error code " + std::to_string(error));
    }
    
    // Get the plugin creation function
    typedef IPlugin* (*create_plugin_t)();
    create_plugin_t createPlugin = reinterpret_cast<create_plugin_t>(
        GetProcAddress(handle, "createPlugin")
    );
    
    if (!createPlugin) {
        DWORD error = GetLastError();
        FreeLibrary(handle);
        throw std::runtime_error("Cannot load symbol 'createPlugin': Error code " + std::to_string(error));
    }
#else
    // Unix-like systems (Linux, macOS)
    void* handle = dlopen(filePath.c_str(), RTLD_LAZY);
    if (!handle) {
        throw std::runtime_error("Failed to load plugin: " + std::string(dlerror()));
    }
    
    // Get the plugin creation function
    typedef IPlugin* (*create_plugin_t)();
    create_plugin_t createPlugin = reinterpret_cast<create_plugin_t>(
        dlsym(handle, "createPlugin")
    );
    
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        dlclose(handle);
        throw std::runtime_error("Cannot load symbol 'createPlugin': " + std::string(dlsym_error));
    }
#endif
    
    // Create plugin instance
    IPlugin* plugin = createPlugin();
    if (!plugin) {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        throw std::runtime_error("Failed to create plugin instance");
    }
    
    // Validate plugin
    if (!validatePlugin(plugin)) {
        delete plugin;
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        throw std::runtime_error("Plugin validation failed");
    }
    
    auto loadedPlugin = std::make_unique<LoadedPlugin>();
    loadedPlugin->handle = handle;
    loadedPlugin->plugin = std::shared_ptr<IPlugin>(plugin);
    loadedPlugin->metadata = plugin->getMetadata();
    loadedPlugin->filePath = filePath;
    
    return loadedPlugin;
}

bool PluginLoader::unloadPlugin(LoadedPlugin* plugin) {
    if (!plugin || !plugin->handle) {
        return false;
    }
    
    // Get the plugin destruction function
    typedef void (*destroy_plugin_t)(IPlugin*);
#ifdef _WIN32
    destroy_plugin_t destroyPlugin = reinterpret_cast<destroy_plugin_t>(
        GetProcAddress(static_cast<HMODULE>(plugin->handle), "destroyPlugin")
    );
#else
    destroy_plugin_t destroyPlugin = reinterpret_cast<destroy_plugin_t>(
        dlsym(plugin->handle, "destroyPlugin")
    );
#endif
    
    if (destroyPlugin) {
        destroyPlugin(plugin->plugin.get());
    }
    
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(plugin->handle));
#else
    dlclose(plugin->handle);
#endif
    plugin->handle = nullptr;
    
    return true;
}

std::vector<std::string> PluginLoader::discoverPlugins(const std::string& directory) {
    std::vector<std::string> plugins;
    
    if (!std::filesystem::exists(directory)) {
        return plugins;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            const std::string& path = entry.path().string();
            // Check file extension (compatible with C++17)
            if ((path.length() > 3 && path.substr(path.length() - 3) == ".so") ||
                (path.length() > 4 && path.substr(path.length() - 4) == ".dll") ||
                (path.length() > 6 && path.substr(path.length() - 6) == ".dylib")) {
                plugins.push_back(path);
            }
        }
    }
    
    return plugins;
}

bool PluginLoader::validatePlugin(IPlugin* plugin) {
    if (!plugin) {
        return false;
    }
    
    PluginMetadata metadata = plugin->getMetadata();
    return metadata.isValid();
}

// PluginContextImpl implementation
PluginContextImpl::PluginContextImpl(EditorStore* editorStore, IntegratedEditor* integratedEditor)
    : editorStore_(editorStore), integratedEditor_(integratedEditor) {
}

void PluginContextImpl::subscribeToEvent(PluginEventType eventType, std::function<void(const PluginEvent&)> handler) {
    eventHandlers_[eventType].push_back(handler);
}

void PluginContextImpl::unsubscribeFromEvent(PluginEventType eventType) {
    eventHandlers_.erase(eventType);
}

void PluginContextImpl::publishEvent(const PluginEvent& event) {
    auto it = eventHandlers_.find(event.type);
    if (it != eventHandlers_.end()) {
        for (const auto& handler : it->second) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                logError("Error in plugin event handler: " + std::string(e.what()));
            }
        }
    }
}

std::any PluginContextImpl::getConfigValue(const std::string& key) {
    auto it = configValues_.find(key);
    if (it != configValues_.end()) {
        return it->second;
    }
    throw std::runtime_error("Configuration key not found: " + key);
}

void PluginContextImpl::setConfigValue(const std::string& key, const std::any& value) {
    configValues_[key] = value;
}

void PluginContextImpl::logInfo(const std::string& message) {
    std::cout << "[PLUGIN INFO] " << message << std::endl;
}

void PluginContextImpl::logWarning(const std::string& message) {
    std::cout << "[PLUGIN WARNING] " << message << std::endl;
}

void PluginContextImpl::logError(const std::string& message) {
    std::cerr << "[PLUGIN ERROR] " << message << std::endl;
}

std::shared_ptr<IPlugin> PluginContextImpl::getPlugin(const std::string& name) {
    if (pluginGetter_) {
        return pluginGetter_(name);
    }
    return nullptr;
}

std::vector<std::shared_ptr<IPlugin>> PluginContextImpl::getPlugins() {
    if (allPluginsGetter_) {
        return allPluginsGetter_();
    }
    return {};
}

// PluginSystem implementation
void PluginSystem::initialize(EditorStore* editorStore, IntegratedEditor* integratedEditor) {
    context_ = std::make_unique<PluginContextImpl>(editorStore, integratedEditor);
    
    // Set up plugin getters for context
    context_->setPluginGetter([this](const std::string& name) {
        return getPlugin(name);
    });
    
    context_->setAllPluginsGetter([this]() {
        return getPlugins();
    });
    
    // Add default plugin directories
    addPluginDirectory("./plugins");
    addPluginDirectory("/usr/local/lib/bolt/plugins");
    addPluginDirectory("~/.bolt/plugins");
}

void PluginSystem::shutdown() {
    // Cleanup all plugins
    cleanupAll();
    
    // Unload modern plugins
    loadedPlugins_.write([](auto& plugins) {
        for (auto& [name, plugin] : plugins) {
            if (plugin->plugin) {
                plugin->plugin->cleanup();
            }
        }
        plugins.clear();
    });
    
    activePlugins_.write([](auto& plugins) {
        plugins.clear();
    });
    
    // Clear registries
    editorPlugins_.write([](auto& plugins) { plugins.clear(); });
    languagePlugins_.write([](auto& plugins) { plugins.clear(); });
    uiPlugins_.write([](auto& plugins) { plugins.clear(); });
    themePlugins_.write([](auto& plugins) { plugins.clear(); });
}

// Legacy plugin support
void PluginSystem::registerPlugin(Plugin plugin) {
    legacyPlugins_.write([&](auto& plugs) {
        plugs.push_back(std::move(plugin));
    });
}

void PluginSystem::initializeAll() {
    legacyPlugins_.read([](const auto& plugs) {
        for (const auto& plugin : plugs) {
            try {
                plugin.initialize();
            } catch (const std::exception& e) {
                std::cerr << "Error initializing legacy plugin " << plugin.name << ": " << e.what() << std::endl;
            }
        }
    });
}

void PluginSystem::cleanupAll() {
    legacyPlugins_.read([](const auto& plugs) {
        for (const auto& plugin : plugs) {
            try {
                plugin.cleanup();
            } catch (const std::exception& e) {
                std::cerr << "Error cleaning up legacy plugin " << plugin.name << ": " << e.what() << std::endl;
            }
        }
    });
}

// Modern plugin management
bool PluginSystem::loadPlugin(const std::string& filePath) {
    try {
        auto loadedPlugin = PluginLoader::loadPlugin(filePath);
        std::string name = loadedPlugin->metadata.name;
        
        // Check if plugin is already loaded
        if (isPluginLoaded(name)) {
            handlePluginError(name, "Plugin already loaded");
            return false;
        }
        
        // Check API compatibility
        if (!validatePluginCompatibility(loadedPlugin->metadata)) {
            handlePluginError(name, "Plugin API version incompatible");
            return false;
        }
        
        // Initialize plugin
        if (!loadedPlugin->plugin->initialize(context_.get())) {
            handlePluginError(name, "Plugin initialization failed");
            return false;
        }
        
        // Store loaded plugin
        loadedPlugins_.write([&](auto& plugins) {
            plugins[name] = std::move(loadedPlugin);
        });
        
        // Add to active plugins
        auto plugin = getPlugin(name);
        if (plugin) {
            registerPluginByType(plugin);
            activePlugins_.write([&](auto& plugins) {
                plugins[name] = plugin;
            });
        }
        
        return true;
    } catch (const std::exception& e) {
        handlePluginError(filePath, e.what());
        return false;
    }
}

bool PluginSystem::loadPlugin(std::shared_ptr<IPlugin> plugin) {
    if (!plugin) {
        return false;
    }
    
    PluginMetadata metadata = plugin->getMetadata();
    std::string name = metadata.name;
    
    // Check if plugin is already loaded
    if (isPluginLoaded(name)) {
        handlePluginError(name, "Plugin already loaded");
        return false;
    }
    
    // Check API compatibility
    if (!validatePluginCompatibility(metadata)) {
        handlePluginError(name, "Plugin API version incompatible");
        return false;
    }
    
    // Initialize plugin
    if (!plugin->initialize(context_.get())) {
        handlePluginError(name, "Plugin initialization failed");
        return false;
    }
    
    // Create a LoadedPlugin wrapper for in-memory plugins
    auto loadedPlugin = std::make_unique<PluginLoader::LoadedPlugin>();
    loadedPlugin->handle = nullptr;  // No dynamic library handle for in-memory plugins
    loadedPlugin->plugin = plugin;
    loadedPlugin->metadata = metadata;
    loadedPlugin->filePath = "<in-memory>";  // Mark as in-memory plugin
    
    // Store in loadedPlugins_ so isPluginLoaded() works correctly
    loadedPlugins_.write([&](auto& plugins) {
        plugins[name] = std::move(loadedPlugin);
    });
    
    // Register plugin by type but don't add to activePlugins yet
    // Plugins must be explicitly activated via activatePlugin()
    registerPluginByType(plugin);
    
    return true;
}

bool PluginSystem::unloadPlugin(const std::string& name) {
    // Remove from active plugins
    bool wasActive = false;
    activePlugins_.write([&](auto& plugins) {
        auto it = plugins.find(name);
        if (it != plugins.end()) {
            try {
                it->second->cleanup();
            } catch (const std::exception& e) {
                handlePluginError(name, "Error during cleanup: " + std::string(e.what()));
            }
            plugins.erase(it);
            wasActive = true;
        }
    });
    
    if (wasActive) {
        unregisterPluginByType(name);
    }
    
    // Remove from loaded plugins
    bool wasLoaded = false;
    loadedPlugins_.write([&](auto& plugins) {
        auto it = plugins.find(name);
        if (it != plugins.end()) {
            plugins.erase(it);
            wasLoaded = true;
        }
    });
    
    return wasActive || wasLoaded;
}

void PluginSystem::reloadPlugin(const std::string& name) {
    std::string filePath;
    
    // Get the file path before unloading
    loadedPlugins_.read([&](const auto& plugins) {
        auto it = plugins.find(name);
        if (it != plugins.end()) {
            filePath = it->second->filePath;
        }
    });
    
    if (!filePath.empty()) {
        unloadPlugin(name);
        loadPlugin(filePath);
    }
}

// Plugin discovery
void PluginSystem::addPluginDirectory(const std::string& directory) {
    auto it = std::find(pluginDirectories_.begin(), pluginDirectories_.end(), directory);
    if (it == pluginDirectories_.end()) {
        pluginDirectories_.push_back(directory);
    }
}

void PluginSystem::removePluginDirectory(const std::string& directory) {
    auto it = std::find(pluginDirectories_.begin(), pluginDirectories_.end(), directory);
    if (it != pluginDirectories_.end()) {
        pluginDirectories_.erase(it);
    }
}

void PluginSystem::discoverAndLoadPlugins() {
    for (const auto& directory : pluginDirectories_) {
        try {
            auto plugins = PluginLoader::discoverPlugins(directory);
            for (const auto& pluginPath : plugins) {
                loadPlugin(pluginPath);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error discovering plugins in " << directory << ": " << e.what() << std::endl;
        }
    }
}

// Plugin queries
std::vector<std::string> PluginSystem::getLoadedPluginNames() const {
    std::vector<std::string> names;
    loadedPlugins_.read([&](const auto& plugins) {
        names.reserve(plugins.size());
        for (const auto& [name, plugin] : plugins) {
            names.push_back(name);
        }
    });
    return names;
}

std::shared_ptr<IPlugin> PluginSystem::getPlugin(const std::string& name) const {
    std::shared_ptr<IPlugin> result;
    
    activePlugins_.read([&](const auto& plugins) {
        auto it = plugins.find(name);
        if (it != plugins.end()) {
            result = it->second;
        }
    });
    
    if (!result) {
        loadedPlugins_.read([&](const auto& plugins) {
            auto it = plugins.find(name);
            if (it != plugins.end()) {
                result = it->second->plugin;
            }
        });
    }
    
    return result;
}

std::vector<std::shared_ptr<IPlugin>> PluginSystem::getPlugins() const {
    std::vector<std::shared_ptr<IPlugin>> plugins;
    loadedPlugins_.read([&](const auto& pluginMap) {
        plugins.reserve(pluginMap.size());
        for (const auto& [name, loadedPlugin] : pluginMap) {
            plugins.push_back(loadedPlugin->plugin);
        }
    });
    return plugins;
}

std::vector<PluginMetadata> PluginSystem::getPluginMetadata() const {
    std::vector<PluginMetadata> metadata;
    
    loadedPlugins_.read([&](const auto& plugins) {
        metadata.reserve(plugins.size());
        for (const auto& [name, plugin] : plugins) {
            metadata.push_back(plugin->metadata);
        }
    });
    
    return metadata;
}

// Type-specific plugin access
std::vector<std::shared_ptr<IEditorPlugin>> PluginSystem::getEditorPlugins() const {
    std::vector<std::shared_ptr<IEditorPlugin>> plugins;
    editorPlugins_.read([&](const auto& pluginMap) {
        for (const auto& [type, typePlugins] : pluginMap) {
            plugins.insert(plugins.end(), typePlugins.begin(), typePlugins.end());
        }
    });
    return plugins;
}

std::vector<std::shared_ptr<ILanguagePlugin>> PluginSystem::getLanguagePluginsForFile(const std::string& filePath) const {
    std::string extension = getFileExtension(filePath);
    std::vector<std::shared_ptr<ILanguagePlugin>> plugins;
    
    languagePlugins_.read([&](const auto& pluginMap) {
        auto it = pluginMap.find(extension);
        if (it != pluginMap.end()) {
            plugins = it->second;
        }
    });
    
    return plugins;
}

std::vector<std::shared_ptr<IUIPlugin>> PluginSystem::getUIPlugins() const {
    std::vector<std::shared_ptr<IUIPlugin>> plugins;
    uiPlugins_.read([&](const auto& pluginMap) {
        for (const auto& [type, typePlugins] : pluginMap) {
            plugins.insert(plugins.end(), typePlugins.begin(), typePlugins.end());
        }
    });
    return plugins;
}

std::shared_ptr<IThemePlugin> PluginSystem::getActiveThemePlugin() const {
    std::shared_ptr<IThemePlugin> result;
    themePlugins_.read([&](const auto& plugins) {
        // Return the first active theme plugin (could be enhanced with priority system)
        if (!plugins.empty()) {
            result = plugins.begin()->second;
        }
    });
    return result;
}

// Plugin state management
bool PluginSystem::isPluginLoaded(const std::string& name) const {
    bool loaded = false;
    loadedPlugins_.read([&](const auto& plugins) {
        loaded = plugins.find(name) != plugins.end();
    });
    return loaded;
}

bool PluginSystem::isPluginActive(const std::string& name) const {
    bool active = false;
    activePlugins_.read([&](const auto& plugins) {
        active = plugins.find(name) != plugins.end();
    });
    return active;
}

PluginState PluginSystem::getPluginState(const std::string& name) const {
    auto plugin = getPlugin(name);
    if (plugin) {
        return plugin->getState();
    }
    return PluginState::Unloaded;
}

bool PluginSystem::activatePlugin(const std::string& name) {
    auto plugin = getPlugin(name);
    if (plugin && !isPluginActive(name)) {
        try {
            plugin->activate();
            // Add to active plugins after successful activation
            activePlugins_.write([&](auto& plugins) {
                plugins[name] = plugin;
            });
            return true;
        } catch (const std::exception& e) {
            handlePluginError(name, "Activation failed: " + std::string(e.what()));
        }
    }
    return false;
}

bool PluginSystem::deactivatePlugin(const std::string& name) {
    auto plugin = getPlugin(name);
    if (plugin && isPluginActive(name)) {
        try {
            plugin->deactivate();
            // Remove from active plugins after deactivation
            activePlugins_.write([&](auto& plugins) {
                plugins.erase(name);
            });
            return true;
        } catch (const std::exception& e) {
            handlePluginError(name, "Deactivation failed: " + std::string(e.what()));
        }
    }
    return false;
}

// Configuration
bool PluginSystem::configurePlugin(const std::string& name, const std::unordered_map<std::string, std::any>& config) {
    auto plugin = getPlugin(name);
    if (plugin) {
        try {
            return plugin->configure(config);
        } catch (const std::exception& e) {
            handlePluginError(name, "Configuration failed: " + std::string(e.what()));
        }
    }
    return false;
}

std::unordered_map<std::string, std::any> PluginSystem::getPluginConfiguration(const std::string& name) const {
    auto plugin = getPlugin(name);
    if (plugin) {
        try {
            return plugin->getConfiguration();
        } catch (const std::exception& e) {
            const_cast<PluginSystem*>(this)->handlePluginError(name, "Failed to get configuration: " + std::string(e.what()));
        }
    }
    return {};
}

// Event system
void PluginSystem::publishEvent(const PluginEvent& event) {
    // Publish to context handlers
    if (context_) {
        context_->publishEvent(event);
    }
    
    // Publish to all plugins
    publishEventToPlugins(event);
}

void PluginSystem::publishEventToPlugins(const PluginEvent& event) {
    activePlugins_.read([&](const auto& plugins) {
        for (const auto& [name, plugin] : plugins) {
            try {
                plugin->onEvent(event);
            } catch (const std::exception& e) {
                const_cast<PluginSystem*>(this)->handlePluginError(name, "Error handling event: " + std::string(e.what()));
            }
        }
    });
}

// Error handling
void PluginSystem::handlePluginError(const std::string& pluginName, const std::string& error) {
    std::string errorMsg = pluginName + ": " + error;
    std::cerr << "[PLUGIN ERROR] " << errorMsg << std::endl;
    
    pluginErrors_.write([&](auto& errors) {
        errors.push_back(errorMsg);
        // Keep only last 100 errors
        if (errors.size() > 100) {
            errors.erase(errors.begin());
        }
    });
}

std::vector<std::string> PluginSystem::getPluginErrors() const {
    std::vector<std::string> errors;
    pluginErrors_.read([&](const auto& err) {
        errors = err;
    });
    return errors;
}

void PluginSystem::clearPluginErrors() {
    pluginErrors_.write([](auto& errors) {
        errors.clear();
    });
}

// Internal helpers
std::string PluginSystem::getFileExtension(const std::string& filePath) const {
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot != std::string::npos && lastDot < filePath.length() - 1) {
        return filePath.substr(lastDot + 1);
    }
    return "";
}

void PluginSystem::registerPluginByType(std::shared_ptr<IPlugin> plugin) {
    // Register by type
    if (auto editorPlugin = std::dynamic_pointer_cast<IEditorPlugin>(plugin)) {
        editorPlugins_.write([&](auto& plugins) {
            plugins["editor"].push_back(editorPlugin);
        });
    }
    
    if (auto langPlugin = std::dynamic_pointer_cast<ILanguagePlugin>(plugin)) {
        languagePlugins_.write([&](auto& plugins) {
            for (const auto& ext : langPlugin->getSupportedExtensions()) {
                plugins[ext].push_back(langPlugin);
            }
        });
    }
    
    if (auto uiPlugin = std::dynamic_pointer_cast<IUIPlugin>(plugin)) {
        uiPlugins_.write([&](auto& plugins) {
            plugins["ui"].push_back(uiPlugin);
        });
    }
    
    if (auto themePlugin = std::dynamic_pointer_cast<IThemePlugin>(plugin)) {
        themePlugins_.write([&](auto& plugins) {
            plugins[themePlugin->getThemeName()] = themePlugin;
        });
    }
}

void PluginSystem::unregisterPluginByType(const std::string& name) {
    // Remove from all type registries
    editorPlugins_.write([&](auto& plugins) {
        for (auto& [type, typePlugins] : plugins) {
            typePlugins.erase(
                std::remove_if(typePlugins.begin(), typePlugins.end(),
                    [&](const auto& plugin) { return plugin->getMetadata().name == name; }),
                typePlugins.end());
        }
    });
    
    languagePlugins_.write([&](auto& plugins) {
        for (auto& [ext, typePlugins] : plugins) {
            typePlugins.erase(
                std::remove_if(typePlugins.begin(), typePlugins.end(),
                    [&](const auto& plugin) { return plugin->getMetadata().name == name; }),
                typePlugins.end());
        }
    });
    
    uiPlugins_.write([&](auto& plugins) {
        for (auto& [type, typePlugins] : plugins) {
            typePlugins.erase(
                std::remove_if(typePlugins.begin(), typePlugins.end(),
                    [&](const auto& plugin) { return plugin->getMetadata().name == name; }),
                typePlugins.end());
        }
    });
    
    themePlugins_.write([&](auto& plugins) {
        auto it = std::find_if(plugins.begin(), plugins.end(),
            [&](const auto& pair) { return pair.second->getMetadata().name == name; });
        if (it != plugins.end()) {
            plugins.erase(it);
        }
    });
}

bool PluginSystem::validatePluginCompatibility(const PluginMetadata& metadata) const {
    PluginAPIVersion currentAPI{1, 0, 0};
    return currentAPI.isCompatible(metadata.apiVersion);
}

} // namespace bolt