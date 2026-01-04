#include <iostream>
#include "bolt/bolt.hpp"

int main() {
    std::cout << "Running basic Bolt C++ tests..." << std::endl;
    
    try {
        // Basic test that the classes can be instantiated
        bolt::Chat chat;
        std::cout << "✓ Chat class instantiated successfully" << std::endl;
        
        // Test adding a message
        bolt::ChatMessage msg(bolt::ChatMessage::Role::User, "Test message");
        chat.addMessage(msg);
        
        auto history = chat.getHistory();
        if (history.size() == 1) {
            std::cout << "✓ Chat message added successfully" << std::endl;
        } else {
            std::cout << "✗ Chat message not added correctly" << std::endl;
            return 1;
        }
        
        std::cout << "All basic tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}