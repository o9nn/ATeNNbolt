
#ifndef BOLT_HPP
#define BOLT_HPP

#include <memory>
#include <string>
#include <vector>
#include "filesystem_utils.hpp"
#include "bolt_commands.hpp"
#include "network_commands.hpp"
#include "http_server.hpp"
#include "ggml_wrapper.hpp"

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

    void initialize() {
        workbench_ = std::make_unique<Workbench>();
        try {
            GGMLWrapper::getInstance().initialize("models/model.bin");
        } catch (const GGMLException& e) {
            std::cerr << "Failed to initialize GGML: " << e.what() << std::endl;
        }
    }
    
    void run() {
        if (workbench_) {
            workbench_->run();
        }
    }

private:
    BoltApp() = default;
    std::unique_ptr<Workbench> workbench_;
};

} // namespace bolt

#endif
