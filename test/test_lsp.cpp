/**
 * @file test_lsp.cpp
 * @brief Unit tests for LSP (Language Server Protocol) components
 */

#include <bolt/test_framework.hpp>
#include <bolt/lsp/lsp_client.hpp>
#include <bolt/lsp/lsp_server.hpp>
#include <bolt/lsp/lsp_types.hpp>
#include <bolt/editor/lsp_json_rpc.hpp>

#include <string>
#include <vector>
#include <memory>

using namespace bolt::test;

// ============================================
// LSP Types Tests
// ============================================

BOLT_TEST(LSPTypes, PositionCreation) {
    bolt::lsp::Position pos(10, 25);
    BOLT_ASSERT_EQ(10u, pos.line);
    BOLT_ASSERT_EQ(25u, pos.character);
}

BOLT_TEST(LSPTypes, RangeCreation) {
    bolt::lsp::Position start(0, 0);
    bolt::lsp::Position end(10, 50);
    bolt::lsp::Range range(start, end);

    BOLT_ASSERT_EQ(0u, range.start.line);
    BOLT_ASSERT_EQ(10u, range.end.line);
}

BOLT_TEST(LSPTypes, LocationCreation) {
    bolt::lsp::Position pos(5, 10);
    bolt::lsp::Location loc("file:///path/to/file.cpp", bolt::lsp::Range(pos, pos));

    BOLT_ASSERT(!loc.uri.empty());
    BOLT_ASSERT_EQ(5u, loc.range.start.line);
}

BOLT_TEST(LSPTypes, DiagnosticSeverity) {
    bolt::lsp::Diagnostic diag;
    diag.severity = bolt::lsp::DiagnosticSeverity::Error;
    diag.message = "Undefined variable";
    diag.range = bolt::lsp::Range(bolt::lsp::Position(10, 0), bolt::lsp::Position(10, 15));

    BOLT_ASSERT_EQ(bolt::lsp::DiagnosticSeverity::Error, diag.severity);
    BOLT_ASSERT_EQ("Undefined variable", diag.message);
}

BOLT_TEST(LSPTypes, CompletionItem) {
    bolt::lsp::CompletionItem item;
    item.label = "printf";
    item.kind = bolt::lsp::CompletionItemKind::Function;
    item.detail = "Print formatted output";
    item.insertText = "printf($1)";

    BOLT_ASSERT_EQ("printf", item.label);
    BOLT_ASSERT_EQ(bolt::lsp::CompletionItemKind::Function, item.kind);
}

BOLT_TEST(LSPTypes, TextDocumentIdentifier) {
    bolt::lsp::TextDocumentIdentifier doc;
    doc.uri = "file:///home/user/project/main.cpp";

    BOLT_ASSERT(!doc.uri.empty());
    BOLT_ASSERT(doc.uri.find("main.cpp") != std::string::npos);
}

BOLT_TEST(LSPTypes, TextEdit) {
    bolt::lsp::TextEdit edit;
    edit.range = bolt::lsp::Range(bolt::lsp::Position(0, 0), bolt::lsp::Position(0, 10));
    edit.newText = "new content";

    BOLT_ASSERT_EQ("new content", edit.newText);
}

// ============================================
// LSP JSON-RPC Tests
// ============================================

BOLT_TEST(LSPJsonRpc, CreateRequest) {
    bolt::editor::LspJsonRpc rpc;

    auto request = rpc.createRequest("initialize", 1);

    BOLT_ASSERT(!request.empty());
    BOLT_ASSERT(request.find("jsonrpc") != std::string::npos);
    BOLT_ASSERT(request.find("2.0") != std::string::npos);
    BOLT_ASSERT(request.find("initialize") != std::string::npos);
}

BOLT_TEST(LSPJsonRpc, CreateNotification) {
    bolt::editor::LspJsonRpc rpc;

    auto notification = rpc.createNotification("textDocument/didOpen");

    BOLT_ASSERT(!notification.empty());
    BOLT_ASSERT(notification.find("textDocument/didOpen") != std::string::npos);
    // Notifications don't have an id field
}

BOLT_TEST(LSPJsonRpc, CreateResponse) {
    bolt::editor::LspJsonRpc rpc;

    auto response = rpc.createSuccessResponse(1, "result data");

    BOLT_ASSERT(!response.empty());
    BOLT_ASSERT(response.find("result") != std::string::npos);
}

BOLT_TEST(LSPJsonRpc, CreateErrorResponse) {
    bolt::editor::LspJsonRpc rpc;

    auto error = rpc.createErrorResponse(1, -32600, "Invalid Request");

    BOLT_ASSERT(!error.empty());
    BOLT_ASSERT(error.find("error") != std::string::npos);
    BOLT_ASSERT(error.find("-32600") != std::string::npos);
}

BOLT_TEST(LSPJsonRpc, ParseMessage) {
    bolt::editor::LspJsonRpc rpc;

    std::string message = R"({"jsonrpc":"2.0","id":1,"method":"initialize"})";

    auto parsed = rpc.parseMessage(message);

    BOLT_ASSERT(parsed.isValid);
    BOLT_ASSERT_EQ("initialize", parsed.method);
    BOLT_ASSERT_EQ(1, parsed.id);
}

BOLT_TEST(LSPJsonRpc, ParseInvalidMessage) {
    bolt::editor::LspJsonRpc rpc;

    std::string invalid = "not json";

    auto parsed = rpc.parseMessage(invalid);

    BOLT_ASSERT(!parsed.isValid);
}

BOLT_TEST(LSPJsonRpc, HeaderGeneration) {
    bolt::editor::LspJsonRpc rpc;

    std::string content = R"({"jsonrpc":"2.0","id":1})";
    auto withHeader = rpc.addContentLengthHeader(content);

    BOLT_ASSERT(withHeader.find("Content-Length:") != std::string::npos);
    BOLT_ASSERT(withHeader.find("\r\n\r\n") != std::string::npos);
}

// ============================================
// LSP Client Tests
// ============================================

BOLT_TEST(LSPClient, Initialization) {
    bolt::lsp::LspClient client;

    BOLT_ASSERT(!client.isConnected());
    BOLT_ASSERT(!client.isInitialized());
}

BOLT_TEST(LSPClient, CapabilitiesRegistration) {
    bolt::lsp::LspClient client;

    bolt::lsp::ClientCapabilities caps;
    caps.textDocument.completion.completionItem.snippetSupport = true;
    caps.textDocument.hover.contentFormat = {"markdown", "plaintext"};

    client.setCapabilities(caps);

    auto registeredCaps = client.getCapabilities();
    BOLT_ASSERT(registeredCaps.textDocument.completion.completionItem.snippetSupport);
}

BOLT_TEST(LSPClient, DocumentManagement) {
    bolt::lsp::LspClient client;

    // Open document
    bolt::lsp::TextDocumentItem doc;
    doc.uri = "file:///test.cpp";
    doc.languageId = "cpp";
    doc.version = 1;
    doc.text = "int main() { return 0; }";

    client.openDocument(doc);

    // Check document is tracked
    BOLT_ASSERT(client.hasDocument(doc.uri));

    // Update document
    client.updateDocument(doc.uri, "int main() { return 1; }", 2);

    // Close document
    client.closeDocument(doc.uri);
    BOLT_ASSERT(!client.hasDocument(doc.uri));
}

BOLT_TEST(LSPClient, PendingRequestTracking) {
    bolt::lsp::LspClient client;

    // Add pending request
    int requestId = client.addPendingRequest("textDocument/completion");

    BOLT_ASSERT(requestId > 0);
    BOLT_ASSERT(client.hasPendingRequest(requestId));

    // Complete request
    client.completePendingRequest(requestId);
    BOLT_ASSERT(!client.hasPendingRequest(requestId));
}

// ============================================
// LSP Server Tests
// ============================================

BOLT_TEST(LSPServer, Initialization) {
    bolt::lsp::LspServer server;

    BOLT_ASSERT(!server.isRunning());
}

BOLT_TEST(LSPServer, ServerCapabilities) {
    bolt::lsp::LspServer server;

    bolt::lsp::ServerCapabilities caps;
    caps.textDocumentSync = bolt::lsp::TextDocumentSyncKind::Incremental;
    caps.completionProvider.triggerCharacters = {".", ":"};
    caps.hoverProvider = true;
    caps.definitionProvider = true;

    server.setCapabilities(caps);

    auto serverCaps = server.getCapabilities();
    BOLT_ASSERT(serverCaps.hoverProvider);
    BOLT_ASSERT(serverCaps.definitionProvider);
}

BOLT_TEST(LSPServer, MethodRegistration) {
    bolt::lsp::LspServer server;

    bool methodCalled = false;

    server.registerMethod("custom/method", [&](const std::string& params) -> std::string {
        methodCalled = true;
        return "{}";
    });

    // Dispatch method
    server.handleRequest("custom/method", "{}");

    BOLT_ASSERT(methodCalled);
}

BOLT_TEST(LSPServer, NotificationHandling) {
    bolt::lsp::LspServer server;

    int notificationCount = 0;

    server.onNotification("textDocument/didChange", [&](const std::string& params) {
        notificationCount++;
    });

    // Send notification
    server.handleNotification("textDocument/didChange", "{}");
    server.handleNotification("textDocument/didChange", "{}");

    BOLT_ASSERT_EQ(2, notificationCount);
}

// ============================================
// LSP Protocol Tests
// ============================================

BOLT_TEST(LSPProtocol, InitializeRequest) {
    bolt::lsp::LspClient client;

    auto initParams = client.createInitializeParams("/home/user/project");

    BOLT_ASSERT(!initParams.empty());
    BOLT_ASSERT(initParams.find("rootUri") != std::string::npos ||
                initParams.find("rootPath") != std::string::npos);
}

BOLT_TEST(LSPProtocol, CompletionRequest) {
    bolt::lsp::LspClient client;

    auto params = client.createCompletionParams("file:///test.cpp", 10, 5);

    BOLT_ASSERT(!params.empty());
    BOLT_ASSERT(params.find("textDocument") != std::string::npos);
    BOLT_ASSERT(params.find("position") != std::string::npos);
}

BOLT_TEST(LSPProtocol, HoverRequest) {
    bolt::lsp::LspClient client;

    auto params = client.createHoverParams("file:///test.cpp", 5, 10);

    BOLT_ASSERT(!params.empty());
}

BOLT_TEST(LSPProtocol, DefinitionRequest) {
    bolt::lsp::LspClient client;

    auto params = client.createDefinitionParams("file:///test.cpp", 20, 15);

    BOLT_ASSERT(!params.empty());
}

BOLT_TEST(LSPProtocol, ReferencesRequest) {
    bolt::lsp::LspClient client;

    auto params = client.createReferencesParams("file:///test.cpp", 10, 5, true);

    BOLT_ASSERT(!params.empty());
}

BOLT_TEST(LSPProtocol, FormattingRequest) {
    bolt::lsp::LspClient client;

    bolt::lsp::FormattingOptions options;
    options.tabSize = 4;
    options.insertSpaces = true;

    auto params = client.createFormattingParams("file:///test.cpp", options);

    BOLT_ASSERT(!params.empty());
}

// ============================================
// LSP Diagnostics Tests
// ============================================

BOLT_TEST(LSPDiagnostics, DiagnosticCollection) {
    bolt::lsp::DiagnosticCollection collection;

    bolt::lsp::Diagnostic diag1;
    diag1.severity = bolt::lsp::DiagnosticSeverity::Error;
    diag1.message = "Error 1";
    diag1.range = bolt::lsp::Range(bolt::lsp::Position(0, 0), bolt::lsp::Position(0, 10));

    bolt::lsp::Diagnostic diag2;
    diag2.severity = bolt::lsp::DiagnosticSeverity::Warning;
    diag2.message = "Warning 1";
    diag2.range = bolt::lsp::Range(bolt::lsp::Position(5, 0), bolt::lsp::Position(5, 20));

    collection.add("file:///test.cpp", diag1);
    collection.add("file:///test.cpp", diag2);

    auto diagnostics = collection.get("file:///test.cpp");
    BOLT_ASSERT_EQ(2u, diagnostics.size());

    // Clear for file
    collection.clear("file:///test.cpp");
    diagnostics = collection.get("file:///test.cpp");
    BOLT_ASSERT_EQ(0u, diagnostics.size());
}

BOLT_TEST(LSPDiagnostics, DiagnosticFiltering) {
    bolt::lsp::DiagnosticCollection collection;

    for (int i = 0; i < 10; i++) {
        bolt::lsp::Diagnostic diag;
        diag.severity = (i % 2 == 0) ?
            bolt::lsp::DiagnosticSeverity::Error :
            bolt::lsp::DiagnosticSeverity::Warning;
        diag.message = "Diagnostic " + std::to_string(i);
        collection.add("file:///test.cpp", diag);
    }

    auto errors = collection.getByServerity("file:///test.cpp",
        bolt::lsp::DiagnosticSeverity::Error);
    BOLT_ASSERT_EQ(5u, errors.size());

    auto warnings = collection.getByServerity("file:///test.cpp",
        bolt::lsp::DiagnosticSeverity::Warning);
    BOLT_ASSERT_EQ(5u, warnings.size());
}

// ============================================
// LSP Workspace Tests
// ============================================

BOLT_TEST(LSPWorkspace, WorkspaceFolders) {
    bolt::lsp::Workspace workspace;

    workspace.addFolder("/home/user/project1", "Project 1");
    workspace.addFolder("/home/user/project2", "Project 2");

    auto folders = workspace.getFolders();
    BOLT_ASSERT_EQ(2u, folders.size());

    workspace.removeFolder("/home/user/project1");
    folders = workspace.getFolders();
    BOLT_ASSERT_EQ(1u, folders.size());
}

BOLT_TEST(LSPWorkspace, WorkspaceConfiguration) {
    bolt::lsp::Workspace workspace;

    workspace.setConfiguration("editor.tabSize", "4");
    workspace.setConfiguration("editor.formatOnSave", "true");

    auto tabSize = workspace.getConfiguration("editor.tabSize");
    BOLT_ASSERT_EQ("4", tabSize);

    auto formatOnSave = workspace.getConfiguration("editor.formatOnSave");
    BOLT_ASSERT_EQ("true", formatOnSave);
}

// Main function for standalone testing
#ifndef BOLT_INCLUDE_IN_TEST_RUNNER
int main(int argc, char** argv) {
    std::cout << "=== Bolt LSP Test Suite ===" << std::endl;
    std::cout << std::endl;

    auto& suite = bolt::test::TestSuite::getInstance();

    if (argc > 1) {
        std::string suiteName = argv[1];
        if (suiteName == "--list") {
            suite.listTests();
            return 0;
        }
        return suite.runTestsInSuite(suiteName);
    }

    return suite.runAllTests();
}
#endif
