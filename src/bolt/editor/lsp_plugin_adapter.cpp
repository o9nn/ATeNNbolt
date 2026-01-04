#include "bolt/editor/lsp_plugin_adapter.hpp"
#include "bolt/core/editor_store.hpp"
#include <fstream>
#include <sstream>

namespace bolt {
namespace lsp {

// LanguagePluginLspAdapter implementation
LanguagePluginLspAdapter::LanguagePluginLspAdapter(std::shared_ptr<ILanguagePlugin> plugin, const std::string& languageId)
    : plugin_(plugin), languageId_(languageId) {
}

CompletionList LanguagePluginLspAdapter::getCompletion(const std::string& filePath, const Position& position) {
    CompletionList result;
    
    if (plugin_) {
        auto completions = plugin_->getCodeCompletions(filePath, position.line, position.character);
        
        for (const auto& completion : completions) {
            result.items.push_back(convertToLspCompletionItem(completion));
        }
    }
    
    return result;
}

std::optional<Hover> LanguagePluginLspAdapter::getHover(const std::string& filePath, const Position& position) {
    if (plugin_) {
        std::string hoverInfo = plugin_->getHoverInfo(filePath, position.line, position.character);
        
        if (!hoverInfo.empty()) {
            return Hover(hoverInfo);
        }
    }
    
    return std::nullopt;
}

std::vector<Location> LanguagePluginLspAdapter::getDefinition(const std::string& filePath, const Position& position) {
    std::vector<Location> result;
    
    if (plugin_) {
        auto references = plugin_->findReferences(filePath, position.line, position.character);
        
        for (const auto& ref : references) {
            result.push_back(createLocationFromReferences(filePath, ref));
        }
    }
    
    return result;
}

std::vector<Location> LanguagePluginLspAdapter::getReferences(const std::string& filePath, const Position& position) {
    return getDefinition(filePath, position); // For simplicity, use same implementation
}

std::vector<Diagnostic> LanguagePluginLspAdapter::getDiagnostics(const std::string& filePath, const std::string& content) {
    std::vector<Diagnostic> result;
    
    if (plugin_) {
        auto diagnosticStrings = plugin_->getDiagnostics(filePath, content);
        
        for (const auto& diagStr : diagnosticStrings) {
            result.push_back(createDiagnosticFromString(diagStr, filePath));
        }
    }
    
    return result;
}

std::vector<TextEdit> LanguagePluginLspAdapter::formatDocument(const std::string& filePath, const std::string& content, const FormattingOptions& options) {
    std::vector<TextEdit> result;
    
    if (plugin_) {
        std::string formatted = plugin_->formatCode(content);
        
        if (formatted != content) {
            TextEdit edit;
            edit.range = Range(Position(0, 0), Position(SIZE_MAX, 0)); // Full document range
            edit.newText = formatted;
            result.push_back(edit);
        }
    }
    
    return result;
}

CompletionItem LanguagePluginLspAdapter::convertToLspCompletionItem(const std::string& item) {
    CompletionItem lspItem(item);
    lspItem.kind = CompletionItemKind::Text;
    
    // Try to determine kind based on content
    if (item.find("()") != std::string::npos) {
        lspItem.kind = CompletionItemKind::Function;
    } else if (item.find("class") != std::string::npos) {
        lspItem.kind = CompletionItemKind::Class;
    } else if (item.find("#include") != std::string::npos) {
        lspItem.kind = CompletionItemKind::Module;
    }
    
    return lspItem;
}

Location LanguagePluginLspAdapter::createLocationFromReferences(const std::string& filePath, const std::pair<size_t, size_t>& ref) {
    Location location;
    location.uri = "file://" + filePath;
    location.range = Range(Position(ref.first, 0), Position(ref.first, ref.second));
    return location;
}

Diagnostic LanguagePluginLspAdapter::createDiagnosticFromString(const std::string& diagnostic, const std::string& filePath) {
    Diagnostic diag;
    diag.message = diagnostic;
    diag.severity = DiagnosticSeverity::Error;
    diag.range = Range(Position(0, 0), Position(0, 1)); // Default range
    diag.source = languageId_;
    return diag;
}

std::string LanguagePluginLspAdapter::getFileContent(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

// LspIntegrationPlugin implementation
LspIntegrationPlugin::LspIntegrationPlugin() {
    initializeLspServer();
    initializeClientManager();
}

bool LspIntegrationPlugin::initialize(PluginContext* context) {
    // Initialize with context if needed
    return true;
}

void LspIntegrationPlugin::activate() {
    enabled_ = true;
}

void LspIntegrationPlugin::deactivate() {
    enabled_ = false;
}

void LspIntegrationPlugin::cleanup() {
    if (lspServer_) {
        lspServer_->shutdown();
    }
    
    if (clientManager_) {
        clientManager_->shutdown();
    }
    
    pluginAdapters_.clear();
    languageIdMapping_.clear();
}

std::unordered_map<std::string, std::any> LspIntegrationPlugin::getConfiguration() const {
    std::unordered_map<std::string, std::any> config;
    config["enabled"] = enabled_;
    config["serverCapabilities"] = std::string("textDocumentSync,hover,completion");
    return config;
}

bool LspIntegrationPlugin::configure(const std::unordered_map<std::string, std::any>& config) {
    auto it = config.find("enabled");
    if (it != config.end()) {
        try {
            enabled_ = std::any_cast<bool>(it->second);
        } catch (const std::bad_any_cast&) {
            // Ignore invalid configuration
        }
    }
    return true;
}

void LspIntegrationPlugin::onDocumentOpened(const std::string& filePath, const std::string& content) {
    if (!enabled_) return;
    
    if (clientManager_) {
        std::string languageId = getLanguageIdFromFilePath(filePath);
        clientManager_->didOpenTextDocument(filePath, content, languageId);
    }
}

void LspIntegrationPlugin::onDocumentClosed(const std::string& filePath) {
    if (!enabled_) return;
    
    if (clientManager_) {
        clientManager_->didCloseTextDocument(filePath);
    }
}

void LspIntegrationPlugin::onDocumentModified(const std::string& filePath, const std::string& content) {
    if (!enabled_) return;
    
    if (clientManager_) {
        clientManager_->didChangeTextDocument(filePath, content);
    }
}

void LspIntegrationPlugin::onCursorChanged(const std::string& filePath, size_t line, size_t column) {
    // LSP doesn't typically need cursor change notifications, but we could use this for hover requests
}

std::vector<std::string> LspIntegrationPlugin::getCodeCompletions(const std::string& filePath, size_t line, size_t column) {
    if (!enabled_) return {};
    
    std::vector<std::string> result;
    
    // Try LSP client first
    if (clientManager_) {
        // This would be async in real implementation
        // For now, return empty to keep it simple
    }
    
    // Fallback to direct language plugins
    std::string languageId = getLanguageIdFromFilePath(filePath);
    auto it = pluginAdapters_.find(languageId);
    if (it != pluginAdapters_.end()) {
        auto completions = it->second->getCompletion(filePath, Position(line, column));
        result = convertLspCompletions(completions);
    }
    
    return result;
}

std::vector<std::string> LspIntegrationPlugin::getDiagnostics(const std::string& filePath, const std::string& content) {
    if (!enabled_) return {};
    
    std::vector<std::string> result;
    
    // Use direct language plugins for diagnostics
    std::string languageId = getLanguageIdFromFilePath(filePath);
    auto it = pluginAdapters_.find(languageId);
    if (it != pluginAdapters_.end()) {
        auto diagnostics = it->second->getDiagnostics(filePath, content);
        result = convertLspDiagnostics(diagnostics);
    }
    
    return result;
}

std::string LspIntegrationPlugin::formatCode(const std::string& code) {
    if (!enabled_) return code;
    
    // Basic formatting - in real implementation, would determine language and format accordingly
    return code;
}

std::vector<std::pair<size_t, size_t>> LspIntegrationPlugin::findReferences(const std::string& filePath, size_t line, size_t column) {
    if (!enabled_) return {};
    
    std::string languageId = getLanguageIdFromFilePath(filePath);
    auto it = pluginAdapters_.find(languageId);
    if (it != pluginAdapters_.end()) {
        auto locations = it->second->getReferences(filePath, Position(line, column));
        return convertLspReferences(locations);
    }
    
    return {};
}

std::string LspIntegrationPlugin::getHoverInfo(const std::string& filePath, size_t line, size_t column) {
    if (!enabled_) return "";
    
    std::string languageId = getLanguageIdFromFilePath(filePath);
    auto it = pluginAdapters_.find(languageId);
    if (it != pluginAdapters_.end()) {
        auto hover = it->second->getHover(filePath, Position(line, column));
        return convertLspHover(hover);
    }
    
    return "";
}

std::vector<std::string> LspIntegrationPlugin::getSupportedExtensions() const {
    std::vector<std::string> extensions;
    
    // Collect extensions from all registered plugins
    for (const auto& adapter : pluginAdapters_) {
        auto plugin = adapter.second->getPlugin();
        if (plugin) {
            auto pluginExtensions = plugin->getSupportedExtensions();
            extensions.insert(extensions.end(), pluginExtensions.begin(), pluginExtensions.end());
        }
    }
    
    // Add common LSP extensions
    extensions.insert(extensions.end(), {".cpp", ".hpp", ".h", ".c", ".py", ".js", ".ts"});
    
    return extensions;
}

void LspIntegrationPlugin::registerLanguagePlugin(const std::string& languageId, std::shared_ptr<ILanguagePlugin> plugin) {
    auto adapter = std::make_unique<LanguagePluginLspAdapter>(plugin, languageId);
    pluginAdapters_[languageId] = std::move(adapter);
    
    // Register with LSP server
    if (lspServer_) {
        lspServer_->registerLanguagePlugin(languageId, plugin);
    }
}

void LspIntegrationPlugin::registerExternalLanguageServer(const std::string& languageId, const LspClientConfig& config) {
    if (clientManager_) {
        clientManager_->registerLanguageServer(languageId, config);
    }
}

void LspIntegrationPlugin::unregisterLanguagePlugin(const std::string& languageId) {
    pluginAdapters_.erase(languageId);
    
    if (lspServer_) {
        lspServer_->unregisterLanguagePlugin(languageId);
    }
}

void LspIntegrationPlugin::unregisterExternalLanguageServer(const std::string& languageId) {
    if (clientManager_) {
        clientManager_->unregisterLanguageServer(languageId);
    }
}

std::string LspIntegrationPlugin::processLspMessage(const std::string& message) {
    if (lspServer_) {
        return lspServer_->processMessage(message);
    }
    return "";
}

std::string LspIntegrationPlugin::getLanguageIdFromFilePath(const std::string& filePath) {
    std::string extension = getFileExtension(filePath);
    
    if (extension == ".cpp" || extension == ".hpp" || extension == ".h" || extension == ".c") {
        return "cpp";
    } else if (extension == ".py") {
        return "python";
    } else if (extension == ".js" || extension == ".ts") {
        return "javascript";
    } else if (extension == ".java") {
        return "java";
    } else if (extension == ".rs") {
        return "rust";
    }
    
    return "text";
}

std::string LspIntegrationPlugin::getFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        return filePath.substr(dotPos);
    }
    return "";
}

void LspIntegrationPlugin::initializeLspServer() {
    lspServer_ = std::make_unique<LanguageServer>();
    
    // Set up basic capabilities
    ServerCapabilities capabilities;
    capabilities.textDocumentSync = true;
    capabilities.hoverProvider = true;
    capabilities.completionProvider = true;
    capabilities.definitionProvider = true;
    capabilities.referencesProvider = true;
    capabilities.documentFormattingProvider = true;
    
    lspServer_->setCapabilities(capabilities);
}

void LspIntegrationPlugin::initializeClientManager() {
    clientManager_ = std::make_unique<LspClientManager>();
}

std::vector<std::string> LspIntegrationPlugin::convertLspCompletions(const CompletionList& completions) {
    std::vector<std::string> result;
    for (const auto& item : completions.items) {
        result.push_back(item.label);
    }
    return result;
}

std::vector<std::string> LspIntegrationPlugin::convertLspDiagnostics(const std::vector<Diagnostic>& diagnostics) {
    std::vector<std::string> result;
    for (const auto& diag : diagnostics) {
        result.push_back(diag.message);
    }
    return result;
}

std::vector<std::pair<size_t, size_t>> LspIntegrationPlugin::convertLspReferences(const std::vector<Location>& references) {
    std::vector<std::pair<size_t, size_t>> result;
    for (const auto& location : references) {
        result.emplace_back(location.range.start.line, location.range.start.character);
    }
    return result;
}

std::string LspIntegrationPlugin::convertLspHover(const std::optional<Hover>& hover) {
    if (hover.has_value()) {
        return hover->contents;
    }
    return "";
}

// Factory function
std::shared_ptr<LspIntegrationPlugin> createLspIntegrationPlugin() {
    return std::make_shared<LspIntegrationPlugin>();
}

} // namespace lsp
} // namespace bolt