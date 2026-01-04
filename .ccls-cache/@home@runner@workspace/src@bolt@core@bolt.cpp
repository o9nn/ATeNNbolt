
#include "bolt.hpp"
#include "ggml_wrapper.hpp"
#include <iostream>
#include <string>
#include <algorithm>

namespace bolt {

void Chat::addMessage(const ChatMessage& message) {
    history_.push_back(message);
}

std::vector<ChatMessage> Chat::getHistory() const {
    return history_;
}

void Chat::clear() {
    history_.clear();
}

class BoltImpl {
public:
    static BoltImpl& getInstance() {
        static BoltImpl instance;
        return instance;
    }

    void initialize() {
        try {
            GGMLWrapper::getInstance().initialize("models/model.bin");
            std::cout << "GGML model initialized successfully\n";
        } catch (const GGMLException& e) {
            std::cerr << "GGML initialization failed: " << e.what() << "\n";
        }
    }

    std::string generateResponse(const std::string& prompt) {
        try {
            return GGMLWrapper::getInstance().generateResponse(prompt);
        } catch (const GGMLException& e) {
            return "Error generating response: " + std::string(e.what());
        }
    }

    void processMessage(const std::string& input) {
        chat_.addMessage(ChatMessage(ChatMessage::Role::User, input));
        
        // Process command if it starts with '/'
        if (!input.empty() && input[0] == '/') {
            processCommand(input.substr(1));
            return;
        }

        // Generate AI response
        std::string response = generateResponse(input);
        chat_.addMessage(ChatMessage(ChatMessage::Role::Assistant, response));
    }

private:
    BoltImpl() = default;
    Chat chat_;

    void processCommand(const std::string& cmd) {
        if (cmd == "clear") {
            chat_.clear();
            std::cout << "Chat history cleared\n";
        } else if (cmd == "help") {
            std::cout << "Available commands:\n"
                     << "/clear - Clear chat history\n"
                     << "/help - Show this help message\n"
                     << "/exit - Exit application\n";
        }
    }
};

void BoltApp::initialize() {
    std::cout << "Bolt C++ - AI-Powered Development Environment\n";
    std::cout << "Type '/help' for commands, '/exit' to quit\n\n";
    
    BoltImpl::getInstance().initialize();
    running_ = true;
}

void BoltApp::processUserInput(const std::string& input) {
    if (input == "/exit") {
        running_ = false;
        return;
    }

    BoltImpl::getInstance().processMessage(input);
}

void BoltApp::run() {
    initialize();
    
    std::string input;
    while (running_) {
        std::cout << "\nUser > ";
        std::getline(std::cin, input);
        
        if (!input.empty()) {
            processUserInput(input);
        }
    }
}

} // namespace bolt
