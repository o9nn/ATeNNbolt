#include "bolt/editor/lsp_server.hpp"
#include "bolt/editor/lsp_protocol.hpp"
#include <algorithm>
#include <sstream>

namespace bolt {
namespace lsp {

LanguageServer::LanguageServer() {
    setupMessageHandlers();
}

void LanguageServer::initialize(const InitializeParams& params) {
    rootPath_ = params.rootPath;
    capabilities_ = params.capabilities;
    initialized_ = true;
}

void LanguageServer::shutdown() {
    initialized_ = false;
    documents_.clear();
    languagePlugins_.clear();
}

void LanguageServer::didOpenTextDocument(const TextDocumentItem& document) {
    documents_[document.uri] = document;
    
    // Notify language plugin
    auto plugin = getLanguagePlugin(document.uri);
    if (plugin) {
        std::string filePath = uriToFilePath(document.uri);
        plugin->onDocumentOpened(filePath, document.text);
    }
}

void LanguageServer::didChangeTextDocument(const VersionedTextDocumentIdentifier& document, 
                                          const std::vector<TextDocumentContentChangeEvent>& changes) {
    auto it = documents_.find(document.uri);
    if (it != documents_.end()) {
        // Apply changes to document
        for (const auto& change : changes) {
            if (change.range.has_value()) {
                // Incremental change - in a real implementation, apply the range-based change
                // For simplicity, we'll treat it as a full document change
                it->second.text = change.text;
            } else {
                // Full document change
                it->second.text = change.text;
            }
        }
        it->second.version = document.version;
        
        // Notify language plugin
        auto plugin = getLanguagePlugin(document.uri);
        if (plugin) {
            std::string filePath = uriToFilePath(document.uri);
            plugin->onDocumentModified(filePath, it->second.text);
        }
    }
}

void LanguageServer::didCloseTextDocument(const TextDocumentIdentifier& document) {
    auto it = documents_.find(document.uri);
    if (it != documents_.end()) {
        // Notify language plugin
        auto plugin = getLanguagePlugin(document.uri);
        if (plugin) {
            std::string filePath = uriToFilePath(document.uri);
            plugin->onDocumentClosed(filePath);
        }
        
        documents_.erase(it);
    }
}

void LanguageServer::didSaveTextDocument(const TextDocumentIdentifier& document) {
    // Handle save event if needed
}

CompletionList LanguageServer::completion(const TextDocumentPositionParams& params) {
    CompletionList result;
    
    auto plugin = getLanguagePlugin(params.textDocument.uri);
    if (plugin) {
        std::string filePath = uriToFilePath(params.textDocument.uri);
        auto completions = plugin->getCodeCompletions(filePath, params.position.line, params.position.character);
        
        for (const auto& completion : completions) {
            CompletionItem item(completion);
            item.kind = CompletionItemKind::Text;
            result.items.push_back(item);
        }
    }
    
    return result;
}

std::optional<Hover> LanguageServer::hover(const TextDocumentPositionParams& params) {
    auto plugin = getLanguagePlugin(params.textDocument.uri);
    if (plugin) {
        std::string filePath = uriToFilePath(params.textDocument.uri);
        std::string hoverInfo = plugin->getHoverInfo(filePath, params.position.line, params.position.character);
        
        if (!hoverInfo.empty()) {
            return Hover(hoverInfo);
        }
    }
    
    return std::nullopt;
}

std::vector<Location> LanguageServer::definition(const TextDocumentPositionParams& params) {
    // Basic implementation - in a real LSP server, this would find actual definitions
    return {};
}

std::vector<Location> LanguageServer::references(const TextDocumentPositionParams& params) {
    std::vector<Location> result;
    
    auto plugin = getLanguagePlugin(params.textDocument.uri);
    if (plugin) {
        std::string filePath = uriToFilePath(params.textDocument.uri);
        auto refs = plugin->findReferences(filePath, params.position.line, params.position.character);
        
        for (const auto& ref : refs) {
            Location location;
            location.uri = params.textDocument.uri;
            location.range = Range(Position(ref.first, 0), Position(ref.first, ref.second));
            result.push_back(location);
        }
    }
    
    return result;
}

std::vector<DocumentSymbol> LanguageServer::documentSymbol(const TextDocumentIdentifier& document) {
    // Basic implementation - would normally parse document for symbols
    return {};
}

std::vector<TextEdit> LanguageServer::formatting(const TextDocumentIdentifier& document, const FormattingOptions& options) {
    std::vector<TextEdit> result;
    
    auto it = documents_.find(document.uri);
    if (it != documents_.end()) {
        auto plugin = getLanguagePlugin(document.uri);
        if (plugin) {
            std::string formatted = plugin->formatCode(it->second.text);
            if (formatted != it->second.text) {
                TextEdit edit;
                edit.range = Range(Position(0, 0), Position(SIZE_MAX, 0)); // Full document
                edit.newText = formatted;
                result.push_back(edit);
            }
        }
    }
    
    return result;
}

std::vector<Diagnostic> LanguageServer::diagnostics(const TextDocumentIdentifier& document) {
    std::vector<Diagnostic> result;
    
    auto it = documents_.find(document.uri);
    if (it != documents_.end()) {
        auto plugin = getLanguagePlugin(document.uri);
        if (plugin) {
            std::string filePath = uriToFilePath(document.uri);
            auto diagnosticStrings = plugin->getDiagnostics(filePath, it->second.text);
            
            for (const auto& diagStr : diagnosticStrings) {
                Diagnostic diag;
                diag.message = diagStr;
                diag.severity = DiagnosticSeverity::Error;
                diag.range = Range(Position(0, 0), Position(0, 1)); // Default range
                result.push_back(diag);
            }
        }
    }
    
    return result;
}

void LanguageServer::registerLanguagePlugin(const std::string& languageId, std::shared_ptr<ILanguagePlugin> plugin) {
    languagePlugins_[languageId] = plugin;
}

void LanguageServer::unregisterLanguagePlugin(const std::string& languageId) {
    languagePlugins_.erase(languageId);
}

std::shared_ptr<ILanguagePlugin> LanguageServer::getLanguagePlugin(const std::string& uri) {
    std::string languageId = getLanguageIdFromUri(uri);
    auto it = languagePlugins_.find(languageId);
    return it != languagePlugins_.end() ? it->second : nullptr;
}

std::string LanguageServer::processMessage(const std::string& message) {
    return rpcHandler_.processMessage(message);
}

void LanguageServer::setupMessageHandlers() {
    // Request handlers
    rpcHandler_.registerRequestHandler("initialize", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            return handleInitialize(method, params);
        });
    
    rpcHandler_.registerRequestHandler("textDocument/completion", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            return handleTextDocumentCompletion(method, params);
        });
    
    rpcHandler_.registerRequestHandler("textDocument/hover", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            return handleTextDocumentHover(method, params);
        });
    
    rpcHandler_.registerRequestHandler("textDocument/definition", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            return handleTextDocumentDefinition(method, params);
        });
    
    rpcHandler_.registerRequestHandler("textDocument/references", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            return handleTextDocumentReferences(method, params);
        });
    
    rpcHandler_.registerRequestHandler("textDocument/documentSymbol", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            return handleTextDocumentDocumentSymbol(method, params);
        });
    
    rpcHandler_.registerRequestHandler("textDocument/formatting", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            return handleTextDocumentFormatting(method, params);
        });
    
    // Notification handlers
    rpcHandler_.registerNotificationHandler("textDocument/didOpen", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            handleTextDocumentDidOpen(method, params);
        });
    
    rpcHandler_.registerNotificationHandler("textDocument/didChange", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            handleTextDocumentDidChange(method, params);
        });
    
    rpcHandler_.registerNotificationHandler("textDocument/didClose", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            handleTextDocumentDidClose(method, params);
        });
    
    rpcHandler_.registerNotificationHandler("textDocument/didSave", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            handleTextDocumentDidSave(method, params);
        });
    
    rpcHandler_.registerNotificationHandler("initialized", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            handleInitialized(method, params);
        });
    
    rpcHandler_.registerNotificationHandler("shutdown", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            handleShutdown(method, params);
        });
    
    rpcHandler_.registerNotificationHandler("exit", 
        [this](const std::string& method, std::shared_ptr<JsonValue> params) {
            handleExit(method, params);
        });
}

std::string LanguageServer::getLanguageIdFromUri(const std::string& uri) {
    std::string filePath = uriToFilePath(uri);
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = filePath.substr(dotPos);
        if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".c") {
            return "cpp";
        } else if (ext == ".py") {
            return "python";
        } else if (ext == ".js" || ext == ".ts") {
            return "javascript";
        }
    }
    return "text";
}

std::string LanguageServer::uriToFilePath(const std::string& uri) {
    // Simple file:// URI to path conversion
    if (uri.substr(0, 7) == "file://") {
        return uri.substr(7);
    }
    return uri;
}

std::string LanguageServer::filePathToUri(const std::string& filePath) {
    return "file://" + filePath;
}

// JSON-RPC handler implementations (simplified)
std::shared_ptr<JsonValue> LanguageServer::handleInitialize(const std::string& method, std::shared_ptr<JsonValue> params) {
    auto result = std::make_shared<JsonValue>();
    result->setObject();
    
    auto capabilities = std::make_shared<JsonValue>();
    capabilities->setObject();
    capabilities->setProperty("textDocumentSync", std::make_shared<JsonValue>(1.0)); // Full sync
    capabilities->setProperty("hoverProvider", std::make_shared<JsonValue>(true));
    capabilities->setProperty("completionProvider", std::make_shared<JsonValue>(true));
    capabilities->setProperty("definitionProvider", std::make_shared<JsonValue>(true));
    capabilities->setProperty("referencesProvider", std::make_shared<JsonValue>(true));
    capabilities->setProperty("documentFormattingProvider", std::make_shared<JsonValue>(true));
    
    result->setProperty("capabilities", capabilities);
    
    initialized_ = true;
    return result;
}

std::shared_ptr<JsonValue> LanguageServer::handleTextDocumentCompletion(const std::string& method, std::shared_ptr<JsonValue> params) {
    if (!params) return std::make_shared<JsonValue>();
    
    auto textDocParams = jsonToTextDocumentPositionParams(params);
    auto completionList = completion(textDocParams);
    return completionListToJson(completionList);
}

std::shared_ptr<JsonValue> LanguageServer::handleTextDocumentHover(const std::string& method, std::shared_ptr<JsonValue> params) {
    if (!params) return std::make_shared<JsonValue>();
    
    auto textDocParams = jsonToTextDocumentPositionParams(params);
    auto hoverResult = hover(textDocParams);
    
    if (hoverResult.has_value()) {
        return hoverToJson(hoverResult.value());
    }
    
    return std::make_shared<JsonValue>(); // null
}

std::shared_ptr<JsonValue> LanguageServer::handleTextDocumentDefinition(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would go here
    return std::make_shared<JsonValue>();
}

std::shared_ptr<JsonValue> LanguageServer::handleTextDocumentReferences(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would go here
    return std::make_shared<JsonValue>();
}

std::shared_ptr<JsonValue> LanguageServer::handleTextDocumentDocumentSymbol(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would go here
    return std::make_shared<JsonValue>();
}

std::shared_ptr<JsonValue> LanguageServer::handleTextDocumentFormatting(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would go here
    return std::make_shared<JsonValue>();
}

// Notification handlers
void LanguageServer::handleTextDocumentDidOpen(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would parse params and call didOpenTextDocument
}

void LanguageServer::handleTextDocumentDidChange(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would parse params and call didChangeTextDocument
}

void LanguageServer::handleTextDocumentDidClose(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would parse params and call didCloseTextDocument
}

void LanguageServer::handleTextDocumentDidSave(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Implementation would parse params and call didSaveTextDocument
}

void LanguageServer::handleInitialized(const std::string& method, std::shared_ptr<JsonValue> params) {
    // Server is now initialized
}

void LanguageServer::handleShutdown(const std::string& method, std::shared_ptr<JsonValue> params) {
    shutdown();
}

void LanguageServer::handleExit(const std::string& method, std::shared_ptr<JsonValue> params) {
    shutdown();
}

// JSON conversion helpers (basic implementations)
Position LanguageServer::jsonToPosition(std::shared_ptr<JsonValue> json) {
    if (!json || json->getType() != JsonValue::Object) return Position();
    
    auto line = json->getProperty("line");
    auto character = json->getProperty("character");
    
    return Position(
        line && line->getType() == JsonValue::Number ? static_cast<size_t>(line->asNumber()) : 0,
        character && character->getType() == JsonValue::Number ? static_cast<size_t>(character->asNumber()) : 0
    );
}

Range LanguageServer::jsonToRange(std::shared_ptr<JsonValue> json) {
    if (!json || json->getType() != JsonValue::Object) return Range();
    
    auto start = json->getProperty("start");
    auto end = json->getProperty("end");
    
    return Range(
        jsonToPosition(start),
        jsonToPosition(end)
    );
}

TextDocumentIdentifier LanguageServer::jsonToTextDocumentIdentifier(std::shared_ptr<JsonValue> json) {
    TextDocumentIdentifier identifier;
    if (json && json->getType() == JsonValue::Object) {
        auto uri = json->getProperty("uri");
        if (uri && uri->getType() == JsonValue::String) {
            identifier.uri = uri->asString();
        }
    }
    return identifier;
}

TextDocumentPositionParams LanguageServer::jsonToTextDocumentPositionParams(std::shared_ptr<JsonValue> json) {
    TextDocumentPositionParams params;
    if (json && json->getType() == JsonValue::Object) {
        auto textDocument = json->getProperty("textDocument");
        auto position = json->getProperty("position");
        
        params.textDocument = jsonToTextDocumentIdentifier(textDocument);
        params.position = jsonToPosition(position);
    }
    return params;
}

std::shared_ptr<JsonValue> LanguageServer::positionToJson(const Position& pos) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("line", std::make_shared<JsonValue>(static_cast<double>(pos.line)));
    json->setProperty("character", std::make_shared<JsonValue>(static_cast<double>(pos.character)));
    return json;
}

std::shared_ptr<JsonValue> LanguageServer::rangeToJson(const Range& range) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("start", positionToJson(range.start));
    json->setProperty("end", positionToJson(range.end));
    return json;
}

std::shared_ptr<JsonValue> LanguageServer::completionItemToJson(const CompletionItem& item) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("label", std::make_shared<JsonValue>(item.label));
    
    if (item.kind.has_value()) {
        json->setProperty("kind", std::make_shared<JsonValue>(static_cast<double>(static_cast<int>(item.kind.value()))));
    }
    
    if (item.detail.has_value()) {
        json->setProperty("detail", std::make_shared<JsonValue>(item.detail.value()));
    }
    
    if (item.documentation.has_value()) {
        json->setProperty("documentation", std::make_shared<JsonValue>(item.documentation.value()));
    }
    
    return json;
}

std::shared_ptr<JsonValue> LanguageServer::completionListToJson(const CompletionList& list) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("isIncomplete", std::make_shared<JsonValue>(list.isIncomplete));
    
    auto items = std::make_shared<JsonValue>();
    items->setArray();
    for (const auto& item : list.items) {
        items->addArrayElement(completionItemToJson(item));
    }
    json->setProperty("items", items);
    
    return json;
}

std::shared_ptr<JsonValue> LanguageServer::hoverToJson(const Hover& hover) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("contents", std::make_shared<JsonValue>(hover.contents));
    
    if (hover.range.has_value()) {
        json->setProperty("range", rangeToJson(hover.range.value()));
    }
    
    return json;
}

std::shared_ptr<JsonValue> LanguageServer::locationToJson(const Location& location) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("uri", std::make_shared<JsonValue>(location.uri));
    json->setProperty("range", rangeToJson(location.range));
    return json;
}

std::shared_ptr<JsonValue> LanguageServer::diagnosticToJson(const Diagnostic& diagnostic) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("range", rangeToJson(diagnostic.range));
    json->setProperty("message", std::make_shared<JsonValue>(diagnostic.message));
    
    if (diagnostic.severity.has_value()) {
        json->setProperty("severity", std::make_shared<JsonValue>(static_cast<double>(static_cast<int>(diagnostic.severity.value()))));
    }
    
    return json;
}

std::shared_ptr<JsonValue> LanguageServer::textEditToJson(const TextEdit& edit) {
    auto json = std::make_shared<JsonValue>();
    json->setObject();
    json->setProperty("range", rangeToJson(edit.range));
    json->setProperty("newText", std::make_shared<JsonValue>(edit.newText));
    return json;
}

} // namespace lsp
} // namespace bolt