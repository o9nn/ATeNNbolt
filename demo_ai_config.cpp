#include "bolt/ai/enhanced_ai_manager.hpp"
#include "bolt/ai/ai_http_client.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "🚀 Bolt AI Chat Configuration Demo" << std::endl;
    std::cout << "==================================" << std::endl << std::endl;
    
    try {
        // Initialize enhanced AI manager
        std::cout << "🔧 Initializing Enhanced AI Manager..." << std::endl;
        bolt::ai::EnhancedAIManager ai_manager;
        
        // Run quick setup wizard
        std::cout << "\n🧙 Running Quick Setup Wizard..." << std::endl;
        bool setup_success = bolt::ai::AutoSetup::quick_setup_wizard();
        
        if (!setup_success) {
            std::cout << "\n⚠️ No AI providers detected. Let's configure one manually." << std::endl;
            
            // Show available configurations
            auto recommended = bolt::ai::AIHttpClientFactory::get_recommended_configs();
            std::cout << "\n📋 Recommended configurations:" << std::endl;
            for (size_t i = 0; i < recommended.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << recommended[i].base_url 
                          << " (" << (int)recommended[i].api_type << ")" << std::endl;
            }
        }
        
        // Test current configuration
        std::cout << "\n🔍 Testing current configuration..." << std::endl;
        bool is_ready = ai_manager.is_ready();
        std::cout << "AI Manager ready: " << (is_ready ? "✅ YES" : "❌ NO") << std::endl;
        
        if (is_ready) {
            std::cout << "Current provider: " << ai_manager.get_current_provider() << std::endl;
            
            // Test a simple chat
            std::cout << "\n💬 Testing AI chat..." << std::endl;
            auto response = ai_manager.chat("Hello! Can you help me with C++ programming?", "test_session");
            
            if (response.success) {
                std::cout << "✅ Chat test successful!" << std::endl;
                std::cout << "Response: " << response.response << std::endl;
                std::cout << "Inference time: " << response.inference_time_ms << " ms" << std::endl;
            } else {
                std::cout << "❌ Chat test failed: " << response.error << std::endl;
            }
            
            // Test code completion
            std::cout << "\n🚀 Testing code completion..." << std::endl;
            std::string test_code = "std::vector<int> numbers = {1, 2, 3, 4, 5};\nfor (";
            auto completion_response = ai_manager.complete_code(test_code, "cpp");
            
            if (completion_response.success) {
                std::cout << "✅ Code completion test successful!" << std::endl;
                std::cout << "Completion: " << completion_response.response << std::endl;
            } else {
                std::cout << "❌ Code completion test failed: " << completion_response.error << std::endl;
            }
            
            // Show statistics
            std::cout << "\n📊 AI Statistics:" << std::endl;
            auto stats = ai_manager.get_statistics();
            std::cout << "  Total requests: " << stats.total_requests << std::endl;
            std::cout << "  Successful: " << stats.successful_requests << std::endl;
            std::cout << "  Failed: " << stats.failed_requests << std::endl;
            std::cout << "  Avg response time: " << stats.average_inference_time_ms << " ms" << std::endl;
            std::cout << "  Tokens generated: " << stats.total_tokens_generated << std::endl;
        }
        
        // List available providers
        std::cout << "\n🔧 Available Providers:" << std::endl;
        auto providers = ai_manager.list_providers();
        for (const auto& provider : providers) {
            std::string status = provider.is_active ? " (ACTIVE)" : "";
            std::cout << "  • " << provider.name << ": " << provider.config.base_url << status << std::endl;
        }
        
        std::cout << "\n✅ Configuration demo completed!" << std::endl;
        
        if (is_ready) {
            std::cout << "\n🎉 Your AI chat is ready to use!" << std::endl;
            std::cout << "💡 Start the GUI application to chat with the AI assistant." << std::endl;
        } else {
            std::cout << "\n⚠️  AI chat is not yet configured." << std::endl;
            std::cout << "💡 Follow the setup instructions above or configure manually." << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
