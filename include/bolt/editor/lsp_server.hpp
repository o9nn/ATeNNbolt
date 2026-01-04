#ifndef LSP_SERVER_HPP
#define LSP_SERVER_HPP

#include "bolt/editor/lsp_protocol.hpp"
#include "bolt/editor/lsp_json_rpc.hpp"
#include "bolt/core/plugin_interface.hpp"
#include <string>
#include <memory>
#include <map>
#include <functional>

namespace bolt {
namespace lsp {

// LSP Server Capabilities
struct ServerCapabilities {
    bool textDocumentSync = true;
    bool hoverProvider = true;
    bool completionProvider = true;
    bool definitionProvider = true;
    bool referencesProvider = true;
    bool documentHighlightProvider = false;
    bool documentSymbolProvider = true;
    bool workspaceSymbolProvider = false;
    bool codeActionProvider = false;
    bool documentFormattingProvider = true;
    bool documentRangeFormattingProvider = false;
    bool documentOnTypeFormattingProvider = false;
    bool renameProvider = false;
    bool signatureHelpProvider = false;
};

// LSP Initialize Params
struct InitializeParams {
    std::string rootPath;
    std::string rootUri;
    std::map<std::string, std::string> initializationOptions;
    ServerCapabilities capabilities;
};

// LSP Initialize Result
struct InitializeResult {
    ServerCapabilities capabilities;
};

// Note: TextDocumentItem, TextDocumentIdentifier, VersionedTextDocumentIdentifier,
// TextDocumentPositionParams, and TextDocumentContentChangeEvent are now defined
// in lsp_protocol.hpp to avoid duplication

// LSP Language Server
class LanguageServer {
private:
    JsonRpcHandler rpcHandler_;
    ServerCapabilities capabilities_;
    bool initialized_ = false;
    std::string rootPath_;
    
    // Document storage
    std::map<std::string, TextDocumentItem> documents_;
    
    // Language plugin registry
    std::map<std::string, std::shared_ptr<ILanguagePlugin>> languagePlugins_;
    
public:
    LanguageServer();
    ~LanguageServer() = default;
    
    // Server lifecycle
    void initialize(const InitializeParams& params);
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Document management
    void didOpenTextDocument(const TextDocumentItem& document);
    void didChangeTextDocument(const VersionedTextDocumentIdentifier& document, 
                              const std::vector<TextDocumentContentChangeEvent>& changes);
    void didCloseTextDocument(const TextDocumentIdentifier& document);
    void didSaveTextDocument(const TextDocumentIdentifier& document);
    
    // Language features
    CompletionList completion(const TextDocumentPositionParams& params);
    std::optional<Hover> hover(const TextDocumentPositionParams& params);
    std::vector<Location> definition(const TextDocumentPositionParams& params);
    std::vector<Location> references(const TextDocumentPositionParams& params);
    std::vector<DocumentSymbol> documentSymbol(const TextDocumentIdentifier& document);
    std::vector<TextEdit> formatting(const TextDocumentIdentifier& document, const FormattingOptions& options);
    std::vector<Diagnostic> diagnostics(const TextDocumentIdentifier& document);
    
    // Plugin management
    void registerLanguagePlugin(const std::string& languageId, std::shared_ptr<ILanguagePlugin> plugin);
    void unregisterLanguagePlugin(const std::string& languageId);
    std::shared_ptr<ILanguagePlugin> getLanguagePlugin(const std::string& uri);
    
    // Message processing
    std::string processMessage(const std::string& message);
    
    // Capabilities
    const ServerCapabilities& getCapabilities() const { return capabilities_; }
    void setCapabilities(const ServerCapabilities& caps) { capabilities_ = caps; }
    
private:
    void setupMessageHandlers();
    std::string getLanguageIdFromUri(const std::string& uri);
    std::string uriToFilePath(const std::string& uri);
    std::string filePathToUri(const std::string& filePath);
    
    // JSON-RPC handlers
    std::shared_ptr<JsonValue> handleInitialize(const std::string& method, std::shared_ptr<JsonValue> params);
    std::shared_ptr<JsonValue> handleTextDocumentCompletion(const std::string& method, std::shared_ptr<JsonValue> params);
    std::shared_ptr<JsonValue> handleTextDocumentHover(const std::string& method, std::shared_ptr<JsonValue> params);
    std::shared_ptr<JsonValue> handleTextDocumentDefinition(const std::string& method, std::shared_ptr<JsonValue> params);
    std::shared_ptr<JsonValue> handleTextDocumentReferences(const std::string& method, std::shared_ptr<JsonValue> params);
    std::shared_ptr<JsonValue> handleTextDocumentDocumentSymbol(const std::string& method, std::shared_ptr<JsonValue> params);
    std::shared_ptr<JsonValue> handleTextDocumentFormatting(const std::string& method, std::shared_ptr<JsonValue> params);
    
    void handleTextDocumentDidOpen(const std::string& method, std::shared_ptr<JsonValue> params);
    void handleTextDocumentDidChange(const std::string& method, std::shared_ptr<JsonValue> params);
    void handleTextDocumentDidClose(const std::string& method, std::shared_ptr<JsonValue> params);
    void handleTextDocumentDidSave(const std::string& method, std::shared_ptr<JsonValue> params);
    void handleInitialized(const std::string& method, std::shared_ptr<JsonValue> params);
    void handleShutdown(const std::string& method, std::shared_ptr<JsonValue> params);
    void handleExit(const std::string& method, std::shared_ptr<JsonValue> params);
    
    // JSON conversion helpers
    Position jsonToPosition(std::shared_ptr<JsonValue> json);
    Range jsonToRange(std::shared_ptr<JsonValue> json);
    TextDocumentIdentifier jsonToTextDocumentIdentifier(std::shared_ptr<JsonValue> json);
    TextDocumentPositionParams jsonToTextDocumentPositionParams(std::shared_ptr<JsonValue> json);
    
    std::shared_ptr<JsonValue> positionToJson(const Position& pos);
    std::shared_ptr<JsonValue> rangeToJson(const Range& range);
    std::shared_ptr<JsonValue> completionItemToJson(const CompletionItem& item);
    std::shared_ptr<JsonValue> completionListToJson(const CompletionList& list);
    std::shared_ptr<JsonValue> hoverToJson(const Hover& hover);
    std::shared_ptr<JsonValue> locationToJson(const Location& location);
    std::shared_ptr<JsonValue> diagnosticToJson(const Diagnostic& diagnostic);
    std::shared_ptr<JsonValue> textEditToJson(const TextEdit& edit);
};

} // namespace lsp
} // namespace bolt

#endif