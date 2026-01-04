/**
 * @file demo_external_lsp.cpp
 * @brief Demo application for testing external language server integration.
 * 
 * This demo tests the integration with real language servers like clangd and pyright.
 * It demonstrates:
 * - Auto-configuration of available language servers
 * - Connecting to external language servers via stdio
 * - Sending didOpen/didClose notifications
 * - Requesting code completion
 * - Requesting hover information
 */

#include "bolt/editor/lsp_client.hpp"
#include "bolt/editor/lsp_server_configs.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>

using namespace bolt::lsp;

// Test file content
const char* CPP_TEST_CODE = R"(
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    
    for (const auto& num : numbers) {
        std::cout << num << std::endl;
    }
    
    std::string message = "Hello, World!";
    std::cout << message << std::endl;
    
    return 0;
}
)";

const char* PYTHON_TEST_CODE = R"(
import os
import sys
from typing import List, Optional

def greet(name: str) -> str:
    """Return a greeting message."""
    return f"Hello, {name}!"

def main() -> None:
    names: List[str] = ["Alice", "Bob", "Charlie"]
    
    for name in names:
        print(greet(name))
    
    path: Optional[str] = os.getenv("PATH")
    if path:
        print(f"PATH has {len(path.split(':'))} entries")

if __name__ == "__main__":
    main()
)";

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

void printSubsection(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n\n";
}

bool testClangd(const std::string& testDir) {
    printSeparator("Testing clangd (C/C++ Language Server)");
    
    if (!LspServerConfigs::isServerAvailable("cpp")) {
        std::cout << "❌ clangd is not available on this system.\n";
        std::cout << "   Install with: sudo apt-get install clangd\n";
        return false;
    }
    
    std::cout << "✅ clangd is available\n";
    
    // Get configuration
    auto config = LspServerConfigs::clangd(testDir);
    std::cout << "   Command: " << config.serverCommand << "\n";
    std::cout << "   Args: ";
    for (const auto& arg : config.serverArgs) {
        std::cout << arg << " ";
    }
    std::cout << "\n";
    
    // Create test file
    std::string testFile = testDir + "/test.cpp";
    {
        std::ofstream file(testFile);
        file << CPP_TEST_CODE;
    }
    std::cout << "   Created test file: " << testFile << "\n\n";
    
    // Create client
    printSubsection("Connecting to clangd");
    
    auto connection = std::make_unique<ProcessLspConnection>();
    LspClient client(std::move(connection));
    
    if (!client.connect(config)) {
        std::cout << "❌ Failed to connect to clangd\n";
        return false;
    }
    std::cout << "✅ Connected to clangd\n";
    
    // Initialize
    printSubsection("Initializing LSP session");
    
    bool initSuccess = false;
    client.initialize([&initSuccess](bool success) {
        initSuccess = success;
    });
    
    // Wait for initialization (with timeout)
    auto startTime = std::chrono::steady_clock::now();
    while (!initSuccess && 
           std::chrono::steady_clock::now() - startTime < std::chrono::seconds(10)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!initSuccess) {
        std::cout << "❌ Failed to initialize LSP session (timeout)\n";
        client.disconnect();
        return false;
    }
    std::cout << "✅ LSP session initialized\n";
    
    // Open document
    printSubsection("Opening document");
    
    TextDocumentItem doc;
    doc.uri = "file://" + testFile;
    doc.languageId = "cpp";
    doc.version = 1;
    doc.text = CPP_TEST_CODE;
    
    client.didOpenTextDocument(doc);
    std::cout << "✅ Sent didOpen notification\n";
    
    // Wait for clangd to process the file
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Request completion
    printSubsection("Requesting code completion");
    
    TextDocumentPositionParams completionParams;
    completionParams.textDocument.uri = doc.uri;
    completionParams.position = Position(8, 9);  // After "std::" on line 8
    
    CompletionList completions;
    bool completionReceived = false;
    
    client.completion(completionParams, [&completions, &completionReceived](const CompletionList& result) {
        completions = result;
        completionReceived = true;
    });
    
    // Wait for completion response
    startTime = std::chrono::steady_clock::now();
    while (!completionReceived && 
           std::chrono::steady_clock::now() - startTime < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (completionReceived) {
        std::cout << "✅ Received " << completions.items.size() << " completion items\n";
        int shown = 0;
        for (const auto& item : completions.items) {
            if (shown++ < 5) {
                std::cout << "   - " << item.label << "\n";
            }
        }
        if (completions.items.size() > 5) {
            std::cout << "   ... and " << (completions.items.size() - 5) << " more\n";
        }
    } else {
        std::cout << "⚠️  Completion request timed out\n";
    }
    
    // Request hover
    printSubsection("Requesting hover information");
    
    TextDocumentPositionParams hoverParams;
    hoverParams.textDocument.uri = doc.uri;
    hoverParams.position = Position(6, 10);  // On "vector"
    
    std::optional<Hover> hoverResult;
    bool hoverReceived = false;
    
    client.hover(hoverParams, [&hoverResult, &hoverReceived](const std::optional<Hover>& result) {
        hoverResult = result;
        hoverReceived = true;
    });
    
    // Wait for hover response
    startTime = std::chrono::steady_clock::now();
    while (!hoverReceived && 
           std::chrono::steady_clock::now() - startTime < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (hoverReceived && hoverResult.has_value()) {
        std::cout << "✅ Received hover information:\n";
        std::string content = hoverResult->contents;
        // Truncate if too long
        if (content.length() > 200) {
            content = content.substr(0, 200) + "...";
        }
        std::cout << "   " << content << "\n";
    } else {
        std::cout << "⚠️  Hover request timed out or returned empty\n";
    }
    
    // Close document
    printSubsection("Closing document");
    
    TextDocumentIdentifier closeDoc;
    closeDoc.uri = doc.uri;
    client.didCloseTextDocument(closeDoc);
    std::cout << "✅ Sent didClose notification\n";
    
    // Shutdown
    printSubsection("Shutting down");
    
    client.shutdown([](bool success) {
        std::cout << (success ? "✅" : "❌") << " Shutdown " << (success ? "successful" : "failed") << "\n";
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    client.disconnect();
    std::cout << "✅ Disconnected from clangd\n";
    
    return true;
}

bool testPyright(const std::string& testDir) {
    printSeparator("Testing pyright (Python Language Server)");
    
    if (!LspServerConfigs::isServerAvailable("python")) {
        std::cout << "❌ pyright is not available on this system.\n";
        std::cout << "   Install with: npm install -g pyright\n";
        return false;
    }
    
    std::cout << "✅ pyright is available\n";
    
    // Get configuration
    auto config = LspServerConfigs::pyright(testDir);
    std::cout << "   Command: " << config.serverCommand << "\n";
    std::cout << "   Args: ";
    for (const auto& arg : config.serverArgs) {
        std::cout << arg << " ";
    }
    std::cout << "\n";
    
    // Create test file
    std::string testFile = testDir + "/test.py";
    {
        std::ofstream file(testFile);
        file << PYTHON_TEST_CODE;
    }
    std::cout << "   Created test file: " << testFile << "\n\n";
    
    // Create client
    printSubsection("Connecting to pyright");
    
    auto connection = std::make_unique<ProcessLspConnection>();
    LspClient client(std::move(connection));
    
    if (!client.connect(config)) {
        std::cout << "❌ Failed to connect to pyright\n";
        return false;
    }
    std::cout << "✅ Connected to pyright\n";
    
    // Initialize
    printSubsection("Initializing LSP session");
    
    bool initSuccess = false;
    client.initialize([&initSuccess](bool success) {
        initSuccess = success;
    });
    
    // Wait for initialization
    auto startTime = std::chrono::steady_clock::now();
    while (!initSuccess && 
           std::chrono::steady_clock::now() - startTime < std::chrono::seconds(10)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!initSuccess) {
        std::cout << "❌ Failed to initialize LSP session (timeout)\n";
        client.disconnect();
        return false;
    }
    std::cout << "✅ LSP session initialized\n";
    
    // Open document
    printSubsection("Opening document");
    
    TextDocumentItem doc;
    doc.uri = "file://" + testFile;
    doc.languageId = "python";
    doc.version = 1;
    doc.text = PYTHON_TEST_CODE;
    
    client.didOpenTextDocument(doc);
    std::cout << "✅ Sent didOpen notification\n";
    
    // Wait for pyright to process
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Request completion
    printSubsection("Requesting code completion");
    
    TextDocumentPositionParams completionParams;
    completionParams.textDocument.uri = doc.uri;
    completionParams.position = Position(1, 7);  // After "import "
    
    CompletionList completions;
    bool completionReceived = false;
    
    client.completion(completionParams, [&completions, &completionReceived](const CompletionList& result) {
        completions = result;
        completionReceived = true;
    });
    
    // Wait for completion response
    startTime = std::chrono::steady_clock::now();
    while (!completionReceived && 
           std::chrono::steady_clock::now() - startTime < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (completionReceived) {
        std::cout << "✅ Received " << completions.items.size() << " completion items\n";
        int shown = 0;
        for (const auto& item : completions.items) {
            if (shown++ < 5) {
                std::cout << "   - " << item.label << "\n";
            }
        }
        if (completions.items.size() > 5) {
            std::cout << "   ... and " << (completions.items.size() - 5) << " more\n";
        }
    } else {
        std::cout << "⚠️  Completion request timed out\n";
    }
    
    // Close document
    printSubsection("Closing document");
    
    TextDocumentIdentifier closeDoc;
    closeDoc.uri = doc.uri;
    client.didCloseTextDocument(closeDoc);
    std::cout << "✅ Sent didClose notification\n";
    
    // Shutdown
    printSubsection("Shutting down");
    
    client.shutdown([](bool success) {
        std::cout << (success ? "✅" : "❌") << " Shutdown " << (success ? "successful" : "failed") << "\n";
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    client.disconnect();
    std::cout << "✅ Disconnected from pyright\n";
    
    return true;
}

void testAutoConfiguration(const std::string& testDir) {
    printSeparator("Testing Auto-Configuration");
    
    // Print availability report
    std::cout << LspAutoConfigurator::getAvailabilityReport();
    
    // Create a client manager
    LspClientManager manager;
    
    // Auto-configure
    int configured = LspAutoConfigurator::autoConfigureAll(manager, testDir);
    std::cout << "Auto-configured " << configured << " language server(s)\n";
    
    // List registered languages
    auto languages = manager.getRegisteredLanguages();
    std::cout << "Registered languages:\n";
    for (const auto& lang : languages) {
        std::cout << "   - " << lang << "\n";
    }
    
    manager.shutdown();
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Bolt C++ External LSP Integration Demo               ║\n";
    std::cout << "║     Testing Real Language Server Connections             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    // Create test directory
    std::string testDir = "/tmp/bolt_lsp_test";
    std::filesystem::create_directories(testDir);
    std::cout << "\nTest directory: " << testDir << "\n";
    
    // Test auto-configuration
    testAutoConfiguration(testDir);
    
    // Test clangd
    bool clangdSuccess = testClangd(testDir);
    
    // Test pyright
    bool pyrightSuccess = testPyright(testDir);
    
    // Summary
    printSeparator("Test Summary");
    
    std::cout << "clangd (C/C++):  " << (clangdSuccess ? "✅ PASSED" : "❌ FAILED") << "\n";
    std::cout << "pyright (Python): " << (pyrightSuccess ? "✅ PASSED" : "❌ FAILED") << "\n";
    
    // Cleanup
    std::filesystem::remove_all(testDir);
    std::cout << "\nCleaned up test directory.\n";
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  External LSP Integration Demo Complete\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    return (clangdSuccess && pyrightSuccess) ? 0 : 1;
}
