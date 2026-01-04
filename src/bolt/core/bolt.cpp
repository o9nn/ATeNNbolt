
#include "bolt/bolt.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include "bolt/editor/keyboard_shortcuts.hpp"
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
        std::cout << "Bolt implementation initialized\n";
    }

    std::string generateResponse(const std::string& prompt) {
        return "Echo: " + prompt;
    }

    void processMessage(const std::string& input) {
        chat_.addMessage(ChatMessage(ChatMessage::Role::User, input));
        
        // Process command if it starts with '/'
        if (!input.empty() && input[0] == '/') {
            processCommand(input.substr(1));
            return;
        }

        // Generate simple response
        std::string response = generateResponse(input);
        chat_.addMessage(ChatMessage(ChatMessage::Role::Assistant, response));
        
        // Display the response to the user
        std::cout << "Assistant > " << response << std::endl;
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
    std::cout << "Initializing keyboard shortcuts system...\n";
    
    // Initialize the integrated editor and keyboard shortcuts
    IntegratedEditor& editor = IntegratedEditor::getInstance();
    editor.initializeKeyboardShortcuts();
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Add application-specific shortcuts
    shortcuts.registerShortcut("Ctrl+Q", "quit", [this]() {
        std::cout << "\nQuitting application via keyboard shortcut...\n";
        running_ = false;
    }, ShortcutContext::Global, "Quit application");
    
    shortcuts.registerShortcut("F1", "showKeyboardHelp", []() {
        KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
        std::cout << "\n" << shortcuts.getHelpText(ShortcutContext::Global) << std::endl;
        std::cout << shortcuts.getHelpText(ShortcutContext::Editor) << std::endl;
    }, ShortcutContext::Global, "Show keyboard shortcuts help");
    
    std::cout << "Keyboard shortcuts system initialized!\n";
    std::cout << "Press F1 to see available shortcuts, Ctrl+Q to quit\n";
    std::cout << "Type '/help' for commands, '/exit' to quit\n\n";
    
    BoltImpl::getInstance().initialize();
    running_ = true;
}

void BoltApp::processUserInput(const std::string& input) {
    if (input == "/exit") {
        running_ = false;
        return;
    }
    
    // Check if input looks like a keyboard shortcut (for demonstration)
    if (input.find('+') != std::string::npos || input == "F1") {
        KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
        bool handled = shortcuts.executeShortcut(input, ShortcutContext::Global);
        if (handled) {
            return;
        }
        // If not handled in global context, try editor context
        handled = shortcuts.executeShortcut(input, ShortcutContext::Editor);
        if (handled) {
            return;
        }
        std::cout << "Unknown keyboard shortcut: " << input << std::endl;
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
