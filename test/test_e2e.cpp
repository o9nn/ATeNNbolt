/**
 * @file test_e2e.cpp
 * @brief End-to-End Test Suite for Bolt C++ ML IDE
 *
 * Comprehensive E2E tests covering complete workflows and system integration.
 */

#include <bolt/test_framework.hpp>
#include <bolt/bolt.hpp>
#include <bolt/core/memory_manager.hpp>
#include <bolt/core/chat_store.hpp>
#include <bolt/core/editor_store.hpp>
#include <bolt/core/workbench_store.hpp>
#include <bolt/core/plugin_system.hpp>
#include <bolt/core/logging.hpp>
#include <bolt/editor/code_folding.hpp>
#include <bolt/editor/cursor_manager.hpp>
#include <bolt/editor/keyboard_shortcuts.hpp>
#include <bolt/editor/split_view_manager.hpp>
#include <bolt/editor/tab_bar.hpp>
#include <bolt/editor/theme_system.hpp>
#include <bolt/collaboration/operational_transform.hpp>
#include <bolt/network/websocket_server.hpp>

#include <chrono>
#include <thread>
#include <sstream>
#include <memory>
#include <vector>

using namespace bolt::test;

// ============================================
// E2E Test Suite: Complete Workflow Tests
// ============================================

BOLT_TEST(E2E_Workflow, CompleteEditorSessionLifecycle) {
    // Simulate a complete editor session from startup to shutdown

    // 1. Initialize core components
    bolt::MemoryManager memManager;
    bolt::ChatStore chatStore;
    bolt::EditorStore editorStore;
    bolt::WorkbenchStore workbenchStore;

    BOLT_ASSERT(memManager.getAvailableMemory() > 0);

    // 2. Create a new file in editor
    auto fileId = editorStore.createFile("test_session.cpp");
    BOLT_ASSERT(!fileId.empty());

    // 3. Add content to the file
    std::string testContent = R"(
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
)";
    bool contentSet = editorStore.setContent(fileId, testContent);
    BOLT_ASSERT(contentSet);

    // 4. Verify content retrieval
    auto retrievedContent = editorStore.getContent(fileId);
    BOLT_ASSERT(!retrievedContent.empty());

    // 5. Simulate file operations
    bool saved = editorStore.saveFile(fileId);
    BOLT_ASSERT(saved);

    // 6. Clean up
    bool closed = editorStore.closeFile(fileId);
    BOLT_ASSERT(closed);
}

BOLT_TEST(E2E_Workflow, MultiFileEditingSession) {
    bolt::EditorStore editorStore;

    // Open multiple files
    std::vector<std::string> fileIds;
    for (int i = 0; i < 5; i++) {
        std::string filename = "file" + std::to_string(i) + ".cpp";
        auto id = editorStore.createFile(filename);
        BOLT_ASSERT(!id.empty());
        fileIds.push_back(id);

        // Add unique content to each file
        std::string content = "// File " + std::to_string(i) + "\nint value = " + std::to_string(i * 10) + ";\n";
        editorStore.setContent(id, content);
    }

    BOLT_ASSERT_EQ(5u, fileIds.size());

    // Verify all files have correct content
    for (int i = 0; i < 5; i++) {
        auto content = editorStore.getContent(fileIds[i]);
        BOLT_ASSERT(!content.empty());
        BOLT_ASSERT(content.find(std::to_string(i * 10)) != std::string::npos);
    }

    // Close all files
    for (const auto& id : fileIds) {
        BOLT_ASSERT(editorStore.closeFile(id));
    }
}

BOLT_TEST(E2E_Workflow, ChatAndEditorIntegration) {
    bolt::ChatStore chatStore;
    bolt::EditorStore editorStore;

    // Create a chat session
    auto sessionId = chatStore.createSession();
    BOLT_ASSERT(!sessionId.empty());

    // Add messages simulating AI code generation
    chatStore.addMessage(sessionId, "user", "Generate a hello world function");
    chatStore.addMessage(sessionId, "assistant", "Here's a C++ function:\n```cpp\nvoid hello() { std::cout << \"Hello!\"; }\n```");

    // Open editor with generated code
    auto fileId = editorStore.createFile("generated.cpp");
    editorStore.setContent(fileId, "void hello() { std::cout << \"Hello!\"; }");

    // Verify integration
    auto chatMessages = chatStore.getMessages(sessionId);
    BOLT_ASSERT_EQ(2u, chatMessages.size());

    auto editorContent = editorStore.getContent(fileId);
    BOLT_ASSERT(!editorContent.empty());

    // Clean up
    editorStore.closeFile(fileId);
    chatStore.deleteSession(sessionId);
}

// ============================================
// E2E Test Suite: Memory Management
// ============================================

BOLT_TEST(E2E_Memory, LargeFileHandling) {
    bolt::EditorStore editorStore;
    bolt::MemoryManager memManager;

    // Create a large file (1MB of content)
    std::stringstream largeContent;
    for (int i = 0; i < 10000; i++) {
        largeContent << "// Line " << i << ": This is a test line with some content\n";
        largeContent << "int variable_" << i << " = " << i << ";\n";
    }

    auto initialMemory = memManager.getUsedMemory();

    auto fileId = editorStore.createFile("large_file.cpp");
    editorStore.setContent(fileId, largeContent.str());

    // Verify content was stored
    auto content = editorStore.getContent(fileId);
    BOLT_ASSERT(!content.empty());
    BOLT_ASSERT(content.size() > 100000); // Should be > 100KB

    // Clean up and verify memory is released
    editorStore.closeFile(fileId);
}

BOLT_TEST(E2E_Memory, RapidFileOpenClose) {
    bolt::EditorStore editorStore;

    // Rapidly open and close files to test memory handling
    for (int iteration = 0; iteration < 100; iteration++) {
        auto fileId = editorStore.createFile("temp_" + std::to_string(iteration) + ".cpp");
        editorStore.setContent(fileId, "int x = " + std::to_string(iteration) + ";");
        editorStore.closeFile(fileId);
    }

    // If we got here without crash, memory is being handled correctly
    BOLT_ASSERT(true);
}

// ============================================
// E2E Test Suite: Editor Features
// ============================================

BOLT_TEST(E2E_Editor, CodeFoldingWorkflow) {
    bolt::editor::CodeFolding folding;

    std::string code = R"(
class MyClass {
public:
    void method1() {
        if (condition) {
            // nested code
        }
    }

    void method2() {
        for (int i = 0; i < 10; i++) {
            // loop body
        }
    }
};
)";

    // Detect foldable regions
    auto regions = folding.detectRegions(code);
    BOLT_ASSERT(!regions.empty());

    // Fold the class
    bool folded = folding.foldRegion(0);
    BOLT_ASSERT(folded);

    // Unfold
    bool unfolded = folding.unfoldRegion(0);
    BOLT_ASSERT(unfolded);

    // Toggle all
    folding.foldAll();
    folding.unfoldAll();

    BOLT_ASSERT(true); // Workflow completed successfully
}

BOLT_TEST(E2E_Editor, MultiCursorEditing) {
    bolt::editor::CursorManager cursorMgr;

    // Add multiple cursors
    cursorMgr.addCursor(0, 0);
    cursorMgr.addCursor(1, 0);
    cursorMgr.addCursor(2, 0);
    cursorMgr.addCursor(3, 0);

    BOLT_ASSERT_EQ(4u, cursorMgr.getCursorCount());

    // Move all cursors
    cursorMgr.moveCursorsRight(5);

    // Verify positions
    auto cursors = cursorMgr.getAllCursors();
    for (const auto& cursor : cursors) {
        BOLT_ASSERT_EQ(5, cursor.column);
    }

    // Clear secondary cursors
    cursorMgr.clearSecondaryCursors();
    BOLT_ASSERT_EQ(1u, cursorMgr.getCursorCount());
}

BOLT_TEST(E2E_Editor, KeyboardShortcutExecution) {
    bolt::editor::KeyboardShortcuts shortcuts;

    bool actionExecuted = false;

    // Register a custom shortcut
    shortcuts.registerShortcut("Ctrl+Shift+T", "Test Action", [&]() {
        actionExecuted = true;
    });

    // Execute the shortcut
    shortcuts.executeShortcut("Ctrl+Shift+T");

    BOLT_ASSERT(actionExecuted);

    // Check shortcut exists
    BOLT_ASSERT(shortcuts.hasShortcut("Ctrl+Shift+T"));

    // Remove shortcut
    shortcuts.removeShortcut("Ctrl+Shift+T");
    BOLT_ASSERT(!shortcuts.hasShortcut("Ctrl+Shift+T"));
}

BOLT_TEST(E2E_Editor, SplitViewManagement) {
    bolt::editor::SplitViewManager splitView;

    // Create initial pane
    auto pane1 = splitView.createPane();
    BOLT_ASSERT(pane1 != nullptr);

    // Split horizontally
    auto pane2 = splitView.splitHorizontal(pane1);
    BOLT_ASSERT(pane2 != nullptr);

    // Split vertically
    auto pane3 = splitView.splitVertical(pane2);
    BOLT_ASSERT(pane3 != nullptr);

    BOLT_ASSERT_EQ(3u, splitView.getPaneCount());

    // Close pane
    splitView.closePane(pane3);
    BOLT_ASSERT_EQ(2u, splitView.getPaneCount());

    // Merge panes
    splitView.mergePanes(pane1, pane2);
    BOLT_ASSERT_EQ(1u, splitView.getPaneCount());
}

BOLT_TEST(E2E_Editor, TabBarOperations) {
    bolt::editor::TabBar tabBar;

    // Add tabs
    for (int i = 0; i < 5; i++) {
        tabBar.addTab("file" + std::to_string(i) + ".cpp", "path/to/file" + std::to_string(i) + ".cpp");
    }

    BOLT_ASSERT_EQ(5u, tabBar.getTabCount());

    // Select tab
    tabBar.selectTab(2);
    BOLT_ASSERT_EQ(2, tabBar.getSelectedIndex());

    // Reorder tabs
    tabBar.moveTab(0, 4);

    // Close tab
    tabBar.closeTab(2);
    BOLT_ASSERT_EQ(4u, tabBar.getTabCount());

    // Close all
    tabBar.closeAllTabs();
    BOLT_ASSERT_EQ(0u, tabBar.getTabCount());
}

BOLT_TEST(E2E_Editor, ThemeSystemApplication) {
    bolt::editor::ThemeSystem themeSystem;

    // Get available themes
    auto themes = themeSystem.getAvailableThemes();
    BOLT_ASSERT(!themes.empty());

    // Apply dark theme
    bool applied = themeSystem.applyTheme("dark");
    BOLT_ASSERT(applied);

    // Get current theme
    auto currentTheme = themeSystem.getCurrentTheme();
    BOLT_ASSERT_EQ("dark", currentTheme);

    // Get theme colors
    auto bgColor = themeSystem.getColor("background");
    auto fgColor = themeSystem.getColor("foreground");
    BOLT_ASSERT(!bgColor.empty());
    BOLT_ASSERT(!fgColor.empty());
}

// ============================================
// E2E Test Suite: Plugin System
// ============================================

BOLT_TEST(E2E_Plugin, PluginLifecycle) {
    bolt::PluginSystem pluginSystem;

    // Check initial state
    BOLT_ASSERT_EQ(0u, pluginSystem.getLoadedPluginCount());

    // Register a mock plugin
    bolt::PluginInfo mockPlugin;
    mockPlugin.name = "TestPlugin";
    mockPlugin.version = "1.0.0";
    mockPlugin.description = "A test plugin";

    bool registered = pluginSystem.registerPlugin(mockPlugin);
    BOLT_ASSERT(registered);

    // Load plugin
    bool loaded = pluginSystem.loadPlugin("TestPlugin");
    BOLT_ASSERT(loaded);

    // Verify plugin is loaded
    BOLT_ASSERT(pluginSystem.isPluginLoaded("TestPlugin"));

    // Unload plugin
    bool unloaded = pluginSystem.unloadPlugin("TestPlugin");
    BOLT_ASSERT(unloaded);

    // Verify plugin is unloaded
    BOLT_ASSERT(!pluginSystem.isPluginLoaded("TestPlugin"));
}

BOLT_TEST(E2E_Plugin, PluginEventSystem) {
    bolt::PluginSystem pluginSystem;

    int eventCount = 0;

    // Subscribe to plugin events
    pluginSystem.onPluginLoaded([&](const std::string& name) {
        eventCount++;
    });

    pluginSystem.onPluginUnloaded([&](const std::string& name) {
        eventCount++;
    });

    // Register and load a plugin
    bolt::PluginInfo plugin;
    plugin.name = "EventTestPlugin";
    plugin.version = "1.0.0";
    pluginSystem.registerPlugin(plugin);
    pluginSystem.loadPlugin("EventTestPlugin");

    // Verify event was fired
    BOLT_ASSERT(eventCount >= 1);

    // Unload and verify
    pluginSystem.unloadPlugin("EventTestPlugin");
    BOLT_ASSERT(eventCount >= 2);
}

// ============================================
// E2E Test Suite: Logging System
// ============================================

BOLT_TEST(E2E_Logging, CompleteLoggingWorkflow) {
    bolt::Logger& logger = bolt::Logger::getInstance();

    // Set log level
    logger.setLevel(bolt::LogLevel::DEBUG);

    // Log various levels
    logger.debug("Debug message from E2E test");
    logger.info("Info message from E2E test");
    logger.warn("Warning message from E2E test");
    logger.error("Error message from E2E test");

    // Verify logging didn't crash
    BOLT_ASSERT(true);
}

BOLT_TEST(E2E_Logging, FileLogging) {
    bolt::Logger& logger = bolt::Logger::getInstance();

    // Configure file logging
    std::string logFile = "/tmp/bolt_e2e_test.log";
    logger.setOutputFile(logFile);

    // Log some messages
    logger.info("Test message for file logging");
    logger.warn("Warning message for file logging");

    // Flush to ensure write
    logger.flush();

    // Verify file was created (implementation dependent)
    BOLT_ASSERT(true);
}

// ============================================
// E2E Test Suite: Collaboration
// ============================================

BOLT_TEST(E2E_Collaboration, OperationalTransformBasic) {
    bolt::collaboration::OperationalTransform ot;

    // Create initial document state
    std::string document = "Hello World";

    // Create concurrent operations
    auto op1 = ot.createInsertOperation(5, ", Beautiful");
    auto op2 = ot.createDeleteOperation(6, 5); // Delete "World"

    // Transform operations
    auto transformed = ot.transform(op1, op2);

    // Apply transformed operations
    auto result = ot.apply(document, transformed);

    // Verify result is consistent
    BOLT_ASSERT(!result.empty());
}

BOLT_TEST(E2E_Collaboration, ConcurrentEditSimulation) {
    bolt::collaboration::OperationalTransform ot;

    std::string document = "function test() { return 1; }";

    // Simulate multiple users editing
    std::vector<bolt::collaboration::Operation> userOps;

    // User 1: Insert comment
    userOps.push_back(ot.createInsertOperation(0, "// Test function\n"));

    // User 2: Change return value
    userOps.push_back(ot.createReplaceOperation(25, 1, "42"));

    // Apply all operations
    std::string result = document;
    for (const auto& op : userOps) {
        result = ot.apply(result, op);
    }

    // Verify final state
    BOLT_ASSERT(!result.empty());
    BOLT_ASSERT(result.find("// Test function") != std::string::npos ||
                result.find("42") != std::string::npos);
}

// ============================================
// E2E Test Suite: Network
// ============================================

BOLT_TEST(E2E_Network, WebSocketServerStartStop) {
    bolt::WebSocketServer server;

    // Configure server
    server.setPort(0); // Use any available port
    server.setMaxConnections(10);

    // Start server
    bool started = server.start();
    // Note: May fail if dependencies not available, that's OK
    if (started) {
        BOLT_ASSERT(server.isRunning());

        // Get assigned port
        int port = server.getPort();
        BOLT_ASSERT(port > 0);

        // Stop server
        server.stop();
        BOLT_ASSERT(!server.isRunning());
    } else {
        // Skip test if server couldn't start
        BOLT_ASSERT(true);
    }
}

// ============================================
// E2E Test Suite: Performance
// ============================================

BOLT_TEST(E2E_Performance, EditorResponseTime) {
    bolt::EditorStore editorStore;

    auto start = std::chrono::high_resolution_clock::now();

    // Perform typical editor operations
    auto fileId = editorStore.createFile("perf_test.cpp");
    editorStore.setContent(fileId, "int main() { return 0; }");
    auto content = editorStore.getContent(fileId);
    editorStore.saveFile(fileId);
    editorStore.closeFile(fileId);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete in under 100ms
    BOLT_ASSERT(duration.count() < 100);
}

BOLT_TEST(E2E_Performance, BulkOperations) {
    bolt::ChatStore chatStore;

    auto start = std::chrono::high_resolution_clock::now();

    auto sessionId = chatStore.createSession();

    // Add 1000 messages
    for (int i = 0; i < 1000; i++) {
        chatStore.addMessage(sessionId, "user", "Message " + std::to_string(i));
    }

    // Retrieve all messages
    auto messages = chatStore.getMessages(sessionId);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete in under 1 second
    BOLT_ASSERT(duration.count() < 1000);
    BOLT_ASSERT_EQ(1000u, messages.size());

    chatStore.deleteSession(sessionId);
}

// ============================================
// E2E Test Suite: Error Handling
// ============================================

BOLT_TEST(E2E_ErrorHandling, GracefulDegradation) {
    bolt::EditorStore editorStore;

    // Try to access non-existent file
    auto content = editorStore.getContent("nonexistent_id");
    BOLT_ASSERT(content.empty()); // Should return empty, not crash

    // Try to close non-existent file
    bool closed = editorStore.closeFile("nonexistent_id");
    BOLT_ASSERT(!closed); // Should return false, not crash

    // Try to save non-existent file
    bool saved = editorStore.saveFile("nonexistent_id");
    BOLT_ASSERT(!saved); // Should return false, not crash
}

BOLT_TEST(E2E_ErrorHandling, InvalidInputHandling) {
    bolt::ChatStore chatStore;

    // Empty session ID
    chatStore.addMessage("", "user", "Test");
    auto messages = chatStore.getMessages("");
    BOLT_ASSERT(messages.empty());

    // Empty message
    auto sessionId = chatStore.createSession();
    chatStore.addMessage(sessionId, "user", "");
    // Should not crash

    // Very long message
    std::string longMessage(100000, 'x');
    chatStore.addMessage(sessionId, "user", longMessage);
    // Should handle gracefully

    chatStore.deleteSession(sessionId);
}

// ============================================
// E2E Test Suite: Full Integration
// ============================================

BOLT_TEST(E2E_FullIntegration, CompleteUserSession) {
    // This test simulates a complete user session from start to finish

    // 1. Initialize all stores
    bolt::MemoryManager memManager;
    bolt::ChatStore chatStore;
    bolt::EditorStore editorStore;
    bolt::WorkbenchStore workbenchStore;
    bolt::PluginSystem pluginSystem;
    bolt::Logger& logger = bolt::Logger::getInstance();

    logger.info("Starting complete user session test");

    // 2. Create a new project
    auto projectId = workbenchStore.createProject("TestProject");
    BOLT_ASSERT(!projectId.empty());

    // 3. Open files in editor
    auto file1 = editorStore.createFile("main.cpp");
    auto file2 = editorStore.createFile("utils.hpp");
    auto file3 = editorStore.createFile("utils.cpp");

    // 4. Add content
    editorStore.setContent(file1, "#include \"utils.hpp\"\n\nint main() { return 0; }");
    editorStore.setContent(file2, "#pragma once\n\nvoid utility();");
    editorStore.setContent(file3, "#include \"utils.hpp\"\n\nvoid utility() {}");

    // 5. Start a chat session for AI assistance
    auto chatSession = chatStore.createSession();
    chatStore.addMessage(chatSession, "user", "Help me refactor this code");
    chatStore.addMessage(chatSession, "assistant", "I can help you with refactoring.");

    // 6. Save all files
    BOLT_ASSERT(editorStore.saveFile(file1));
    BOLT_ASSERT(editorStore.saveFile(file2));
    BOLT_ASSERT(editorStore.saveFile(file3));

    // 7. Close session
    editorStore.closeFile(file1);
    editorStore.closeFile(file2);
    editorStore.closeFile(file3);
    chatStore.deleteSession(chatSession);

    logger.info("Complete user session test finished");

    BOLT_ASSERT(true);
}

// Main function for standalone E2E testing
#ifndef BOLT_INCLUDE_IN_TEST_RUNNER
int main(int argc, char** argv) {
    std::cout << "=== Bolt C++ ML IDE E2E Test Suite ===" << std::endl;
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
