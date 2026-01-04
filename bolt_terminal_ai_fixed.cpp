#include "include/bolt/ai/enhanced_ai_manager.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>  // For isatty

class BoltTerminalAI {
private:
    std::unique_ptr<bolt::ai::EnhancedAIManager> ai_manager_;
    std::string session_id_;
    
public:
    BoltTerminalAI() {
        ai_manager_ = std::make_unique<bolt::ai::EnhancedAIManager>();
        session_id_ = "terminal_session";
        ai_manager_->create_session(session_id_);
    }
    
    void show_status() {
        std::cout << "\n📊 AI Status:\n";
        std::cout << "  Current Provider: " << ai_manager_->get_current_provider() << "\n";
        std::cout << "  Ready: " << (ai_manager_->is_ready() ? "✅" : "❌") << "\n";
        std::cout << "  Direct Model: " << (ai_manager_->has_direct_model() ? "✅ " + ai_manager_->get_model_info() : "❌ Using fallback") << "\n";
        
        auto stats = ai_manager_->get_statistics();
        std::cout << "  Total Requests: " << stats.total_requests << "\n";
        std::cout << "  Successful: " << stats.successful_requests << "\n";
        std::cout << "  Failed: " << stats.failed_requests << "\n\n";
    }
    
    void chat_loop() {
        std::cout << "🤖 Bolt AI Terminal Chat\n";
        std::cout << "========================\n\n";
        
        show_status();
        
        // Check if input is from a pipe or terminal
        bool is_piped = !isatty(fileno(stdin));
        
        if (!is_piped) {
            std::cout << "Commands:\n";
            std::cout << "  /status  - Show AI status\n";
            std::cout << "  /history - Show chat history\n";
            std::cout << "  /clear   - Clear chat history\n";
            std::cout << "  /load    - Try to load a GGUF model\n";
            std::cout << "  /quit    - Exit the application\n\n";
            
            std::cout << "💬 Start chatting (type your message and press Enter):\n\n";
        }
        
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input.empty()) {
                if (is_piped) break; // Exit on empty line when piped
                continue;
            }
            
            if (!is_piped) {
                std::cout << "You: " << input << "\n";
            }
            
            // Handle commands
            if (input == "/quit" || input == "/exit") {
                std::cout << "\n👋 Goodbye!\n";
                break;
            }
            
            if (input == "/status") {
                show_status();
                if (!is_piped) continue;
                else break;
            }
            
            if (input == "/history") {
                show_history();
                if (!is_piped) continue;
                else break;
            }
            
            if (input == "/clear") {
                ai_manager_->clear_session_history(session_id_);
                std::cout << "🗑️ Chat history cleared.\n\n";
                if (!is_piped) continue;
                else break;
            }
            
            if (input == "/load") {
                try_load_model();
                if (!is_piped) continue;
                else break;
            }
            
            // Send to AI
            std::cout << "AI:  ";
            auto response = ai_manager_->chat(input, session_id_);
            
            if (response.success) {
                std::cout << response.response << "\n";
                if (response.inference_time_ms > 0) {
                    std::cout << "     ⏱️ " << response.inference_time_ms << "ms";
                    if (response.tokens_generated > 0) {
                        std::cout << " | 🔤 " << response.tokens_generated << " tokens";
                    }
                    std::cout << "\n";
                }
            } else {
                std::cout << "❌ Error: " << response.error << "\n";
            }
            
            std::cout << "\n";
            
            // For piped input, process one question and exit
            if (is_piped) {
                break;
            }
        }
    }

private:
    void show_history() {
        auto history = ai_manager_->get_session_history(session_id_);
        if (history.empty()) {
            std::cout << "📝 No chat history yet.\n\n";
            return;
        }
        
        std::cout << "\n📜 Chat History:\n";
        std::cout << "================\n";
        for (const auto& entry : history) {
            std::cout << entry << "\n";
        }
        std::cout << "\n";
    }
    
    void try_load_model() {
        std::cout << "🔍 Attempting to auto-detect GGUF models...\n";
        if (ai_manager_->auto_detect_models()) {
            std::cout << "✅ Successfully loaded a GGUF model!\n";
            show_status();
        } else {
            std::cout << "❌ No GGUF models found. You can:\n";
            std::cout << "   1. Download a small model:\n";
            std::cout << "      wget -P ./models/ https://huggingface.co/microsoft/DialoGPT-small/resolve/main/model.gguf\n";
            std::cout << "   2. Or continue using intelligent fallback responses\n\n";
        }
    }
};

int main() {
    try {
        BoltTerminalAI app;
        app.chat_loop();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
