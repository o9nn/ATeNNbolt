#include "bolt/core/plugin_interface.hpp"
#include <iostream>
#include <algorithm>

/**
 * Example C++ Language Plugin
 * 
 * This plugin provides basic C++ language support including:
 * - Code completions for common C++ keywords and STL components
 * - Basic syntax validation
 * - Code formatting
 */
class CppLanguagePlugin : public bolt::ILanguagePlugin {
private:
    bolt::PluginMetadata metadata_;
    bolt::PluginState state_ = bolt::PluginState::Unloaded;
    bolt::PluginContext* context_ = nullptr;

public:
    CppLanguagePlugin() {
        metadata_.name = "CppLanguagePlugin";
        metadata_.version = "1.0.0";
        metadata_.description = "C++ Language Support Plugin";
        metadata_.author = "Bolt IDE Team";
        metadata_.license = "MIT";
        metadata_.apiVersion = {1, 0, 0};
        metadata_.dependencies = {};
        metadata_.customProperties["language"] = "cpp";
    }

    // IPlugin interface
    bolt::PluginMetadata getMetadata() const override {
        return metadata_;
    }

    bool initialize(bolt::PluginContext* context) override {
        context_ = context;
        state_ = bolt::PluginState::Initialized;
        
        if (context_) {
            context_->logInfo("C++ Language Plugin initialized");
            
            // Subscribe to relevant events
            context_->subscribeToEvent(bolt::PluginEventType::DocumentOpened, 
                [this](const bolt::PluginEvent& event) {
                    try {
                        std::string filePath = event.getData<std::string>("filePath");
                        if (canHandleFileType(getFileExtension(filePath))) {
                            context_->logInfo("C++ plugin handling file: " + filePath);
                        }
                    } catch (const std::exception& e) {
                        context_->logError("Error handling document opened event: " + std::string(e.what()));
                    }
                });
        }
        
        return true;
    }

    void activate() override {
        state_ = bolt::PluginState::Active;
        if (context_) {
            context_->logInfo("C++ Language Plugin activated");
        }
    }

    void deactivate() override {
        state_ = bolt::PluginState::Loaded;
        if (context_) {
            context_->logInfo("C++ Language Plugin deactivated");
        }
    }

    void cleanup() override {
        if (context_) {
            context_->unsubscribeFromEvent(bolt::PluginEventType::DocumentOpened);
            context_->logInfo("C++ Language Plugin cleaned up");
        }
        state_ = bolt::PluginState::Unloaded;
        context_ = nullptr;
    }

    bool configure(const std::unordered_map<std::string, std::any>& config) override {
        // Handle configuration options
        try {
            if (config.find("enableAdvancedCompletions") != config.end()) {
                bool enabled = std::any_cast<bool>(config.at("enableAdvancedCompletions"));
                if (context_) {
                    context_->logInfo("Advanced completions " + std::string(enabled ? "enabled" : "disabled"));
                }
            }
            
            if (config.find("maxCompletions") != config.end()) {
                int maxCompletions = std::any_cast<int>(config.at("maxCompletions"));
                if (context_) {
                    context_->logInfo("Max completions set to: " + std::to_string(maxCompletions));
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            if (context_) {
                context_->logError("Configuration error: " + std::string(e.what()));
            }
            return false;
        }
    }

    // IEditorPlugin interface
    void onDocumentOpened(const std::string& filePath, const std::string& content) override {
        if (canHandleFileType(getFileExtension(filePath))) {
            if (context_) {
                context_->logInfo("Processing C++ file: " + filePath);
            }
            // Could analyze the content here for syntax errors, etc.
        }
    }

    void onDocumentModified(const std::string& filePath, const std::string& content) override {
        if (canHandleFileType(getFileExtension(filePath))) {
            // Could provide real-time syntax checking here
        }
    }

    bool canHandleFileType(const std::string& fileExtension) override {
        auto extensions = getSupportedExtensions();
        return std::find(extensions.begin(), extensions.end(), fileExtension) != extensions.end();
    }

    // ILanguagePlugin interface
    std::vector<std::string> getSupportedExtensions() const override {
        return {"cpp", "cxx", "cc", "hpp", "hxx", "h", "c"};
    }

    std::vector<std::string> getCodeCompletions(const std::string& filePath, size_t line, size_t column) override {
        std::vector<std::string> completions;
        
        // Basic C++ completions
        completions.push_back("std::");
        completions.push_back("auto");
        completions.push_back("const");
        completions.push_back("constexpr");
        completions.push_back("decltype");
        completions.push_back("template");
        completions.push_back("typename");
        completions.push_back("namespace");
        completions.push_back("class");
        completions.push_back("struct");
        completions.push_back("enum");
        completions.push_back("union");
        completions.push_back("public:");
        completions.push_back("private:");
        completions.push_back("protected:");
        completions.push_back("virtual");
        completions.push_back("override");
        completions.push_back("final");
        completions.push_back("static");
        completions.push_back("inline");
        completions.push_back("explicit");
        completions.push_back("consteval");
        completions.push_back("constinit");
        completions.push_back("concept");
        completions.push_back("requires");
        completions.push_back("noexcept");
        completions.push_back("nullptr");
        
        // STL containers
        completions.push_back("std::vector");
        completions.push_back("std::string");
        completions.push_back("std::map");
        completions.push_back("std::unordered_map");
        completions.push_back("std::set");
        completions.push_back("std::unordered_set");
        completions.push_back("std::list");
        completions.push_back("std::deque");
        completions.push_back("std::array");
        completions.push_back("std::queue");
        completions.push_back("std::stack");
        completions.push_back("std::priority_queue");
        
        // Smart pointers
        completions.push_back("std::unique_ptr");
        completions.push_back("std::shared_ptr");
        completions.push_back("std::weak_ptr");
        completions.push_back("std::make_unique");
        completions.push_back("std::make_shared");
        
        // Common algorithms
        completions.push_back("std::for_each");
        completions.push_back("std::find");
        completions.push_back("std::find_if");
        completions.push_back("std::sort");
        completions.push_back("std::transform");
        completions.push_back("std::copy");
        completions.push_back("std::move");
        completions.push_back("std::forward");
        
        return completions;
    }

    std::vector<std::string> getDiagnostics(const std::string& filePath, const std::string& content) override {
        std::vector<std::string> diagnostics;
        
        // Basic syntax checks (simplified)
        if (content.find("#include") == std::string::npos && content.find("int main") != std::string::npos) {
            diagnostics.push_back("Warning: No includes found but main function detected");
        }
        
        // Check for common issues
        if (content.find("using namespace std;") != std::string::npos) {
            diagnostics.push_back("Info: Consider avoiding 'using namespace std;' in headers");
        }
        
        return diagnostics;
    }

    std::string formatCode(const std::string& code) override {
        // Basic formatting (in a real implementation, this would use a proper formatter)
        std::string formatted = code;
        
        // Simple formatting rules
        // Add space after keywords
        std::vector<std::string> keywords = {"if", "for", "while", "switch"};
        for (const auto& keyword : keywords) {
            size_t pos = 0;
            while ((pos = formatted.find(keyword + "(", pos)) != std::string::npos) {
                formatted.insert(pos + keyword.length(), " ");
                pos += keyword.length() + 2;
            }
        }
        
        return formatted;
    }

    std::string getHoverInfo(const std::string& filePath, size_t line, size_t column) override {
        // In a real implementation, this would analyze the symbol at the cursor position
        return "C++ symbol information would appear here";
    }

private:
    std::string getFileExtension(const std::string& filePath) const {
        size_t lastDot = filePath.find_last_of('.');
        if (lastDot != std::string::npos && lastDot < filePath.length() - 1) {
            return filePath.substr(lastDot + 1);
        }
        return "";
    }
};

// Plugin export functions (required for dynamic loading)
extern "C" {
    bolt::IPlugin* createPlugin() {
        return new CppLanguagePlugin();
    }
    
    void destroyPlugin(bolt::IPlugin* plugin) {
        delete plugin;
    }
}

// For static linking, provide a factory function
std::shared_ptr<bolt::IPlugin> createCppLanguagePlugin() {
    return std::make_shared<CppLanguagePlugin>();
}