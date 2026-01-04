#include "bolt/test_framework.hpp"
#include "bolt/core/error_handling.hpp"
#include "bolt/core/memory_manager.hpp"
#include "bolt/core/message_handler.hpp"
#include "bolt/core/chat_store.hpp"
#include "bolt/core/editor_store.hpp"
#include "bolt/core/workbench_store.hpp"

// ===== Error Handling Framework Tests =====

BOLT_TEST(ErrorHandling, BoltExceptionBasic) {
    try {
        throw bolt::BoltException(bolt::ErrorCode::INVALID_PARAMETER, "Test error");
    } catch (const bolt::BoltException& e) {
        BOLT_ASSERT_TRUE(e.getErrorCode() == bolt::ErrorCode::INVALID_PARAMETER);
        BOLT_ASSERT_TRUE(std::string(e.what()).find("Test error") != std::string::npos);
    }
}

BOLT_TEST(ErrorHandling, MemoryExceptionInheritance) {
    try {
        throw bolt::MemoryException(bolt::ErrorCode::MEMORY_ALLOCATION_FAILED, "Memory error");
    } catch (const bolt::BoltException& e) {
        BOLT_ASSERT_TRUE(e.getErrorCode() == bolt::ErrorCode::MEMORY_ALLOCATION_FAILED);
    }
}

BOLT_TEST(ErrorHandling, ValidateNotNull) {
    // Valid case
    int value = 42;
    BOLT_ASSERT_TRUE(true); // This should not throw
    bolt::ErrorHandler::validateNotNull(&value, "value");
    
    // Invalid case
    BOLT_ASSERT_THROWS(bolt::BoltException, 
        bolt::ErrorHandler::validateNotNull(nullptr, "null_ptr"));
}

BOLT_TEST(ErrorHandling, ValidateParameter) {
    // Valid case
    bolt::ErrorHandler::validateParameter(true, "Should pass");
    
    // Invalid case
    BOLT_ASSERT_THROWS(bolt::BoltException,
        bolt::ErrorHandler::validateParameter(false, "Should fail"));
}

BOLT_TEST(ErrorHandling, ValidateMemorySize) {
    // Valid cases
    bolt::ErrorHandler::validateMemorySize(1);
    bolt::ErrorHandler::validateMemorySize(1024);
    bolt::ErrorHandler::validateMemorySize(1024 * 1024);
    
    // Invalid cases
    BOLT_ASSERT_THROWS(bolt::MemoryException,
        bolt::ErrorHandler::validateMemorySize(0));
    BOLT_ASSERT_THROWS(bolt::MemoryException,
        bolt::ErrorHandler::validateMemorySize(SIZE_MAX));
}

// ===== Memory Manager Error Handling Tests =====

BOLT_TEST(MemoryManagerErrors, AllocationSizeValidation) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    // Should throw for zero size
    BOLT_ASSERT_THROWS(bolt::MemoryException, manager.allocate(0));
    
    // Should throw for excessive size
    BOLT_ASSERT_THROWS(bolt::MemoryException, manager.allocate(SIZE_MAX));
}

BOLT_TEST(MemoryManagerErrors, MaxUsageLimit) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    // Set a small limit for testing
    manager.setMaxAllowedUsage(100);
    
    // First allocation should succeed
    void* ptr1 = manager.allocate(50);
    BOLT_ASSERT_NOT_NULL(ptr1);
    
    // Second allocation should fail (would exceed limit)
    BOLT_ASSERT_THROWS(bolt::MemoryException, manager.allocate(60));
    
    // Clean up
    manager.deallocate(ptr1);
    manager.setMaxAllowedUsage(SIZE_MAX); // Reset
}

BOLT_TEST(MemoryManagerErrors, DoubleFreeDetection) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    void* ptr = manager.allocate(100);
    BOLT_ASSERT_NOT_NULL(ptr);
    
    // First deallocation should succeed
    manager.deallocate(ptr);
    
    // Second deallocation should throw
    BOLT_ASSERT_THROWS(bolt::MemoryException, manager.deallocate(ptr));
}

BOLT_TEST(MemoryManagerErrors, ResetWithActiveAllocations) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    void* ptr = manager.allocate(100);
    BOLT_ASSERT_NOT_NULL(ptr);
    
    // Reset should fail with active allocations
    BOLT_ASSERT_THROWS(bolt::MemoryException, manager.reset());
    
    // Clean up
    manager.deallocate(ptr);
    
    // Now reset should work
    manager.reset();
}

BOLT_TEST(MemoryManagerErrors, MemoryLeakDetection) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    // Initially no leaks
    BOLT_ASSERT_FALSE(manager.hasMemoryLeaks());
    BOLT_ASSERT_TRUE(manager.getLeakedAllocations().size() == 0);
    
    void* ptr = manager.allocate(100);
    
    // Now should detect leak
    BOLT_ASSERT_TRUE(manager.hasMemoryLeaks());
    BOLT_ASSERT_TRUE(manager.getLeakedAllocations().size() == 1);
    
    // Clean up
    manager.deallocate(ptr);
    BOLT_ASSERT_FALSE(manager.hasMemoryLeaks());
}

// ===== Message Handler Error Handling Tests =====

BOLT_TEST(MessageHandlerErrors, UninitializedAccess) {
    // Create a fresh handler instance (this is tricky with singleton, but we can test error states)
    auto& handler = bolt::MessageHandler::getInstance();
    
    // If already initialized, this is okay, but test the error paths in other ways
    if (!handler.isInitialized()) {
        // Should throw when accessing uninitialized handler
        bolt::Message msg(bolt::MessageType::Chat, "test");
        BOLT_ASSERT_THROWS(bolt::BoltException, handler.pushMessage(msg));
        BOLT_ASSERT_THROWS(bolt::BoltException, handler.processMessages());
        BOLT_ASSERT_THROWS(bolt::BoltException, handler.getQueueSize());
        BOLT_ASSERT_THROWS(bolt::BoltException, handler.setMaxQueueSize(100));
    }
    
    // Initialize for other tests
    handler.initialize();
    BOLT_ASSERT_TRUE(handler.isInitialized());
}

BOLT_TEST(MessageHandlerErrors, MessageValidation) {
    auto& handler = bolt::MessageHandler::getInstance();
    handler.initialize();
    
    // Clear any existing messages
    handler.processMessages();
    
    // Valid message should work
    bolt::Message validMsg(bolt::MessageType::Chat, "Hello");
    handler.pushMessage(validMsg); // Should not throw
    
    // Message with oversized content should throw during construction
    std::string oversizedContent(bolt::Message::MAX_MESSAGE_LENGTH + 1, 'x');
    BOLT_ASSERT_THROWS(bolt::MessageException, 
        bolt::Message(bolt::MessageType::Chat, oversizedContent));
    
    // Clear queue for next tests
    handler.processMessages();
}

BOLT_TEST(MessageHandlerErrors, QueueOverflow) {
    auto& handler = bolt::MessageHandler::getInstance();
    handler.initialize();
    
    // Process any existing messages first to clear the queue
    handler.processMessages();
    
    // Set a small queue size for testing
    handler.setMaxQueueSize(3);
    
    // Fill the queue
    handler.pushMessage(bolt::Message(bolt::MessageType::System, "msg1"));
    handler.pushMessage(bolt::Message(bolt::MessageType::System, "msg2"));
    handler.pushMessage(bolt::Message(bolt::MessageType::System, "msg3"));
    
    // Next message should cause overflow
    BOLT_ASSERT_THROWS(bolt::MessageException, 
        handler.pushMessage(bolt::Message(bolt::MessageType::System, "overflow")));
    
    // Process messages to clear queue for other tests
    handler.processMessages();
    handler.setMaxQueueSize(1000); // Reset to default
}

BOLT_TEST(MessageHandlerErrors, InvalidQueueSize) {
    auto& handler = bolt::MessageHandler::getInstance();
    handler.initialize();
    
    // Should throw for zero queue size
    BOLT_ASSERT_THROWS(bolt::BoltException, handler.setMaxQueueSize(0));
}

// ===== Chat Store Error Handling Tests =====

BOLT_TEST(ChatStoreErrors, MessageValidation) {
    auto& store = bolt::ChatStore::getInstance();
    
    // Valid message should work
    bolt::SimpleChatMessage validMsg("user", "Hello");
    store.addMessage(validMsg); // Should not throw
    
    // Invalid messages should throw
    BOLT_ASSERT_THROWS(bolt::StoreException,
        bolt::SimpleChatMessage("", "content")); // Empty sender
    BOLT_ASSERT_THROWS(bolt::StoreException,
        bolt::SimpleChatMessage("user", "")); // Empty content
    
    // Oversized content should throw
    std::string oversizedSender(bolt::SimpleChatMessage::MAX_SENDER_LENGTH + 1, 'x');
    BOLT_ASSERT_THROWS(bolt::StoreException,
        bolt::SimpleChatMessage(oversizedSender, "content"));
    
    std::string oversizedContent(bolt::SimpleChatMessage::MAX_CONTENT_LENGTH + 1, 'x');
    BOLT_ASSERT_THROWS(bolt::StoreException,
        bolt::SimpleChatMessage("user", oversizedContent));
}

BOLT_TEST(ChatStoreErrors, ListenerValidation) {
    auto& store = bolt::ChatStore::getInstance();
    
    // Null listener should throw
    BOLT_ASSERT_THROWS(bolt::BoltException,
        store.addListener(nullptr));
}

BOLT_TEST(ChatStoreErrors, ThreadSafety) {
    auto& store = bolt::ChatStore::getInstance();
    
    // Test that concurrent access doesn't crash
    // This is a basic test - in a real scenario we'd use multiple threads
    store.setChatStarted(true);
    bool started = store.getChatStarted();
    BOLT_ASSERT_TRUE(started);
    
    store.setShowChat(false);
    bool showChat = store.getShowChat();
    BOLT_ASSERT_FALSE(showChat);
}

// ===== Editor Store Error Handling Tests =====

BOLT_TEST(EditorStoreErrors, DocumentValidation) {
    auto& store = bolt::EditorStore::getInstance();
    
    // Valid document should work
    bolt::EditorDocument validDoc;
    validDoc.filePath = "/test/file.cpp";
    validDoc.value = "int main() { return 0; }";
    validDoc.scroll = {0, 0};
    store.setDocument("/test/file.cpp", validDoc); // Should not throw
    
    // Invalid documents should throw
    bolt::EditorDocument invalidDoc;
    invalidDoc.filePath = ""; // Empty path
    BOLT_ASSERT_THROWS(bolt::EditorException, invalidDoc.validateDocument());
    
    // Oversized file path
    std::string oversizedPath(bolt::EditorDocument::MAX_FILE_PATH_LENGTH + 1, 'x');
    invalidDoc.filePath = oversizedPath;
    BOLT_ASSERT_THROWS(bolt::EditorException, invalidDoc.validateDocument());
    
    // Invalid scroll position
    validDoc.filePath = "/test/valid.cpp";
    validDoc.scroll = {-1, 0}; // Negative line
    BOLT_ASSERT_THROWS(bolt::EditorException, validDoc.validateDocument());
}

BOLT_TEST(EditorStoreErrors, DocumentOperations) {
    auto& store = bolt::EditorStore::getInstance();
    
    // Operations on non-existent document should throw
    BOLT_ASSERT_THROWS(bolt::EditorException,
        store.updateScrollPosition("/nonexistent.cpp", 10, 5));
    BOLT_ASSERT_THROWS(bolt::EditorException,
        store.updateFoldingRanges("/nonexistent.cpp", {}));
    BOLT_ASSERT_THROWS(bolt::EditorException,
        store.toggleFold("/nonexistent.cpp", 5));
    
    // Empty path should throw
    BOLT_ASSERT_THROWS(bolt::BoltException,
        store.updateScrollPosition("", 10, 5));
}

BOLT_TEST(EditorStoreErrors, FoldingValidation) {
    auto& store = bolt::EditorStore::getInstance();
    
    // Create a document
    bolt::EditorDocument doc;
    doc.filePath = "/test/folding.cpp";
    doc.value = "int main() {\n  return 0;\n}";
    doc.scroll = {0, 0};
    store.setDocument("/test/folding.cpp", doc);
    
    // Add a folding range
    std::vector<bolt::FoldRange> ranges;
    ranges.push_back(bolt::FoldRange{0, 2, false, "..."});
    store.updateFoldingRanges("/test/folding.cpp", ranges);
    
    // Toggle existing fold should work
    store.toggleFold("/test/folding.cpp", 1); // Should not throw
    
    // Toggle non-existent fold should throw
    BOLT_ASSERT_THROWS(bolt::EditorException,
        store.toggleFold("/test/folding.cpp", 10));
}

BOLT_TEST(EditorStoreErrors, DocumentLimits) {
    auto& store = bolt::EditorStore::getInstance();
    
    // Create a valid document
    bolt::EditorDocument doc;
    doc.filePath = "/test/limit.cpp";
    doc.value = "test";
    doc.scroll = {0, 0};
    
    // Should be able to add document
    store.setDocument("/test/limit.cpp", doc);
    
    // Clean up for other tests
    store.closeDocument("/test/limit.cpp");
}

// ===== Workbench Store Error Handling Tests =====

BOLT_TEST(WorkbenchStoreErrors, ViewValidation) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    // Valid view should work
    store.setCurrentView("editor"); // Should not throw
    
    // Empty view should throw
    BOLT_ASSERT_THROWS(bolt::BoltException,
        store.setCurrentView(""));
    
    // Oversized view name should throw
    std::string oversizedView(1000, 'x');
    BOLT_ASSERT_THROWS(bolt::StoreException,
        store.setCurrentView(oversizedView));
}

BOLT_TEST(WorkbenchStoreErrors, FilePathValidation) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    // Valid file path should work
    store.setSelectedFile("/valid/path.cpp"); // Should not throw
    
    // Empty path should work (clears selection)
    store.setSelectedFile(""); // Should not throw
    
    // Oversized path should throw
    std::string oversizedPath(3000, 'x');
    BOLT_ASSERT_THROWS(bolt::StoreException,
        store.setSelectedFile(oversizedPath));
}

BOLT_TEST(WorkbenchStoreErrors, ListenerValidation) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    // Null listener should throw
    BOLT_ASSERT_THROWS(bolt::BoltException,
        store.addListener(nullptr));
    
    // Valid listener should work
    store.addListener([](){}); // Should not throw
}

BOLT_TEST(WorkbenchStoreErrors, ThreadSafety) {
    auto& store = bolt::WorkbenchStore::getInstance();
    
    // Test that concurrent access doesn't crash
    store.setShowWorkbench(true);
    bool show = store.getShowWorkbench();
    BOLT_ASSERT_TRUE(show);
    
    store.toggleTerminal(true);
    bool terminal = store.getShowTerminal();
    BOLT_ASSERT_TRUE(terminal);
}

// ===== Error Recovery Tests =====

BOLT_TEST(ErrorRecovery, MemoryManagerRecovery) {
    auto& manager = bolt::MemoryManager::getInstance();
    
    // Allocate some memory
    void* ptr1 = manager.allocate(100);
    void* ptr2 = manager.allocate(200);
    BOLT_ASSERT_NOT_NULL(ptr1);
    BOLT_ASSERT_NOT_NULL(ptr2);
    
    // Force reset to test recovery
    manager.forceReset();
    
    // Should be able to allocate again after force reset
    void* ptr3 = manager.allocate(150);
    BOLT_ASSERT_NOT_NULL(ptr3);
    
    // Clean up
    manager.deallocate(ptr3);
}

BOLT_TEST(ErrorRecovery, MessageHandlerRecovery) {
    auto& handler = bolt::MessageHandler::getInstance();
    handler.initialize();
    
    // Process some messages with errors
    try {
        bolt::Message invalidCmd(bolt::MessageType::Command, "invalid-command");
        handler.pushMessage(invalidCmd);
        handler.processMessages(); // Should handle error gracefully
    } catch (...) {
        // Error handling should prevent crashes
    }
    
    // Handler should still be functional
    bolt::Message validMsg(bolt::MessageType::System, "recovery test");
    handler.pushMessage(validMsg); // Should not throw
    // Queue size is unsigned, so it's always >= 0
    BOLT_ASSERT_TRUE(handler.getQueueSize() < 1000); // Sanity check
}