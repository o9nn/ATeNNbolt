#ifndef CHAT_STORE_HPP
#define CHAT_STORE_HPP

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "error_handling.hpp"

namespace bolt {

// Simple struct to avoid circular dependency
struct SimpleChatMessage {
    std::string sender;
    std::string content;
    
    SimpleChatMessage() = default;
    SimpleChatMessage(const std::string& s, const std::string& c) 
        : sender(s), content(c) {
        validateMessage();
    }
    
    void validateMessage() const {
        if (sender.empty()) {
            throw StoreException(ErrorCode::INVALID_PARAMETER, "Message sender cannot be empty");
        }
        if (content.empty()) {
            throw StoreException(ErrorCode::INVALID_PARAMETER, "Message content cannot be empty");
        }
        if (sender.length() > MAX_SENDER_LENGTH) {
            throw StoreException(ErrorCode::INVALID_PARAMETER, 
                               "Sender name too long: " + std::to_string(sender.length()) + 
                               " > " + std::to_string(MAX_SENDER_LENGTH));
        }
        if (content.length() > MAX_CONTENT_LENGTH) {
            throw StoreException(ErrorCode::INVALID_PARAMETER,
                               "Message content too long: " + std::to_string(content.length()) + 
                               " > " + std::to_string(MAX_CONTENT_LENGTH));
        }
    }
    
    static constexpr size_t MAX_SENDER_LENGTH = 256;
    static constexpr size_t MAX_CONTENT_LENGTH = 65536; // 64KB
};

class ChatStore {
public:
    static ChatStore& getInstance() {
        static ChatStore instance;
        return instance;
    }

    void setChatStarted(bool started) {
        std::lock_guard<std::mutex> lock(mutex_);
        chatStarted_ = started;
        notifyListeners();
    }

    void setShowChat(bool show) {
        std::lock_guard<std::mutex> lock(mutex_);
        showChat_ = show;
        notifyListeners();
    }

    void setAborted(bool aborted) {
        std::lock_guard<std::mutex> lock(mutex_);
        aborted_ = aborted;
        notifyListeners();
    }

    bool getChatStarted() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return chatStarted_; 
    }
    
    bool getShowChat() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return showChat_; 
    }
    
    bool getAborted() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return aborted_; 
    }

    void addListener(std::function<void()> listener) {
        ErrorHandler::validateParameter(listener != nullptr, "Listener cannot be null");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (listeners_.size() >= MAX_LISTENERS) {
            throw StoreException(ErrorCode::STORE_STATE_INVALID,
                "Maximum number of listeners reached: " + std::to_string(MAX_LISTENERS));
        }
        
        listeners_.push_back(listener);
    }

    void addMessage(const SimpleChatMessage& msg) {
        msg.validateMessage(); // Validate message before adding
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (messages_.size() >= MAX_MESSAGES) {
            throw StoreException(ErrorCode::STORE_STATE_INVALID,
                "Maximum number of messages reached: " + std::to_string(MAX_MESSAGES));
        }
        
        messages_.push_back(msg);
    }

    const std::vector<SimpleChatMessage>& getMessages() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_; 
    }
    
    size_t getMessageCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_.size();
    }
    
    size_t getListenerCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }
    
    void clearMessages() {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.clear();
        notifyListeners();
    }

private:
    ChatStore() : chatStarted_(false), showChat_(true), aborted_(false) {}

    void notifyListeners() {
        // Note: mutex should already be locked when this is called
        for (auto& listener : listeners_) {
            try {
                if (listener) {
                    listener();
                }
            } catch (const std::exception& e) {
                // Don't let listener failures break the store
                // In a real implementation, this would go to a logging system
                // For now, we'll continue with other listeners
            }
        }
    }

    bool chatStarted_;
    bool showChat_;
    bool aborted_;
    std::vector<std::function<void()>> listeners_;
    std::vector<SimpleChatMessage> messages_;
    mutable std::mutex mutex_;
    
    static constexpr size_t MAX_LISTENERS = 100;
    static constexpr size_t MAX_MESSAGES = 10000;
};

} // namespace bolt

#endif