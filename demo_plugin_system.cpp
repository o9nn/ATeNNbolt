#include "bolt/core/plugin_system.hpp"
#include "bolt/core/plugin_interface.hpp"
#include "bolt/core/editor_store.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include "../examples/plugins/cpp_language_plugin.cpp" // Include for static linking
#include <iostream>
#include <thread>
#include <chrono>

// Create a simple editor store for demo purposes
class DemoEditorStore : public bolt::EditorStore {
public:
    DemoEditorStore() = default;
    
    // Implement minimal interface for demo
    void openDocument(const std::string& filePath, const std::string& content) {
        std::cout << "Demo: Opening document " << filePath << std::endl;
    }
    
    void closeDocument(const std::string& filePath) {
        std::cout << "Demo: Closing document " << filePath << std::endl;
    }
};

// Create a simple integrated editor for demo purposes
class DemoIntegratedEditor : public bolt::IntegratedEditor {
public:
    DemoIntegratedEditor() = default;
    
    void openDocument(const std::string& filePath, const std::string& content) {
        std::cout << "Demo: Integrated editor opening " << filePath << std::endl;
    }
};

// Create a simple editor store for demo purposes - just use nullptr since we only need interface
void demonstratePluginSystem() {
    std::cout << "=== Bolt Plugin System Demonstration ===" << std::endl;
    
    // Get plugin system instance
    bolt::PluginSystem& pluginSystem = bolt::PluginSystem::getInstance();
    
    // Clean any previous state
    pluginSystem.shutdown();
    
    // Initialize plugin system (use nullptr for editor components since this is just a demo)
    std::cout << "\n1. Initializing Plugin System..." << std::endl;
    pluginSystem.initialize(nullptr, nullptr);
    
    // Add plugin directories for discovery
    std::cout << "\n2. Setting up plugin directories..." << std::endl;
    pluginSystem.addPluginDirectory("./plugins");
    pluginSystem.addPluginDirectory("./examples/plugins");
    
    // Load a plugin statically (since we included the source)
    std::cout << "\n3. Loading C++ Language Plugin..." << std::endl;
    auto cppPlugin = createCppLanguagePlugin();
    if (pluginSystem.loadPlugin(cppPlugin)) {
        std::cout << "✓ C++ Language Plugin loaded successfully!" << std::endl;
    } else {
        std::cout << "✗ Failed to load C++ Language Plugin" << std::endl;
    }
    
    // Activate the plugin
    std::cout << "\n4. Activating plugin..." << std::endl;
    if (pluginSystem.activatePlugin("CppLanguagePlugin")) {
        std::cout << "✓ C++ Language Plugin activated!" << std::endl;
    } else {
        std::cout << "✗ Failed to activate C++ Language Plugin" << std::endl;
    }
    
    // Query loaded plugins
    std::cout << "\n5. Querying loaded plugins..." << std::endl;
    auto loadedNames = pluginSystem.getLoadedPluginNames();
    std::cout << "Loaded plugins (" << loadedNames.size() << "):" << std::endl;
    for (const auto& name : loadedNames) {
        std::cout << "  - " << name << std::endl;
    }
    
    // Get plugin metadata
    std::cout << "\n6. Plugin metadata:" << std::endl;
    auto metadata = pluginSystem.getPluginMetadata();
    for (const auto& meta : metadata) {
        std::cout << "  Plugin: " << meta.name << std::endl;
        std::cout << "    Version: " << meta.version << std::endl;
        std::cout << "    Description: " << meta.description << std::endl;
        std::cout << "    Author: " << meta.author << std::endl;
        std::cout << "    API Version: " << meta.apiVersion.toString() << std::endl;
    }
    
    // Test language plugin features
    std::cout << "\n7. Testing language plugin features..." << std::endl;
    auto langPlugins = pluginSystem.getLanguagePluginsForFile("test.cpp");
    if (!langPlugins.empty()) {
        auto plugin = langPlugins[0];
        std::cout << "  Language plugin for .cpp files: " << plugin->getMetadata().name << std::endl;
        
        // Test code completions
        auto completions = plugin->getCodeCompletions("test.cpp", 10, 5);
        std::cout << "  Code completions (first 10):" << std::endl;
        for (size_t i = 0; i < std::min(completions.size(), size_t(10)); ++i) {
            std::cout << "    - " << completions[i] << std::endl;
        }
        
        // Test diagnostics
        std::string testCode = R"(
            int main() {
                using namespace std;
                cout << "Hello World" << endl;
                return 0;
            }
        )";
        auto diagnostics = plugin->getDiagnostics("test.cpp", testCode);
        if (!diagnostics.empty()) {
            std::cout << "  Diagnostics:" << std::endl;
            for (const auto& diag : diagnostics) {
                std::cout << "    - " << diag << std::endl;
            }
        }
    }
    
    // Test event system
    std::cout << "\n8. Testing event system..." << std::endl;
    bolt::PluginEvent documentOpenEvent;
    documentOpenEvent.type = bolt::PluginEventType::DocumentOpened;
    documentOpenEvent.source = "demo";
    documentOpenEvent.data["filePath"] = std::string("demo.cpp");
    documentOpenEvent.data["content"] = std::string("int main() { return 0; }");
    
    pluginSystem.publishEvent(documentOpenEvent);
    std::cout << "  Document open event published" << std::endl;
    
    // Test plugin configuration
    std::cout << "\n9. Testing plugin configuration..." << std::endl;
    std::unordered_map<std::string, std::any> config;
    config["enableAdvancedCompletions"] = true;
    config["maxCompletions"] = 20;
    
    if (pluginSystem.configurePlugin("CppLanguagePlugin", config)) {
        std::cout << "✓ Plugin configured successfully!" << std::endl;
    } else {
        std::cout << "✗ Plugin configuration failed" << std::endl;
    }
    
    // Test plugin state management
    std::cout << "\n10. Testing plugin state management..." << std::endl;
    auto state = pluginSystem.getPluginState("CppLanguagePlugin");
    std::cout << "  Plugin state: " << static_cast<int>(state) << std::endl;
    
    std::cout << "  Is loaded: " << (pluginSystem.isPluginLoaded("CppLanguagePlugin") ? "Yes" : "No") << std::endl;
    std::cout << "  Is active: " << (pluginSystem.isPluginActive("CppLanguagePlugin") ? "Yes" : "No") << std::endl;
    
    // Deactivate plugin
    std::cout << "\n11. Deactivating plugin..." << std::endl;
    if (pluginSystem.deactivatePlugin("CppLanguagePlugin")) {
        std::cout << "✓ Plugin deactivated" << std::endl;
    }
    
    // Reactivate plugin
    std::cout << "\n12. Reactivating plugin..." << std::endl;
    if (pluginSystem.activatePlugin("CppLanguagePlugin")) {
        std::cout << "✓ Plugin reactivated" << std::endl;
    }
    
    // Legacy plugin support demo
    std::cout << "\n13. Testing legacy plugin support..." << std::endl;
    bool legacyInitialized = false;
    bool legacyCleaned = false;
    
    bolt::Plugin legacyPlugin;
    legacyPlugin.name = "LegacyDemo";
    legacyPlugin.version = "1.0.0";
    legacyPlugin.initialize = [&]() { 
        legacyInitialized = true; 
        std::cout << "  Legacy plugin initialized" << std::endl;
    };
    legacyPlugin.cleanup = [&]() { 
        legacyCleaned = true; 
        std::cout << "  Legacy plugin cleaned up" << std::endl;
    };
    
    pluginSystem.registerPlugin(legacyPlugin);
    pluginSystem.initializeAll();
    
    // Error handling demo
    std::cout << "\n14. Testing error handling..." << std::endl;
    // Try to load non-existent plugin
    pluginSystem.loadPlugin("/nonexistent/plugin.so");
    
    auto errors = pluginSystem.getPluginErrors();
    if (!errors.empty()) {
        std::cout << "  Plugin errors captured:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "    - " << error << std::endl;
        }
    }
    
    // Cleanup
    std::cout << "\n15. Cleaning up..." << std::endl;
    pluginSystem.cleanupAll(); // Legacy plugins
    pluginSystem.shutdown();   // Modern plugins
    
    std::cout << "\n=== Plugin System Demonstration Complete ===" << std::endl;
}

int main() {
    try {
        demonstratePluginSystem();
        std::cout << "\nDemo completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with error: " << e.what() << std::endl;
        return 1;
    }
}