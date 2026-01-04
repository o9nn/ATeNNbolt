#include "bolt/ai/ai_http_client.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef BOLT_HAVE_JSONCPP
    #include <json/json.h>
#endif

namespace bolt {
namespace ai {

// AIHttpClientFactory implementation
std::unique_ptr<AIHttpClient> AIHttpClientFactory::create_openai_client(const std::string& api_key, const std::string& model) {
    AIHttpConfig config;
    config.base_url = "https://api.openai.com";
    config.api_key = api_key;
    config.model_name = model;
    config.api_type = APIType::OPENAI;
    config.system_prompt = "You are a helpful AI assistant specializing in software development and programming.";
    config.max_tokens = 1000;
    config.temperature = 0.7f;
    
    return std::make_unique<AIHttpClient>(config);
}

std::unique_ptr<AIHttpClient> AIHttpClientFactory::create_anthropic_client(const std::string& api_key, const std::string& model) {
    AIHttpConfig config;
    config.base_url = "https://api.anthropic.com";
    config.api_key = api_key;
    config.model_name = model;
    config.api_type = APIType::ANTHROPIC;
    config.system_prompt = "You are Claude, a helpful AI assistant created by Anthropic. You're great at helping with programming and development tasks.";
    config.max_tokens = 1000;
    config.temperature = 0.7f;
    
    return std::make_unique<AIHttpClient>(config);
}

std::unique_ptr<AIHttpClient> AIHttpClientFactory::create_llama_cpp_client(const std::string& base_url) {
    AIHttpConfig config;
    config.base_url = base_url;
    config.api_type = APIType::LLAMA_CPP;
    config.model_name = "local-model";
    config.system_prompt = "You are a helpful AI programming assistant. Help users with code, debugging, and development questions.";
    config.max_tokens = 512;
    config.temperature = 0.7f;
    config.verify_ssl = false;  // Often local servers don't use SSL
    
    return std::make_unique<AIHttpClient>(config);
}

std::unique_ptr<AIHttpClient> AIHttpClientFactory::create_ollama_client(const std::string& base_url, const std::string& model) {
    AIHttpConfig config;
    config.base_url = base_url;
    config.model_name = model;
    config.api_type = APIType::CUSTOM;
    config.system_prompt = "You are a helpful AI assistant. Focus on clear, practical advice for programming and development.";
    config.max_tokens = 512;
    config.temperature = 0.8f;
    config.verify_ssl = false;
    
    return std::make_unique<AIHttpClient>(config);
}

AIHttpConfig AIHttpClientFactory::detect_local_server() {
    std::vector<std::string> common_urls = {
        "http://localhost:8080",
        "http://localhost:11434",  // Ollama default
        "http://127.0.0.1:8080",
        "http://127.0.0.1:5000",   // Common Flask/Python server
        "http://127.0.0.1:3000",   // Common Node.js server
    };
    
    for (const auto& url : common_urls) {
        AIHttpConfig config;
        config.base_url = url;
        config.api_type = APIType::LLAMA_CPP;
        config.verify_ssl = false;
        config.timeout_seconds = 5;  // Quick timeout for detection
        
        try {
            AIHttpClient client(config);
            if (client.test_connection()) {
                std::cout << "✅ Detected local AI server at: " << url << std::endl;
                config.timeout_seconds = 30;  // Reset to normal timeout
                return config;
            }
        } catch (const std::exception& e) {
            // Continue to next URL
        }
    }
    
    // Return default config if nothing found
    AIHttpConfig default_config;
    default_config.base_url = "http://localhost:8080";
    default_config.api_type = APIType::LLAMA_CPP;
    std::cout << "⚠️ No local AI server detected. Using default: " << default_config.base_url << std::endl;
    return default_config;
}

std::vector<AIHttpConfig> AIHttpClientFactory::get_recommended_configs() {
    std::vector<AIHttpConfig> configs;
    
    // Local llama.cpp server
    AIHttpConfig llamacpp;
    llamacpp.base_url = "http://localhost:8080";
    llamacpp.api_type = APIType::LLAMA_CPP;
    llamacpp.model_name = "local-llama";
    configs.push_back(llamacpp);
    
    // Ollama
    AIHttpConfig ollama;
    ollama.base_url = "http://localhost:11434";
    ollama.api_type = APIType::CUSTOM;
    ollama.model_name = "llama2";
    configs.push_back(ollama);
    
    // OpenAI (requires API key)
    AIHttpConfig openai;
    openai.base_url = "https://api.openai.com";
    openai.api_type = APIType::OPENAI;
    openai.model_name = "gpt-3.5-turbo";
    configs.push_back(openai);
    
    return configs;
}

// AIConfigManager implementation
AIConfigManager::AIConfigManager() : active_provider_("default") {
    config_file_path_ = (std::filesystem::current_path() / "bolt_ai_config.json").string();
    load_default_providers();
}

AIConfigManager::~AIConfigManager() {
    save_config();
}

bool AIConfigManager::load_config(const std::string& config_file) {
#ifndef BOLT_HAVE_JSONCPP
    std::cout << "⚠️ JSON support not available, using default providers only" << std::endl;
    load_default_providers();
    return false;
#else
    std::string file_path = config_file.empty() ? config_file_path_ : config_file;
    
    if (!std::filesystem::exists(file_path)) {
        std::cout << "📄 Config file not found, creating default: " << file_path << std::endl;
        load_default_providers();
        return save_config(file_path);
    }
    
    try {
        std::ifstream file(file_path);
        Json::Value root;
        file >> root;
        
        if (root.isMember("active_provider")) {
            active_provider_ = root["active_provider"].asString();
        }
        
        if (root.isMember("providers")) {
            Json::Value providers = root["providers"];
            for (const auto& name : providers.getMemberNames()) {
                Json::Value provider = providers[name];
                
                AIHttpConfig config;
                config.base_url = provider.get("base_url", "http://localhost:8080").asString();
                config.api_key = provider.get("api_key", "").asString();
                config.model_name = provider.get("model_name", "local-model").asString();
                config.max_tokens = provider.get("max_tokens", 512).asInt();
                config.temperature = provider.get("temperature", 0.7).asFloat();
                config.system_prompt = provider.get("system_prompt", "You are a helpful AI assistant.").asString();
                config.timeout_seconds = provider.get("timeout_seconds", 30).asInt();
                config.verify_ssl = provider.get("verify_ssl", true).asBool();
                
                std::string api_type_str = provider.get("api_type", "llama_cpp").asString();
                if (api_type_str == "openai") config.api_type = APIType::OPENAI;
                else if (api_type_str == "anthropic") config.api_type = APIType::ANTHROPIC;
                else if (api_type_str == "custom") config.api_type = APIType::CUSTOM;
                else config.api_type = APIType::LLAMA_CPP;
                
                providers_[name] = config;
            }
        }
        
        std::cout << "✅ Loaded AI configuration from: " << file_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load AI config: " << e.what() << std::endl;
        load_default_providers();
        return false;
    }
#endif
}

bool AIConfigManager::save_config(const std::string& config_file) {
#ifndef BOLT_HAVE_JSONCPP
    std::cout << "⚠️ JSON support not available, cannot save config" << std::endl;
    return false;
#else
    std::string file_path = config_file.empty() ? config_file_path_ : config_file;
    
    try {
        Json::Value root;
        root["active_provider"] = active_provider_;
        
        Json::Value providers;
        for (const auto& [name, config] : providers_) {
            Json::Value provider;
            provider["base_url"] = config.base_url;
            provider["api_key"] = config.api_key;
            provider["model_name"] = config.model_name;
            provider["max_tokens"] = config.max_tokens;
            provider["temperature"] = config.temperature;
            provider["system_prompt"] = config.system_prompt;
            provider["timeout_seconds"] = config.timeout_seconds;
            provider["verify_ssl"] = config.verify_ssl;
            
            std::string api_type_str;
            switch (config.api_type) {
                case APIType::OPENAI: api_type_str = "openai"; break;
                case APIType::ANTHROPIC: api_type_str = "anthropic"; break;
                case APIType::CUSTOM: api_type_str = "custom"; break;
                default: api_type_str = "llama_cpp"; break;
            }
            provider["api_type"] = api_type_str;
            
            providers[name] = provider;
        }
        root["providers"] = providers;
        
        std::ofstream file(file_path);
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(root, &file);
        
        std::cout << "💾 Saved AI configuration to: " << file_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to save AI config: " << e.what() << std::endl;
        return false;
    }
#endif
}

void AIConfigManager::add_provider(const std::string& name, const AIHttpConfig& config) {
    providers_[name] = config;
    std::cout << "➕ Added AI provider: " << name << " (" << config.base_url << ")" << std::endl;
    
    if (providers_.size() == 1) {
        active_provider_ = name;
    }
}

void AIConfigManager::remove_provider(const std::string& name) {
    auto it = providers_.find(name);
    if (it != providers_.end()) {
        providers_.erase(it);
        std::cout << "➖ Removed AI provider: " << name << std::endl;
        
        if (active_provider_ == name && !providers_.empty()) {
            active_provider_ = providers_.begin()->first;
            std::cout << "🔄 Switched active provider to: " << active_provider_ << std::endl;
        }
    }
}

AIHttpConfig AIConfigManager::get_provider_config(const std::string& name) const {
    auto it = providers_.find(name);
    if (it != providers_.end()) {
        return it->second;
    }
    return create_default_config();
}

std::vector<std::string> AIConfigManager::list_providers() const {
    std::vector<std::string> names;
    for (const auto& [name, config] : providers_) {
        names.push_back(name);
    }
    return names;
}

void AIConfigManager::set_active_provider(const std::string& name) {
    if (providers_.count(name)) {
        active_provider_ = name;
        std::cout << "🔄 Switched active AI provider to: " << name << std::endl;
    } else {
        std::cerr << "❌ Provider not found: " << name << std::endl;
    }
}

AIHttpConfig AIConfigManager::get_active_config() const {
    return get_provider_config(active_provider_);
}

std::map<std::string, bool> AIConfigManager::test_all_providers() {
    std::map<std::string, bool> results;
    
    std::cout << "🔍 Testing all AI providers..." << std::endl;
    for (const auto& [name, config] : providers_) {
        std::cout << "  Testing " << name << "..." << std::endl;
        results[name] = test_provider(name);
    }
    
    return results;
}

bool AIConfigManager::test_provider(const std::string& name) {
    auto it = providers_.find(name);
    if (it == providers_.end()) {
        std::cout << "❌ Provider not found: " << name << std::endl;
        return false;
    }
    
    try {
        AIHttpClient client(it->second);
        bool success = client.test_connection();
        
        if (success) {
            std::cout << "✅ " << name << " - Connection successful" << std::endl;
        } else {
            std::cout << "❌ " << name << " - Connection failed" << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cout << "❌ " << name << " - Error: " << e.what() << std::endl;
        return false;
    }
}

void AIConfigManager::load_default_providers() {
    std::cout << "📋 Loading default AI providers..." << std::endl;
    
    // Default local llama.cpp server
    AIHttpConfig llamacpp = create_default_config();
    add_provider("llama_cpp", llamacpp);
    
    // Try to detect local server
    AIHttpConfig detected = AIHttpClientFactory::detect_local_server();
    if (detected.base_url != llamacpp.base_url) {
        add_provider("detected_local", detected);
        active_provider_ = "detected_local";
    }
    
    // Ollama
    AIHttpConfig ollama;
    ollama.base_url = "http://localhost:11434";
    ollama.api_type = APIType::CUSTOM;
    ollama.model_name = "llama2";
    ollama.verify_ssl = false;
    add_provider("ollama", ollama);
    
    // OpenAI (template - requires API key)
    AIHttpConfig openai;
    openai.base_url = "https://api.openai.com";
    openai.api_type = APIType::OPENAI;
    openai.model_name = "gpt-3.5-turbo";
    openai.api_key = "your-openai-api-key-here";
    openai.system_prompt = "You are a helpful AI assistant specializing in software development and programming.";
    add_provider("openai_template", openai);
}

AIHttpConfig AIConfigManager::create_default_config() const {
    AIHttpConfig config;
    config.base_url = "http://localhost:8080";
    config.api_type = APIType::LLAMA_CPP;
    config.model_name = "local-model";
    config.max_tokens = 512;
    config.temperature = 0.7f;
    config.system_prompt = "You are a helpful AI programming assistant. Help users with code, debugging, and development questions.";
    config.timeout_seconds = 30;
    config.verify_ssl = false;
    
    return config;
}

} // namespace ai
} // namespace bolt
