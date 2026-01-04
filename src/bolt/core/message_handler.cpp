
#include "bolt/core/message_handler.hpp"
#include "bolt/core/chat_store.hpp"
#include <queue>
#include <mutex>

namespace bolt {

class MessageHandlerImpl {
private:
    std::queue<Message> messageQueue_;
    mutable std::mutex queueMutex_;
    size_t maxQueueSize_ = MessageHandler::DEFAULT_MAX_QUEUE_SIZE;
    
public:
    void setMaxQueueSize(size_t maxSize) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        ErrorHandler::validateParameter(maxSize > 0, "Max queue size must be greater than 0");
        maxQueueSize_ = maxSize;
    }
    
    void pushMessage(const Message& msg) {
        // Only validate non-empty messages  
        if (!msg.content.empty()) {
            msg.validateMessage(); // Validate message before adding to queue
        }
        
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (messageQueue_.size() >= maxQueueSize_) {
            throw MessageException(ErrorCode::MESSAGE_QUEUE_FULL,
                "Message queue is full. Size: " + std::to_string(messageQueue_.size()) + 
                ", Max: " + std::to_string(maxQueueSize_));
        }
        
        messageQueue_.push(msg);
    }
    
    bool popMessage(Message& msg) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (messageQueue_.empty()) {
            return false;
        }
        msg = messageQueue_.front();
        messageQueue_.pop();
        return true;
    }
    
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return messageQueue_.size();
    }

    void processMessages() {
        Message msg;
        size_t processedCount = 0;
        const size_t maxProcessPerCall = 100; // Prevent infinite processing
        
        while (popMessage(msg) && processedCount < maxProcessPerCall) {
            try {
                switch (msg.type) {
                    case MessageType::Chat:
                        processChatMessage(msg);
                        break;
                    case MessageType::Command:
                        processCommandMessage(msg);
                        break;
                    case MessageType::System:
                        processSystemMessage(msg);
                        break;
                    default:
                        throw MessageException(ErrorCode::INVALID_MESSAGE_TYPE,
                            "Unknown message type: " + std::to_string(static_cast<int>(msg.type)));
                }
                processedCount++;
            } catch (const BoltException& e) {
                // Log the error but continue processing other messages
                // In a real implementation, this would go to a proper logging system
                processSystemMessage(Message(MessageType::System, 
                    "Error processing message: " + std::string(e.what())));
            }
        }
    }

private:
    void processChatMessage(const Message& msg) {
        // Allow empty messages for chat (can be valid in some cases)
        // But validate that we have meaningful content for non-whitespace
        if (!msg.content.empty() && msg.content.find_first_not_of(" \t\n\r") == std::string::npos) {
            throw MessageException(ErrorCode::INVALID_PARAMETER, 
                                 "Chat message cannot be whitespace only");
        }
        
        // Handle chat messages - simplified for now
        // ChatStore::getInstance().addMessage(...);
    }

    void processCommandMessage(const Message& msg) {
        // Commands must be non-empty and start with '/'
        if (msg.content.empty()) {
            throw MessageException(ErrorCode::INVALID_PARAMETER,
                                 "Command messages cannot be empty");
        }
        if (msg.content[0] != '/') {
            throw MessageException(ErrorCode::INVALID_PARAMETER,
                                 "Command messages must start with '/'");
        }
        
        // Handle command messages
        // This will be expanded based on command processing needs
    }

    void processSystemMessage(const Message& msg) {
        // System messages can be empty (for internal use)
        if (msg.content.length() > Message::MAX_MESSAGE_LENGTH) {
            throw MessageException(ErrorCode::INVALID_PARAMETER,
                                 "System message too long");
        }
        
        // Handle system messages
        // This will be expanded based on system message needs
    }
};

MessageHandler& MessageHandler::getInstance() {
    static MessageHandler instance;
    return instance;
}

void MessageHandler::initialize() {
    if (impl_) {
        // Already initialized - this is okay, just return
        return;
    }
    impl_ = std::make_unique<MessageHandlerImpl>();
}

void MessageHandler::pushMessage(const Message& msg) {
    ErrorHandler::validateInitialized(impl_ != nullptr, "MessageHandler");
    impl_->pushMessage(msg);
}

void MessageHandler::processMessages() {
    ErrorHandler::validateInitialized(impl_ != nullptr, "MessageHandler");
    impl_->processMessages();
}

bool MessageHandler::isInitialized() const {
    return impl_ != nullptr;
}

size_t MessageHandler::getQueueSize() const {
    ErrorHandler::validateInitialized(impl_ != nullptr, "MessageHandler");
    return impl_->getQueueSize();
}

void MessageHandler::setMaxQueueSize(size_t maxSize) {
    ErrorHandler::validateInitialized(impl_ != nullptr, "MessageHandler");
    impl_->setMaxQueueSize(maxSize);
}

} // namespace bolt
