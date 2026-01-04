/*
 * Example: Real-time Collaborative Editor Client
 * 
 * This example demonstrates how to integrate collaborative editing
 * into a client application using the Bolt C++ collaborative editing system.
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include "bolt/collaboration/collaborative_editor_integration.hpp"
#include "bolt/core/editor_store.hpp"

using namespace bolt::collaboration;
using namespace std::chrono_literals;

class SimpleCollaborativeEditor {
private:
    std::string userId_;
    std::string currentFile_;
    std::atomic<bool> running_;
    CollaborativeEditorIntegration& collaboration_;
    bolt::EditorStore& editorStore_;

public:
    SimpleCollaborativeEditor(const std::string& userId) 
        : userId_(userId), running_(false),
          collaboration_(CollaborativeEditorIntegration::getInstance()),
          editorStore_(bolt::EditorStore::getInstance()) {
    }

    void start() {
        std::cout << "Starting collaborative editor for user: " << userId_ << "\n";
        
        // Initialize collaboration system (starts WebSocket server)
        collaboration_.initialize(8081);
        
        // Create and open a sample document
        createSampleDocument();
        
        // Enable collaboration for the document
        if (collaboration_.enableCollaboration(currentFile_, userId_)) {
            std::cout << "✓ Collaboration enabled for " << currentFile_ << "\n";
        } else {
            std::cout << "✗ Failed to enable collaboration\n";
            return;
        }
        
        running_ = true;
        
        // Start the editor loop
        runEditorLoop();
    }
    
    void stop() {
        running_ = false;
        collaboration_.shutdown();
        std::cout << "Collaborative editor stopped.\n";
    }

private:
    void createSampleDocument() {
        currentFile_ = "/shared/example.cpp";
        
        bolt::EditorDocument doc;
        doc.filePath = currentFile_;
        doc.value = R"(#include <iostream>
#include <vector>
#include <string>

class CollaborativeExample {
private:
    std::vector<std::string> data_;
    
public:
    void processData() {
        // Process data by collecting example information
        data_.push_back("Processing collaborative editing features");
        data_.push_back("Supporting multi-user document editing");
        data_.push_back("Providing real-time synchronization");
        
        // Simulate data processing
        for (auto& item : data_) {
            // Transform data (example: convert to uppercase first char)
            if (!item.empty()) {
                item[0] = std::toupper(item[0]);
            }
        }
    }
    
    void displayResults() {
        std::cout << "Results: " << std::endl;
        for (const auto& item : data_) {
            std::cout << "  - " << item << std::endl;
        }
    }
};

int main() {
    CollaborativeExample app;
    app.processData();
    app.displayResults();
    return 0;
})";
        doc.scroll = {0, 0};
        doc.cursor = {0, std::nullopt};
        
        editorStore_.setDocument(currentFile_, doc);
        editorStore_.setSelectedFile(currentFile_);
        
        std::cout << "Created sample document: " << currentFile_ << "\n";
    }
    
    void runEditorLoop() {
        std::cout << "\n=== Collaborative Editor Commands ===\n";
        std::cout << "Commands:\n";
        std::cout << "  insert <line> <col> <text>  - Insert text at position\n";
        std::cout << "  delete <line> <col> <text>  - Delete text at position\n";
        std::cout << "  cursor <line> <col>         - Move cursor to position\n";
        std::cout << "  show                        - Show current document\n";
        std::cout << "  users                       - Show active users\n";
        std::cout << "  cursors                     - Show remote cursors\n";
        std::cout << "  stats                       - Show collaboration stats\n";
        std::cout << "  quit                        - Exit editor\n";
        std::cout << "=====================================\n\n";
        
        std::string command;
        while (running_ && std::cout << userId_ << "> " && std::getline(std::cin, command)) {
            if (command.empty()) continue;
            
            if (!processCommand(command)) {
                break;
            }
        }
    }
    
    bool processCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "quit" || cmd == "exit") {
            return false;
        }
        else if (cmd == "insert") {
            handleInsertCommand(iss);
        }
        else if (cmd == "delete") {
            handleDeleteCommand(iss);
        }
        else if (cmd == "cursor") {
            handleCursorCommand(iss);
        }
        else if (cmd == "show") {
            showDocument();
        }
        else if (cmd == "users") {
            showActiveUsers();
        }
        else if (cmd == "cursors") {
            showRemoteCursors();
        }
        else if (cmd == "stats") {
            showStats();
        }
        else if (cmd == "help") {
            std::cout << "Available commands: insert, delete, cursor, show, users, cursors, stats, quit\n";
        }
        else {
            std::cout << "Unknown command: " << cmd << " (type 'help' for commands)\n";
        }
        
        return true;
    }
    
    void handleInsertCommand(std::istringstream& iss) {
        int line, col;
        std::string text;
        
        if (!(iss >> line >> col)) {
            std::cout << "Usage: insert <line> <col> <text>\n";
            return;
        }
        
        // Read the rest as text (including spaces)
        std::string remaining;
        std::getline(iss, remaining);
        text = remaining.substr(remaining.find_first_not_of(" \t"));
        
        if (text.empty()) {
            std::cout << "Usage: insert <line> <col> <text>\n";
            return;
        }
        
        Position pos(line, col);
        bool success = collaboration_.applyTextEdit(currentFile_, pos, text, userId_);
        
        if (success) {
            std::cout << "✓ Inserted \"" << text << "\" at (" << line << "," << col << ")\n";
        } else {
            std::cout << "✗ Failed to insert text\n";
        }
    }
    
    void handleDeleteCommand(std::istringstream& iss) {
        int line, col;
        std::string text;
        
        if (!(iss >> line >> col >> text)) {
            std::cout << "Usage: delete <line> <col> <text>\n";
            return;
        }
        
        Position pos(line, col);
        bool success = collaboration_.applyTextEdit(currentFile_, pos, text, userId_, true);
        
        if (success) {
            std::cout << "✓ Deleted \"" << text << "\" at (" << line << "," << col << ")\n";
        } else {
            std::cout << "✗ Failed to delete text\n";
        }
    }
    
    void handleCursorCommand(std::istringstream& iss) {
        int line, col;
        
        if (!(iss >> line >> col)) {
            std::cout << "Usage: cursor <line> <col>\n";
            return;
        }
        
        Position pos(line, col);
        bool success = collaboration_.updateCursorPosition(currentFile_, pos, userId_);
        
        if (success) {
            std::cout << "✓ Moved cursor to (" << line << "," << col << ")\n";
        } else {
            std::cout << "✗ Failed to move cursor\n";
        }
    }
    
    void showDocument() {
        auto* doc = editorStore_.getCurrentDocument();
        if (doc) {
            std::cout << "\n=== Current Document: " << doc->filePath << " ===\n";
            
            // Split content into lines and add line numbers
            std::istringstream iss(doc->value);
            std::string line;
            int lineNum = 0;
            
            while (std::getline(iss, line)) {
                std::cout << std::setw(3) << lineNum << ": " << line << "\n";
                lineNum++;
            }
            std::cout << "===================================\n\n";
        } else {
            std::cout << "No document loaded\n";
        }
    }
    
    void showActiveUsers() {
        // Note: This would require accessing the collaborative session directly
        // For this example, we'll show collaboration status
        auto stats = collaboration_.getStats();
        std::cout << "Collaboration Status:\n";
        std::cout << "  Server Running: " << (stats.isServerRunning ? "Yes" : "No") << "\n";
        std::cout << "  Active Documents: " << stats.activeDocuments << "\n";
        std::cout << "  Total Users: " << stats.totalUsers << "\n";
    }
    
    void showRemoteCursors() {
        auto cursors = collaboration_.getRemoteCursors(currentFile_);
        
        if (cursors.empty()) {
            std::cout << "No remote cursors\n";
        } else {
            std::cout << "Remote Cursors:\n";
            for (const auto& [userId, position] : cursors) {
                std::cout << "  " << userId << ": line " << position.line 
                         << ", column " << position.character << "\n";
            }
        }
    }
    
    void showStats() {
        auto stats = collaboration_.getStats();
        std::cout << "Collaboration Statistics:\n";
        std::cout << "  Server Running: " << (stats.isServerRunning ? "Yes" : "No") << "\n";
        std::cout << "  Active Documents: " << stats.activeDocuments << "\n";
        std::cout << "  Total Users: " << stats.totalUsers << "\n";
        std::cout << "  Current User: " << userId_ << "\n";
        std::cout << "  Current File: " << currentFile_ << "\n";
        std::cout << "  Collaboration Enabled: " 
                 << (collaboration_.isCollaborationEnabled(currentFile_) ? "Yes" : "No") << "\n";
    }
};

int main(int argc, char* argv[]) {
    std::string userId = "user1";
    
    // Allow user to specify user ID
    if (argc > 1) {
        userId = argv[1];
    }
    
    std::cout << "Bolt C++ Collaborative Editor Example\n";
    std::cout << "=====================================\n\n";
    
    try {
        SimpleCollaborativeEditor editor(userId);
        
        // Start the editor
        editor.start();
        
        // Cleanup
        editor.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

/*
 * Usage Examples:
 * 
 * 1. Start the editor:
 *    ./collaborative_editor_example alice
 * 
 * 2. In another terminal:
 *    ./collaborative_editor_example bob
 * 
 * 3. Try commands like:
 *    alice> insert 5 4 "// Alice was here"
 *    bob> cursor 10 0
 *    alice> show
 *    bob> users
 *    alice> cursors
 * 
 * Note: This example shows the API usage but requires full WebSocket
 * integration for true real-time collaboration between multiple instances.
 */