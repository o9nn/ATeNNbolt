
#ifndef BOLT_HPP
#define BOLT_HPP

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "core/chat_store.hpp"
#include "core/editor_store.hpp"
#include "core/workbench_store.hpp"

namespace bolt {

class ChatMessage {
public:
    enum class Role { User, Assistant };
    
    ChatMessage(Role role, const std::string& content) 
        : role_(role), content_(content) {}
    
    Role getRole() const { return role_; }
    std::string getContent() const { return content_; }

private:
    Role role_;
    std::string content_;
};

class Chat {
public:
    void addMessage(const ChatMessage& message);
    std::vector<ChatMessage> getHistory() const;
    void clear();

private:
    std::vector<ChatMessage> history_;
};

class BoltApp {
public:
    static BoltApp& getInstance() {
        static BoltApp instance;
        return instance;
    }

    void initialize();
    void run();
    void processUserInput(const std::string& input);

private:
    BoltApp() = default;
    std::unique_ptr<Workbench> workbench_;
    bool running_ = false;
};

} // namespace bolt

#endif
