#include "bolt/editor/lsp_protocol.hpp"
#include "bolt/editor/lsp_json_rpc.hpp"
#include "bolt/editor/lsp_server.hpp"
#include "bolt/editor/lsp_plugin_adapter.hpp"
#include "bolt/core/plugin_interface.hpp"
#include <iostream>
#include <cassert>
#include <memory>

namespace bolt {
namespace lsp {
namespace test {

// Simple test language plugin for testing LSP integration
class TestLanguagePlugin : public ILanguagePlugin {
public:
    PluginMetadata getMetadata() const override {
        PluginMetadata metadata;
        metadata.name = "Test Language Plugin";
        metadata.version = "1.0.0";
        metadata.description = "Test plugin for LSP testing";
        metadata.author = "Test";
        metadata.apiVersion = {1, 0, 0};
        return metadata;
    }
    
    bool initialize(PluginContext* context) override { return true; }
    void activate() override {}
    void deactivate() override {}
    void cleanup() override {}
    
    PluginState getState() const override { return PluginState::Active; }
    std::unordered_map<std::string, std::any> getConfiguration() const override { return {}; }
    bool configure(const std::unordered_map<std::string, std::any>& config) override { return true; }
    
    // ILanguagePlugin interface
    std::vector<std::string> getCodeCompletions(const std::string& filePath, size_t line, size_t column) override {
        return {"test_function", "test_variable", "test_class"};
    }
    
    std::vector<std::string> getDiagnostics(const std::string& filePath, const std::string& content) override {
        if (content.find("error") != std::string::npos) {
            return {"Test error found"};
        }
        return {};
    }
    
    std::string formatCode(const std::string& code) override {
        // Simple formatting - add newline if missing
        if (!code.empty() && code.back() != '\n') {
            return code + '\n';
        }
        return code;
    }
    
    std::vector<std::pair<size_t, size_t>> findReferences(const std::string& filePath, size_t line, size_t column) override {
        return {{line, column}, {line + 1, column}};
    }
    
    std::string getHoverInfo(const std::string& filePath, size_t line, size_t column) override {
        return "Test hover information";
    }
    
    std::vector<std::string> getSupportedExtensions() const override {
        return {".test", ".txt"};
    }
};

// Test functions
void testLspProtocolTypes() {
    std::cout << "Testing LSP protocol types..." << std::endl;
    
    // Test Position
    Position pos(10, 5);
    assert(pos.line == 10);
    assert(pos.character == 5);
    
    // Test Range
    Range range(Position(1, 0), Position(2, 10));
    assert(range.start.line == 1);
    assert(range.end.line == 2);
    
    // Test CompletionItem
    CompletionItem item("test_function");
    item.kind = CompletionItemKind::Function;
    item.detail = "void test_function()";
    assert(item.label == "test_function");
    assert(item.kind == CompletionItemKind::Function);
    
    // Test Diagnostic
    Diagnostic diag(Range(Position(5, 0), Position(5, 10)), "Test error");
    diag.severity = DiagnosticSeverity::Error;
    assert(diag.message == "Test error");
    assert(diag.severity == DiagnosticSeverity::Error);
    
    std::cout << "LSP protocol types test passed!" << std::endl;
}

void testJsonRpc() {
    std::cout << "Testing JSON-RPC functionality..." << std::endl;
    
    JsonRpcHandler handler;
    bool requestHandled = false;
    bool notificationHandled = false;
    
    // Register request handler
    handler.registerRequestHandler("test/request", 
        [&requestHandled](const std::string& method, std::shared_ptr<JsonValue> params) {
            requestHandled = true;
            auto result = std::make_shared<JsonValue>();
            result->setString("test result");
            return result;
        });
    
    // Register notification handler
    handler.registerNotificationHandler("test/notification", 
        [&notificationHandled](const std::string& method, std::shared_ptr<JsonValue> params) {
            notificationHandled = true;
        });
    
    // Test request
    std::string requestMsg = handler.createRequest("test/request", nullptr, "1");
    std::string response = handler.processMessage(requestMsg);
    assert(requestHandled);
    assert(!response.empty());
    
    // Test notification
    std::string notificationMsg = handler.createNotification("test/notification", nullptr);
    std::string notificationResponse = handler.processMessage(notificationMsg);
    assert(notificationHandled);
    assert(notificationResponse.empty()); // No response for notifications
    
    std::cout << "JSON-RPC test passed!" << std::endl;
}

void testLspServer() {
    std::cout << "Testing LSP server..." << std::endl;
    
    LanguageServer server;
    
    // Test initialization
    InitializeParams initParams;
    initParams.rootPath = "/test/path";
    initParams.capabilities.textDocumentSync = true;
    initParams.capabilities.completionProvider = true;
    
    server.initialize(initParams);
    assert(server.isInitialized());
    
    // Register test language plugin
    auto testPlugin = std::make_shared<TestLanguagePlugin>();
    server.registerLanguagePlugin("test", testPlugin);
    
    // Test document operations
    TextDocumentItem document;
    document.uri = "file:///test/file.test";
    document.languageId = "test";
    document.version = 1;
    document.text = "test content";
    
    server.didOpenTextDocument(document);
    
    // Test completion
    TextDocumentPositionParams posParams;
    posParams.textDocument.uri = document.uri;
    posParams.position = Position(0, 5);
    
    CompletionList completions = server.completion(posParams);
    assert(!completions.items.empty());
    assert(completions.items[0].label == "test_function");
    
    // Test hover
    auto hover = server.hover(posParams);
    assert(hover.has_value());
    assert(hover->contents == "Test hover information");
    
    // Test diagnostics
    std::vector<Diagnostic> diagnostics = server.diagnostics(posParams.textDocument);
    assert(diagnostics.empty()); // No "error" in content
    
    // Test with error content
    document.text = "test error content";
    server.didChangeTextDocument(
        VersionedTextDocumentIdentifier{document.uri, 2}, 
        {TextDocumentContentChangeEvent{{}, {}, document.text}}
    );
    
    diagnostics = server.diagnostics(posParams.textDocument);
    assert(!diagnostics.empty());
    assert(diagnostics[0].message == "Test error found");
    
    std::cout << "LSP server test passed!" << std::endl;
}

void testLspPluginAdapter() {
    std::cout << "Testing LSP plugin adapter..." << std::endl;
    
    auto testPlugin = std::make_shared<TestLanguagePlugin>();
    LanguagePluginLspAdapter adapter(testPlugin, "test");
    
    // Test completion adaptation
    CompletionList completions = adapter.getCompletion("/test/file.test", Position(0, 5));
    assert(!completions.items.empty());
    assert(completions.items[0].label == "test_function");
    
    // Test hover adaptation
    auto hover = adapter.getHover("/test/file.test", Position(0, 5));
    assert(hover.has_value());
    assert(hover->contents == "Test hover information");
    
    // Test diagnostics adaptation
    std::vector<Diagnostic> diagnostics = adapter.getDiagnostics("/test/file.test", "test error content");
    assert(!diagnostics.empty());
    assert(diagnostics[0].message == "Test error found");
    
    // Test formatting adaptation
    std::vector<TextEdit> edits = adapter.formatDocument("/test/file.test", "test content", FormattingOptions{});
    assert(!edits.empty());
    assert(edits[0].newText == "test content\n");
    
    std::cout << "LSP plugin adapter test passed!" << std::endl;
}

void testLspIntegrationPlugin() {
    std::cout << "Testing LSP integration plugin..." << std::endl;
    
    auto lspPlugin = createLspIntegrationPlugin();
    assert(lspPlugin != nullptr);
    
    // Test plugin interface
    auto metadata = lspPlugin->getMetadata();
    assert(metadata.name == "LSP Integration Plugin");
    assert(metadata.version == "1.0.0");
    
    // Test initialization
    bool initResult = lspPlugin->initialize(nullptr);
    assert(initResult);
    
    lspPlugin->activate();
    
    // Test language plugin registration
    auto testPlugin = std::make_shared<TestLanguagePlugin>();
    lspPlugin->registerLanguagePlugin("test", testPlugin);
    
    // Test supported extensions
    auto extensions = lspPlugin->getSupportedExtensions();
    bool hasTestExt = std::find(extensions.begin(), extensions.end(), ".test") != extensions.end();
    assert(hasTestExt);
    
    // Test document operations
    lspPlugin->onDocumentOpened("/test/file.test", "test content");
    
    // Test language features
    auto completions = lspPlugin->getCodeCompletions("/test/file.test", 0, 5);
    assert(!completions.empty());
    assert(completions[0] == "test_function");
    
    auto diagnostics = lspPlugin->getDiagnostics("/test/file.test", "test error content");
    assert(!diagnostics.empty());
    assert(diagnostics[0] == "Test error found");
    
    std::string hoverInfo = lspPlugin->getHoverInfo("/test/file.test", 0, 5);
    assert(hoverInfo == "Test hover information");
    
    // Test configuration
    std::unordered_map<std::string, std::any> config = {{"enabled", false}};
    lspPlugin->configure(config);
    assert(!lspPlugin->isEnabled());
    
    // Test cleanup
    lspPlugin->cleanup();
    
    std::cout << "LSP integration plugin test passed!" << std::endl;
}

} // namespace test
} // namespace lsp
} // namespace bolt

int main() {
    using namespace bolt::lsp::test;
    
    std::cout << "Running LSP implementation tests..." << std::endl;
    
    try {
        testLspProtocolTypes();
        testJsonRpc();
        testLspServer();
        testLspPluginAdapter();
        testLspIntegrationPlugin();
        
        std::cout << "All LSP tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}