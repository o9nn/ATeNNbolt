#include "bolt/lsp/lsp_client.hpp"
#include <iostream>
#include <sstream>
#include <json/json.h>

namespace bolt {
namespace lsp {

LSPClient::LSPClient(const std::string& server_command)
    : server_command_(server_command), initialized_(false), next_request_id_(1) {
}

LSPClient::~LSPClient() {
    shutdown();
}

bool LSPClient::initialize(const std::string& root_uri) {
    if (initialized_) {
        return true;
    }
    
    // Start LSP server process
    if (!startServer()) {
        std::cerr << "Failed to start LSP server" << std::endl;
        return false;
    }
    
    // Send initialize request
    Json::Value params;
    params["processId"] = static_cast<int>(getpid());
    params["rootUri"] = root_uri;
    params["capabilities"] = buildClientCapabilities();
    
    auto response = sendRequest("initialize", params);
    if (response.isNull()) {
        std::cerr << "Initialize request failed" << std::endl;
        return false;
    }
    
    // Send initialized notification
    sendNotification("initialized", Json::Value());
    
    initialized_ = true;
    return true;
}

void LSPClient::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Send shutdown request
    sendRequest("shutdown", Json::Value());
    
    // Send exit notification
    sendNotification("exit", Json::Value());
    
    // Stop server process
    stopServer();
    
    initialized_ = false;
}

void LSPClient::didOpen(const std::string& uri, const std::string& language_id, const std::string& text) {
    Json::Value params;
    params["textDocument"]["uri"] = uri;
    params["textDocument"]["languageId"] = language_id;
    params["textDocument"]["version"] = 1;
    params["textDocument"]["text"] = text;
    
    sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const std::string& uri, int version, const std::vector<TextDocumentContentChangeEvent>& changes) {
    Json::Value params;
    params["textDocument"]["uri"] = uri;
    params["textDocument"]["version"] = version;
    
    Json::Value changesArray(Json::arrayValue);
    for (const auto& change : changes) {
        Json::Value changeJson;
        if (change.range.has_value()) {
            const auto& range = change.range.value();
            changeJson["range"]["start"]["line"] = range.start.line;
            changeJson["range"]["start"]["character"] = range.start.character;
            changeJson["range"]["end"]["line"] = range.end.line;
            changeJson["range"]["end"]["character"] = range.end.character;
        }
        changeJson["text"] = change.text;
        changesArray.append(changeJson);
    }
    params["contentChanges"] = changesArray;
    
    sendNotification("textDocument/didChange", params);
}

void LSPClient::didClose(const std::string& uri) {
    Json::Value params;
    params["textDocument"]["uri"] = uri;
    
    sendNotification("textDocument/didClose", params);
}

std::vector<CompletionItem> LSPClient::completion(const std::string& uri, int line, int character) {
    Json::Value params;
    params["textDocument"]["uri"] = uri;
    params["position"]["line"] = line;
    params["position"]["character"] = character;
    
    auto response = sendRequest("textDocument/completion", params);
    
    std::vector<CompletionItem> items;
    if (response.isNull()) {
        return items;
    }
    
    // Parse completion items
    Json::Value itemsArray = response.isArray() ? response : response["items"];
    for (const auto& item : itemsArray) {
        CompletionItem completion;
        completion.label = item["label"].asString();
        completion.kind = item.get("kind", 1).asInt();
        completion.detail = item.get("detail", "").asString();
        completion.documentation = item.get("documentation", "").asString();
        completion.insertText = item.get("insertText", completion.label).asString();
        items.push_back(completion);
    }
    
    return items;
}

std::optional<Location> LSPClient::gotoDefinition(const std::string& uri, int line, int character) {
    Json::Value params;
    params["textDocument"]["uri"] = uri;
    params["position"]["line"] = line;
    params["position"]["character"] = character;
    
    auto response = sendRequest("textDocument/definition", params);
    
    if (response.isNull()) {
        return std::nullopt;
    }
    
    // Handle both single location and array of locations
    Json::Value locationJson = response.isArray() ? response[0] : response;
    
    Location location;
    location.uri = locationJson["uri"].asString();
    location.range.start.line = locationJson["range"]["start"]["line"].asInt();
    location.range.start.character = locationJson["range"]["start"]["character"].asInt();
    location.range.end.line = locationJson["range"]["end"]["line"].asInt();
    location.range.end.character = locationJson["range"]["end"]["character"].asInt();
    
    return location;
}

std::vector<Location> LSPClient::findReferences(const std::string& uri, int line, int character, bool include_declaration) {
    Json::Value params;
    params["textDocument"]["uri"] = uri;
    params["position"]["line"] = line;
    params["position"]["character"] = character;
    params["context"]["includeDeclaration"] = include_declaration;
    
    auto response = sendRequest("textDocument/references", params);
    
    std::vector<Location> locations;
    if (response.isNull() || !response.isArray()) {
        return locations;
    }
    
    for (const auto& locationJson : response) {
        Location location;
        location.uri = locationJson["uri"].asString();
        location.range.start.line = locationJson["range"]["start"]["line"].asInt();
        location.range.start.character = locationJson["range"]["start"]["character"].asInt();
        location.range.end.line = locationJson["range"]["end"]["line"].asInt();
        location.range.end.character = locationJson["range"]["end"]["character"].asInt();
        locations.push_back(location);
    }
    
    return locations;
}

std::optional<Hover> LSPClient::hover(const std::string& uri, int line, int character) {
    Json::Value params;
    params["textDocument"]["uri"] = uri;
    params["position"]["line"] = line;
    params["position"]["character"] = character;
    
    auto response = sendRequest("textDocument/hover", params);
    
    if (response.isNull()) {
        return std::nullopt;
    }
    
    Hover hover;
    
    // Parse contents
    Json::Value contents = response["contents"];
    if (contents.isString()) {
        hover.contents = contents.asString();
    } else if (contents.isObject() && contents.isMember("value")) {
        hover.contents = contents["value"].asString();
    } else if (contents.isArray() && contents.size() > 0) {
        hover.contents = contents[0].asString();
    }
    
    // Parse range if present
    if (response.isMember("range")) {
        Range range;
        range.start.line = response["range"]["start"]["line"].asInt();
        range.start.character = response["range"]["start"]["character"].asInt();
        range.end.line = response["range"]["end"]["line"].asInt();
        range.end.character = response["range"]["end"]["character"].asInt();
        hover.range = range;
    }
    
    return hover;
}

std::vector<Diagnostic> LSPClient::getDiagnostics(const std::string& uri) {
    std::lock_guard<std::mutex> lock(diagnostics_mutex_);
    auto it = diagnostics_.find(uri);
    if (it != diagnostics_.end()) {
        return it->second;
    }
    return {};
}

Json::Value LSPClient::sendRequest(const std::string& method, const Json::Value& params) {
    int request_id = next_request_id_++;
    
    Json::Value request;
    request["jsonrpc"] = "2.0";
    request["id"] = request_id;
    request["method"] = method;
    request["params"] = params;
    
    std::string message = jsonToString(request);
    
    if (!sendMessage(message)) {
        return Json::Value();
    }
    
    // Wait for response
    return waitForResponse(request_id);
}

void LSPClient::sendNotification(const std::string& method, const Json::Value& params) {
    Json::Value notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;
    notification["params"] = params;
    
    std::string message = jsonToString(notification);
    sendMessage(message);
}

bool LSPClient::startServer() {
    // In a real implementation, this would:
    // 1. Fork and exec the LSP server process
    // 2. Set up pipes for stdin/stdout communication
    // 3. Start reader thread for server responses
    
    std::cout << "Starting LSP server: " << server_command_ << std::endl;
    
    // Placeholder: would use fork/exec or platform-specific process creation
    // For now, just indicate success
    
    return true;
}

void LSPClient::stopServer() {
    // In a real implementation, this would:
    // 1. Close pipes
    // 2. Wait for server process to exit
    // 3. Kill server if it doesn't exit gracefully
    
    std::cout << "Stopping LSP server" << std::endl;
}

bool LSPClient::sendMessage(const std::string& message) {
    // Format: Content-Length: <length>\r\n\r\n<content>
    std::string header = "Content-Length: " + std::to_string(message.length()) + "\r\n\r\n";
    std::string full_message = header + message;
    
    // In a real implementation, write to server's stdin
    // For now, just log
    std::cout << "Sending LSP message: " << method << std::endl;
    
    return true;
}

Json::Value LSPClient::waitForResponse(int request_id) {
    // In a real implementation, this would:
    // 1. Wait for response with matching ID
    // 2. Handle timeout
    // 3. Return parsed response
    
    // Placeholder: return empty response
    return Json::Value();
}

void LSPClient::handleNotification(const Json::Value& notification) {
    std::string method = notification["method"].asString();
    
    if (method == "textDocument/publishDiagnostics") {
        handleDiagnostics(notification["params"]);
    }
    // Handle other notifications as needed
}

void LSPClient::handleDiagnostics(const Json::Value& params) {
    std::string uri = params["uri"].asString();
    
    std::vector<Diagnostic> diagnostics;
    for (const auto& diagJson : params["diagnostics"]) {
        Diagnostic diag;
        diag.range.start.line = diagJson["range"]["start"]["line"].asInt();
        diag.range.start.character = diagJson["range"]["start"]["character"].asInt();
        diag.range.end.line = diagJson["range"]["end"]["line"].asInt();
        diag.range.end.character = diagJson["range"]["end"]["character"].asInt();
        diag.severity = diagJson.get("severity", 1).asInt();
        diag.message = diagJson["message"].asString();
        diag.source = diagJson.get("source", "").asString();
        diagnostics.push_back(diag);
    }
    
    std::lock_guard<std::mutex> lock(diagnostics_mutex_);
    diagnostics_[uri] = diagnostics;
}

Json::Value LSPClient::buildClientCapabilities() {
    Json::Value capabilities;
    
    // Text document capabilities
    capabilities["textDocument"]["completion"]["completionItem"]["snippetSupport"] = true;
    capabilities["textDocument"]["completion"]["completionItem"]["commitCharactersSupport"] = true;
    capabilities["textDocument"]["hover"]["contentFormat"] = Json::arrayValue;
    capabilities["textDocument"]["hover"]["contentFormat"].append("markdown");
    capabilities["textDocument"]["hover"]["contentFormat"].append("plaintext");
    capabilities["textDocument"]["synchronization"]["dynamicRegistration"] = true;
    capabilities["textDocument"]["synchronization"]["willSave"] = true;
    capabilities["textDocument"]["synchronization"]["didSave"] = true;
    
    return capabilities;
}

std::string LSPClient::jsonToString(const Json::Value& json) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

} // namespace lsp
} // namespace bolt
