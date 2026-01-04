#ifndef LSP_PLUGIN_ADAPTER_HPP
#define LSP_PLUGIN_ADAPTER_HPP

#include "bolt/core/plugin_interface.hpp"
#include "bolt/editor/lsp_protocol.hpp"
#include "bolt/editor/lsp_server.hpp"
#include "bolt/editor/lsp_client.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include <memory>
#include <map>

namespace bolt {
namespace lsp {

// Adapter that converts existing ILanguagePlugin to LSP protocol
class LanguagePluginLspAdapter {
private:
    std::shared_ptr<ILanguagePlugin> plugin_;
    std::string languageId_;
    
public:
    LanguagePluginLspAdapter(std::shared_ptr<ILanguagePlugin> plugin, const std::string& languageId);
    ~LanguagePluginLspAdapter() = default;
    
    // Convert plugin methods to LSP types
    CompletionList getCompletion(const std::string& filePath, const Position& position);
    std::optional<Hover> getHover(const std::string& filePath, const Position& position);
    std::vector<Location> getDefinition(const std::string& filePath, const Position& position);
    std::vector<Location> getReferences(const std::string& filePath, const Position& position);
    std::vector<Diagnostic> getDiagnostics(const std::string& filePath, const std::string& content);
    std::vector<TextEdit> formatDocument(const std::string& filePath, const std::string& content, const FormattingOptions& options);
    
    // Get the underlying plugin
    std::shared_ptr<ILanguagePlugin> getPlugin() const { return plugin_; }
    const std::string& getLanguageId() const { return languageId_; }
    
private:
    // Helper methods for conversion
    CompletionItem convertToLspCompletionItem(const std::string& item);
    Location createLocationFromReferences(const std::string& filePath, const std::pair<size_t, size_t>& ref);
    Diagnostic createDiagnosticFromString(const std::string& diagnostic, const std::string& filePath);
    std::string getFileContent(const std::string& filePath);
};

// LSP Integration Plugin that provides LSP support within Bolt's plugin system
class LspIntegrationPlugin : public ILanguagePlugin {
private:
    std::unique_ptr<LanguageServer> lspServer_;
    std::unique_ptr<LspClientManager> clientManager_;
    std::map<std::string, std::unique_ptr<LanguagePluginLspAdapter>> pluginAdapters_;
    std::map<std::string, std::string> languageIdMapping_;
    bool enabled_ = true;
    
public:
    LspIntegrationPlugin();
    ~LspIntegrationPlugin() override = default;
    
    // IPlugin interface
    PluginMetadata getMetadata() const override {
        PluginMetadata metadata;
        metadata.name = "LSP Integration Plugin";
        metadata.version = "1.0.0";
        metadata.description = "Provides Language Server Protocol support";
        metadata.author = "Bolt C++ IDE";
        metadata.apiVersion = {1, 0, 0};
        return metadata;
    }
    
    bool initialize(PluginContext* context) override;
    void activate() override;
    void deactivate() override;
    void cleanup() override;
    
    PluginState getState() const override { return enabled_ ? PluginState::Active : PluginState::Loaded; }
    std::unordered_map<std::string, std::any> getConfiguration() const override;
    bool configure(const std::unordered_map<std::string, std::any>& config) override;
    
    // IEditorPlugin interface
    void onDocumentOpened(const std::string& filePath, const std::string& content) override;
    void onDocumentClosed(const std::string& filePath) override;
    void onDocumentModified(const std::string& filePath, const std::string& content) override;
    void onCursorChanged(const std::string& filePath, size_t line, size_t column) override;
    
    // ILanguagePlugin interface
    std::vector<std::string> getCodeCompletions(const std::string& filePath, size_t line, size_t column) override;
    std::vector<std::string> getDiagnostics(const std::string& filePath, const std::string& content) override;
    std::string formatCode(const std::string& code) override;
    std::vector<std::pair<size_t, size_t>> findReferences(const std::string& filePath, size_t line, size_t column) override;
    std::string getHoverInfo(const std::string& filePath, size_t line, size_t column) override;
    std::vector<std::string> getSupportedExtensions() const override;
    
    // LSP-specific functionality
    void registerLanguagePlugin(const std::string& languageId, std::shared_ptr<ILanguagePlugin> plugin);
    void registerExternalLanguageServer(const std::string& languageId, const LspClientConfig& config);
    void unregisterLanguagePlugin(const std::string& languageId);
    void unregisterExternalLanguageServer(const std::string& languageId);
    
    // LSP server access
    LanguageServer* getLspServer() const { return lspServer_.get(); }
    LspClientManager* getClientManager() const { return clientManager_.get(); }
    
    // Message processing for external LSP clients
    std::string processLspMessage(const std::string& message);
    
    // Enable/disable LSP features
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
private:
    std::string getLanguageIdFromFilePath(const std::string& filePath);
    std::string getFileExtension(const std::string& filePath);
    void initializeLspServer();
    void initializeClientManager();
    
    // Helper conversion methods
    std::vector<std::string> convertLspCompletions(const CompletionList& completions);
    std::vector<std::string> convertLspDiagnostics(const std::vector<Diagnostic>& diagnostics);
    std::vector<std::pair<size_t, size_t>> convertLspReferences(const std::vector<Location>& references);
    std::string convertLspHover(const std::optional<Hover>& hover);
};

// Factory function to create LSP Integration Plugin
std::shared_ptr<LspIntegrationPlugin> createLspIntegrationPlugin();

} // namespace lsp
} // namespace bolt

#endif