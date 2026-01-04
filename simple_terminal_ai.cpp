#include "bolt/ai/direct_gguf_inference.hpp"
#include <iostream>
#include <string>
#include <unistd.h>  // For isatty

class SimpleBoltTerminalAI {
private:
    std::unique_ptr<bolt::ai::DirectGGUFInference> direct_ai_;
    
public:
    SimpleBoltTerminalAI() {
        direct_ai_ = std::make_unique<bolt::ai::DirectGGUFInference>();
    }
    
    void show_status() {
        std::cout << "\n📊 Direct GGUF AI Status:\n";
        std::cout << "  Model Loaded: " << (direct_ai_->is_loaded() ? "✅" : "❌") << "\n";
        if (direct_ai_->is_loaded()) {
            std::cout << "  Model Info: " << direct_ai_->get_model_info() << "\n";
        }
        std::cout << "\n";
    }
    
    void chat_loop() {
        std::cout << "🤖 Simple Bolt AI Terminal Chat (Direct GGUF)\n";
        std::cout << "==============================================\n\n";
        
        // Try to load a GGUF model
        std::cout << "🔍 Searching for GGUF models...\n";
        if (!direct_ai_->auto_detect_and_load()) {
            std::cout << "❌ No GGUF models found. Using intelligent fallback responses.\n";
        } else {
            std::cout << "✅ GGUF model loaded successfully!\n";
        }
        
        show_status();
        
        // Check if input is from a pipe or terminal
        bool is_piped = !isatty(fileno(stdin));
        
        if (!is_piped) {
            std::cout << "Commands:\n";
            std::cout << "  /status  - Show AI status\n";
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
            
            if (input == "/load") {
                try_load_model();
                if (!is_piped) continue;
                else break;
            }
            
            // Send to AI
            std::cout << "AI:  ";
            auto start_time = std::chrono::high_resolution_clock::now();
            
            auto response = direct_ai_->generate_text(input, 150, 0.7f);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            
            if (response.success) {
                std::cout << response.response << "\n";
                std::cout << "     ⏱️ " << duration << "ms";
                if (response.tokens_generated > 0) {
                    std::cout << " | 🔤 " << response.tokens_generated << " tokens";
                }
                std::cout << "\n";
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
    void try_load_model() {
        std::cout << "🔍 Attempting to auto-detect GGUF models...\n";
        if (direct_ai_->auto_detect_and_load()) {
            std::cout << "✅ Successfully loaded a GGUF model!\n";
            show_status();
        } else {
            std::cout << "❌ No GGUF models found. You can:\n";
            std::cout << "   1. Download a small model:\n";
            std::cout << "      mkdir -p models\n";
            std::cout << "      wget -P models/ https://huggingface.co/ggml-org/models/resolve/main/tinyllamas/stories260K.gguf\n";
            std::cout << "   2. Or continue using intelligent fallback responses\n\n";
        }
    }
};

int main() {
    try {
        SimpleBoltTerminalAI app;
        app.chat_loop();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
