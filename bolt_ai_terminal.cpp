#include <iostream>
#include <string>

// Include our existing Bolt AI components
#include "bolt/ai/direct_gguf_inference.hpp"
#include "bolt/ai/enhanced_ai_manager.hpp"

int main() {
    std::cout << "🦙 Bolt TinyLlama Chat Terminal" << std::endl;
    std::cout << "==============================" << std::endl;
    
    // Create our AI manager
    bolt::ai::EnhancedAIManager ai_manager;
    std::cout << "✅ AI Manager initialized" << std::endl;
    
    // Try to auto-detect and load GGUF models
    std::cout << "🔍 Looking for TinyLlama models..." << std::endl;
    bool model_found = ai_manager.auto_detect_models();
    
    if (model_found) {
        std::cout << "✅ GGUF model loaded!" << std::endl;
        std::cout << "📋 Model info: " << ai_manager.get_model_info() << std::endl;
    } else {
        std::cout << "⚠️  No GGUF models found, but intelligent fallback responses available!" << std::endl;
        std::cout << "💡 To use a real model, place a .gguf file in the ./models/ directory" << std::endl;
    }
    
    std::cout << "\n💬 Chat with AI (type 'quit' to exit, 'info' for model details):" << std::endl;
    std::cout << "────────────────────────────────────────────────────────────────" << std::endl;
    
    std::string input;
    std::string session_id = "terminal_chat";
    
    // Create a session
    ai_manager.create_session(session_id);
    
    while (true) {
        std::cout << "\n👤 You: ";
        
        if (!std::getline(std::cin, input)) {
            break; // EOF or error
        }
        
        if (input == "quit" || input == "exit") {
            std::cout << "👋 Goodbye!" << std::endl;
            break;
        }
        
        if (input == "info") {
            if (ai_manager.has_direct_model()) {
                std::cout << "📋 " << ai_manager.get_model_info() << std::endl;
            } else {
                std::cout << "⚠️ No direct model loaded - using intelligent fallback responses" << std::endl;
                std::cout << "💡 Current provider: " << ai_manager.get_current_provider() << std::endl;
            }
            continue;
        }
        
        if (input == "stats") {
            auto stats = ai_manager.get_statistics();
            std::cout << "📊 Statistics:" << std::endl;
            std::cout << "  Total requests: " << stats.total_requests << std::endl;
            std::cout << "  Successful: " << stats.successful_requests << std::endl;
            std::cout << "  Failed: " << stats.failed_requests << std::endl;
            std::cout << "  Avg response time: " << stats.average_inference_time_ms << "ms" << std::endl;
            continue;
        }
        
        if (input.empty()) {
            continue;
        }
        
        std::cout << "\n🤖 AI: ";
        std::cout.flush();
        
        // Get AI response
        auto response = ai_manager.chat(input, session_id);
        
        if (response.success) {
            std::cout << response.response << std::endl;
            std::cout << "⏱️  Response time: " << response.inference_time_ms << "ms" << std::endl;
            if (response.tokens_generated > 0) {
                std::cout << "🔢 Tokens generated: " << response.tokens_generated << std::endl;
            }
        } else {
            std::cout << "❌ Error: " << response.error << std::endl;
        }
        
        std::cout << "────────────────────────────────────────────────────────────────" << std::endl;
    }
    
    return 0;
}
