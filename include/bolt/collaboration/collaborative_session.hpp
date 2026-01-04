#ifndef COLLABORATIVE_SESSION_HPP
#define COLLABORATIVE_SESSION_HPP

#include "document_operation.hpp"
#include "operational_transform.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <queue>
#include <functional>
#include <chrono>

namespace bolt {
namespace collaboration {

struct UserSession {
    std::string userId;
    std::string userName;
    Position cursorPosition;
    std::chrono::steady_clock::time_point lastActivity;
    bool isActive;
    
    UserSession() : cursorPosition(0, 0), lastActivity(std::chrono::steady_clock::now()), isActive(true) {}
    
    UserSession(const std::string& id, const std::string& name)
        : userId(id), userName(name), cursorPosition(0, 0),
          lastActivity(std::chrono::steady_clock::now()), isActive(true) {}
};

struct DocumentState {
    std::string documentId;
    std::vector<std::string> lines;
    std::vector<std::unique_ptr<DocumentOperation>> operationHistory;
    std::map<std::string, UserSession> activeUsers;
    uint64_t lastSequence;
    
    DocumentState(const std::string& id) 
        : documentId(id), lastSequence(0) {
        lines.push_back(""); // Start with one empty line
    }
};

class CollaborativeSession {
public:
    using OperationCallback = std::function<void(const DocumentOperation&, const std::string&)>;
    using UserJoinCallback = std::function<void(const UserSession&, const std::string&)>;
    using UserLeaveCallback = std::function<void(const std::string&, const std::string&)>;
    using CursorUpdateCallback = std::function<void(const std::string&, const Position&, const std::string&)>;
    
    static CollaborativeSession& getInstance() {
        static CollaborativeSession instance;
        return instance;
    }
    
    // Document management
    bool createDocument(const std::string& documentId, const std::string& initialContent = "") {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        if (documents_.find(documentId) != documents_.end()) {
            return false; // Document already exists
        }
        
        auto doc = std::make_unique<DocumentState>(documentId);
        if (!initialContent.empty()) {
            doc->lines = splitLines(initialContent);
        }
        
        documents_[documentId] = std::move(doc);
        return true;
    }
    
    bool removeDocument(const std::string& documentId) {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        auto it = documents_.find(documentId);
        if (it == documents_.end()) {
            return false;
        }
        
        // Notify all users in this document
        for (const auto& [userId, session] : it->second->activeUsers) {
            if (userLeaveCallback_) {
                userLeaveCallback_(userId, documentId);
            }
        }
        
        documents_.erase(it);
        return true;
    }
    
    // User management
    bool joinDocument(const std::string& userId, const std::string& userName,
                     const std::string& documentId) {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        auto docIt = documents_.find(documentId);
        if (docIt == documents_.end()) {
            return false; // Document doesn't exist
        }
        
        auto& doc = *docIt->second;
        
        // Add user to document
        UserSession session(userId, userName);
        doc.activeUsers[userId] = session;
        
        // Notify other users
        if (userJoinCallback_) {
            userJoinCallback_(session, documentId);
        }
        
        return true;
    }
    
    bool leaveDocument(const std::string& userId, const std::string& documentId) {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        auto docIt = documents_.find(documentId);
        if (docIt == documents_.end()) {
            return false;
        }
        
        auto& doc = *docIt->second;
        auto userIt = doc.activeUsers.find(userId);
        if (userIt == doc.activeUsers.end()) {
            return false;
        }
        
        doc.activeUsers.erase(userIt);
        
        // Notify other users
        if (userLeaveCallback_) {
            userLeaveCallback_(userId, documentId);
        }
        
        return true;
    }
    
    // Operation handling
    bool applyOperation(const DocumentOperation& operation, const std::string& documentId) {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        auto docIt = documents_.find(documentId);
        if (docIt == documents_.end()) {
            return false;
        }
        
        auto& doc = *docIt->second;
        
        // Create a copy of the operation for transformation
        auto transformedOp = std::make_unique<DocumentOperation>(operation);
        
        // Transform against all operations that have occurred since this operation was created
        for (const auto& historyOp : doc.operationHistory) {
            if (historyOp->getSequence() > operation.getSequence()) {
                auto temp = OperationalTransform::transform(*transformedOp, *historyOp);
                transformedOp = std::move(temp);
            }
        }
        
        // Apply the transformed operation
        bool success = transformedOp->apply(doc.lines);
        if (success) {
            // Add to history
            doc.operationHistory.push_back(
                std::make_unique<DocumentOperation>(*transformedOp));
            doc.lastSequence = transformedOp->getSequence();
            
            // Update user cursor if it's a cursor move operation
            if (transformedOp->getType() == OperationType::CURSOR_MOVE) {
                auto userIt = doc.activeUsers.find(transformedOp->getUserId());
                if (userIt != doc.activeUsers.end()) {
                    userIt->second.cursorPosition = transformedOp->getPosition();
                    userIt->second.lastActivity = std::chrono::steady_clock::now();
                    
                    if (cursorUpdateCallback_) {
                        cursorUpdateCallback_(transformedOp->getUserId(),
                                            transformedOp->getPosition(), documentId);
                    }
                }
            }
            
            // Notify listeners
            if (operationCallback_) {
                operationCallback_(*transformedOp, documentId);
            }
        }
        
        return success;
    }
    
    // Get document state
    std::vector<std::string> getDocumentLines(const std::string& documentId) const {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        auto it = documents_.find(documentId);
        if (it != documents_.end()) {
            return it->second->lines;
        }
        return {};
    }
    
    std::string getDocumentContent(const std::string& documentId) const {
        auto lines = getDocumentLines(documentId);
        if (lines.empty()) return "";
        
        std::string content;
        for (size_t i = 0; i < lines.size(); ++i) {
            content += lines[i];
            if (i < lines.size() - 1) {
                content += "\n";
            }
        }
        return content;
    }
    
    // Get active users in document
    std::vector<UserSession> getActiveUsers(const std::string& documentId) const {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        std::vector<UserSession> users;
        auto it = documents_.find(documentId);
        if (it != documents_.end()) {
            for (const auto& [userId, session] : it->second->activeUsers) {
                users.push_back(session);
            }
        }
        return users;
    }
    
    // Callback registration
    void onOperation(OperationCallback callback) {
        operationCallback_ = callback;
    }
    
    void onUserJoin(UserJoinCallback callback) {
        userJoinCallback_ = callback;
    }
    
    void onUserLeave(UserLeaveCallback callback) {
        userLeaveCallback_ = callback;
    }
    
    void onCursorUpdate(CursorUpdateCallback callback) {
        cursorUpdateCallback_ = callback;
    }
    
    // Maintenance
    void cleanupInactiveUsers(std::chrono::seconds inactiveThreshold = std::chrono::seconds(300)) {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        for (auto& [docId, doc] : documents_) {
            auto userIt = doc->activeUsers.begin();
            while (userIt != doc->activeUsers.end()) {
                if (now - userIt->second.lastActivity > inactiveThreshold) {
                    if (userLeaveCallback_) {
                        userLeaveCallback_(userIt->second.userId, docId);
                    }
                    userIt = doc->activeUsers.erase(userIt);
                } else {
                    ++userIt;
                }
            }
        }
    }
    
    // Get statistics
    struct SessionStats {
        size_t totalDocuments;
        size_t totalActiveUsers;
        size_t totalOperations;
    };
    
    SessionStats getStats() const {
        std::lock_guard<std::mutex> lock(documentsMutex_);
        
        SessionStats stats = {0, 0, 0};
        stats.totalDocuments = documents_.size();
        
        for (const auto& [docId, doc] : documents_) {
            stats.totalActiveUsers += doc->activeUsers.size();
            stats.totalOperations += doc->operationHistory.size();
        }
        
        return stats;
    }

private:
    CollaborativeSession() = default;
    
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
        lines.push_back(line); // Add last line
        return lines;
    }
    
    mutable std::mutex documentsMutex_;
    std::map<std::string, std::unique_ptr<DocumentState>> documents_;
    
    OperationCallback operationCallback_;
    UserJoinCallback userJoinCallback_;
    UserLeaveCallback userLeaveCallback_;
    CursorUpdateCallback cursorUpdateCallback_;
};

} // namespace collaboration
} // namespace bolt

#endif