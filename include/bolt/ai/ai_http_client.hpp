#pragma once
#include <string>
#include <map>
#include <memory>
#include "bolt/drawkern/ai_integration.hpp"

namespace bolt {
namespace ai {

// Supported API types
enum class APIType {
    OPENAI,      // OpenAI compatible API
    ANTHROPIC,   // Anthropic Claude API
    LLAMA_CPP,   // llama.cpp server
    LOCAL_GGML,  // Local GGML model
    CUSTOM       // Custom endpoint
};

// Configuration for HTTP-based AI services
struct AIHttpConfig {
    std::string base_url = "http://localhost:8080";
    std::string api_key = "";
    std::string model_name = "gpt-3.5-turbo";
    APIType api_type = APIType::LLAMA_CPP;
    
    // Request parameters
    int max_tokens = 512;
    float temperature = 0.7f;
    float top_p = 0.9f;
    std::string system_prompt = "You are a helpful AI programming assistant.";
    
    // Connection settings
    int timeout_seconds = 30;
    bool verify_ssl = true;
    bool use_streaming = false;
    
    // Retry settings
    int max_retries = 3;
    int retry_delay_ms = 1000;
};

// HTTP request structure
struct HttpRequest {
    std::string url;
    std::string body;
    std::map<std::string, std::string> headers;
    APIType api_type = APIType::LLAMA_CPP;
};

// HTTP client for AI services
class AIHttpClient {
public:
    AIHttpClient(const AIHttpConfig& config);
    ~AIHttpClient();
    
    // AI inference methods
    drawkern::AIInferenceResponse chat(const std::string& message, const std::string& session_id);
    drawkern::AIInferenceResponse complete_code(const std::string& code, const std::string& language);
    
    // Configuration and testing
    bool test_connection();
    void update_config(const AIHttpConfig& config);
    const AIHttpConfig& get_config() const { return config_; }
    
private:
    AIHttpConfig config_;
    void* curl_;  // CURL handle
    
    // HTTP operations
    bolt::drawkern::AIInferenceResponse send_request(const HttpRequest& request);
    HttpRequest create_chat_request(const std::string& message, const std::string& session_id);
    HttpRequest create_completion_request(const std::string& code, const std::string& language);
    bolt::drawkern::AIInferenceResponse parse_response(const std::string& response_data, APIType api_type);
};

// Factory for creating pre-configured HTTP clients
class AIHttpClientFactory {
public:
    // Create clients for popular services
    static std::unique_ptr<AIHttpClient> create_openai_client(const std::string& api_key, const std::string& model = "gpt-3.5-turbo");
    static std::unique_ptr<AIHttpClient> create_anthropic_client(const std::string& api_key, const std::string& model = "claude-3-sonnet-20240229");
    static std::unique_ptr<AIHttpClient> create_llama_cpp_client(const std::string& base_url = "http://localhost:8080");
    static std::unique_ptr<AIHttpClient> create_ollama_client(const std::string& base_url = "http://localhost:11434", const std::string& model = "llama2");
    
    // Create client from configuration file
    static std::unique_ptr<AIHttpClient> create_from_config(const std::string& config_file_path);
    
    // Detect and suggest configuration
    static AIHttpConfig detect_local_server();
    static std::vector<AIHttpConfig> get_recommended_configs();
};

// Configuration manager for AI settings
class AIConfigManager {
public:
    AIConfigManager();
    ~AIConfigManager();
    
    // Configuration management
    bool load_config(const std::string& config_file = "");
    bool save_config(const std::string& config_file = "");
    
    // Provider management
    void add_provider(const std::string& name, const AIHttpConfig& config);
    void remove_provider(const std::string& name);
    AIHttpConfig get_provider_config(const std::string& name) const;
    std::vector<std::string> list_providers() const;
    
    // Active provider
    void set_active_provider(const std::string& name);
    std::string get_active_provider() const { return active_provider_; }
    AIHttpConfig get_active_config() const;
    
    // Test providers
    std::map<std::string, bool> test_all_providers();
    bool test_provider(const std::string& name);
    
private:
    std::map<std::string, AIHttpConfig> providers_;
    std::string active_provider_;
    std::string config_file_path_;
    
    void load_default_providers();
    AIHttpConfig create_default_config() const;
};

} // namespace ai
} // namespace bolt
