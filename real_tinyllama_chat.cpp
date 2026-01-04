#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

// Include llama.cpp headers
extern "C" {
#include "ggml/llama.cpp/include/llama.h"
}

class RealTinyLlamaChat {
private:
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    std::string model_path;
    bool model_loaded = false;

public:
    ~RealTinyLlamaChat() {
        if (ctx) {
            llama_free(ctx);
        }
        if (model) {
            llama_free_model(model);
        }
        llama_backend_free();
    }

    bool find_and_load_model() {
        std::vector<std::string> search_paths = {
            "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/",
            "./models/TinyLlama-1.1B-Chat-v1.0-GGUF/",
            "models/TinyLlama-1.1B-Chat-v1.0-GGUF/"
        };

        std::cout << "🔍 Searching for TinyLlama GGUF models..." << std::endl;
        
        for (const auto& search_path : search_paths) {
            std::cout << "  📁 Checking: " << search_path << std::endl;
            
            if (std::filesystem::exists(search_path)) {
                // Look for GGUF files (prefer smaller quantized models)
                std::vector<std::string> preferred_models = {
                    "tinyllama-1.1b-chat-v1.0.Q2_K.gguf",
                    "tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf",
                    "tinyllama-1.1b-chat-v1.0.Q4_0.gguf"
                };
                
                for (const auto& model_name : preferred_models) {
                    std::string full_path = search_path + model_name;
                    if (std::filesystem::exists(full_path)) {
                        model_path = full_path;
                        std::cout << "  ✅ Found preferred model: " << model_path << std::endl;
                        return load_model();
                    }
                }
                
                // If no preferred model, take any GGUF file
                for (const auto& entry : std::filesystem::directory_iterator(search_path)) {
                    if (entry.path().extension() == ".gguf") {
                        model_path = entry.path().string();
                        std::cout << "  ✅ Found GGUF model: " << model_path << std::endl;
                        return load_model();
                    }
                }
            } else {
                std::cout << "  ❌ Path not found: " << search_path << std::endl;
            }
        }
        
        std::cout << "❌ No GGUF models found in any search path" << std::endl;
        return false;
    }
    
    bool load_model() {
        std::cout << "📥 Loading model: " << model_path << std::endl;
        
        // Initialize llama backend
        llama_backend_init();
        
        // Load model
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = 0; // CPU only for compatibility
        
        model = llama_load_model_from_file(model_path.c_str(), model_params);
        if (!model) {
            std::cerr << "❌ Failed to load model" << std::endl;
            return false;
        }
        
        // Create context
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = 2048; // Context size
        ctx_params.seed = -1;    // Random seed
        
        ctx = llama_new_context_with_model(model, ctx_params);
        if (!ctx) {
            std::cerr << "❌ Failed to create context" << std::endl;
            llama_free_model(model);
            model = nullptr;
            return false;
        }
        
        model_loaded = true;
        std::cout << "✅ Model loaded successfully!" << std::endl;
        return true;
    }
    
    std::string chat(const std::string& message) {
        if (!model_loaded) {
            return "❌ No model loaded. Please ensure TinyLlama GGUF model is available.";
        }
        
        // Format prompt for chat
        std::string prompt = "<|system|>\nYou are a helpful AI assistant.</s>\n<|user|>\n" + message + "</s>\n<|assistant|>\n";
        
        // Tokenize prompt
        std::vector<llama_token> tokens = tokenize(prompt);
        
        if (tokens.empty()) {
            return "❌ Failed to tokenize input";
        }
        
        // Clear previous context and evaluate prompt
        llama_kv_cache_clear(ctx);
        
        int n_eval = 0;
        const int n_batch = 512;
        
        // Process prompt in batches
        for (size_t i = 0; i < tokens.size(); i += n_batch) {
            int n_tokens = std::min(n_batch, (int)(tokens.size() - i));
            
            if (llama_decode(ctx, llama_batch_get_one(&tokens[i], n_tokens, n_eval, 0))) {
                return "❌ Failed to evaluate prompt";
            }
            
            n_eval += n_tokens;
        }
        
        // Generate response
        std::string response;
        const int max_tokens = 256;
        
        for (int i = 0; i < max_tokens; i++) {
            llama_token new_token = sample_next_token();
            
            if (new_token == llama_token_eos(model)) {
                break;
            }
            
            // Convert token to text and add to response
            char buf[256];
            int n = llama_token_to_piece(model, new_token, buf, sizeof(buf), 0, true);
            if (n > 0) {
                response.append(buf, n);
            }
            
            // Evaluate the new token
            if (llama_decode(ctx, llama_batch_get_one(&new_token, 1, n_eval, 0))) {
                break;
            }
            
            n_eval++;
        }
        
        return response;
    }
    
private:
    std::vector<llama_token> tokenize(const std::string& text) {
        std::vector<llama_token> tokens(text.length() + 64);
        int n = llama_tokenize(model, text.c_str(), text.length(), tokens.data(), tokens.size(), true, true);
        
        if (n < 0) {
            tokens.resize(-n);
            n = llama_tokenize(model, text.c_str(), text.length(), tokens.data(), tokens.size(), true, true);
        }
        
        if (n >= 0) {
            tokens.resize(n);
        } else {
            tokens.clear();
        }
        
        return tokens;
    }
    
    llama_token sample_next_token() {
        auto* logits = llama_get_logits(ctx);
        auto n_vocab = llama_n_vocab(model);
        
        // Simple greedy sampling (take highest probability token)
        llama_token max_token = 0;
        float max_logit = logits[0];
        
        for (llama_token token = 1; token < n_vocab; token++) {
            if (logits[token] > max_logit) {
                max_logit = logits[token];
                max_token = token;
            }
        }
        
        return max_token;
    }
    
public:
    void show_model_info() {
        if (model_loaded) {
            std::cout << "\n📋 Model Information:" << std::endl;
            std::cout << "  📄 File: " << std::filesystem::path(model_path).filename().string() << std::endl;
            std::cout << "  📍 Path: " << model_path << std::endl;
            
            // Show file size
            try {
                auto size = std::filesystem::file_size(model_path);
                std::cout << "  📊 Size: " << (size / 1024 / 1024) << " MB" << std::endl;
            } catch (...) {
                std::cout << "  📊 Size: Unknown" << std::endl;
            }
            
            // Show model details
            std::cout << "  🔢 Vocab size: " << llama_n_vocab(model) << std::endl;
            std::cout << "  📏 Context length: " << llama_n_ctx(ctx) << std::endl;
        } else {
            std::cout << "❌ No model loaded" << std::endl;
        }
    }
};

int main() {
    std::cout << "🦙 Real TinyLlama Chat Terminal (with llama.cpp)" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    RealTinyLlamaChat chat;
    
    // Try to find and load model
    if (!chat.find_and_load_model()) {
        std::cout << "❌ Could not load TinyLlama model. Exiting." << std::endl;
        return 1;
    }
    
    std::cout << "✅ TinyLlama model ready for real inference!" << std::endl;
    chat.show_model_info();
    
    std::cout << "\n💬 Chat with Real TinyLlama (type 'quit' to exit, 'info' for model details):" << std::endl;
    std::cout << "────────────────────────────────────────────────────────────────────────────" << std::endl;
    
    std::string input;
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
            chat.show_model_info();
            continue;
        }
        
        if (input.empty()) {
            continue;
        }
        
        std::cout << "\n🤖 TinyLlama: ";
        std::cout.flush();
        
        std::string response = chat.chat(input);
        std::cout << response << std::endl;
        std::cout << "────────────────────────────────────────────────────────────────────────────" << std::endl;
    }
    
    return 0;
}
