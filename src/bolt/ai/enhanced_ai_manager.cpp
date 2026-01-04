#include "bolt/ai/enhanced_ai_manager.hpp"
#include "bolt/ai/ai_http_client.hpp"
#include "bolt/ai/direct_gguf_inference.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

namespace bolt {
namespace ai {

// New GGUF direct loading methods
bool EnhancedAIManager::load_gguf_model(const std::string& model_path) {
    std::cout << "📥 Loading GGUF model: " << model_path << std::endl;
    
    if (!direct_inference_) {
        direct_inference_ = std::make_unique<bolt::ai::DirectGGUFInference>();
    }
    
    if (direct_inference_->load_model(model_path)) {
        use_direct_inference_ = true;
        current_provider_ = "direct_gguf";
        std::cout << "✅ GGUF model loaded successfully!" << std::endl;
        return true;
    }
    
    std::cout << "❌ Failed to load GGUF model" << std::endl;
    return false;
}

bool EnhancedAIManager::has_direct_model() const {
    return use_direct_inference_ && direct_inference_ && direct_inference_->is_loaded();
}

std::string EnhancedAIManager::get_model_info() const {
    if (has_direct_model()) {
        return direct_inference_->get_model_info();
    }
    if (has_rwkv_model()) {
        return std::string("RWKV model loaded: ") + rwkv_model_path_ + " (minimal stub)";
    }
    return "No direct model loaded";
}

bool EnhancedAIManager::auto_detect_models() {
    std::cout << "🔍 Auto-detecting GGUF models..." << std::endl;
    
    direct_inference_ = bolt::ai::DirectGGUFFactory::create_auto_detect();
    if (direct_inference_->is_loaded()) {
        use_direct_inference_ = true;
        current_provider_ = "direct_gguf";
        std::cout << "✅ Auto-detected and loaded GGUF model!" << std::endl;
        return true;
    }
    
    std::cout << "⚠️ No GGUF models found for auto-detection" << std::endl;
    return false;
}

} // namespace ai  
} // namespace bolt

namespace bolt {
namespace ai {

EnhancedAIManager::EnhancedAIManager() {
    config_manager_ = std::make_unique<AIConfigManager>();
    
    std::cout << "🔧 Initializing Enhanced AI Manager..." << std::endl;
    
    // Try direct GGUF inference first
    direct_inference_ = bolt::ai::DirectGGUFFactory::create_auto_detect();
    if (direct_inference_->is_loaded()) {
        use_direct_inference_ = true;
        current_provider_ = "direct_gguf";
        std::cout << "✅ Direct GGUF model loaded - using direct inference!" << std::endl;
    } else {
        std::cout << "📋 No direct models found, loading HTTP providers..." << std::endl;
        
        // Load configuration
        config_manager_->load_config();
        
        // Initialize with active provider
        initialize_active_provider();
    }
    
    std::cout << "✅ Enhanced AI Manager initialized" << std::endl;
}

EnhancedAIManager::~EnhancedAIManager() {
    if (config_manager_) {
        config_manager_->save_config();
    }
}

bool EnhancedAIManager::initialize_active_provider() {
    std::string active_provider = config_manager_->get_active_provider();
    bolt::ai::AIHttpConfig config = config_manager_->get_active_config();
    
    try {
        http_client_ = std::make_unique<bolt::ai::AIHttpClient>(config);
        current_provider_ = active_provider;
        
        std::cout << "🔄 Initialized AI provider: " << active_provider 
                  << " (" << config.base_url << ")" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to initialize AI provider: " << e.what() << std::endl;
        return false;
    }
}

bolt::drawkern::AIInferenceResponse EnhancedAIManager::chat(const std::string& message, const std::string& session_id) {
    // Use direct GGUF inference if available and loaded
    if (use_direct_inference_ && direct_inference_ && direct_inference_->is_loaded()) {
        std::vector<std::string> history;
        if (!session_id.empty() && session_history_.find(session_id) != session_history_.end()) {
            history = session_history_[session_id];
        }
        auto response = direct_inference_->chat(message, history);
        if (!session_id.empty()) {
            session_history_[session_id].push_back("Human: " + message);
            if (response.success) session_history_[session_id].push_back("AI: " + response.response);
        }
        update_stats(response);
        return response;
    }

    // Minimal RWKV direct stub: not a real inference; returns helpful message
    if (use_rwkv_direct_ && !rwkv_model_path_.empty()) {
        bolt::drawkern::AIInferenceResponse response;
        response.success = true;
        response.response = "[RWKV stub] Model loaded: " + rwkv_model_path_ + ". Real RWKV generation will be added next phase.";
        update_stats(response);
        return response;
    }
    
    // Try HTTP client
    if (http_client_) {
        if (!session_id.empty()) session_history_[session_id].push_back("Human: " + message);
        auto response = http_client_->chat(message, session_id);
        if (!response.success && direct_inference_) {
            std::cout << "🔄 HTTP failed, using intelligent fallback..." << std::endl;
            std::vector<std::string> history;
            if (!session_id.empty() && session_history_.find(session_id) != session_history_.end()) {
                history = session_history_[session_id];
            }
            response = direct_inference_->chat(message, history);
        }
        if (response.success && !session_id.empty()) {
            session_history_[session_id].push_back("AI: " + response.response);
            auto& history = session_history_[session_id];
            if (history.size() > 40) history.erase(history.begin(), history.begin() + (history.size() - 40));
        }
        update_stats(response);
        return response;
    }
    
    // Final fallback to direct inference (even without loaded model)
    if (direct_inference_) {
        std::vector<std::string> history;
        if (!session_id.empty() && session_history_.find(session_id) != session_history_.end()) {
            history = session_history_[session_id];
        }
        auto response = direct_inference_->chat(message, history);
        if (!session_id.empty()) {
            session_history_[session_id].push_back("Human: " + message);
            if (response.success) session_history_[session_id].push_back("AI: " + response.response);
        }
        update_stats(response);
        return response;
    }
    
    bolt::drawkern::AIInferenceResponse error_response;
    error_response.error = "No AI provider available";
    error_response.success = false;
    update_stats(error_response);
    return error_response;
}

bolt::drawkern::AIInferenceResponse EnhancedAIManager::complete_code(const std::string& code, const std::string& language) {
    if (!http_client_) {
        bolt::drawkern::AIInferenceResponse error_response;
        error_response.error = "No AI provider initialized";
        error_response.success = false;
        return error_response;
    }
    auto response = http_client_->complete_code(code, language);
    update_stats(response);
    return response;
}

bolt::drawkern::AIInferenceResponse EnhancedAIManager::analyze_code(const std::string& code, const std::string& language) {
    std::string analysis_prompt = "Please analyze this " + language + " code and provide feedback:\n\n" + code + 
                                 "\n\nPlease provide:\n1. Code quality assessment\n2. Potential improvements\n3. Any bugs or issues\n4. Best practices suggestions";
    return chat(analysis_prompt, "analysis_session");
}

bool EnhancedAIManager::switch_provider(const std::string& provider_name) {
    if (!config_manager_->list_providers().size() || 
        std::find(config_manager_->list_providers().begin(), 
                 config_manager_->list_providers().end(), 
                 provider_name) == config_manager_->list_providers().end()) {
        std::cerr << "❌ Provider not found: " << provider_name << std::endl;
        return false;
    }
    config_manager_->set_active_provider(provider_name);
    return initialize_active_provider();
}

bool EnhancedAIManager::test_connection() {
    if (!http_client_) {
        std::cout << "❌ No AI provider initialized" << std::endl;
        return false;
    }
    return http_client_->test_connection();
}

bool EnhancedAIManager::test_all_providers() {
    auto results = config_manager_->test_all_providers();
    bool any_working = false;
    std::cout << "\n🔍 AI Provider Test Results:" << std::endl;
    std::cout << "=============================" << std::endl;
    for (const auto& [name, success] : results) {
        std::string status = success ? "✅ Working" : "❌ Failed";
        std::cout << "  " << name << ": " << status << std::endl;
        if (success) any_working = true;
    }
    if (!any_working) {
        std::cout << "\n⚠️  No working AI providers found!" << std::endl;
        std::cout << "💡 Suggestions:" << std::endl;
        std::cout << "  • Start a local llama.cpp server: ./server -m model.gguf" << std::endl;
        std::cout << "  • Install and run Ollama: ollama run llama2" << std::endl;
        std::cout << "  • Configure an OpenAI API key in the config file" << std::endl;
    } else {
        std::cout << "\n✅ Found working AI providers!" << std::endl;
    }
    return any_working;
}

void EnhancedAIManager::add_provider(const std::string& name, const bolt::ai::AIHttpConfig& config) {
    config_manager_->add_provider(name, config);
}

std::vector<bolt::ai::ProviderInfo> EnhancedAIManager::list_providers() const {
    std::vector<bolt::ai::ProviderInfo> providers;
    for (const auto& name : config_manager_->list_providers()) {
        bolt::ai::ProviderInfo info;
        info.name = name;
        info.config = config_manager_->get_provider_config(name);
        info.is_active = (name == current_provider_);
        providers.push_back(info);
    }
    return providers;
}

void EnhancedAIManager::create_session(const std::string& session_id) {
    session_history_[session_id] = {};
    std::cout << "📝 Created AI session: " << session_id << std::endl;
}

void EnhancedAIManager::destroy_session(const std::string& session_id) {
    session_history_.erase(session_id);
    std::cout << "🗑️  Destroyed AI session: " << session_id << std::endl;
}

std::vector<std::string> EnhancedAIManager::list_sessions() const {
    std::vector<std::string> sessions;
    for (const auto& [session_id, history] : session_history_) {
        sessions.push_back(session_id);
    }
    return sessions;
}

std::vector<std::string> EnhancedAIManager::get_session_history(const std::string& session_id) const {
    auto it = session_history_.find(session_id);
    if (it != session_history_.end()) {
        return it->second;
    }
    return {};
}

void EnhancedAIManager::clear_session_history(const std::string& session_id) {
    session_history_[session_id].clear();
    std::cout << "🧹 Cleared history for session: " << session_id << std::endl;
}

bolt::ai::AIStats EnhancedAIManager::get_statistics() const {
    return stats_;
}

void EnhancedAIManager::reset_statistics() {
    stats_ = bolt::ai::AIStats{};
    std::cout << "🔄 Reset AI statistics" << std::endl;
}

void EnhancedAIManager::update_config(const bolt::ai::AIHttpConfig& config) {
    if (http_client_) {
        http_client_->update_config(config);
        config_manager_->add_provider(current_provider_, config);
        std::cout << "🔄 Updated configuration for provider: " << current_provider_ << std::endl;
    }
}

bool EnhancedAIManager::is_ready() const {
    return has_direct_model() || has_rwkv_model() || (http_client_ != nullptr);
}

std::string EnhancedAIManager::get_current_provider() const {
    return current_provider_;
}

bolt::ai::AIHttpConfig EnhancedAIManager::get_current_config() const {
    if (http_client_) {
        return http_client_->get_config();
    }
    return bolt::ai::AIHttpConfig{};
}

void EnhancedAIManager::update_stats(const bolt::drawkern::AIInferenceResponse& response) {
    stats_.total_requests++;
    if (response.success) {
        stats_.successful_requests++;
        stats_.total_tokens_generated += response.tokens_generated;
        stats_.total_tokens_processed += response.tokens_processed;
        if (stats_.successful_requests > 0) {
            float total_time = stats_.average_inference_time_ms * (stats_.successful_requests - 1) + response.inference_time_ms;
            stats_.average_inference_time_ms = total_time / stats_.successful_requests;
        }
    } else {
        stats_.failed_requests++;
    }
}

// Minimal RWKV load (stub)
bool EnhancedAIManager::load_rwkv_model(const std::string& model_path) {
    rwkv_model_path_ = model_path;
    use_rwkv_direct_ = true;
    current_provider_ = "direct_rwkv";
    std::cout << "✅ RWKV model path set: " << model_path << " (stub)" << std::endl;
    return true;
}

bool EnhancedAIManager::has_rwkv_model() const {
    return use_rwkv_direct_ && !rwkv_model_path_.empty();
}

// Auto-detection and setup utilities
namespace AutoSetup {

bool detect_and_setup_local_server() {
    std::cout << "🔍 Detecting local AI servers..." << std::endl;
    std::vector<std::tuple<std::string, std::string, bolt::ai::APIType>> servers = {
        {"http://localhost:8080", "llama.cpp", bolt::ai::APIType::LLAMA_CPP},
        {"http://localhost:11434", "Ollama", bolt::ai::APIType::CUSTOM},
        {"http://127.0.0.1:5000", "Local Flask", bolt::ai::APIType::CUSTOM},
        {"http://127.0.0.1:8000", "Local FastAPI", bolt::ai::APIType::CUSTOM},
    };
    for (const auto& [url, name, type] : servers) {
        std::cout << "  Testing " << name << " at " << url << "..." << std::endl;
        bolt::ai::AIHttpConfig config;
        config.base_url = url;
        config.api_type = type;
        config.verify_ssl = false;
        config.timeout_seconds = 5;
        try {
            bolt::ai::AIHttpClient client(config);
            if (client.test_connection()) {
                std::cout << "✅ Found working " << name << " server!" << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cout << "  ❌ " << name << " not available" << std::endl;
        }
    }
    std::cout << "⚠️  No local AI servers detected" << std::endl;
    return false;
}

void print_setup_instructions() {
    std::cout << "\n🛠️  AI Setup Instructions:" << std::endl;
    std::cout << " - Local llama.cpp server: ./server -m model.gguf" << std::endl;
    std::cout << " - Ollama: ollama run llama2" << std::endl;
    std::cout << " - OpenAI: set API key and provider in config" << std::endl;
}

bool quick_setup_wizard() {
    std::cout << "🧙 Starting quick setup wizard..." << std::endl;
    bool found = detect_and_setup_local_server();
    std::cout << (found ? "✅ Setup complete" : "⚠️  No servers detected - configure manually") << std::endl;
    return found;
}

} // namespace AutoSetup

} // namespace ai
} // namespace bolt
