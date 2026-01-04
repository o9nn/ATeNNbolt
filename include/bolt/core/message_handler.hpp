
#ifndef BOLT_MESSAGE_HANDLER_HPP
#define BOLT_MESSAGE_HANDLER_HPP

#include <string>
#include <memory>
#include "error_handling.hpp"

namespace bolt {

enum class MessageType {
    Chat,
    Command,
    System
};

struct Message {
    MessageType type;
    std::string content;
    
    Message() : type(MessageType::System), content("") {
        // Default constructor creates empty system message - this is allowed
    }
    
    Message(MessageType t, const std::string& c) 
        : type(t), content(c) {
        // Only validate non-empty messages for explicit construction
        if (!content.empty()) {
            validateMessage();
        }
    }
    
    // Validate message content
    void validateMessage() const {
        if (content.length() > MAX_MESSAGE_LENGTH) {
            throw MessageException(ErrorCode::INVALID_PARAMETER,
                                 "Message content too long: " + std::to_string(content.length()) + 
                                 " > " + std::to_string(MAX_MESSAGE_LENGTH));
        }
    }
    
    static constexpr size_t MAX_MESSAGE_LENGTH = 65536; // 64KB max message size
};

class MessageHandlerImpl;

class MessageHandler {
public:
    static MessageHandler& getInstance();
    
    void initialize();
    void pushMessage(const Message& msg);
    void processMessages();
    bool isInitialized() const;
    size_t getQueueSize() const;
    void setMaxQueueSize(size_t maxSize);

    static constexpr size_t DEFAULT_MAX_QUEUE_SIZE = 1000;
    
private:
    MessageHandler() = default;
    std::unique_ptr<MessageHandlerImpl> impl_;
};

} // namespace bolt

#endif // BOLT_MESSAGE_HANDLER_HPP
