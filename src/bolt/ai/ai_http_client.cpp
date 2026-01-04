#include "bolt/ai/ai_http_client.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#ifdef BOLT_HAVE_CURL
#include <curl/curl.h>
#endif

#ifdef BOLT_HAVE_JSONCPP
#include <json/json.h>
#endif

namespace bolt {
namespace ai {

#ifdef BOLT_HAVE_CURL

// Callback function to write HTTP response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t total_size = size * nmemb;
    userp->append((char*)contents, total_size);
    return total_size;
}

AIHttpClient::AIHttpClient(const AIHttpConfig& config) : config_(config) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
    
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    // Set up default options
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, config_.timeout_seconds);
    curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, config_.verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, config_.verify_ssl ? 2L : 0L);
    
    std::cout << "✅ AI HTTP Client initialized for: " << config_.base_url << std::endl;
}

AIHttpClient::~AIHttpClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
    curl_global_cleanup();
}

drawkern::AIInferenceResponse AIHttpClient::chat(const std::string& message, const std::string& session_id) {
    return send_request(create_chat_request(message, session_id));
}

drawkern::AIInferenceResponse AIHttpClient::complete_code(const std::string& code, const std::string& language) {
    return send_request(create_completion_request(code, language));
}

drawkern::AIInferenceResponse AIHttpClient::send_request(const HttpRequest& request) {
    drawkern::AIInferenceResponse response;
    
    if (!curl_) {
        response.error = "HTTP client not initialized";
        response.success = false;
        return response;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::string response_data;
    
    // Set URL
    curl_easy_setopt(curl_, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_data);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    for (const auto& header : request.headers) {
        headers = curl_slist_append(headers, (header.first + ": " + header.second).c_str());
    }
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Set POST data if present
    if (!request.body.empty()) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request.body.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, request.body.length());
    }
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl_);
    
    // Clean up headers
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
    
    if (res != CURLE_OK) {
        response.error = "HTTP request failed: " + std::string(curl_easy_strerror(res));
        response.success = false;
        return response;
    }
    
    // Get HTTP response code
    long response_code;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code != 200) {
        response.error = "HTTP error " + std::to_string(response_code) + ": " + response_data;
        response.success = false;
        return response;
    }
    
    // Parse response based on API type
    return parse_response(response_data, request.api_type);
}

HttpRequest AIHttpClient::create_chat_request(const std::string& message, const std::string& session_id) {
    HttpRequest request;
    request.url = config_.base_url + "/v1/chat/completions";
    request.api_type = config_.api_type;
    
    // Set headers
    request.headers["Content-Type"] = "application/json";
    if (!config_.api_key.empty()) {
        request.headers["Authorization"] = "Bearer " + config_.api_key;
    }
    
    // Create request body based on API type
    Json::Value root;
    Json::Value messages(Json::arrayValue);
    
    if (config_.api_type == APIType::OPENAI || config_.api_type == APIType::ANTHROPIC) {
        // OpenAI/Anthropic format
        root["model"] = config_.model_name;
        root["max_tokens"] = config_.max_tokens;
        root["temperature"] = config_.temperature;
        root["stream"] = false;
        
        // Add system message if configured
        if (!config_.system_prompt.empty()) {
            Json::Value system_msg;
            system_msg["role"] = "system";
            system_msg["content"] = config_.system_prompt;
            messages.append(system_msg);
        }
        
        // Add user message
        Json::Value user_msg;
        user_msg["role"] = "user";
        user_msg["content"] = message;
        messages.append(user_msg);
        
        root["messages"] = messages;
    } else if (config_.api_type == APIType::LLAMA_CPP) {
        // llama.cpp server format
        root["prompt"] = message;
        root["n_predict"] = config_.max_tokens;
        root["temperature"] = config_.temperature;
        root["stop"] = Json::Value(Json::arrayValue);
        root["stream"] = false;
    }
    
    Json::StreamWriterBuilder builder;
    request.body = Json::writeString(builder, root);
    
    return request;
}

HttpRequest AIHttpClient::create_completion_request(const std::string& code, const std::string& language) {
    HttpRequest request;
    request.api_type = config_.api_type;
    
    std::string prompt = "Complete this " + language + " code:\n" + code + "\n\nCompletion:";
    
    if (config_.api_type == APIType::OPENAI || config_.api_type == APIType::ANTHROPIC) {
        request.url = config_.base_url + "/v1/chat/completions";
        return create_chat_request(prompt, "");
    } else {
        request.url = config_.base_url + "/completion";
        
        Json::Value root;
        root["prompt"] = prompt;
        root["n_predict"] = config_.max_tokens;
        root["temperature"] = config_.temperature;
        root["stream"] = false;
        
        Json::StreamWriterBuilder builder;
        request.body = Json::writeString(builder, root);
    }
    
    return request;
}

drawkern::AIInferenceResponse AIHttpClient::parse_response(const std::string& response_data, APIType api_type) {
    drawkern::AIInferenceResponse response;
    
    try {
        Json::CharReaderBuilder builder;
        Json::Value root;
        std::string errors;
        
        std::istringstream stream(response_data);
        if (!Json::parseFromStream(builder, stream, &root, &errors)) {
            response.error = "Failed to parse JSON response: " + errors;
            response.success = false;
            return response;
        }
        
        if (api_type == APIType::OPENAI || api_type == APIType::ANTHROPIC) {
            // OpenAI/Anthropic response format
            if (root.isMember("choices") && root["choices"].isArray() && !root["choices"].empty()) {
                Json::Value choice = root["choices"][0];
                if (choice.isMember("message") && choice["message"].isMember("content")) {
                    response.response = choice["message"]["content"].asString();
                    response.success = true;
                }
                
                // Get usage info if available
                if (root.isMember("usage")) {
                    Json::Value usage = root["usage"];
                    if (usage.isMember("prompt_tokens")) {
                        response.tokens_processed = usage["prompt_tokens"].asInt();
                    }
                    if (usage.isMember("completion_tokens")) {
                        response.tokens_generated = usage["completion_tokens"].asInt();
                    }
                }
            } else if (root.isMember("error")) {
                response.error = "API Error: " + root["error"]["message"].asString();
                response.success = false;
            }
        } else if (api_type == APIType::LLAMA_CPP) {
            // llama.cpp server response format
            if (root.isMember("content")) {
                response.response = root["content"].asString();
                response.success = true;
            } else if (root.isMember("error")) {
                response.error = "llama.cpp Error: " + root["error"].asString();
                response.success = false;
            }
            
            // Get tokens info if available
            if (root.isMember("tokens_predicted")) {
                response.tokens_generated = root["tokens_predicted"].asInt();
            }
            if (root.isMember("tokens_evaluated")) {
                response.tokens_processed = root["tokens_evaluated"].asInt();
            }
        }
        
        if (!response.success && response.error.empty()) {
            response.error = "Unknown response format";
            response.success = false;
        }
        
    } catch (const std::exception& e) {
        response.error = "Exception parsing response: " + std::string(e.what());
        response.success = false;
    }
    
    return response;
}

bool AIHttpClient::test_connection() {
    std::cout << "🔍 Testing connection to: " << config_.base_url << std::endl;
    
    HttpRequest request;
    request.url = config_.base_url + "/v1/models";
    request.headers["Content-Type"] = "application/json";
    if (!config_.api_key.empty()) {
        request.headers["Authorization"] = "Bearer " + config_.api_key;
    }
    
    drawkern::AIInferenceResponse response = send_request(request);
    
    if (response.success) {
        std::cout << "✅ Connection test successful!" << std::endl;
        return true;
    } else {
        std::cout << "❌ Connection test failed: " << response.error << std::endl;
        return false;
    }
}

void AIHttpClient::update_config(const AIHttpConfig& config) {
    config_ = config;
    std::cout << "🔄 Updated HTTP client configuration" << std::endl;
}

#else // !BOLT_HAVE_CURL

// Stub implementations when CURL is not available
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    return size * nmemb; // Dummy implementation
}

AIHttpClient::AIHttpClient(const AIHttpConfig& config) : config_(config), curl_(nullptr) {
    std::cout << "⚠️  HTTP client initialized without CURL support" << std::endl;
}

AIHttpClient::~AIHttpClient() {
    // Nothing to cleanup when CURL is not available
}

drawkern::AIInferenceResponse AIHttpClient::chat(const std::string& message, const std::string& session_id) {
    drawkern::AIInferenceResponse response;
    response.success = false;
    response.error = "HTTP client built without CURL support";
    return response;
}

drawkern::AIInferenceResponse AIHttpClient::complete_code(const std::string& code, const std::string& language) {
    drawkern::AIInferenceResponse response;
    response.success = false;
    response.error = "HTTP client built without CURL support";
    return response;
}

bool AIHttpClient::test_connection() {
    std::cout << "❌ HTTP client built without CURL support" << std::endl;
    return false;
}

void AIHttpClient::update_config(const AIHttpConfig& config) {
    config_ = config;
    std::cout << "⚠️  Updated HTTP client configuration (no CURL support)" << std::endl;
}

drawkern::AIInferenceResponse AIHttpClient::send_request(const HttpRequest& request) {
    drawkern::AIInferenceResponse response;
    response.success = false;
    response.error = "HTTP client built without CURL support";
    return response;
}

HttpRequest AIHttpClient::create_chat_request(const std::string& message, const std::string& session_id) {
    return HttpRequest{}; // Return empty request
}

HttpRequest AIHttpClient::create_completion_request(const std::string& code, const std::string& language) {
    return HttpRequest{}; // Return empty request
}

drawkern::AIInferenceResponse AIHttpClient::parse_response(const std::string& response_data, APIType api_type) {
    drawkern::AIInferenceResponse response;
    response.success = false;
    response.error = "HTTP client built without CURL support";
    return response;
}

#endif // BOLT_HAVE_CURL

} // namespace ai
} // namespace bolt
