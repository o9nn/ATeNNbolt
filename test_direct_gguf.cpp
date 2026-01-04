#include "bolt/ai/direct_gguf_inference.hpp"
#include <iostream>

int main() {
    std::cout << "🔍 Testing Direct GGUF Inference with KoboldCpp Integration\n";
    std::cout << "========================================================\n\n";
    
    bolt::ai::DirectGGUFInference ai;
    
    // Load the model
    std::string model_path = "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf";
    bool loaded = ai.load_model(model_path);
    
    if (!loaded) {
        std::cout << "❌ Failed to load model" << std::endl;
        return 1;
    }
    
    std::cout << "\n📊 Model Info:" << std::endl;
    std::cout << ai.get_model_info() << std::endl;
    
    // Test basic generation
    std::cout << "\n🧪 Testing text generation..." << std::endl;
    
    auto response = ai.generate_text("Hello, can you help me with C++ programming?", 100, 0.7f);
    
    std::cout << "✅ Response:" << std::endl;
    std::cout << "Success: " << (response.success ? "Yes" : "No") << std::endl;
    std::cout << "Tokens: " << response.tokens_generated << std::endl;
    std::cout << "Time: " << response.inference_time_ms << "ms" << std::endl;
    std::cout << "Response: " << response.response << std::endl;
    
    // Test chat functionality  
    std::cout << "\n💬 Testing chat..." << std::endl;
    
    std::vector<std::string> history;
    auto chat_response = ai.chat("What is a pointer in C++?", history);
    
    std::cout << "✅ Chat Response:" << std::endl;
    std::cout << "Success: " << (chat_response.success ? "Yes" : "No") << std::endl;
    std::cout << "Response: " << chat_response.response << std::endl;
    
    return 0;
}
