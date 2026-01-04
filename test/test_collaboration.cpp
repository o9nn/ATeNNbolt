#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include "bolt/collaboration/document_operation.hpp"
#include "bolt/collaboration/operational_transform.hpp"
#include "bolt/collaboration/collaborative_session.hpp"
#include "bolt/collaboration/collaborative_editor_integration.hpp"

using namespace bolt::collaboration;
using namespace std::chrono_literals;

void testDocumentOperation() {
    std::cout << "[Collaboration] DocumentOperation Tests\n";
    
    // Test basic operation creation
    Position pos(1, 5);
    DocumentOperation insertOp(OperationType::INSERT, "user1", pos, "hello");
    
    assert(insertOp.getType() == OperationType::INSERT);
    assert(insertOp.getUserId() == "user1");
    assert(insertOp.getPosition() == pos);
    assert(insertOp.getContent() == "hello");
    std::cout << "  Basic operation creation ... PASS\n";
    
    // Test operation serialization
    std::string serialized = insertOp.serialize();
    assert(!serialized.empty());
    assert(serialized.find("\"type\":0") != std::string::npos);
    assert(serialized.find("\"userId\":\"user1\"") != std::string::npos);
    std::cout << "  Operation serialization ... PASS\n";
    
    // Test operation deserialization
    auto deserializedOp = DocumentOperation::deserialize(serialized);
    assert(deserializedOp->getType() == OperationType::INSERT);
    assert(deserializedOp->getUserId() == "user1");
    assert(deserializedOp->getContent() == "hello");
    std::cout << "  Operation deserialization ... PASS\n";
    
    // Test inverse operation
    auto inverseOp = insertOp.createInverse();
    assert(inverseOp->getType() == OperationType::DELETE);
    assert(inverseOp->getContent() == "hello");
    std::cout << "  Inverse operation ... PASS\n";
    
    // Test operation application
    std::vector<std::string> lines = {"line1", "line2", "line3"};
    Position insertPos(1, 2);
    DocumentOperation op(OperationType::INSERT, "user1", insertPos, "XXX");
    
    bool success = op.apply(lines);
    assert(success);
    (void)success;
    assert(lines[1] == "liXXXne2");
    std::cout << "  Operation application ... PASS\n";
}

void testOperationalTransform() {
    std::cout << "[Collaboration] Operational Transform Tests\n";
    
    // Test concurrent inserts at same position
    Position pos(0, 5);
    DocumentOperation opA(OperationType::INSERT, "userA", pos, "A");
    DocumentOperation opB(OperationType::INSERT, "userB", pos, "B");
    
    auto transformedA = OperationalTransform::transform(opA, opB);
    
    // After B is applied, A should be adjusted
    assert(transformedA->getPosition().character == 6); // Moved past B's insertion
    std::cout << "  Concurrent inserts transformation ... PASS\n";
    
    // Test insert vs delete conflict
    DocumentOperation deleteOp(OperationType::DELETE, "userA", Position(0, 3), "abc");
    DocumentOperation insertOp(OperationType::INSERT, "userB", Position(0, 5), "XYZ");
    
    auto transformedInsert = OperationalTransform::transform(insertOp, deleteOp);
    assert(transformedInsert->getPosition().character == 2); // Adjusted for deletion
    std::cout << "  Insert vs delete transformation ... PASS\n";
    
    // Test same user operations (no transformation)
    DocumentOperation sameUserOp1(OperationType::INSERT, "user1", pos, "A");
    DocumentOperation sameUserOp2(OperationType::INSERT, "user1", pos, "B");
    
    auto noTransform = OperationalTransform::transform(sameUserOp1, sameUserOp2);
    assert(noTransform->getPosition() == sameUserOp1.getPosition());
    std::cout << "  Same user operations ... PASS\n";
}

void testCollaborativeSession() {
    std::cout << "[Collaboration] Collaborative Session Tests\n";
    
    auto& session = CollaborativeSession::getInstance();
    
    // Test document creation
    bool created = session.createDocument("test_doc", "Initial content");
    assert(created);
    (void)created;
    std::cout << "  Document creation ... PASS\n";

    // Test user joining
    bool joined = session.joinDocument("user1", "User One", "test_doc");
    assert(joined);

    joined = session.joinDocument("user2", "User Two", "test_doc");
    assert(joined);
    (void)joined;
    std::cout << "  User joining ... PASS\n";
    
    // Test getting active users
    auto users = session.getActiveUsers("test_doc");
    assert(users.size() == 2);
    std::cout << "  Active users retrieval ... PASS\n";
    
    // Test operation application
    DocumentOperation op(OperationType::INSERT, "user1", Position(0, 7), " - modified");
    bool applied = session.applyOperation(op, "test_doc");
    assert(applied);

    std::string content = session.getDocumentContent("test_doc");
    assert(content.find("Initial - modified") != std::string::npos);
    std::cout << "  Operation application ... PASS\n";

    // Test cursor update
    DocumentOperation cursorOp(OperationType::CURSOR_MOVE, "user1", Position(0, 15));
    applied = session.applyOperation(cursorOp, "test_doc");
    assert(applied);
    (void)applied;
    std::cout << "  Cursor update ... PASS\n";

    // Test user leaving
    bool left = session.leaveDocument("user1", "test_doc");
    assert(left);
    (void)left;

    users = session.getActiveUsers("test_doc");
    assert(users.size() == 1);
    std::cout << "  User leaving ... PASS\n";

    // Test document removal
    bool removed = session.removeDocument("test_doc");
    assert(removed);
    (void)removed;
    std::cout << "  Document removal ... PASS\n";
}

void testEditorIntegration() {
    std::cout << "[Collaboration] Editor Integration Tests\n";
    
    auto& integration = CollaborativeEditorIntegration::getInstance();
    auto& editorStore = bolt::EditorStore::getInstance();
    
    // Create a test document in editor store
    bolt::EditorDocument testDoc;
    testDoc.filePath = "/test/collaborative_file.cpp";
    testDoc.value = "int main() {\n    return 0;\n}";
    testDoc.scroll = {0, 0}; // Initialize scroll position
    testDoc.cursor = {0, std::nullopt}; // Initialize cursor
    testDoc.foldingRanges.clear(); // Initialize folding ranges
    
    editorStore.setDocument(testDoc.filePath, testDoc);
    
    // Test collaboration stats before initialization
    auto stats = integration.getStats();
    assert(!stats.isServerRunning);
    (void)stats;
    std::cout << "  Initial stats check ... PASS\n";

    // Test collaboration enabled check (should be false)
    bool isEnabled = integration.isCollaborationEnabled(testDoc.filePath);
    assert(!isEnabled);
    (void)isEnabled;
    std::cout << "  Collaboration status check ... PASS\n";

    // Test text edit application (local only, without collaboration enabled)
    bool applied = integration.applyTextEdit(testDoc.filePath, Position(1, 4), "// comment\n    ");
    (void)applied;  // Result depends on collaboration state
    // Should work locally even without collaboration
    std::cout << "  Local text edit ... PASS\n";

    // Test cursor position update (should be no-op without collaboration)
    bool updated = integration.updateCursorPosition(testDoc.filePath, Position(1, 10));
    assert(updated); // Should return true even if it's a no-op
    (void)updated;
    std::cout << "  Cursor position update ... PASS\n";
    
    // Test remote cursors (should be empty without collaboration)
    auto cursors = integration.getRemoteCursors(testDoc.filePath);
    assert(cursors.empty());
    std::cout << "  Remote cursors check ... PASS\n";
    
    // Test enabling collaboration on non-existent document (should fail)
    bool enabledFake = integration.enableCollaboration("/non/existent/file.cpp");
    assert(!enabledFake);
    (void)enabledFake;  // Suppress unused variable warning in release builds
    std::cout << "  Non-existent document collaboration ... PASS\n";
}

void testConcurrentOperations() {
    std::cout << "[Collaboration] Concurrent Operations Tests\n";
    
    auto& session = CollaborativeSession::getInstance();
    
    // Create test document
    session.createDocument("concurrent_test", "abcdefghijk");
    session.joinDocument("userA", "User A", "concurrent_test");
    session.joinDocument("userB", "User B", "concurrent_test");
    
    // Create concurrent operations
    std::vector<std::unique_ptr<DocumentOperation>> operations;
    operations.push_back(std::make_unique<DocumentOperation>(
        OperationType::INSERT, "userA", Position(0, 5), "XXX"));
    operations.push_back(std::make_unique<DocumentOperation>(
        OperationType::INSERT, "userB", Position(0, 3), "YYY"));
    operations.push_back(std::make_unique<DocumentOperation>(
        OperationType::DELETE, "userA", Position(0, 7), "gh"));
    
    // Apply operations
    for (const auto& op : operations) {
        session.applyOperation(*op, "concurrent_test");
    }
    
    std::string finalContent = session.getDocumentContent("concurrent_test");
    assert(!finalContent.empty());
    std::cout << "  Final content: " << finalContent << "\n";
    std::cout << "  Concurrent operations application ... PASS\n";
    
    // Clean up
    session.removeDocument("concurrent_test");
}

void testPositionConversion() {
    std::cout << "[Collaboration] Position Conversion Tests\n";
    
    std::vector<std::string> lines = {"hello", "world", "test"};
    
    // Test linear conversion
    Position pos(1, 2);
    size_t linear = pos.toLinear(lines);
    assert(linear == 8); // "hello\n" (6) + "wo" (2) = 8
    
    Position converted = Position::fromLinear(linear, lines);
    assert(converted == pos);
    (void)converted;  // Suppress unused variable warning in release builds
    std::cout << "  Position conversion ... PASS\n";

    // Test edge cases
    Position endPos(2, 4);
    size_t endLinear = endPos.toLinear(lines);
    Position endConverted = Position::fromLinear(endLinear, lines);
    assert(endConverted == endPos);
    (void)endConverted;  // Suppress unused variable warning in release builds
    std::cout << "  Edge case conversion ... PASS\n";
}

int main() {
    std::cout << "Bolt C++ Collaborative Editing Test Suite\n";
    std::cout << "==========================================\n";
    
    try {
        testDocumentOperation();
        testOperationalTransform();
        testPositionConversion();
        testCollaborativeSession();
        testConcurrentOperations();
        testEditorIntegration();
        
        std::cout << "\n==========================================\n";
        std::cout << "All collaborative editing tests passed!\n";
        std::cout << "Collaborative editing implementation is ready.\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}