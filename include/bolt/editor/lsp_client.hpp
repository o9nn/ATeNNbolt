#ifndef LSP_CLIENT_HPP
#define LSP_CLIENT_HPP

#include "bolt/editor/lsp_protocol.hpp"
#include "bolt/editor/lsp_json_rpc.hpp"
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace bolt {
namespace lsp {

// LSP Client Connection Interface
class ILspConnection {
public:
    virtual ~ILspConnection() = default;
    virtual bool start(const std::string& command, const std::vector<std::string>& args) = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const = 0;
    virtual void sendMessage(const std::string& message) = 0;
    virtual std::string receiveMessage() = 0;
    virtual bool hasMessage() const = 0;
};

// Process-based LSP Connection (stdio)
class ProcessLspConnection : public ILspConnection {
private:
    std::string command_;
    std::vector<std::string> args_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> shouldStop_{false};
    
    // Process handles (platform-specific implementation needed)
    int processId_ = -1;
    int stdinPipe_ = -1;
    int stdoutPipe_ = -1;
    
    std::thread readerThread_;
    std::queue<std::string> messageQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable messageAvailable_;
    
public:
    ProcessLspConnection() = default;
    ~ProcessLspConnection() override;
    
    bool start(const std::string& command, const std::vector<std::string>& args) override;
    void stop() override;
    bool isConnected() const override { return connected_.load(); }
    void sendMessage(const std::string& message) override;
    std::string receiveMessage() override;
    bool hasMessage() const override;
    
private:
    void readerThreadFunc();
    bool createProcess();
    void terminateProcess();
    std::string readLspMessage();
    void writeLspMessage(const std::string& message);
};

// LSP Client Configuration
struct LspClientConfig {
    std::string serverCommand;
    std::vector<std::string> serverArgs;
    std::string languageId;
    std::vector<std::string> fileExtensions;
    std::string rootPath;
    std::map<std::string, std::string> initializationOptions;
    int timeoutMs = 5000;
};

// LSP Client for communicating with external language servers
class LspClient {
private:
    std::unique_ptr<ILspConnection> connection_;
    JsonRpcHandler rpcHandler_;
    LspClientConfig config_;
    bool initialized_ = false;
    std::atomic<bool> running_{false};
    
    std::thread messageProcessor_;
    std::string nextRequestId_;
    std::map<std::string, std::function<void(std::shared_ptr<JsonValue>)>> pendingRequests_;
    mutable std::mutex requestMutex_;
    
public:
    explicit LspClient(std::unique_ptr<ILspConnection> connection);
    ~LspClient();
    
    // Client lifecycle
    bool connect(const LspClientConfig& config);
    void disconnect();
    bool isConnected() const;
    bool isInitialized() const { return initialized_; }
    
    // LSP Operations (async)
    void initialize(std::function<void(bool)> callback = nullptr);
    void shutdown(std::function<void(bool)> callback = nullptr);
    
    // Document operations
    void didOpenTextDocument(const TextDocumentItem& document);
    void didChangeTextDocument(const VersionedTextDocumentIdentifier& document, 
                              const std::vector<TextDocumentContentChangeEvent>& changes);
    void didCloseTextDocument(const TextDocumentIdentifier& document);
    void didSaveTextDocument(const TextDocumentIdentifier& document);
    
    // Language features (async with callbacks)
    void completion(const TextDocumentPositionParams& params, 
                   std::function<void(const CompletionList&)> callback);
    void hover(const TextDocumentPositionParams& params, 
              std::function<void(const std::optional<Hover>&)> callback);
    void definition(const TextDocumentPositionParams& params, 
                   std::function<void(const std::vector<Location>&)> callback);
    void references(const TextDocumentPositionParams& params, 
                   std::function<void(const std::vector<Location>&)> callback);
    void documentSymbol(const TextDocumentIdentifier& document, 
                       std::function<void(const std::vector<DocumentSymbol>&)> callback);
    void formatting(const TextDocumentIdentifier& document, const FormattingOptions& options,
                   std::function<void(const std::vector<TextEdit>&)> callback);
    
    // Synchronous versions with timeout
    CompletionList completionSync(const TextDocumentPositionParams& params, int timeoutMs = 5000);
    std::optional<Hover> hoverSync(const TextDocumentPositionParams& params, int timeoutMs = 5000);
    std::vector<Location> definitionSync(const TextDocumentPositionParams& params, int timeoutMs = 5000);
    std::vector<Location> referencesSync(const TextDocumentPositionParams& params, int timeoutMs = 5000);
    std::vector<DocumentSymbol> documentSymbolSync(const TextDocumentIdentifier& document, int timeoutMs = 5000);
    std::vector<TextEdit> formattingSync(const TextDocumentIdentifier& document, const FormattingOptions& options, int timeoutMs = 5000);
    
    // Configuration
    const LspClientConfig& getConfig() const { return config_; }
    
private:
    void messageProcessorFunc();
    std::string generateRequestId();
    
    void sendRequest(const std::string& method, std::shared_ptr<JsonValue> params, 
                    std::function<void(std::shared_ptr<JsonValue>)> callback = nullptr);
    void sendNotification(const std::string& method, std::shared_ptr<JsonValue> params);
    
    // JSON conversion helpers (similar to server)
    std::shared_ptr<JsonValue> textDocumentItemToJson(const TextDocumentItem& item);
    std::shared_ptr<JsonValue> textDocumentIdentifierToJson(const TextDocumentIdentifier& identifier);
    std::shared_ptr<JsonValue> versionedTextDocumentIdentifierToJson(const VersionedTextDocumentIdentifier& identifier);
    std::shared_ptr<JsonValue> textDocumentPositionParamsToJson(const TextDocumentPositionParams& params);
    std::shared_ptr<JsonValue> formattingOptionsToJson(const FormattingOptions& options);
    
    CompletionList jsonToCompletionList(std::shared_ptr<JsonValue> json);
    Hover jsonToHover(std::shared_ptr<JsonValue> json);
    Location jsonToLocation(std::shared_ptr<JsonValue> json);
    DocumentSymbol jsonToDocumentSymbol(std::shared_ptr<JsonValue> json);
    TextEdit jsonToTextEdit(std::shared_ptr<JsonValue> json);
    Position jsonToPosition(std::shared_ptr<JsonValue> json);
    Range jsonToRange(std::shared_ptr<JsonValue> json);
};

// LSP Client Manager for managing multiple language servers
class LspClientManager {
private:
    std::map<std::string, std::unique_ptr<LspClient>> clients_;
    std::map<std::string, std::string> fileExtensionToLanguage_;
    std::map<std::string, LspClientConfig> languageConfigs_;
    mutable std::mutex clientsMutex_;
    
public:
    LspClientManager() = default;
    ~LspClientManager();
    
    // Configuration
    void registerLanguageServer(const std::string& languageId, const LspClientConfig& config);
    void unregisterLanguageServer(const std::string& languageId);
    
    // Client management
    LspClient* getClientForFile(const std::string& filePath);
    LspClient* getClientForLanguage(const std::string& languageId);
    std::vector<std::string> getRegisteredLanguages() const;
    
    // Document operations (routes to appropriate client)
    void didOpenTextDocument(const std::string& filePath, const std::string& content, const std::string& languageId = "");
    void didChangeTextDocument(const std::string& filePath, const std::string& content, int version = 1);
    void didCloseTextDocument(const std::string& filePath);
    void didSaveTextDocument(const std::string& filePath);
    
    // Language features (routes to appropriate client)
    void completion(const std::string& filePath, size_t line, size_t character, 
                   std::function<void(const CompletionList&)> callback);
    void hover(const std::string& filePath, size_t line, size_t character, 
              std::function<void(const std::optional<Hover>&)> callback);
    void definition(const std::string& filePath, size_t line, size_t character, 
                   std::function<void(const std::vector<Location>&)> callback);
    
    // Shutdown all clients
    void shutdown();
    
private:
    std::string getLanguageIdFromFilePath(const std::string& filePath);
    std::string filePathToUri(const std::string& filePath);
    LspClient* createClientForLanguage(const std::string& languageId);
};

} // namespace lsp
} // namespace bolt

#endif