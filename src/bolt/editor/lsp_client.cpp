#include "bolt/editor/lsp_client.hpp"
#include "bolt/editor/lsp_protocol.hpp"

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#else
#include <windows.h>
#include <io.h>
#endif

#include <cstring>

namespace bolt {
namespace lsp {

// ProcessLspConnection implementation
ProcessLspConnection::~ProcessLspConnection() {
    stop();
}

bool ProcessLspConnection::start(const std::string& command, const std::vector<std::string>& args) {
    command_ = command;
    args_ = args;
    
    if (!createProcess()) {
        return false;
    }
    
    connected_ = true;
    shouldStop_ = false;
    
    // Start reader thread
    readerThread_ = std::thread(&ProcessLspConnection::readerThreadFunc, this);
    
    return true;
}

void ProcessLspConnection::stop() {
    shouldStop_ = true;
    connected_ = false;
    
    terminateProcess();
    
    if (readerThread_.joinable()) {
        messageAvailable_.notify_all();
        readerThread_.join();
    }
    
    // Close pipes
    if (stdinPipe_ >= 0) {
        close(stdinPipe_);
        stdinPipe_ = -1;
    }
    if (stdoutPipe_ >= 0) {
        close(stdoutPipe_);
        stdoutPipe_ = -1;
    }
}

void ProcessLspConnection::sendMessage(const std::string& message) {
    if (!connected_) return;
    
    writeLspMessage(message);
}

std::string ProcessLspConnection::receiveMessage() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    if (messageQueue_.empty()) {
        messageAvailable_.wait(lock, [this] { return !messageQueue_.empty() || shouldStop_; });
    }
    
    if (messageQueue_.empty()) {
        return "";
    }
    
    std::string message = messageQueue_.front();
    messageQueue_.pop();
    return message;
}

bool ProcessLspConnection::hasMessage() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return !messageQueue_.empty();
}

void ProcessLspConnection::readerThreadFunc() {
    while (!shouldStop_ && connected_) {
        std::string message = readLspMessage();
        if (!message.empty()) {
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                messageQueue_.push(message);
            }
            messageAvailable_.notify_one();
        }
        
        if (!connected_) break;
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool ProcessLspConnection::createProcess() {
    int stdinPipes[2], stdoutPipes[2];
    
    // Create pipes
    if (pipe(stdinPipes) == -1 || pipe(stdoutPipes) == -1) {
        return false;
    }
    
    processId_ = fork();
    if (processId_ == -1) {
        // Fork failed
        close(stdinPipes[0]);
        close(stdinPipes[1]);
        close(stdoutPipes[0]);
        close(stdoutPipes[1]);
        return false;
    }
    
    if (processId_ == 0) {
        // Child process
        
        // Redirect stdin and stdout
        dup2(stdinPipes[0], STDIN_FILENO);
        dup2(stdoutPipes[1], STDOUT_FILENO);
        
        // Close unused pipe ends
        close(stdinPipes[1]);
        close(stdoutPipes[0]);
        close(stdinPipes[0]);
        close(stdoutPipes[1]);
        
        // Prepare arguments
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(command_.c_str()));
        for (const auto& arg : args_) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        // Execute the language server
        execvp(command_.c_str(), argv.data());
        
        // If we get here, exec failed
        _exit(1);
    } else {
        // Parent process
        stdinPipe_ = stdinPipes[1];   // Write to child's stdin
        stdoutPipe_ = stdoutPipes[0]; // Read from child's stdout
        
        // Close unused pipe ends
        close(stdinPipes[0]);
        close(stdoutPipes[1]);
        
        // Make stdout pipe non-blocking
        int flags = fcntl(stdoutPipe_, F_GETFL);
        fcntl(stdoutPipe_, F_SETFL, flags | O_NONBLOCK);
    }
    
    return true;
}

void ProcessLspConnection::terminateProcess() {
    if (processId_ > 0) {
        kill(processId_, SIGTERM);
        
        // Wait for process to terminate
        int status;
        if (waitpid(processId_, &status, WNOHANG) == 0) {
            // Process still running, force kill
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            kill(processId_, SIGKILL);
            waitpid(processId_, &status, 0);
        }
        
        processId_ = -1;
    }
}

std::string ProcessLspConnection::readLspMessage() {
    if (stdoutPipe_ < 0) return "";
    
    // Read LSP header
    std::string header;
    char ch;
    while (true) {
        ssize_t bytesRead = read(stdoutPipe_, &ch, 1);
        if (bytesRead <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available
                return "";
            }
            // Error or EOF
            connected_ = false;
            return "";
        }
        
        header += ch;
        if (header.length() >= 4 && header.substr(header.length() - 4) == "\r\n\r\n") {
            break;
        }
        
        // Prevent infinite loop
        if (header.length() > 1000) {
            return "";
        }
    }
    
    // Parse Content-Length
    size_t contentLength = 0;
    size_t pos = header.find("Content-Length: ");
    if (pos != std::string::npos) {
        pos += 16; // Length of "Content-Length: "
        size_t endPos = header.find("\r\n", pos);
        if (endPos != std::string::npos) {
            std::string lengthStr = header.substr(pos, endPos - pos);
            contentLength = std::stoul(lengthStr);
        }
    }
    
    if (contentLength == 0) {
        return "";
    }
    
    // Read message content
    std::string content;
    content.reserve(contentLength);
    
    while (content.length() < contentLength) {
        char buffer[1024];
        size_t toRead = std::min(sizeof(buffer), contentLength - content.length());
        ssize_t bytesRead = read(stdoutPipe_, buffer, toRead);
        
        if (bytesRead <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Wait a bit and try again
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            // Error or EOF
            connected_ = false;
            return "";
        }
        
        content.append(buffer, bytesRead);
    }
    
    return content;
}

void ProcessLspConnection::writeLspMessage(const std::string& message) {
    if (stdinPipe_ < 0) return;
    
    // Create LSP header
    std::string header = "Content-Length: " + std::to_string(message.length()) + "\r\n\r\n";
    std::string fullMessage = header + message;
    
    // Write message
    ssize_t bytesWritten = write(stdinPipe_, fullMessage.c_str(), fullMessage.length());
    if (bytesWritten != static_cast<ssize_t>(fullMessage.length())) {
        connected_ = false;
    }
}

// LspClient implementation
LspClient::LspClient(std::unique_ptr<ILspConnection> connection)
    : connection_(std::move(connection)) {
}

LspClient::~LspClient() {
    disconnect();
}

bool LspClient::connect(const LspClientConfig& config) {
    config_ = config;
    
    if (!connection_->start(config.serverCommand, config.serverArgs)) {
        return false;
    }
    
    running_ = true;
    messageProcessor_ = std::thread(&LspClient::messageProcessorFunc, this);
    
    return true;
}

void LspClient::disconnect() {
    running_ = false;
    
    if (connection_) {
        connection_->stop();
    }
    
    if (messageProcessor_.joinable()) {
        messageProcessor_.join();
    }
    
    initialized_ = false;
}

bool LspClient::isConnected() const {
    return connection_ && connection_->isConnected();
}

void LspClient::initialize(std::function<void(bool)> callback) {
    auto params = std::make_shared<JsonValue>();
    params->setObject();
    params->setProperty("processId", std::make_shared<JsonValue>(static_cast<double>(getpid())));
    params->setProperty("rootPath", std::make_shared<JsonValue>(config_.rootPath));
    params->setProperty("rootUri", std::make_shared<JsonValue>("file://" + config_.rootPath));
    
    // Add client capabilities
    auto capabilities = std::make_shared<JsonValue>();
    capabilities->setObject();
    
    auto textDocumentCapabilities = std::make_shared<JsonValue>();
    textDocumentCapabilities->setObject();
    
    auto completionCapabilities = std::make_shared<JsonValue>();
    completionCapabilities->setObject();
    completionCapabilities->setProperty("dynamicRegistration", std::make_shared<JsonValue>(false));
    textDocumentCapabilities->setProperty("completion", completionCapabilities);
    
    auto hoverCapabilities = std::make_shared<JsonValue>();
    hoverCapabilities->setObject();
    hoverCapabilities->setProperty("dynamicRegistration", std::make_shared<JsonValue>(false));
    textDocumentCapabilities->setProperty("hover", hoverCapabilities);
    
    capabilities->setProperty("textDocument", textDocumentCapabilities);
    params->setProperty("capabilities", capabilities);
    
    sendRequest("initialize", params, [this, callback](std::shared_ptr<JsonValue> result) {
        initialized_ = (result != nullptr);
        if (callback) {
            callback(initialized_);
        }
        
        // Send initialized notification
        if (initialized_) {
            sendNotification("initialized", std::make_shared<JsonValue>());
        }
    });
}

void LspClient::shutdown(std::function<void(bool)> callback) {
    sendRequest("shutdown", nullptr, [this, callback](std::shared_ptr<JsonValue> result) {
        sendNotification("exit", nullptr);
        initialized_ = false;
        if (callback) {
            callback(true);
        }
    });
}

void LspClient::didOpenTextDocument(const TextDocumentItem& document) {
    auto params = textDocumentItemToJson(document);
    
    auto notification = std::make_shared<JsonValue>();
    notification->setObject();
    notification->setProperty("textDocument", params);
    
    sendNotification("textDocument/didOpen", notification);
}

void LspClient::didChangeTextDocument(const VersionedTextDocumentIdentifier& document, 
                                     const std::vector<TextDocumentContentChangeEvent>& changes) {
    auto params = std::make_shared<JsonValue>();
    params->setObject();
    params->setProperty("textDocument", versionedTextDocumentIdentifierToJson(document));
    
    auto changesArray = std::make_shared<JsonValue>();
    changesArray->setArray();
    for (const auto& change : changes) {
        auto changeJson = std::make_shared<JsonValue>();
        changeJson->setObject();
        changeJson->setProperty("text", std::make_shared<JsonValue>(change.text));
        changesArray->addArrayElement(changeJson);
    }
    params->setProperty("contentChanges", changesArray);
    
    sendNotification("textDocument/didChange", params);
}

void LspClient::didCloseTextDocument(const TextDocumentIdentifier& document) {
    auto params = std::make_shared<JsonValue>();
    params->setObject();
    params->setProperty("textDocument", textDocumentIdentifierToJson(document));
    
    sendNotification("textDocument/didClose", params);
}

void LspClient::didSaveTextDocument(const TextDocumentIdentifier& document) {
    auto params = std::make_shared<JsonValue>();
    params->setObject();
    params->setProperty("textDocument", textDocumentIdentifierToJson(document));
    
    sendNotification("textDocument/didSave", params);
}

void LspClient::completion(const TextDocumentPositionParams& params, 
                          std::function<void(const CompletionList&)> callback) {
    auto jsonParams = textDocumentPositionParamsToJson(params);
    
    sendRequest("textDocument/completion", jsonParams, [this, callback](std::shared_ptr<JsonValue> result) {
        if (callback) {
            CompletionList completions = jsonToCompletionList(result);
            callback(completions);
        }
    });
}

void LspClient::hover(const TextDocumentPositionParams& params, 
                     std::function<void(const std::optional<Hover>&)> callback) {
    auto jsonParams = textDocumentPositionParamsToJson(params);
    
    sendRequest("textDocument/hover", jsonParams, [this, callback](std::shared_ptr<JsonValue> result) {
        if (callback) {
            if (result && result->getType() != JsonValue::Null) {
                Hover hover = jsonToHover(result);
                callback(hover);
            } else {
                callback(std::nullopt);
            }
        }
    });
}

void LspClient::definition(const TextDocumentPositionParams& params,
                          std::function<void(const std::vector<Location>&)> callback) {
    auto jsonParams = textDocumentPositionParamsToJson(params);
    
    sendRequest("textDocument/definition", jsonParams, [this, callback](std::shared_ptr<JsonValue> result) {
        if (callback) {
            std::vector<Location> locations;
            if (result) {
                if (result->getType() == JsonValue::Array) {
                    // Array of locations
                    for (size_t i = 0; i < result->getArraySize(); ++i) {
                        auto locJson = result->getArrayElement(i);
                        if (locJson) {
                            locations.push_back(jsonToLocation(locJson));
                        }
                    }
                } else if (result->getType() == JsonValue::Object) {
                    // Single location
                    locations.push_back(jsonToLocation(result));
                }
            }
            callback(locations);
        }
    });
}

void LspClient::references(const TextDocumentPositionParams& params,
                          std::function<void(const std::vector<Location>&)> callback) {
    // Create params with context (includeDeclaration)
    auto jsonParams = std::make_shared<JsonValue>(JsonValue::Object);
    jsonParams->setProperty("textDocument", textDocumentIdentifierToJson(params.textDocument));
    
    auto positionJson = std::make_shared<JsonValue>(JsonValue::Object);
    positionJson->setProperty("line", std::make_shared<JsonValue>(static_cast<double>(params.position.line)));
    positionJson->setProperty("character", std::make_shared<JsonValue>(static_cast<double>(params.position.character)));
    jsonParams->setProperty("position", positionJson);
    
    // Add context with includeDeclaration
    auto contextJson = std::make_shared<JsonValue>(JsonValue::Object);
    contextJson->setProperty("includeDeclaration", std::make_shared<JsonValue>(true));
    jsonParams->setProperty("context", contextJson);
    
    sendRequest("textDocument/references", jsonParams, [this, callback](std::shared_ptr<JsonValue> result) {
        if (callback) {
            std::vector<Location> locations;
            if (result && result->getType() == JsonValue::Array) {
                for (size_t i = 0; i < result->getArraySize(); ++i) {
                    auto locJson = result->getArrayElement(i);
                    if (locJson) {
                        locations.push_back(jsonToLocation(locJson));
                    }
                }
            }
            callback(locations);
        }
    });
}

void LspClient::documentSymbol(const TextDocumentIdentifier& document,
                              std::function<void(const std::vector<DocumentSymbol>&)> callback) {
    auto jsonParams = std::make_shared<JsonValue>(JsonValue::Object);
    jsonParams->setProperty("textDocument", textDocumentIdentifierToJson(document));
    
    sendRequest("textDocument/documentSymbol", jsonParams, [this, callback](std::shared_ptr<JsonValue> result) {
        if (callback) {
            std::vector<DocumentSymbol> symbols;
            if (result && result->getType() == JsonValue::Array) {
                for (size_t i = 0; i < result->getArraySize(); ++i) {
                    auto symJson = result->getArrayElement(i);
                    if (symJson) {
                        symbols.push_back(jsonToDocumentSymbol(symJson));
                    }
                }
            }
            callback(symbols);
        }
    });
}

void LspClient::formatting(const TextDocumentIdentifier& document, const FormattingOptions& options,
                          std::function<void(const std::vector<TextEdit>&)> callback) {
    auto jsonParams = std::make_shared<JsonValue>(JsonValue::Object);
    jsonParams->setProperty("textDocument", textDocumentIdentifierToJson(document));
    jsonParams->setProperty("options", formattingOptionsToJson(options));
    
    sendRequest("textDocument/formatting", jsonParams, [this, callback](std::shared_ptr<JsonValue> result) {
        if (callback) {
            std::vector<TextEdit> edits;
            if (result && result->getType() == JsonValue::Array) {
                for (size_t i = 0; i < result->getArraySize(); ++i) {
                    auto editJson = result->getArrayElement(i);
                    if (editJson) {
                        edits.push_back(jsonToTextEdit(editJson));
                    }
                }
            }
            callback(edits);
        }
    });
}

// Synchronous versions (simplified implementations)
CompletionList LspClient::completionSync(const TextDocumentPositionParams& params, int timeoutMs) {
    // In a real implementation, this would wait for the async response with timeout
    CompletionList result;
    return result;
}

std::optional<Hover> LspClient::hoverSync(const TextDocumentPositionParams& params, int timeoutMs) {
    // In a real implementation, this would wait for the async response with timeout
    return std::nullopt;
}

std::vector<Location> LspClient::definitionSync(const TextDocumentPositionParams& params, int timeoutMs) {
    return {};
}

std::vector<Location> LspClient::referencesSync(const TextDocumentPositionParams& params, int timeoutMs) {
    return {};
}

std::vector<DocumentSymbol> LspClient::documentSymbolSync(const TextDocumentIdentifier& document, int timeoutMs) {
    return {};
}

std::vector<TextEdit> LspClient::formattingSync(const TextDocumentIdentifier& document, const FormattingOptions& options, int timeoutMs) {
    return {};
}

void LspClient::messageProcessorFunc() {
    while (running_ && connection_) {
        if (connection_->hasMessage()) {
            std::string message = connection_->receiveMessage();
            if (!message.empty()) {
                // Debug: print received message
                // std::cerr << "[LSP] Received: " << message.substr(0, 200) << "...\n";
                
                // Process response
                auto json = JsonValue::fromString(message);
                if (json && json->getType() == JsonValue::Object) {
                    auto idProperty = json->getProperty("id");
                    std::string id;
                    if (idProperty) {
                        if (idProperty->getType() == JsonValue::String) {
                            id = idProperty->asString();
                        } else if (idProperty->getType() == JsonValue::Number) {
                            // Handle numeric id (some servers return id as number)
                            id = std::to_string(static_cast<int>(idProperty->asNumber()));
                        }
                    }
                    if (!id.empty()) {
                        
                        std::lock_guard<std::mutex> lock(requestMutex_);
                        auto it = pendingRequests_.find(id);
                        if (it != pendingRequests_.end()) {
                            auto resultProperty = json->getProperty("result");
                            it->second(resultProperty);
                            pendingRequests_.erase(it);
                        }
                    }
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::string LspClient::generateRequestId() {
    static int counter = 0;
    return std::to_string(++counter);
}

void LspClient::sendRequest(const std::string& method, std::shared_ptr<JsonValue> params, 
                           std::function<void(std::shared_ptr<JsonValue>)> callback) {
    if (!connection_) return;
    
    std::string id = generateRequestId();
    
    if (callback) {
        std::lock_guard<std::mutex> lock(requestMutex_);
        pendingRequests_[id] = callback;
    }
    
    std::string message = rpcHandler_.createRequest(method, params, id);
    connection_->sendMessage(message);
}

void LspClient::sendNotification(const std::string& method, std::shared_ptr<JsonValue> params) {
    if (!connection_) return;
    
    std::string message = rpcHandler_.createNotification(method, params);
    connection_->sendMessage(message);
}

// JSON conversion helpers (basic implementations)
std::shared_ptr<JsonValue> LspClient::textDocumentItemToJson(const TextDocumentItem& item) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("uri", std::make_shared<JsonValue>(item.uri));
    json->setProperty("languageId", std::make_shared<JsonValue>(item.languageId));
    json->setProperty("version", std::make_shared<JsonValue>(static_cast<double>(item.version)));
    json->setProperty("text", std::make_shared<JsonValue>(item.text));
    return json;
}

std::shared_ptr<JsonValue> LspClient::textDocumentIdentifierToJson(const TextDocumentIdentifier& identifier) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("uri", std::make_shared<JsonValue>(identifier.uri));
    return json;
}

std::shared_ptr<JsonValue> LspClient::versionedTextDocumentIdentifierToJson(const VersionedTextDocumentIdentifier& identifier) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("uri", std::make_shared<JsonValue>(identifier.uri));
    json->setProperty("version", std::make_shared<JsonValue>(static_cast<double>(identifier.version)));
    return json;
}

std::shared_ptr<JsonValue> LspClient::textDocumentPositionParamsToJson(const TextDocumentPositionParams& params) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("textDocument", textDocumentIdentifierToJson(params.textDocument));
    
    auto position = std::make_shared<JsonValue>();
    position->setObject();
    position->setProperty("line", std::make_shared<JsonValue>(static_cast<double>(params.position.line)));
    position->setProperty("character", std::make_shared<JsonValue>(static_cast<double>(params.position.character)));
    json->setProperty("position", position);
    
    return json;
}

std::shared_ptr<JsonValue> LspClient::formattingOptionsToJson(const FormattingOptions& options) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("tabSize", std::make_shared<JsonValue>(static_cast<double>(options.tabSize)));
    json->setProperty("insertSpaces", std::make_shared<JsonValue>(options.insertSpaces));
    return json;
}

CompletionList LspClient::jsonToCompletionList(std::shared_ptr<JsonValue> json) {
    CompletionList result;
    
    if (json && json->getType() == JsonValue::Object) {
        auto items = json->getProperty("items");
        if (items && items->getType() == JsonValue::Array) {
            for (size_t i = 0; i < items->getArraySize(); ++i) {
                auto item = items->getArrayElement(i);
                if (item && item->getType() == JsonValue::Object) {
                    auto label = item->getProperty("label");
                    if (label && label->getType() == JsonValue::String) {
                        CompletionItem completionItem(label->asString());
                        result.items.push_back(completionItem);
                    }
                }
            }
        }
    }
    
    return result;
}

Hover LspClient::jsonToHover(std::shared_ptr<JsonValue> json) {
    Hover result;
    
    if (json && json->getType() == JsonValue::Object) {
        auto contents = json->getProperty("contents");
        if (contents && contents->getType() == JsonValue::String) {
            result.contents = contents->asString();
        }
    }
    
    return result;
}

Position LspClient::jsonToPosition(std::shared_ptr<JsonValue> json) {
    Position pos;
    if (json && json->getType() == JsonValue::Object) {
        auto line = json->getProperty("line");
        auto character = json->getProperty("character");
        if (line && line->getType() == JsonValue::Number) {
            pos.line = static_cast<size_t>(line->asNumber());
        }
        if (character && character->getType() == JsonValue::Number) {
            pos.character = static_cast<size_t>(character->asNumber());
        }
    }
    return pos;
}

Range LspClient::jsonToRange(std::shared_ptr<JsonValue> json) {
    Range range;
    if (json && json->getType() == JsonValue::Object) {
        auto start = json->getProperty("start");
        auto end = json->getProperty("end");
        if (start) {
            range.start = jsonToPosition(start);
        }
        if (end) {
            range.end = jsonToPosition(end);
        }
    }
    return range;
}

Location LspClient::jsonToLocation(std::shared_ptr<JsonValue> json) {
    Location loc;
    if (json && json->getType() == JsonValue::Object) {
        auto uri = json->getProperty("uri");
        auto range = json->getProperty("range");
        if (uri && uri->getType() == JsonValue::String) {
            loc.uri = uri->asString();
        }
        if (range) {
            loc.range = jsonToRange(range);
        }
    }
    return loc;
}

DocumentSymbol LspClient::jsonToDocumentSymbol(std::shared_ptr<JsonValue> json) {
    DocumentSymbol symbol;
    if (json && json->getType() == JsonValue::Object) {
        auto name = json->getProperty("name");
        auto detail = json->getProperty("detail");
        auto kind = json->getProperty("kind");
        auto range = json->getProperty("range");
        auto selectionRange = json->getProperty("selectionRange");
        auto children = json->getProperty("children");
        
        if (name && name->getType() == JsonValue::String) {
            symbol.name = name->asString();
        }
        if (detail && detail->getType() == JsonValue::String) {
            symbol.detail = detail->asString();
        }
        if (kind && kind->getType() == JsonValue::Number) {
            symbol.kind = static_cast<SymbolKind>(static_cast<int>(kind->asNumber()));
        }
        if (range) {
            symbol.range = jsonToRange(range);
        }
        if (selectionRange) {
            symbol.selectionRange = jsonToRange(selectionRange);
        }
        if (children && children->getType() == JsonValue::Array) {
            for (size_t i = 0; i < children->getArraySize(); ++i) {
                auto childJson = children->getArrayElement(i);
                if (childJson) {
                    symbol.children.push_back(jsonToDocumentSymbol(childJson));
                }
            }
        }
    }
    return symbol;
}

TextEdit LspClient::jsonToTextEdit(std::shared_ptr<JsonValue> json) {
    TextEdit edit;
    if (json && json->getType() == JsonValue::Object) {
        auto range = json->getProperty("range");
        auto newText = json->getProperty("newText");
        if (range) {
            edit.range = jsonToRange(range);
        }
        if (newText && newText->getType() == JsonValue::String) {
            edit.newText = newText->asString();
        }
    }
    return edit;
}

// LspClientManager implementation
LspClientManager::~LspClientManager() {
    shutdown();
}

void LspClientManager::registerLanguageServer(const std::string& languageId, const LspClientConfig& config) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    languageConfigs_[languageId] = config;
    
    // Map file extensions to language IDs
    for (const auto& ext : config.fileExtensions) {
        fileExtensionToLanguage_[ext] = languageId;
    }
}

void LspClientManager::unregisterLanguageServer(const std::string& languageId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    // Remove client
    clients_.erase(languageId);
    
    // Remove language config
    languageConfigs_.erase(languageId);
    
    // Remove file extension mappings
    for (auto it = fileExtensionToLanguage_.begin(); it != fileExtensionToLanguage_.end();) {
        if (it->second == languageId) {
            it = fileExtensionToLanguage_.erase(it);
        } else {
            ++it;
        }
    }
}

LspClient* LspClientManager::getClientForFile(const std::string& filePath) {
    std::string languageId = getLanguageIdFromFilePath(filePath);
    return getClientForLanguage(languageId);
}

LspClient* LspClientManager::getClientForLanguage(const std::string& languageId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    auto it = clients_.find(languageId);
    if (it != clients_.end()) {
        return it->second.get();
    }
    
    // Try to create a new client
    return createClientForLanguage(languageId);
}

std::vector<std::string> LspClientManager::getRegisteredLanguages() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    std::vector<std::string> languages;
    for (const auto& config : languageConfigs_) {
        languages.push_back(config.first);
    }
    return languages;
}

void LspClientManager::didOpenTextDocument(const std::string& filePath, const std::string& content, const std::string& languageId) {
    LspClient* client = getClientForFile(filePath);
    if (client) {
        TextDocumentItem item;
        item.uri = filePathToUri(filePath);
        item.languageId = languageId.empty() ? getLanguageIdFromFilePath(filePath) : languageId;
        item.version = 1;
        item.text = content;
        
        client->didOpenTextDocument(item);
    }
}

void LspClientManager::didChangeTextDocument(const std::string& filePath, const std::string& content, int version) {
    LspClient* client = getClientForFile(filePath);
    if (client) {
        VersionedTextDocumentIdentifier identifier;
        identifier.uri = filePathToUri(filePath);
        identifier.version = version;
        
        TextDocumentContentChangeEvent change;
        change.text = content;
        
        client->didChangeTextDocument(identifier, {change});
    }
}

void LspClientManager::didCloseTextDocument(const std::string& filePath) {
    LspClient* client = getClientForFile(filePath);
    if (client) {
        TextDocumentIdentifier identifier;
        identifier.uri = filePathToUri(filePath);
        
        client->didCloseTextDocument(identifier);
    }
}

void LspClientManager::didSaveTextDocument(const std::string& filePath) {
    LspClient* client = getClientForFile(filePath);
    if (client) {
        TextDocumentIdentifier identifier;
        identifier.uri = filePathToUri(filePath);
        
        client->didSaveTextDocument(identifier);
    }
}

void LspClientManager::completion(const std::string& filePath, size_t line, size_t character, 
                                 std::function<void(const CompletionList&)> callback) {
    LspClient* client = getClientForFile(filePath);
    if (client) {
        TextDocumentPositionParams params;
        params.textDocument.uri = filePathToUri(filePath);
        params.position = Position(line, character);
        
        client->completion(params, callback);
    } else if (callback) {
        callback(CompletionList());
    }
}

void LspClientManager::hover(const std::string& filePath, size_t line, size_t character, 
                            std::function<void(const std::optional<Hover>&)> callback) {
    LspClient* client = getClientForFile(filePath);
    if (client) {
        TextDocumentPositionParams params;
        params.textDocument.uri = filePathToUri(filePath);
        params.position = Position(line, character);
        
        client->hover(params, callback);
    } else if (callback) {
        callback(std::nullopt);
    }
}

void LspClientManager::definition(const std::string& filePath, size_t line, size_t character, 
                                 std::function<void(const std::vector<Location>&)> callback) {
    // Implementation would go here
    if (callback) {
        callback({});
    }
}

void LspClientManager::shutdown() {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    
    for (auto& client : clients_) {
        client.second->disconnect();
    }
    
    clients_.clear();
    languageConfigs_.clear();
    fileExtensionToLanguage_.clear();
}

std::string LspClientManager::getLanguageIdFromFilePath(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "text";
    }
    
    std::string extension = filePath.substr(dotPos);
    auto it = fileExtensionToLanguage_.find(extension);
    if (it != fileExtensionToLanguage_.end()) {
        return it->second;
    }
    
    return "text";
}

std::string LspClientManager::filePathToUri(const std::string& filePath) {
    return "file://" + filePath;
}

LspClient* LspClientManager::createClientForLanguage(const std::string& languageId) {
    auto configIt = languageConfigs_.find(languageId);
    if (configIt == languageConfigs_.end()) {
        return nullptr;
    }
    
    auto connection = std::make_unique<ProcessLspConnection>();
    auto client = std::make_unique<LspClient>(std::move(connection));
    
    if (!client->connect(configIt->second)) {
        return nullptr;
    }
    
    // Initialize client
    client->initialize();
    
    LspClient* clientPtr = client.get();
    clients_[languageId] = std::move(client);
    
    return clientPtr;
}

} // namespace lsp
} // namespace bolt