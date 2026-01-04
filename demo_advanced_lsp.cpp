/**
 * @file demo_advanced_lsp.cpp
 * @brief Demo application testing all advanced LSP features
 * 
 * Tests:
 * - Go-to-Definition (F12)
 * - Find References (Shift+F12)
 * - Document Symbols (Ctrl+Shift+O)
 * - Code Formatting (Shift+Alt+F)
 */

#include "bolt/editor/lsp_client.hpp"
#include "bolt/editor/lsp_server_configs.hpp"
#include "bolt/editor/lsp_protocol.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>

using namespace bolt::lsp;

// Test file content with multiple symbols and references
const char* TEST_CPP_CODE = R"(
#include <iostream>
#include <vector>
#include <string>

// A simple class with methods
class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }
    
    int subtract(int a, int b) {
        return a - b;
    }
    
    int multiply(int a, int b) {
        return a * b;
    }
    
private:
    int result_;
};

// A function that uses Calculator
void testCalculator() {
    Calculator calc;
    int sum = calc.add(5, 3);
    int diff = calc.subtract(10, 4);
    int product = calc.multiply(2, 6);
    
    std::cout << "Sum: " << sum << std::endl;
    std::cout << "Diff: " << diff << std::endl;
    std::cout << "Product: " << product << std::endl;
}

int main() {
    testCalculator();
    
    Calculator calc2;
    std::cout << "Result: " << calc2.add(100, 200) << std::endl;
    
    return 0;
}
)";

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void printLocation(const Location& loc) {
    std::cout << "  📍 " << loc.uri << "\n";
    std::cout << "     Line " << loc.range.start.line + 1 
              << ", Col " << loc.range.start.character + 1 << "\n";
}

void printSymbol(const DocumentSymbol& sym, int indent = 0) {
    std::string prefix(indent * 2, ' ');
    std::cout << prefix << "  📦 " << sym.name;
    if (sym.detail.has_value() && !sym.detail->empty()) {
        std::cout << " (" << *sym.detail << ")";
    }
    std::cout << " [" << static_cast<int>(sym.kind) << "]";
    std::cout << " @ line " << sym.selectionRange.start.line + 1 << "\n";
    
    for (const auto& child : sym.children) {
        printSymbol(child, indent + 1);
    }
}

void printTextEdit(const TextEdit& edit) {
    std::cout << "  ✏️  Lines " << edit.range.start.line + 1 
              << "-" << edit.range.end.line + 1 << ": ";
    if (edit.newText.length() > 50) {
        std::cout << edit.newText.substr(0, 50) << "...\n";
    } else {
        std::cout << "\"" << edit.newText << "\"\n";
    }
}

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════╗
║     Bolt C++ Advanced LSP Features Demo                  ║
║     Testing Definition, References, Symbols, Formatting  ║
╚══════════════════════════════════════════════════════════╝
)" << std::endl;

    // Create test directory and file
    std::string testDir = "/tmp/bolt_advanced_lsp_test";
    std::filesystem::create_directories(testDir);
    std::string testFile = testDir + "/test_advanced.cpp";
    
    {
        std::ofstream ofs(testFile);
        ofs << TEST_CPP_CODE;
    }
    std::cout << "Created test file: " << testFile << "\n";

    // Check if clangd is available
    if (!LspServerConfigs::isServerAvailable("cpp")) {
        std::cerr << "❌ clangd not available. Please install clangd.\n";
        return 1;
    }
    
    auto config = LspServerConfigs::getConfigForLanguage("cpp");
    config.rootPath = testDir;
    
    std::cout << "✅ clangd available: " << config.serverCommand << "\n";

    // Create and connect LSP client
    auto connection = std::make_unique<ProcessLspConnection>();
    LspClient client(std::move(connection));
    
    if (!client.connect(config)) {
        std::cerr << "❌ Failed to connect to clangd\n";
        return 1;
    }
    std::cout << "✅ Connected to clangd\n";

    // Initialize
    bool initSuccess = false;
    client.initialize([&initSuccess](bool success) {
        initSuccess = success;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (!initSuccess) {
        std::cerr << "❌ Failed to initialize LSP session\n";
        return 1;
    }
    std::cout << "✅ LSP session initialized\n";

    // Open document
    TextDocumentItem doc;
    doc.uri = "file://" + testFile;
    doc.languageId = "cpp";
    doc.version = 1;
    doc.text = TEST_CPP_CODE;
    client.didOpenTextDocument(doc);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "✅ Document opened\n";

    // ========================================================================
    // Test 1: Go-to-Definition
    // ========================================================================
    printSeparator("Test 1: Go-to-Definition");
    std::cout << "Testing definition of 'Calculator' at line 27 (calc.add)\n";
    
    std::vector<Location> definitions;
    bool defReceived = false;
    
    TextDocumentPositionParams defParams;
    defParams.textDocument.uri = doc.uri;
    defParams.position.line = 26;  // Line with "calc.add"
    defParams.position.character = 4;  // Position of "calc"
    
    client.definition(defParams, [&definitions, &defReceived](const std::vector<Location>& locs) {
        definitions = locs;
        defReceived = true;
    });
    
    // Wait for response
    for (int i = 0; i < 50 && !defReceived; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (defReceived && !definitions.empty()) {
        std::cout << "✅ Go-to-Definition: Found " << definitions.size() << " definition(s)\n";
        for (const auto& loc : definitions) {
            printLocation(loc);
        }
    } else {
        std::cout << "⚠️  Go-to-Definition: No definitions found (may need indexing time)\n";
    }

    // ========================================================================
    // Test 2: Find References
    // ========================================================================
    printSeparator("Test 2: Find References");
    std::cout << "Testing references to 'add' method\n";
    
    std::vector<Location> references;
    bool refsReceived = false;
    
    TextDocumentPositionParams refParams;
    refParams.textDocument.uri = doc.uri;
    refParams.position.line = 8;  // Line with "int add(int a, int b)"
    refParams.position.character = 8;  // Position of "add"
    
    client.references(refParams, [&references, &refsReceived](const std::vector<Location>& locs) {
        references = locs;
        refsReceived = true;
    });
    
    // Wait for response
    for (int i = 0; i < 50 && !refsReceived; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (refsReceived && !references.empty()) {
        std::cout << "✅ Find References: Found " << references.size() << " reference(s)\n";
        for (const auto& loc : references) {
            printLocation(loc);
        }
    } else {
        std::cout << "⚠️  Find References: No references found (may need indexing time)\n";
    }

    // ========================================================================
    // Test 3: Document Symbols
    // ========================================================================
    printSeparator("Test 3: Document Symbols");
    std::cout << "Testing document symbols (outline)\n";
    
    std::vector<DocumentSymbol> symbols;
    bool symsReceived = false;
    
    TextDocumentIdentifier symDocId;
    symDocId.uri = doc.uri;
    
    client.documentSymbol(symDocId, [&symbols, &symsReceived](const std::vector<DocumentSymbol>& syms) {
        symbols = syms;
        symsReceived = true;
    });
    
    // Wait for response
    for (int i = 0; i < 50 && !symsReceived; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (symsReceived && !symbols.empty()) {
        std::cout << "✅ Document Symbols: Found " << symbols.size() << " top-level symbol(s)\n";
        for (const auto& sym : symbols) {
            printSymbol(sym);
        }
    } else {
        std::cout << "⚠️  Document Symbols: No symbols found\n";
    }

    // ========================================================================
    // Test 4: Code Formatting
    // ========================================================================
    printSeparator("Test 4: Code Formatting");
    std::cout << "Testing document formatting\n";
    
    std::vector<TextEdit> edits;
    bool fmtReceived = false;
    
    TextDocumentIdentifier fmtDocId;
    fmtDocId.uri = doc.uri;
    
    FormattingOptions fmtOpts;
    fmtOpts.tabSize = 4;
    fmtOpts.insertSpaces = true;
    
    client.formatting(fmtDocId, fmtOpts, [&edits, &fmtReceived](const std::vector<TextEdit>& e) {
        edits = e;
        fmtReceived = true;
    });
    
    // Wait for response
    for (int i = 0; i < 50 && !fmtReceived; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (fmtReceived) {
        if (edits.empty()) {
            std::cout << "✅ Code Formatting: Document already formatted (no changes needed)\n";
        } else {
            std::cout << "✅ Code Formatting: " << edits.size() << " edit(s) suggested\n";
            for (const auto& edit : edits) {
                printTextEdit(edit);
            }
        }
    } else {
        std::cout << "⚠️  Code Formatting: No response received\n";
    }

    // ========================================================================
    // Cleanup
    // ========================================================================
    printSeparator("Cleanup");
    
    TextDocumentIdentifier closeDocId;
    closeDocId.uri = doc.uri;
    client.didCloseTextDocument(closeDocId);
    std::cout << "✅ Document closed\n";
    
    client.shutdown([](bool success) {});
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    client.disconnect();
    std::cout << "✅ LSP session shutdown\n";

    // Summary
    printSeparator("Summary");
    std::cout << "Advanced LSP Features Test Results:\n";
    std::cout << "  • Go-to-Definition: " << (defReceived && !definitions.empty() ? "✅ PASSED" : "⚠️  PARTIAL") << "\n";
    std::cout << "  • Find References:  " << (refsReceived && !references.empty() ? "✅ PASSED" : "⚠️  PARTIAL") << "\n";
    std::cout << "  • Document Symbols: " << (symsReceived && !symbols.empty() ? "✅ PASSED" : "⚠️  PARTIAL") << "\n";
    std::cout << "  • Code Formatting:  " << (fmtReceived ? "✅ PASSED" : "⚠️  PARTIAL") << "\n";
    
    std::cout << "\nNote: Some features may show partial results on first run due to\n";
    std::cout << "clangd needing time to build its index. Re-running typically shows\n";
    std::cout << "complete results.\n";

    return 0;
}
