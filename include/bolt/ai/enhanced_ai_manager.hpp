#pragma once
#include "ai_http_client.hpp"
#include "direct_gguf_inference.hpp"
#include "../drawkern/ai_integration.hpp"
#include <memory>
#include <string>
#include <map>
#include <vector>

namespace bolt {
namespace ai {

// Enhanced statistics for AI usage
struct AIStats {
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    float average_inference_time_ms = 0.0f;
    size_t total_tokens_generated = 0;
    size_t total_tokens_processed = 0;
    
    // Provider-specific stats could be added here
};

// Provider information
struct ProviderInfo {
    std::string name;
    AIHttpConfig config;
    bool is_active = false;
    std::string status = "unknown";  // "working", "failed", "unknown"
};

// Enhanced AI Manager that combines local models and HTTP clients
class EnhancedAIManager {
public:
    EnhancedAIManager();
    ~EnhancedAIManager();
    
    // Core AI operations
    bolt::drawkern::AIInferenceResponse chat(const std::string& message, const std::string& session_id = "");
    bolt::drawkern::AIInferenceResponse complete_code(const std::string& code, const std::string& language = "cpp");
    bolt::drawkern::AIInferenceResponse analyze_code(const std::string& code, const std::string& language = "cpp");
    
    // Provider management
    bool switch_provider(const std::string& provider_name);
    bool test_connection();
    bool test_all_providers();
    void add_provider(const std::string& name, const AIHttpConfig& config);
    std::vector<ProviderInfo> list_providers() const;
    
    // Session management
    void create_session(const std::string& session_id);
    void destroy_session(const std::string& session_id);
    std::vector<std::string> list_sessions() const;
    std::vector<std::string> get_session_history(const std::string& session_id) const;
    void clear_session_history(const std::string& session_id);
    
    // Statistics and monitoring
    AIStats get_statistics() const;
    void reset_statistics();
    
    // Configuration
    void update_config(const AIHttpConfig& config);
    bool is_ready() const;
    std::string get_current_provider() const;
    AIHttpConfig get_current_config() const;
    
    // GGUF direct file loading
    bool load_gguf_model(const std::string& model_path);
    bool has_direct_model() const;
    std::string get_model_info() const;
    
    // Auto-detect and load available models
    bool auto_detect_models();

    // RWKV direct file loading (minimal)
    bool load_rwkv_model(const std::string& model_path);
    bool has_rwkv_model() const;
    
private:
    std::unique_ptr<AIHttpClient> http_client_;
    std::unique_ptr<AIConfigManager> config_manager_;
    std::unique_ptr<DirectGGUFInference> direct_inference_;
    std::map<std::string, std::vector<std::string>> session_history_;
    AIStats stats_;
    std::string current_provider_;
    bool use_direct_inference_ = false;
    bool use_rwkv_direct_ = false;
    std::string rwkv_model_path_;
    
    bool initialize_active_provider();
    void update_stats(const bolt::drawkern::AIInferenceResponse& response);
};

// Auto-detection and setup utilities
namespace AutoSetup {
    // Detect local AI servers automatically
    bool detect_and_setup_local_server();
    
    // Print setup instructions for various AI providers
    void print_setup_instructions();
    
    // Run interactive setup wizard
    bool quick_setup_wizard();
}

} // namespace ai
} // namespace bolt
