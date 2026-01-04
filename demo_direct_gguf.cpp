// demo_direct_gguf.cpp - Test direct GGUF file loading and inference
#include <iostream>
#include "include/bolt/ai/enhanced_ai_manager.hpp"
#include "include/bolt/ai/direct_gguf_inference.hpp"

using namespace bolt::ai;

void test_direct_inference() {
    std::cout << "🧪 Testing Direct GGUF Inference" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Test auto-detection
    auto direct_ai = DirectGGUFFactory::create_auto_detect();
    
    if (direct_ai->is_loaded()) {
        std::cout << "✅ Model loaded successfully!" << std::endl;
        std::cout << direct_ai->get_model_info() << std::endl;
        
        // Test chat
        std::cout << "\n💬 Testing chat..." << std::endl;
        auto response = direct_ai->chat("Hello! Can you help me with C++ programming?");
        
        std::cout << "AI Response: " << response.response << std::endl;
        std::cout << "Success: " << (response.success ? "✅" : "❌") << std::endl;
        std::cout << "Time: " << response.inference_time_ms << "ms" << std::endl;
        
        // Test code completion
        std::cout << "\n🔧 Testing code completion..." << std::endl;
        auto code_response = direct_ai->generate_text(
            "Complete this C++ function:\nvoid quicksort(std::vector<int>& arr) {\n    if (arr.size() <= 1) return;\n    // ", 
            100
        );
        
        std::cout << "Code Completion: " << code_response.response << std::endl;
        
    } else {
        std::cout << "⚠️ No GGUF model loaded - testing fallback responses" << std::endl;
        
        // Test fallback responses
        auto response = direct_ai->chat("How do I use smart pointers in C++?");
        std::cout << "Fallback Response: " << response.response << std::endl;
        
        auto algorithm_response = direct_ai->chat("Explain binary search algorithm");
        std::cout << "Algorithm Help: " << algorithm_response.response << std::endl;
        
        auto config_response = direct_ai->chat("How do I configure AI models?");
        std::cout << "Config Help: " << config_response.response << std::endl;
    }
}

void test_enhanced_manager_with_gguf() {
    std::cout << "\n🚀 Testing Enhanced AI Manager with GGUF" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    EnhancedAIManager manager;
    
    std::cout << "Manager ready: " << (manager.is_ready() ? "✅" : "❌") << std::endl;
    std::cout << "Current provider: " << manager.get_current_provider() << std::endl;
    
    if (manager.has_direct_model()) {
        std::cout << "📊 Model info: " << manager.get_model_info() << std::endl;
    }
    
    // Test chat functionality
    std::cout << "\n💬 Testing chat with Enhanced Manager..." << std::endl;
    auto chat_response = manager.chat("What are the benefits of using RAII in C++?", "test_session");
    
    std::cout << "Chat Response: " << chat_response.response << std::endl;
    std::cout << "Success: " << (chat_response.success ? "✅" : "❌") << std::endl;
    
    // Test session management
    manager.create_session("cpp_help");
    auto session_response = manager.chat("How do I properly manage memory in C++?", "cpp_help");
    std::cout << "Session Response: " << session_response.response << std::endl;
    
    // Show statistics
    auto stats = manager.get_statistics();
    std::cout << "\n📊 AI Statistics:" << std::endl;
    std::cout << "  Total requests: " << stats.total_requests << std::endl;
    std::cout << "  Successful: " << stats.successful_requests << std::endl;
    std::cout << "  Failed: " << stats.failed_requests << std::endl;
    std::cout << "  Avg response time: " << stats.average_inference_time_ms << " ms" << std::endl;
}

void demonstrate_fallback_intelligence() {
    std::cout << "\n🧠 Demonstrating Intelligent Fallback Responses" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    DirectGGUFInference fallback_ai;
    // Note: No model loaded, so it will use fallback responses
    
    std::vector<std::string> test_questions = {
        "How do I use std::unique_ptr?",
        "What's the difference between stack and heap?", 
        "Explain the quicksort algorithm",
        "How do I configure AI models?",
        "What are C++ templates?",
        "Help me with STL containers"
    };
    
    for (const auto& question : test_questions) {
        std::cout << "\nQuestion: " << question << std::endl;
        auto response = fallback_ai.chat(question);
        std::cout << "Answer: " << response.response.substr(0, 200) << "..." << std::endl;
        std::cout << "────────────────────────────────────" << std::endl;
    }
}

int main() {
    std::cout << "🎯 Direct GGUF Loading and Inference Test" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    try {
        // Test 1: Direct inference engine
        test_direct_inference();
        
        // Test 2: Enhanced manager with GGUF integration
        test_enhanced_manager_with_gguf();
        
        // Test 3: Intelligent fallback responses
        demonstrate_fallback_intelligence();
        
        std::cout << "\n✅ Direct GGUF testing completed!" << std::endl;
        
        std::cout << "\n💡 Next Steps:" << std::endl;
        std::cout << "• Download a small GGUF model to test real AI inference" << std::endl;
        std::cout << "• Try: wget https://huggingface.co/microsoft/DialoGPT-medium/resolve/main/model.gguf" << std::endl;
        std::cout << "• Place in ./models/ directory and rerun this test" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error during testing: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
