#include "bolt/ai/direct_gguf_inference.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <array>
#include <filesystem>

#ifdef LLAMA_AVAILABLE
#include <llama.h>
#endif

namespace bolt {
namespace ai {

struct DirectGGUFInference::ModelData {
#ifdef LLAMA_AVAILABLE
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    const llama_vocab* vocab = nullptr;
    llama_sampler* sampler = nullptr;
#endif
    std::string model_info;
    bool initialized = false;
    std::string model_path;
};

DirectGGUFInference::DirectGGUFInference() 
    : model_data_(std::make_unique<ModelData>())
    , model_loaded_(false) {
    std::cout << "📦 DirectGGUFInference initialized" << std::endl;
}

DirectGGUFInference::~DirectGGUFInference() {
#ifdef LLAMA_AVAILABLE
    if (model_data_->sampler) {
        llama_sampler_free(model_data_->sampler);
        model_data_->sampler = nullptr;
    }
    if (model_data_->ctx) {
        llama_free(model_data_->ctx);
        model_data_->ctx = nullptr;
    }
    if (model_data_->model) {
        llama_model_free(model_data_->model);
        model_data_->model = nullptr;
    }
    llama_backend_free();
#endif
}

bool DirectGGUFInference::load_model(const std::string& model_path) {
    model_path_ = model_path;
    model_data_->model_path = model_path;

    std::cout << "📥 Loading GGUF model: " << model_path << std::endl;

    std::ifstream file(model_path);
    if (!file.good()) {
        std::cout << "❌ Model file not found: " << model_path << std::endl;
        return false;
    }

#ifndef LLAMA_AVAILABLE
    std::cout << "⚠️  llama.cpp not available - using fallback responses" << std::endl;
    model_loaded_ = true;
    model_data_->model_info = "Fallback mode: llama.cpp not available";
    return true;
#else
    // Initialize backend
    llama_backend_init();

    llama_model_params model_params = llama_model_default_params();
    model_params.use_mmap = true;
    model_params.use_mlock = false;

    model_data_->model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model_data_->model) {
        std::cout << "❌ Failed to load model via llama.cpp" << std::endl;
        return false;
    }

    // Get vocabulary from model
    model_data_->vocab = llama_model_get_vocab(model_data_->model);

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048;

    model_data_->ctx = llama_init_from_model(model_data_->model, ctx_params);
    if (!model_data_->ctx) {
        std::cout << "❌ Failed to create llama context" << std::endl;
        llama_model_free(model_data_->model);
        model_data_->model = nullptr;
        return false;
    }

    // Initialize sampler chain
    auto sparams = llama_sampler_chain_default_params();
    model_data_->sampler = llama_sampler_chain_init(sparams);
    llama_sampler_chain_add(model_data_->sampler, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(model_data_->sampler, llama_sampler_init_dist(0));

    model_loaded_ = true;
    model_data_->initialized = true;

    std::ostringstream info;
    info << "GGUF Model loaded via llama.cpp: " << model_path << "\n";
    info << "Context length: " << ctx_params.n_ctx << "\n";
    model_data_->model_info = info.str();

    std::cout << "✅ Model loaded" << std::endl;
    std::cout << model_data_->model_info << std::endl;

    return true;
#endif
}

drawkern::AIInferenceResponse DirectGGUFInference::generate_text(
    const std::string& prompt, 
    int max_tokens,
    float temperature) {
    auto start_time = std::chrono::high_resolution_clock::now();

    drawkern::AIInferenceResponse response;
    response.success = false;

    if (!model_loaded_) {
        response = get_fallback_response(prompt);
        auto end_time = std::chrono::high_resolution_clock::now();
        response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        return response;
    }

    try {
        std::string generated_text = generate_internal(prompt, max_tokens, temperature);
        response.response = generated_text;
        response.success = true;
        response.tokens_generated = generated_text.length() / 4;
    } catch (const std::exception& e) {
        response.response = "Error during generation: " + std::string(e.what());
        response.error = response.response;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();

    return response;
}

drawkern::AIInferenceResponse DirectGGUFInference::chat(
    const std::string& message,
    const std::vector<std::string>& conversation_history) {
    std::string formatted_prompt = format_chat_prompt(message, conversation_history);
    return generate_text(formatted_prompt, 150, 0.7f);
}

std::string DirectGGUFInference::get_model_info() const {
    if (model_data_) {
        return model_data_->model_info;
    }
    return "No model loaded";
}

std::string DirectGGUFInference::generate_internal(const std::string& prompt, int max_tokens, float temperature) {
#ifndef LLAMA_AVAILABLE
    return get_smart_fallback(prompt);
#else
    if (!model_data_->initialized || !model_data_->ctx || !model_data_->vocab) {
        return get_smart_fallback(prompt);
    }

    // Build a simple prompt and run llama.cpp generation
    std::string system_prefix;
    system_prefix.reserve(prompt.size() + 64);
    system_prefix += prompt;

    llama_context* ctx = model_data_->ctx;
    const llama_vocab* vocab = model_data_->vocab;

    // Tokenize input using new API (vocab-based)
    std::vector<llama_token> tokens_in(system_prefix.size() + 8);
    int n_in = llama_tokenize(vocab, system_prefix.c_str(), (int)system_prefix.size(), 
                              tokens_in.data(), (int)tokens_in.size(), true, true);
    if (n_in < 0) return get_smart_fallback(prompt);
    tokens_in.resize(n_in);

    // Create batch for input tokens
    llama_batch batch = llama_batch_get_one(tokens_in.data(), (int)tokens_in.size());
    
    // Decode input
    if (llama_decode(ctx, batch) != 0) {
        return get_smart_fallback(prompt);
    }

    // Sampling loop using new sampler API
    std::string out;
    out.reserve((size_t)max_tokens * 4);

    int n_vocab = llama_vocab_n_tokens(vocab);
    llama_token eos_token = llama_vocab_eos(vocab);

    for (int i = 0; i < max_tokens; ++i) {
        // Sample next token using the sampler chain
        llama_token id = llama_sampler_sample(model_data_->sampler, ctx, -1);
        
        // Accept the token
        llama_sampler_accept(model_data_->sampler, id);

        if (id == eos_token) break;

        // Convert token to text
        char buf[256];
        int n = llama_token_to_piece(vocab, id, buf, sizeof(buf), 0, true);
        if (n > 0) out.append(buf, n);

        // Feed back the token
        llama_batch batch_next = llama_batch_get_one(&id, 1);
        if (llama_decode(ctx, batch_next) != 0) {
            break;
        }
    }

    // Reset sampler for next generation
    llama_sampler_reset(model_data_->sampler);

    return out.empty() ? get_smart_fallback(prompt) : out;
#endif
}

std::string DirectGGUFInference::format_chat_prompt(const std::string& message, const std::vector<std::string>& history) {
    std::ostringstream prompt;
    prompt << "You are a helpful AI programming assistant specialized in C++ development. ";
    prompt << "Provide clear, concise answers focused on coding help, debugging, and best practices.\n\n";

    int history_limit = 6; // Last 3 exchanges
    int start_idx = std::max(0, static_cast<int>(history.size()) - history_limit);

    for (int i = start_idx; i < static_cast<int>(history.size()); i++) {
        if (i % 2 == 0) {
            prompt << "Human: " << history[i] << "\n";
        } else {
            prompt << "Assistant: " << history[i] << "\n";
        }
    }

    prompt << "Human: " << message << "\n";
    prompt << "Assistant: ";

    return prompt.str();
}

bolt::drawkern::AIInferenceResponse DirectGGUFInference::get_fallback_response(const std::string& prompt) {
    bolt::drawkern::AIInferenceResponse response;
    response.success = true;
    response.inference_time_ms = 0.1f;
    response.tokens_generated = 1;
    response.tokens_processed = 1;

    response.response = get_smart_fallback(prompt);
    return response;
}

} // namespace ai
} // namespace bolt

// Implementation of helper methods as non-member functions in the bolt::ai namespace
namespace bolt {
namespace ai {

std::string DirectGGUFInference::get_smart_fallback(const std::string& input) {
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

    if (lower_input.find("code") != std::string::npos || 
        lower_input.find("function") != std::string::npos ||
        lower_input.find("class") != std::string::npos ||
        lower_input.find("template") != std::string::npos ||
        lower_input.find("pointer") != std::string::npos ||
        lower_input.find("c++") != std::string::npos) {
        return get_coding_help(input);
    }

    if (lower_input.find("algorithm") != std::string::npos ||
        lower_input.find("sort") != std::string::npos ||
        lower_input.find("search") != std::string::npos ||
        lower_input.find("complexity") != std::string::npos) {
        return get_algorithm_help(input);
    }

    if (lower_input.find("config") != std::string::npos ||
        lower_input.find("setup") != std::string::npos ||
        lower_input.find("install") != std::string::npos) {
        return "🔧 **Configuration Help**: To use real AI models:\n\n"
               "1. **Local GGUF Model**: Download a small model file and load it directly\n"
               "2. **llama.cpp Server**: Start a local server with `./server -m model.gguf`\n"
               "3. **Ollama**: Install Ollama and run `ollama run llama2`\n\n"
               "Currently using intelligent fallback responses. Load a GGUF model for full AI capabilities!";
    }

    return "I understand you're asking: \"" + input + "\"\n\n"
           "💡 I'm currently using smart fallback responses. For full AI capabilities:\n"
           "• Load a GGUF model file directly\n" 
           "• Start a local llama.cpp server\n"
           "• Configure an external AI provider\n\n"
           "I can still help with C++ coding questions, algorithms, and configuration guidance!";
}

std::string DirectGGUFInference::get_coding_help(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("smart pointer") != std::string::npos || 
        lower.find("unique_ptr") != std::string::npos ||
        lower.find("shared_ptr") != std::string::npos) {
        return "📘 **Smart Pointers in Modern C++**\n\n"
               "**`std::unique_ptr`** - Exclusive ownership:\n"
               "```cpp\n"
               "auto ptr = std::make_unique<MyClass>(args...);\n"
               "// Automatically deleted when out of scope\n"
               "```\n\n"
               "**`std::shared_ptr`** - Shared ownership:\n"
               "```cpp\n"
               "auto ptr = std::make_shared<MyClass>(args...);\n"
               "// Reference counted, deleted when last reference goes away\n"
               "```\n\n"
               "**Best Practice**: Prefer `unique_ptr` unless you need shared ownership.";
    }

    if (lower.find("template") != std::string::npos) {
        return "📘 **C++ Templates**\n\n"
               "**Function Template**:\n"
               "```cpp\n"
               "template<typename T>\n"
               "T max(T a, T b) { return (a > b) ? a : b; }\n"
               "```\n\n"
               "**Class Template**:\n"
               "```cpp\n"
               "template<typename T>\n"
               "class Container {\n"
               "    T data;\n"
               "public:\n"
               "    void set(T value) { data = value; }\n"
               "    T get() const { return data; }\n"
               "};\n"
               "```\n\n"
               "Templates enable generic programming and type-safe code reuse.";
    }

    return "💻 **C++ Coding Help**\n\n"
           "I can help with:\n"
           "• Smart pointers and memory management\n"
           "• Templates and generic programming\n"
           "• STL containers and algorithms\n"
           "• Modern C++ features (C++11/14/17/20)\n"
           "• Design patterns and best practices\n\n"
           "Please ask a specific question about your code!";
}

std::string DirectGGUFInference::get_algorithm_help(const std::string& input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("sort") != std::string::npos) {
        return "📊 **Sorting Algorithms**\n\n"
               "| Algorithm | Time (Avg) | Time (Worst) | Space | Stable |\n"
               "|-----------|------------|--------------|-------|--------|\n"
               "| Quick Sort | O(n log n) | O(n²) | O(log n) | No |\n"
               "| Merge Sort | O(n log n) | O(n log n) | O(n) | Yes |\n"
               "| Heap Sort | O(n log n) | O(n log n) | O(1) | No |\n\n"
               "**C++ STL**: Use `std::sort()` (introsort) for most cases.";
    }

    if (lower.find("search") != std::string::npos) {
        return "🔍 **Search Algorithms**\n\n"
               "**Binary Search** - O(log n) for sorted arrays:\n"
               "```cpp\n"
               "auto it = std::lower_bound(vec.begin(), vec.end(), target);\n"
               "if (it != vec.end() && *it == target) { /* found */ }\n"
               "```\n\n"
               "**Hash Table** - O(1) average:\n"
               "```cpp\n"
               "std::unordered_map<Key, Value> map;\n"
               "if (map.count(key)) { /* found */ }\n"
               "```";
    }

    return "📈 **Algorithm Complexity Guide**\n\n"
           "| Complexity | Name | Example |\n"
           "|------------|------|--------|\n"
           "| O(1) | Constant | Array access |\n"
           "| O(log n) | Logarithmic | Binary search |\n"
           "| O(n) | Linear | Linear search |\n"
           "| O(n log n) | Linearithmic | Merge sort |\n"
           "| O(n²) | Quadratic | Bubble sort |\n\n"
           "Ask about specific algorithms for detailed explanations!";
}

// DirectGGUFFactory implementationn
std::unique_ptr<DirectGGUFInference> DirectGGUFFactory::create_from_file(const std::string& model_path) {
    auto inference = std::make_unique<DirectGGUFInference>();
    if (inference->load_model(model_path)) {
        return inference;
    }
    return nullptr;
}

std::unique_ptr<DirectGGUFInference> DirectGGUFFactory::create_auto_detect() {
    auto models = find_available_models();
    
    if (!models.empty()) {
        auto inference = std::make_unique<DirectGGUFInference>();
        if (inference->load_model(models[0])) {
            return inference;
        }
    }
    
    // Return a fallback instance that uses smart responses
    return std::make_unique<DirectGGUFInference>();
}

std::vector<std::string> DirectGGUFFactory::find_available_models() {
    std::vector<std::string> models;
    
    // Common locations to search for GGUF models
    std::vector<std::string> search_paths = {
        "./models",
        "../models",
        "~/.local/share/bolt/models",
        "/usr/local/share/bolt/models",
        "~/.cache/huggingface/hub"
    };
    
    for (const auto& base_path : search_paths) {
        std::string expanded_path = base_path;
        
        // Expand ~ to home directory
        if (!expanded_path.empty() && expanded_path[0] == '~') {
            const char* home = std::getenv("HOME");
            if (home) {
                expanded_path = std::string(home) + expanded_path.substr(1);
            }
        }
        
        try {
            if (std::filesystem::exists(expanded_path)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(expanded_path)) {
                    if (entry.is_regular_file()) {
                        std::string ext = entry.path().extension().string();
                        if (ext == ".gguf" || ext == ".bin") {
                            models.push_back(entry.path().string());
                        }
                    }
                }
            }
        } catch (const std::exception&) {
            // Ignore filesystem errors
        }
    }
    
    return models;
}

bool DirectGGUFFactory::download_test_model(const std::string& output_path) {
    // For now, just return false - downloading models requires additional setup
    std::cout << "⚠️  Model download not implemented. Please download a GGUF model manually." << std::endl;
    std::cout << "   Suggested: https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF" << std::endl;
    return false;
}

} // namespace ai
} // namespace bolt
