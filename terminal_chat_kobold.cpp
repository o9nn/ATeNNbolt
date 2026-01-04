#include <iostream>
#include <string>
#include <vector>
#include <dlfcn.h> // For dynamic loading
#include <cstdlib>

// KoboldCpp function signatures (from model_adapter.h)
struct load_model_inputs {
    const char * model_filename = nullptr;
    bool use_mmap = true;
    bool use_mlock = false;
    bool use_smartcontext = false;
    bool unban_tokens = false;
    int n_ctx = 512;
    int n_seed = -1;
    int n_threads = 4;
    int n_batch = 8;
    int n_keep = 0;
    bool ignore_eos = false;
    const char * memory_f16 = nullptr;
    int n_gpu_layers = 0;
    int n_vocab = 32000;
    float rope_freq_base = 10000.0f;
    float rope_freq_scale = 1.0f;
    bool low_vram = false;
    bool mul_mat_q = true;
    const char * executable_path = nullptr;
    const char * debugmode = nullptr;
    const char * model_param = nullptr;
    const char * lora_filename = nullptr;
    const char * lora_base = nullptr;
    bool use_mmq = false;
    int blasbatchsize = 512;
    int blasthreads = 0;
    bool use_rowsplit = false;
    const char * tensor_split = nullptr;
    const char * gpulayers = nullptr;
    bool noblas = false;
    bool nommap = false;
    bool perplexity = false;
    bool winogrande = false;
    bool multiple_choice = false;
    const char * multiple_choice_tasks = nullptr;
    bool kquants = false;
    const char * model_type = nullptr;
    const char * multimodal_model = nullptr;
    bool benchmark = false;
    const char * forceversion = nullptr;
    const char * gpusplit = nullptr;
    const char * contextsize = nullptr;
    bool ropeconfig = false;
    float rope_freq_scale_value = 1.0f;
    float rope_freq_base_value = 10000.0f;
    bool bantokens = false;
    bool usefastforward = true;
    bool flash_attention = false;
    bool use_contextshift = true;
    int max_context_length = 4096;
    int blasbatchsize_value = 512;
    int threads = 4;
    int blasthreads_value = 4;
    bool quiet = false;
};

struct generation_inputs {
    int n_predict = 50;
    int n_keep = 0;
    int n_discard = 0;
    float temperature = 0.7f;
    int top_k = 40;
    float top_p = 0.9f;
    float min_p = 0.0f;
    float typical_p = 1.0f;
    float tfs_z = 1.0f;
    float top_a = 0.0f;
    float penalty_repeat = 1.1f;
    float penalty_freq = 0.0f;
    float penalty_present = 0.0f;
    int mirostat = 0;
    float mirostat_tau = 5.0f;
    float mirostat_eta = 0.1f;
    bool penalize_nl = true;
    int n_batch = 8;
    int n_threads = -1;
    const char *grammar = nullptr;
    float guidance_scale = 1.0f;
    const char *banned_tokens = nullptr;
    bool ignore_eos = false;
    const char *prompt = nullptr;
    bool quiet = false;
    const char *stop_sequence = nullptr;
    bool use_default_badwordsids = false;
    const char *sampler_order = nullptr;
    int sampler_len = 6;
    float dynatemp_range = 0.0f;
    float dynatemp_exponent = 1.0f;
    float smoothing_factor = 0.0f;
    int max_tokens = -1;
    const char *images = nullptr;
    bool stream_sse = false;
    const char *api_type = nullptr;
    const char *genkey = nullptr;
};

struct generation_outputs {
    int status = 0;
    char text[65536] = {0}; // Large buffer for generated text
};

// Function pointer types
typedef bool (*load_model_func)(load_model_inputs);
typedef generation_outputs (*generate_func)(generation_inputs);

class KoboldTerminalChat {
private:
    void* kobold_lib;
    load_model_func load_model;
    generate_func generate;
    std::vector<std::string> conversation_history;

public:
    KoboldTerminalChat() : kobold_lib(nullptr), load_model(nullptr), generate(nullptr) {}
    
    ~KoboldTerminalChat() {
        if (kobold_lib) {
            dlclose(kobold_lib);
        }
    }
    
    bool initialize(const std::string& kobold_so_path, const std::string& model_path) {
        std::cout << "🔄 Loading KoboldCpp library: " << kobold_so_path << std::endl;
        
        // Load the shared library
        kobold_lib = dlopen(kobold_so_path.c_str(), RTLD_LAZY);
        if (!kobold_lib) {
            std::cerr << "❌ Cannot load KoboldCpp library: " << dlerror() << std::endl;
            return false;
        }
        
        // Get function pointers (using mangled C++ names from nm output)
        load_model = (load_model_func)dlsym(kobold_lib, "_Z18gpttype_load_model17load_model_inputs10FileFormat19FileFormatExtraMeta");
        if (!load_model) {
            std::cerr << "❌ Cannot find load_model function: " << dlerror() << std::endl;
            return false;
        }
        
        generate = (generate_func)dlsym(kobold_lib, "_Z16gpttype_generate17generation_inputs");
        if (!generate) {
            std::cerr << "❌ Cannot find gpttype_generate function: " << dlerror() << std::endl;
            return false;
        }
        
        std::cout << "✅ KoboldCpp library loaded successfully" << std::endl;
        
        // Load model
        std::cout << "📥 Loading model: " << model_path << std::endl;
        
        load_model_inputs inputs = {};
        inputs.model_filename = model_path.c_str();
        inputs.n_ctx = 4096;
        inputs.n_threads = 4;
        inputs.n_batch = 512;
        inputs.max_context_length = 4096;
        inputs.blasbatchsize = 512;
        inputs.blasthreads = 4;
        inputs.use_mmap = true;
        inputs.use_contextshift = true;
        inputs.usefastforward = true;
        
        bool success = load_model(inputs);
        if (!success) {
            std::cerr << "❌ Failed to load model" << std::endl;
            return false;
        }
        
        std::cout << "✅ Model loaded successfully!" << std::endl;
        return true;
    }
    
    std::string chat(const std::string& message) {
        // Format chat prompt
        std::string prompt = format_chat_prompt(message);
        
        generation_inputs inputs = {};
        inputs.prompt = prompt.c_str();
        inputs.n_predict = 150;
        inputs.temperature = 0.7f;
        inputs.top_k = 40;
        inputs.top_p = 0.9f;
        inputs.penalty_repeat = 1.1f;
        inputs.n_threads = 4;
        inputs.quiet = false;
        
        generation_outputs outputs = generate(inputs);
        
        if (outputs.status == 1) {
            std::string response(outputs.text);
            
            // Add to conversation history
            conversation_history.push_back(message);
            conversation_history.push_back(response);
            
            // Keep history manageable
            if (conversation_history.size() > 10) {
                conversation_history.erase(conversation_history.begin(), conversation_history.begin() + 2);
            }
            
            return response;
        }
        
        return "❌ Generation failed";
    }
    
private:
    std::string format_chat_prompt(const std::string& message) {
        std::string prompt = "You are a helpful AI assistant specialized in programming and technical topics.\n\n";
        
        // Add recent conversation history
        for (size_t i = 0; i < conversation_history.size(); i += 2) {
            if (i + 1 < conversation_history.size()) {
                prompt += "Human: " + conversation_history[i] + "\n";
                prompt += "Assistant: " + conversation_history[i + 1] + "\n\n";
            }
        }
        
        prompt += "Human: " + message + "\n";
        prompt += "Assistant: ";
        
        return prompt;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "🚀 Bolt Terminal Chat with Direct GGUF Access\n";
    std::cout << "============================================\n\n";
    
    // Default paths
    std::string kobold_so = "/workspaces/bolt-cppml/ggml/kobold.cpp/koboldcpp_default.so";
    std::string model_path = "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf";
    
    // Allow overriding paths
    if (argc > 1) {
        model_path = argv[1];
    }
    if (argc > 2) {
        kobold_so = argv[2];
    }
    
    KoboldTerminalChat chat;
    
    if (!chat.initialize(kobold_so, model_path)) {
        std::cerr << "❌ Failed to initialize chat system" << std::endl;
        return 1;
    }
    
    std::cout << "\n💬 Chat initialized! Type 'quit', 'exit', or '/quit' to end.\n";
    std::cout << "Type your message and press Enter:\n\n";
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        if (input == "quit" || input == "exit" || input == "/quit" || input == "/exit") {
            std::cout << "👋 Goodbye!\n";
            break;
        }
        
        std::cout << "🤖 " << chat.chat(input) << "\n\n";
    }
    
    return 0;
}
