#include "bolt/test_framework.hpp"
#include "bolt/core/plugin_system.hpp"
#include "bolt/core/plugin_interface.hpp"
#include "bolt/core/editor_store.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include <memory>
#include <thread>
#include <chrono>

using namespace bolt::test;

namespace bolt {

// Test plugin implementations
class TestEditorPlugin : public IEditorPlugin {
private:
    PluginMetadata metadata_;
    int documentOpenedCount_ = 0;
    int documentClosedCount_ = 0;

public:
    TestEditorPlugin(const std::string& name = "TestEditorPlugin") {
        metadata_.name = name;
        metadata_.version = "1.0.0";
        metadata_.description = "Test editor plugin";
        metadata_.author = "Test Author";
        metadata_.license = "MIT";
        metadata_.apiVersion = {1, 0, 0};
    }

    PluginMetadata getMetadata() const override { return metadata_; }

    bool initialize(PluginContext* context) override {
        IPlugin::state_ = PluginState::Initialized;
        return true;
    }

    void activate() override {
        IPlugin::state_ = PluginState::Active;
    }

    void deactivate() override {
        IPlugin::state_ = PluginState::Loaded;
    }

    void cleanup() override {
        IPlugin::state_ = PluginState::Unloaded;
    }

    void onDocumentOpened(const std::string& filePath, const std::string& content) override {
        documentOpenedCount_++;
    }

    void onDocumentClosed(const std::string& filePath) override {
        documentClosedCount_++;
    }

    int getDocumentOpenedCount() const { return documentOpenedCount_; }
    int getDocumentClosedCount() const { return documentClosedCount_; }
};

class TestLanguagePlugin : public ILanguagePlugin {
private:
    PluginMetadata metadata_;

public:
    TestLanguagePlugin() {
        metadata_.name = "TestLanguagePlugin";
        metadata_.version = "1.0.0";
        metadata_.description = "Test language plugin";
        metadata_.author = "Test Author";
        metadata_.license = "MIT";
        metadata_.apiVersion = {1, 0, 0};
    }

    PluginMetadata getMetadata() const override { return metadata_; }

    bool initialize(PluginContext* context) override {
        IPlugin::state_ = PluginState::Initialized;
        return true;
    }

    void activate() override {
        IPlugin::state_ = PluginState::Active;
    }

    void deactivate() override {
        IPlugin::state_ = PluginState::Loaded;
    }

    void cleanup() override {
        IPlugin::state_ = PluginState::Unloaded;
    }

    std::vector<std::string> getSupportedExtensions() const override {
        return {"cpp", "hpp", "c", "h"};
    }

    std::vector<std::string> getCodeCompletions(const std::string& filePath, size_t line, size_t column) override {
        return {"std::", "boost::", "auto"};
    }
};

} // namespace bolt

// Test cases using the project's test framework

BOLT_TEST(PluginSystem, BasicRegistration) {
    bolt::PluginSystem& pluginSystem = bolt::PluginSystem::getInstance();
    
    // Clean state
    pluginSystem.shutdown();
    pluginSystem.initialize(nullptr, nullptr);
    
    auto plugin = std::make_shared<bolt::TestEditorPlugin>("TestPlugin1");
    
    BOLT_ASSERT_TRUE(pluginSystem.loadPlugin(plugin));
    BOLT_ASSERT_TRUE(pluginSystem.isPluginLoaded("TestPlugin1"));
    BOLT_ASSERT(plugin->getState() == bolt::PluginState::Initialized);
    
    BOLT_ASSERT_TRUE(pluginSystem.activatePlugin("TestPlugin1"));
    BOLT_ASSERT_TRUE(pluginSystem.isPluginActive("TestPlugin1"));
    BOLT_ASSERT(plugin->getState() == bolt::PluginState::Active);
    
    BOLT_ASSERT_TRUE(pluginSystem.deactivatePlugin("TestPlugin1"));
    BOLT_ASSERT_FALSE(pluginSystem.isPluginActive("TestPlugin1"));
    
    BOLT_ASSERT_TRUE(pluginSystem.unloadPlugin("TestPlugin1"));
    BOLT_ASSERT_FALSE(pluginSystem.isPluginLoaded("TestPlugin1"));
    
    pluginSystem.shutdown();
}

BOLT_TEST(PluginSystem, PluginQueries) {
    bolt::PluginSystem& pluginSystem = bolt::PluginSystem::getInstance();
    
    pluginSystem.shutdown();
    pluginSystem.initialize(nullptr, nullptr);
    
    auto plugin1 = std::make_shared<bolt::TestEditorPlugin>("Plugin1");
    auto plugin2 = std::make_shared<bolt::TestEditorPlugin>("Plugin2");
    
    pluginSystem.loadPlugin(plugin1);
    pluginSystem.loadPlugin(plugin2);
    
    auto loadedNames = pluginSystem.getLoadedPluginNames();
    BOLT_ASSERT_EQ(2, loadedNames.size());
    
    auto retrievedPlugin = pluginSystem.getPlugin("Plugin1");
    BOLT_ASSERT_NOT_NULL(retrievedPlugin.get());
    BOLT_ASSERT_EQ("Plugin1", retrievedPlugin->getMetadata().name);
    
    auto allPlugins = pluginSystem.getPlugins();
    BOLT_ASSERT_EQ(2, allPlugins.size());
    
    pluginSystem.unloadPlugin("Plugin1");
    pluginSystem.unloadPlugin("Plugin2");
    pluginSystem.shutdown();
}

BOLT_TEST(PluginSystem, PluginMetadata) {
    bolt::PluginSystem& pluginSystem = bolt::PluginSystem::getInstance();
    
    pluginSystem.shutdown();
    pluginSystem.initialize(nullptr, nullptr);
    
    auto plugin = std::make_shared<bolt::TestEditorPlugin>("MetadataTest");
    pluginSystem.loadPlugin(plugin);
    
    auto metadata = pluginSystem.getPluginMetadata();
    BOLT_ASSERT_EQ(1, metadata.size());
    BOLT_ASSERT_EQ("MetadataTest", metadata[0].name);
    BOLT_ASSERT_EQ("1.0.0", metadata[0].version);
    BOLT_ASSERT_EQ("Test Author", metadata[0].author);
    
    pluginSystem.unloadPlugin("MetadataTest");
    pluginSystem.shutdown();
}

BOLT_TEST(PluginSystem, LanguagePlugins) {
    bolt::PluginSystem& pluginSystem = bolt::PluginSystem::getInstance();
    
    pluginSystem.shutdown();
    pluginSystem.initialize(nullptr, nullptr);
    
    auto langPlugin = std::make_shared<bolt::TestLanguagePlugin>();
    pluginSystem.loadPlugin(langPlugin);
    
    auto cppPlugins = pluginSystem.getLanguagePluginsForFile("test.cpp");
    BOLT_ASSERT_EQ(1, cppPlugins.size());
    BOLT_ASSERT_EQ("TestLanguagePlugin", cppPlugins[0]->getMetadata().name);
    
    auto hppPlugins = pluginSystem.getLanguagePluginsForFile("test.hpp");
    BOLT_ASSERT_EQ(1, hppPlugins.size());
    
    auto jsPlugins = pluginSystem.getLanguagePluginsForFile("test.js");
    BOLT_ASSERT_EQ(0, jsPlugins.size());
    
    pluginSystem.unloadPlugin("TestLanguagePlugin");
    pluginSystem.shutdown();
}

BOLT_TEST(PluginSystem, ErrorHandling) {
    bolt::PluginSystem& pluginSystem = bolt::PluginSystem::getInstance();
    
    pluginSystem.shutdown();
    pluginSystem.initialize(nullptr, nullptr);
    
    // Clear any existing errors
    pluginSystem.clearPluginErrors();
    
    // Try to load a non-existent plugin
    BOLT_ASSERT_FALSE(pluginSystem.loadPlugin("/nonexistent/plugin.so"));
    
    auto errors = pluginSystem.getPluginErrors();
    BOLT_ASSERT_TRUE(errors.size() > 0);
    
    pluginSystem.clearPluginErrors();
    errors = pluginSystem.getPluginErrors();
    BOLT_ASSERT_EQ(0, errors.size());
    
    pluginSystem.shutdown();
}

BOLT_TEST(PluginSystem, LegacyPluginSupport) {
    bolt::PluginSystem& pluginSystem = bolt::PluginSystem::getInstance();
    
    pluginSystem.shutdown();
    pluginSystem.initialize(nullptr, nullptr);
    
    bool initialized = false;
    bool cleaned = false;
    
    bolt::Plugin legacyPlugin;
    legacyPlugin.name = "LegacyTest";
    legacyPlugin.version = "1.0.0";
    legacyPlugin.initialize = [&]() { initialized = true; };
    legacyPlugin.cleanup = [&]() { cleaned = true; };
    
    pluginSystem.registerPlugin(legacyPlugin);
    pluginSystem.initializeAll();
    
    BOLT_ASSERT_TRUE(initialized);
    
    pluginSystem.cleanupAll();
    BOLT_ASSERT_TRUE(cleaned);
    
    pluginSystem.shutdown();
}

BOLT_TEST(PluginAPI, VersionCompatibility) {
    bolt::PluginAPIVersion v1_0_0{1, 0, 0};
    bolt::PluginAPIVersion v1_1_0{1, 1, 0};
    bolt::PluginAPIVersion v1_2_0{1, 2, 0};
    bolt::PluginAPIVersion v2_0_0{2, 0, 0};
    
    // Same major version, newer minor should be compatible
    BOLT_ASSERT_TRUE(v1_0_0.isCompatible(v1_0_0));
    BOLT_ASSERT_TRUE(v1_1_0.isCompatible(v1_0_0));
    BOLT_ASSERT_TRUE(v1_2_0.isCompatible(v1_1_0));
    
    // Older minor version should not be compatible
    BOLT_ASSERT_FALSE(v1_0_0.isCompatible(v1_1_0));
    
    // Different major version should not be compatible
    BOLT_ASSERT_FALSE(v1_0_0.isCompatible(v2_0_0));
    BOLT_ASSERT_FALSE(v2_0_0.isCompatible(v1_0_0));
}

BOLT_TEST(PluginAPI, VersionStringFormatting) {
    bolt::PluginAPIVersion version{1, 2, 3};
    BOLT_ASSERT_EQ("1.2.3", version.toString());
}

BOLT_TEST(PluginMetadata, ValidMetadata) {
    bolt::PluginMetadata metadata;
    metadata.name = "TestPlugin";
    metadata.version = "1.0.0";
    metadata.description = "Test plugin";
    
    BOLT_ASSERT_TRUE(metadata.isValid());
}

BOLT_TEST(PluginMetadata, InvalidMetadataEmptyName) {
    bolt::PluginMetadata metadata;
    metadata.name = "";
    metadata.version = "1.0.0";
    
    BOLT_ASSERT_FALSE(metadata.isValid());
}

BOLT_TEST(PluginMetadata, InvalidMetadataEmptyVersion) {
    bolt::PluginMetadata metadata;
    metadata.name = "TestPlugin";
    metadata.version = "";
    
    BOLT_ASSERT_FALSE(metadata.isValid());
}

int main() {
    bolt::test::TestSuite& suite = bolt::test::TestSuite::getInstance();
    
    try {
        std::cout << "Running Plugin System Tests..." << std::endl;
        int result = suite.runAllTests();
        
        if (result == 0) {
            std::cout << "\n✅ All Plugin System tests passed!" << std::endl;
        } else {
            std::cout << "\n❌ Some Plugin System tests failed!" << std::endl;
        }
        
        return result;
    } catch (...) {
        std::cout << "\n❌ Plugin System tests crashed!" << std::endl;
        return 1;
    }
}