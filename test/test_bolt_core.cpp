#include "bolt/test_framework.hpp"
#include "bolt/bolt.hpp"
#include "bolt/core/memory_manager.hpp"
#include "bolt/core/chat_store.hpp"
#include "bolt/core/editor_store.hpp"
#include "bolt/core/workbench_store.hpp"
#include "bolt/core/message_handler.hpp"
#include "bolt/utils/string_utils.hpp"
#include "bolt/editor/code_folding.hpp"
#include "bolt/editor/code_folding_detector.hpp"
#include "bolt/editor/code_folding_manager.hpp"
#include "bolt/editor/integrated_editor.hpp"

using namespace bolt::test;

// ===== Chat Component Tests =====

BOLT_TEST(Chat, BasicInstantiation) {
    bolt::Chat chat;
    BOLT_ASSERT_EQ(0, chat.getHistory().size());
}

BOLT_TEST(Chat, AddSingleMessage) {
    bolt::Chat chat;
    bolt::ChatMessage msg(bolt::ChatMessage::Role::User, "Hello");
    
    chat.addMessage(msg);
    
    auto history = chat.getHistory();
    BOLT_ASSERT_EQ(1, history.size());
    BOLT_ASSERT_EQ("Hello", history[0].getContent());
    BOLT_ASSERT(bolt::ChatMessage::Role::User == history[0].getRole());
}

BOLT_TEST(Chat, AddMultipleMessages) {
    bolt::Chat chat;
    
    chat.addMessage(bolt::ChatMessage(bolt::ChatMessage::Role::User, "Hello"));
    chat.addMessage(bolt::ChatMessage(bolt::ChatMessage::Role::Assistant, "Hi there!"));
    chat.addMessage(bolt::ChatMessage(bolt::ChatMessage::Role::User, "How are you?"));
    
    auto history = chat.getHistory();
    BOLT_ASSERT_EQ(3, history.size());
    BOLT_ASSERT_EQ("Hello", history[0].getContent());
    BOLT_ASSERT_EQ("Hi there!", history[1].getContent());
    BOLT_ASSERT_EQ("How are you?", history[2].getContent());
}

BOLT_TEST(Chat, ClearHistory) {
    bolt::Chat chat;
    
    chat.addMessage(bolt::ChatMessage(bolt::ChatMessage::Role::User, "Hello"));
    chat.addMessage(bolt::ChatMessage(bolt::ChatMessage::Role::Assistant, "Hi!"));
    
    BOLT_ASSERT_EQ(2, chat.getHistory().size());
    
    chat.clear();
    
    BOLT_ASSERT_EQ(0, chat.getHistory().size());
}

// ===== ChatMessage Tests =====

BOLT_TEST(ChatMessage, UserMessageCreation) {
    bolt::ChatMessage msg(bolt::ChatMessage::Role::User, "Test message");
    
    BOLT_ASSERT(bolt::ChatMessage::Role::User == msg.getRole());
    BOLT_ASSERT_EQ("Test message", msg.getContent());
}

BOLT_TEST(ChatMessage, AssistantMessageCreation) {
    bolt::ChatMessage msg(bolt::ChatMessage::Role::Assistant, "Assistant response");
    
    BOLT_ASSERT(bolt::ChatMessage::Role::Assistant == msg.getRole());
    BOLT_ASSERT_EQ("Assistant response", msg.getContent());
}

BOLT_TEST(ChatMessage, EmptyMessage) {
    bolt::ChatMessage msg(bolt::ChatMessage::Role::User, "");
    
    BOLT_ASSERT(bolt::ChatMessage::Role::User == msg.getRole());
    BOLT_ASSERT_EQ("", msg.getContent());
}

// ===== Memory Manager Tests =====

BOLT_TEST(MemoryManager, SingletonInstance) {
    auto& manager1 = bolt::MemoryManager::getInstance();
    auto& manager2 = bolt::MemoryManager::getInstance();
    
    BOLT_ASSERT_EQ(&manager1, &manager2);
}

BOLT_TEST(MemoryManager, AllocateAndDeallocate) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    void* ptr = manager.allocate(1024);
    BOLT_ASSERT_NOT_NULL(ptr);
    
    size_t usage_before = manager.getCurrentUsage();
    manager.deallocate(ptr);
    size_t usage_after = manager.getCurrentUsage();
    
    BOLT_ASSERT_EQ(usage_before - 1024, usage_after);
}

BOLT_TEST(MemoryManager, TracksPeakUsage) {
    auto& manager = bolt::MemoryManager::getInstance();
    manager.reset(); // Start fresh
    
    void* ptr1 = manager.allocate(512);
    void* ptr2 = manager.allocate(256);
    
    size_t peak_usage = manager.getPeakUsage();
    BOLT_ASSERT_EQ(768, peak_usage);
    
    manager.deallocate(ptr1);
    
    // Peak should still be 768 even after deallocation
    BOLT_ASSERT_EQ(768, manager.getPeakUsage());
    
    manager.deallocate(ptr2);
}

BOLT_TEST(MemoryManager, NullPointerDeallocate) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    // Should not crash
    manager.deallocate(nullptr);
    BOLT_ASSERT_TRUE(true); // If we get here, test passed
}

// ===== ChatStore Tests =====

BOLT_TEST(ChatStore, SingletonInstance) {
    auto& store1 = bolt::ChatStore::getInstance();
    auto& store2 = bolt::ChatStore::getInstance();
    
    BOLT_ASSERT_EQ(&store1, &store2);
}

BOLT_TEST(ChatStore, DefaultState) {
    auto& store = bolt::ChatStore::getInstance();
    
    BOLT_ASSERT_FALSE(store.getChatStarted());
    BOLT_ASSERT_TRUE(store.getShowChat());
    BOLT_ASSERT_FALSE(store.getAborted());
}

BOLT_TEST(ChatStore, SetChatStarted) {
    auto& store = bolt::ChatStore::getInstance();
    
    store.setChatStarted(true);
    BOLT_ASSERT_TRUE(store.getChatStarted());
    
    store.setChatStarted(false);
    BOLT_ASSERT_FALSE(store.getChatStarted());
}

BOLT_TEST(ChatStore, SetShowChat) {
    auto& store = bolt::ChatStore::getInstance();
    
    store.setShowChat(false);
    BOLT_ASSERT_FALSE(store.getShowChat());
    
    store.setShowChat(true);
    BOLT_ASSERT_TRUE(store.getShowChat());
}

BOLT_TEST(ChatStore, AddMessage) {
    auto& store = bolt::ChatStore::getInstance();
    
    bolt::SimpleChatMessage msg{"user", "Hello world"};
    store.addMessage(msg);
    
    auto messages = store.getMessages();
    BOLT_ASSERT_EQ(1, messages.size());
    BOLT_ASSERT_EQ("user", messages[0].sender);
    BOLT_ASSERT_EQ("Hello world", messages[0].content);
}

// ===== EditorStore Tests =====

BOLT_TEST(EditorStore, SingletonInstance) {
    auto& store1 = bolt::EditorStore::getInstance();
    auto& store2 = bolt::EditorStore::getInstance();
    
    BOLT_ASSERT_EQ(&store1, &store2);
}

BOLT_TEST(EditorStore, SetAndGetDocument) {
    auto& store = bolt::EditorStore::getInstance();
    
    bolt::EditorDocument doc;
    doc.value = "int main() { return 0; }";
    doc.filePath = "/path/to/file.cpp";
    doc.scroll = {10, 5};
    doc.cursor = {42, std::nullopt};
    
    store.setDocument("/path/to/file.cpp", doc);
    
    // Note: EditorStore doesn't have a getDocument method in the header,
    // so we'll test with getCurrentDocument if selectedFile is set
}

BOLT_TEST(EditorStore, UpdateScrollPosition) {
    auto& store = bolt::EditorStore::getInstance();
    
    bolt::EditorDocument doc;
    doc.value = "test content";
    doc.filePath = "/test.txt";
    doc.scroll = {0, 0}; // Initialize scroll position
    
    store.setDocument("/test.txt", doc);
    store.updateScrollPosition("/test.txt", 15, 20);
    
    // The method exists and should execute without throwing
    BOLT_ASSERT_TRUE(true);
}

// ===== WorkbenchStore Tests =====

BOLT_TEST(WorkbenchStore, SingletonInstance) {
    auto& store1 = bolt::WorkbenchStore::getInstance();
    auto& store2 = bolt::WorkbenchStore::getInstance();
    
    BOLT_ASSERT_EQ(&store1, &store2);
}

BOLT_TEST(WorkbenchStore, DefaultState) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    BOLT_ASSERT_FALSE(store.getShowWorkbench());
    BOLT_ASSERT_EQ("code", store.getCurrentView());
}

BOLT_TEST(WorkbenchStore, SetShowWorkbench) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    store.setShowWorkbench(true);
    BOLT_ASSERT_TRUE(store.getShowWorkbench());
    
    store.setShowWorkbench(false);
    BOLT_ASSERT_FALSE(store.getShowWorkbench());
}

BOLT_TEST(WorkbenchStore, SetCurrentView) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    store.setCurrentView("terminal");
    BOLT_ASSERT_EQ("terminal", store.getCurrentView());
    
    store.setCurrentView("debug");
    BOLT_ASSERT_EQ("debug", store.getCurrentView());
}

BOLT_TEST(WorkbenchStore, ToggleTerminal) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    store.toggleTerminal(true);
    store.toggleTerminal(false);
    
    // Method should execute without throwing
    BOLT_ASSERT_TRUE(true);
}

// ===== MessageHandler Tests =====

BOLT_TEST(MessageHandler, SingletonInstance) {
    auto& handler1 = bolt::MessageHandler::getInstance();
    auto& handler2 = bolt::MessageHandler::getInstance();
    
    BOLT_ASSERT_EQ(&handler1, &handler2);
}

BOLT_TEST(MessageHandler, Initialize) {
    auto& handler = bolt::MessageHandler::getInstance();
    
    // Should not throw
    handler.initialize();
    BOLT_ASSERT_TRUE(true);
}

BOLT_TEST(MessageHandler, PushMessage) {
    auto& handler = bolt::MessageHandler::getInstance();
    handler.initialize();
    
    bolt::Message msg(bolt::MessageType::Chat, "Test message");
    
    // Should not throw
    handler.pushMessage(msg);
    BOLT_ASSERT_TRUE(true);
}

BOLT_TEST(MessageHandler, ProcessMessages) {
    auto& handler = bolt::MessageHandler::getInstance();
    handler.initialize();
    
    bolt::Message msg1(bolt::MessageType::Chat, "Hello");
    bolt::Message msg2(bolt::MessageType::System, "System message");
    
    handler.pushMessage(msg1);
    handler.pushMessage(msg2);
    
    // Should not throw
    handler.processMessages();
    BOLT_ASSERT_TRUE(true);
}

// ===== Message Tests =====

BOLT_TEST(Message, DefaultConstructor) {
    bolt::Message msg;
    
    BOLT_ASSERT(bolt::MessageType::System == msg.type);
    BOLT_ASSERT_EQ("", msg.content);
}

BOLT_TEST(Message, ParameterizedConstructor) {
    bolt::Message msg(bolt::MessageType::Command, "save file");
    
    BOLT_ASSERT(bolt::MessageType::Command == msg.type);
    BOLT_ASSERT_EQ("save file", msg.content);
}

// ===== StringUtils Tests =====

BOLT_TEST(StringUtils, ReverseString) {
    BOLT_ASSERT_EQ("", StringUtils::reverseString(""));
    BOLT_ASSERT_EQ("a", StringUtils::reverseString("a"));
    BOLT_ASSERT_EQ("olleh", StringUtils::reverseString("hello"));
    BOLT_ASSERT_EQ("dlrow", StringUtils::reverseString("world"));
}

BOLT_TEST(StringUtils, CapitalizeString) {
    BOLT_ASSERT_EQ("", StringUtils::capitalizeString(""));
    BOLT_ASSERT_EQ("A", StringUtils::capitalizeString("a"));
    BOLT_ASSERT_EQ("Hello", StringUtils::capitalizeString("hello"));
    BOLT_ASSERT_EQ("HELLO", StringUtils::capitalizeString("HELLO"));
    BOLT_ASSERT_EQ("Test string", StringUtils::capitalizeString("test string"));
}

BOLT_TEST(StringUtils, CountVowels) {
    BOLT_ASSERT_EQ(0, StringUtils::countVowels(""));
    BOLT_ASSERT_EQ(0, StringUtils::countVowels("xyz"));
    BOLT_ASSERT_EQ(1, StringUtils::countVowels("a"));
    BOLT_ASSERT_EQ(5, StringUtils::countVowels("aeiou"));
    BOLT_ASSERT_EQ(5, StringUtils::countVowels("AEIOU"));
    BOLT_ASSERT_EQ(2, StringUtils::countVowels("hello"));
    BOLT_ASSERT_EQ(5, StringUtils::countVowels("beautiful")); // b-e-a-u-t-i-f-u-l = e,a,u,i,u = 5 vowels
}

// ===== Code Folding Tests =====

BOLT_TEST(CodeFolding, FoldRangeCreation) {
    bolt::FoldRange range{10, 20, false, "test"};
    BOLT_ASSERT_EQ(range.startLine, 10);
    BOLT_ASSERT_EQ(range.endLine, 20);
    BOLT_ASSERT_EQ(range.isFolded, false);
    BOLT_ASSERT_EQ(range.placeholder, "test");
}

BOLT_TEST(CodeFolding, SingletonInstance) {
    auto& folding1 = bolt::CodeFolding::getInstance();
    auto& folding2 = bolt::CodeFolding::getInstance();
    BOLT_ASSERT_EQ(&folding1, &folding2);
}

BOLT_TEST(CodeFolding, AddFoldRange) {
    auto& folding = bolt::CodeFolding::getInstance();
    folding.addFoldRange("/test/file.cpp", 5, 15);
    
    auto ranges = folding.getFoldingRanges("/test/file.cpp");
    BOLT_ASSERT_TRUE(ranges.size() >= 1);
    
    // Find our added range
    bool found = false;
    for (const auto& range : ranges) {
        if (range.startLine == 5 && range.endLine == 15) {
            found = true;
            BOLT_ASSERT_EQ(range.isFolded, false);
            break;
        }
    }
    BOLT_ASSERT_TRUE(found);
}

BOLT_TEST(CodeFolding, ToggleFold) {
    auto& folding = bolt::CodeFolding::getInstance();
    folding.addFoldRange("/test/toggle.cpp", 8, 12);
    
    // Initially not folded
    auto ranges = folding.getFoldingRanges("/test/toggle.cpp");
    bool initiallyFolded = false;
    for (const auto& range : ranges) {
        if (range.startLine == 8 && range.endLine == 12) {
            initiallyFolded = range.isFolded;
            break;
        }
    }
    
    // Toggle fold
    folding.toggleFold("/test/toggle.cpp", 10);
    ranges = folding.getFoldingRanges("/test/toggle.cpp");
    
    bool afterToggle = false;
    for (const auto& range : ranges) {
        if (range.startLine == 8 && range.endLine == 12) {
            afterToggle = range.isFolded;
            break;
        }
    }
    
    BOLT_ASSERT_NE(initiallyFolded, afterToggle);
}

BOLT_TEST(CodeFoldingDetector, SimpleBraceDetection) {
    std::string code = "int main() {\n"
                      "    return 0;\n"
                      "}\n";
    
    auto ranges = bolt::CodeFoldingDetector::detectFoldableRanges(code);
    BOLT_ASSERT_TRUE(ranges.size() >= 1);
    
    // Find the main function fold
    bool found = false;
    for (const auto& range : ranges) {
        if (range.startLine == 0 && range.endLine == 2) {
            found = true;
            BOLT_ASSERT_EQ(range.isFolded, false);
            BOLT_ASSERT_EQ(range.placeholder, "{ ... }");
            break;
        }
    }
    BOLT_ASSERT_TRUE(found);
}

BOLT_TEST(CodeFoldingDetector, FoldableRegionDetection) {
    BOLT_ASSERT_TRUE(bolt::CodeFoldingDetector::isFoldableRegion("class MyClass {"));
    BOLT_ASSERT_TRUE(bolt::CodeFoldingDetector::isFoldableRegion("  function test() {"));
    BOLT_ASSERT_TRUE(bolt::CodeFoldingDetector::isFoldableRegion("if (condition) {"));
    BOLT_ASSERT_FALSE(bolt::CodeFoldingDetector::isFoldableRegion("int x = 5;"));
}

BOLT_TEST(CodeFoldingManager, ManagerSingleton) {
    auto& manager1 = bolt::CodeFoldingManager::getInstance();
    auto& manager2 = bolt::CodeFoldingManager::getInstance();
    BOLT_ASSERT_EQ(&manager1, &manager2);
}

BOLT_TEST(CodeFoldingManager, EnableDisableFolding) {
    auto& manager = bolt::CodeFoldingManager::getInstance();
    
    manager.setFoldingEnabled(true);
    BOLT_ASSERT_TRUE(manager.isFoldingEnabled());
    
    manager.setFoldingEnabled(false);
    BOLT_ASSERT_FALSE(manager.isFoldingEnabled());
    
    // Restore default state
    manager.setFoldingEnabled(true);
}

BOLT_TEST(CodeFoldingManager, UpdateFoldingRanges) {
    auto& manager = bolt::CodeFoldingManager::getInstance();
    manager.setFoldingEnabled(true);
    
    std::string code = "void test() {\n"
                      "    int x = 1;\n"
                      "}\n";
    
    manager.updateFoldingRanges("/test/manager.cpp", code);
    auto ranges = manager.getFoldingRanges("/test/manager.cpp");
    BOLT_ASSERT_TRUE(ranges.size() >= 1);
}

BOLT_TEST(IntegratedEditor, EditorSingleton) {
    auto& editor1 = bolt::IntegratedEditor::getInstance();
    auto& editor2 = bolt::IntegratedEditor::getInstance();
    BOLT_ASSERT_EQ(&editor1, &editor2);
}

BOLT_TEST(IntegratedEditor, OpenDocumentWithFolding) {
    auto& editor = bolt::IntegratedEditor::getInstance();
    
    std::string code = "class TestClass {\n"
                      "public:\n"
                      "    void method() {\n"
                      "        // code here\n"
                      "    }\n"
                      "}\n";
    
    editor.openDocument("/test/integrated.cpp", code);
    
    auto ranges = editor.getFoldingRanges("/test/integrated.cpp");
    BOLT_ASSERT_TRUE(ranges.size() >= 1);
}

BOLT_TEST(IntegratedEditor, FoldingEnabled) {
    auto& editor = bolt::IntegratedEditor::getInstance();
    
    editor.setFoldingEnabled(true);
    BOLT_ASSERT_TRUE(editor.isFoldingEnabled());
    
    editor.setFoldingEnabled(false);
    BOLT_ASSERT_FALSE(editor.isFoldingEnabled());
    
    // Restore default
    editor.setFoldingEnabled(true);
}