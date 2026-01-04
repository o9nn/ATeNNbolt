#ifndef COLLABORATIVE_EDITOR_INTEGRATION_HPP
#define COLLABORATIVE_EDITOR_INTEGRATION_HPP

#include "collaborative_session.hpp"
#include "collaboration_protocol.hpp"
#include "document_operation.hpp"
#include "../core/editor_store.hpp"
#include <string>
#include <functional>
#include <mutex>

// Windows compatibility: undefine conflicting macros
#ifdef _WIN32
#ifdef DELETE
#undef DELETE
#endif
#ifdef INSERT  
#undef INSERT
#endif
#endif

namespace bolt {
namespace collaboration {

class CollaborativeEditorIntegration {
public:
    static CollaborativeEditorIntegration& getInstance() {
        static CollaborativeEditorIntegration instance;
        return instance;
    }
    
    void initialize(int websocketPort = 8081) {
        // Initialize the collaboration protocol
        auto& protocol = CollaborationProtocol::getInstance();
        protocol.initialize(websocketPort);
        
        // Set up bidirectional synchronization
        setupEditorStoreListeners();
        setupCollaborationListeners();
        
        isInitialized_ = true;
    }
    
    void shutdown() {
        if (!isInitialized_) return;
        
        auto& protocol = CollaborationProtocol::getInstance();
        protocol.shutdown();
        
        isInitialized_ = false;
    }
    
    // Enable/disable collaboration for a document
    bool enableCollaboration(const std::string& filePath, const std::string& userId = "default_user") {
        if (!isInitialized_) {
            return false;
        }
        
        std::lock_guard<std::mutex> lock(collaborativeDocsMutex_);
        
        // Get current document content from EditorStore
        auto& editorStore = EditorStore::getInstance();
        if (!editorStore.hasDocument(filePath)) {
            return false;
        }
        
        // Create collaborative document
        auto& session = CollaborativeSession::getInstance();
        std::string docId = generateDocumentId(filePath);
        
        // Get current content and create collaborative document
        auto* currentDoc = editorStore.getCurrentDocument();
        std::string initialContent = currentDoc ? currentDoc->value : "";
        
        bool success = session.createDocument(docId, initialContent);
        if (success) {
            collaborativeDocs_[filePath] = docId;
            
            // Join the document as the current user
            session.joinDocument(userId, userId, docId);
            
            return true;
        }
        
        return false;
    }
    
    bool disableCollaboration(const std::string& filePath) {
        std::lock_guard<std::mutex> lock(collaborativeDocsMutex_);
        
        auto it = collaborativeDocs_.find(filePath);
        if (it == collaborativeDocs_.end()) {
            return false;
        }
        
        auto& session = CollaborativeSession::getInstance();
        session.removeDocument(it->second);
        collaborativeDocs_.erase(it);
        
        return true;
    }
    
    bool isCollaborationEnabled(const std::string& filePath) const {
        std::lock_guard<std::mutex> lock(collaborativeDocsMutex_);
        return collaborativeDocs_.find(filePath) != collaborativeDocs_.end();
    }
    
    // Apply a text edit to both editor store and collaborative session
    bool applyTextEdit(const std::string& filePath, const Position& position,
                      const std::string& text, const std::string& userId = "default_user",
                      bool isDelete = false) {
        
        if (!isCollaborationEnabled(filePath)) {
            // Just apply to editor store if collaboration is not enabled
            return applyToEditorStore(filePath, position, text, isDelete);
        }
        
        // Apply to collaborative session (which will broadcast to others)
        std::lock_guard<std::mutex> lock(collaborativeDocsMutex_);
        auto it = collaborativeDocs_.find(filePath);
        if (it == collaborativeDocs_.end()) {
            return false;
        }
        
        auto& session = CollaborativeSession::getInstance();
        
        OperationType opType = isDelete ? OperationType::DELETE : OperationType::INSERT;
        DocumentOperation op(opType, userId, position, text);
        
        return session.applyOperation(op, it->second);
    }
    
    // Update cursor position
    bool updateCursorPosition(const std::string& filePath, const Position& position,
                             const std::string& userId = "default_user") {
        if (!isCollaborationEnabled(filePath)) {
            return true; // No-op if collaboration is not enabled
        }
        
        std::lock_guard<std::mutex> lock(collaborativeDocsMutex_);
        auto it = collaborativeDocs_.find(filePath);
        if (it == collaborativeDocs_.end()) {
            return false;
        }
        
        auto& session = CollaborativeSession::getInstance();
        DocumentOperation cursorOp(OperationType::CURSOR_MOVE, userId, position);
        
        return session.applyOperation(cursorOp, it->second);
    }
    
    // Get remote users' cursor positions
    std::vector<std::pair<std::string, Position>> getRemoteCursors(const std::string& filePath) const {
        std::vector<std::pair<std::string, Position>> cursors;
        
        if (!isCollaborationEnabled(filePath)) {
            return cursors;
        }
        
        std::lock_guard<std::mutex> lock(collaborativeDocsMutex_);
        auto it = collaborativeDocs_.find(filePath);
        if (it == collaborativeDocs_.end()) {
            return cursors;
        }
        
        auto& session = CollaborativeSession::getInstance();
        auto users = session.getActiveUsers(it->second);
        
        for (const auto& user : users) {
            cursors.emplace_back(user.userId, user.cursorPosition);
        }
        
        return cursors;
    }
    
    // Get collaboration statistics
    struct CollaborationStats {
        size_t activeDocuments;
        size_t totalUsers;
        bool isServerRunning;
    };
    
    CollaborationStats getStats() const {
        CollaborationStats stats = {0, 0, isInitialized_};
        
        if (isInitialized_) {
            auto& session = CollaborativeSession::getInstance();
            auto sessionStats = session.getStats();
            stats.activeDocuments = sessionStats.totalDocuments;
            stats.totalUsers = sessionStats.totalActiveUsers;
        }
        
        return stats;
    }

private:
    CollaborativeEditorIntegration() : isInitialized_(false) {}
    
    void setupEditorStoreListeners() {
        // Note: EditorStore doesn't have change callbacks in current implementation
        // In a real implementation, we would add callbacks to EditorStore
        // and sync changes to collaborative session
    }
    
    void setupCollaborationListeners() {
        auto& session = CollaborativeSession::getInstance();
        
        // Listen for collaborative operations and apply them to EditorStore
        session.onOperation([this](const DocumentOperation& op, const std::string& docId) {
            // Find the file path for this document ID
            std::string filePath = findFilePathForDocId(docId);
            if (filePath.empty()) return;
            
            // Apply operation to EditorStore
            Position pos = op.getPosition();
            bool isDelete = (op.getType() == OperationType::DELETE);
            
            if (op.getType() == OperationType::INSERT || op.getType() == OperationType::DELETE) {
                applyToEditorStore(filePath, pos, op.getContent(), isDelete);
            }
        });
    }
    
    bool applyToEditorStore(const std::string& filePath, const Position& position,
                          const std::string& text, bool isDelete) {
        auto& editorStore = EditorStore::getInstance();
        
        if (!editorStore.hasDocument(filePath)) {
            return false;
        }
        
        // Get current document
        auto* doc = editorStore.getCurrentDocument();
        if (!doc || doc->filePath != filePath) {
            // Set the document as current if it's not
            editorStore.setSelectedFile(filePath);
            doc = editorStore.getCurrentDocument();
        }
        
        if (!doc) return false;
        
        // Convert document value to lines for easier manipulation
        std::vector<std::string> lines = splitLines(doc->value);
        
        // Apply the operation
        if (isDelete) {
            // Delete text
            if (position.line < lines.size()) {
                auto& line = lines[position.line];
                if (position.character < line.length()) {
                    size_t deleteLen = std::min(text.length(), line.length() - position.character);
                    line.erase(position.character, deleteLen);
                }
            }
        } else {
            // Insert text
            if (position.line >= lines.size()) {
                lines.resize(position.line + 1);
            }
            auto& line = lines[position.line];
            if (position.character > line.length()) {
                line.resize(position.character, ' ');
            }
            line.insert(position.character, text);
        }
        
        // Convert back to string and update document
        doc->value = joinLines(lines);
        
        return true;
    }
    
    std::string generateDocumentId(const std::string& filePath) const {
        // Simple document ID generation - in production would use better method
        return "doc_" + std::to_string(std::hash<std::string>{}(filePath));
    }
    
    std::string findFilePathForDocId(const std::string& docId) const {
        std::lock_guard<std::mutex> lock(collaborativeDocsMutex_);
        for (const auto& [filePath, id] : collaborativeDocs_) {
            if (id == docId) {
                return filePath;
            }
        }
        return "";
    }
    
    std::vector<std::string> splitLines(const std::string& content) const {
        std::vector<std::string> lines;
        std::string line;
        for (char c : content) {
            if (c == '\n') {
                lines.push_back(line);
                line.clear();
            } else {
                line += c;
            }
        }
        lines.push_back(line);
        return lines;
    }
    
    std::string joinLines(const std::vector<std::string>& lines) const {
        std::string result;
        for (size_t i = 0; i < lines.size(); ++i) {
            result += lines[i];
            if (i < lines.size() - 1) {
                result += "\n";
            }
        }
        return result;
    }
    
    bool isInitialized_;
    mutable std::mutex collaborativeDocsMutex_;
    std::map<std::string, std::string> collaborativeDocs_; // filePath -> docId mapping
};

} // namespace collaboration
} // namespace bolt

#endif