#include "bolt/ai/direct_gguf_inference.hpp"
#include <iostream>
#include <string>
#include <vector>

class BoltTerminalChat {
private:
    bolt::ai::DirectGGUFInference ai_;
    std::vector<std::string> conversation_history_;
    
public:
    bool initialize() {
        std::cout << "🚀 Bolt Terminal Chat - Direct GGUF Access\n";
        std::cout << "=========================================\n\n";
        
        // Load the TinyLlama model
        std::string model_path = "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf";
        
        std::cout << "📥 Loading AI model...\n";
        bool loaded = ai_.load_model(model_path);
        
        if (!loaded) {
            std::cout << "❌ Failed to load model\n";
            return false;
        }
        
        std::cout << "✅ AI model loaded successfully!\n";
        std::cout << ai_.get_model_info() << "\n";
        return true;
    }
    
    void run_chat() {
        std::cout << "\n💬 Chat ready! Commands:\n";
        std::cout << "  Type your message and press Enter\n";
        std::cout << "  /help - Show help\n";
        std::cout << "  /clear - Clear conversation history\n";
        std::cout << "  /quit, /exit, quit, exit - End chat\n\n";
        
        std::string input;
        while (true) {
            std::cout << "You> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            // Handle commands
            if (input == "/quit" || input == "/exit" || input == "quit" || input == "exit") {
                std::cout << "👋 Goodbye!\n";
                break;
            }
            
            if (input == "/clear") {
                conversation_history_.clear();
                std::cout << "🧹 Conversation history cleared\n";
                continue;
            }
            
            if (input == "/help") {
                std::cout << "Available commands:\n";
                std::cout << "  /help - Show this help\n";
                std::cout << "  /clear - Clear conversation history\n";
                std::cout << "  /quit, /exit - End the chat\n";
                std::cout << "  Just type your message for AI chat\n";
                continue;
            }
            
            // Generate AI response
            std::cout << "AI> 🤔 Thinking...\r" << std::flush;
            
            auto response = ai_.chat(input, conversation_history_);
            
            if (response.success) {
                std::cout << "AI> " << response.response << "\n\n";
                
                // Update conversation history
                conversation_history_.push_back(input);
                conversation_history_.push_back(response.response);
                
                // Keep history manageable (last 10 exchanges = 20 messages)
                if (conversation_history_.size() > 20) {
                    conversation_history_.erase(conversation_history_.begin(), conversation_history_.begin() + 2);
                }
                
                // Show some stats
                std::cout << "⏱️  Response time: " << static_cast<int>(response.inference_time_ms / 1000) << "s | ";
                std::cout << "Tokens: " << response.tokens_generated << "\n\n";
                
            } else {
                std::cout << "AI> ❌ Error: " << response.error << "\n\n";
            }
        }
    }
};

int main(int argc, char* argv[]) {
    BoltTerminalChat chat;
    
    if (!chat.initialize()) {
        return 1;
    }
    
    chat.run_chat();
    return 0;
}
