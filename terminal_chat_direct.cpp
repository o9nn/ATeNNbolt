#include <iostream>
#include <string>
#include <cstdio>

// Include KoboldCpp headers
extern "C" {
#include "/workspaces/bolt-cppml/ggml/kobold.cpp/model_adapter.h"
}

int main() {
    std::cout << "🚀 Bolt Terminal Chat with KoboldCpp Integration\n";
    std::cout << "===============================================\n\n";
    
    std::string model_path = "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf";
    
    std::cout << "📥 Preparing to load model: " << model_path << std::endl;
    
    // Initialize KoboldCpp inputs
    load_model_inputs inputs = {};
    inputs.model_filename = model_path.c_str();
    inputs.max_context_length = 4096;
    inputs.threads = 4;
    inputs.blasthreads = 4;
    inputs.blasbatchsize = 512;
    inputs.quiet = false;
    
    // Detect file format
    FileFormatExtraMeta meta = {};
    FileFormat format = check_file_format(model_path, &meta);
    
    std::cout << "📊 Detected format: " << static_cast<int>(format) << std::endl;
    
    // Load model
    ModelLoadResult result = gpttype_load_model(inputs, format, meta);
    
    if (result == ModelLoadResult::SUCCESS) {
        std::cout << "✅ Model loaded successfully!" << std::endl;
        
        // Interactive chat loop
        std::string input;
        while (true) {
            std::cout << "\n> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            if (input == "quit" || input == "exit" || input == "/quit") {
                break;
            }
            
            // Format chat prompt
            std::string prompt = "You are a helpful AI assistant.\n\nHuman: " + input + "\nAssistant: ";
            
            // Generate response
            generation_inputs gen_inputs = {};
            gen_inputs.prompt = prompt.c_str();
            gen_inputs.n_predict = 150;
            gen_inputs.temperature = 0.7f;
            gen_inputs.top_k = 40;
            gen_inputs.top_p = 0.9f;
            gen_inputs.penalty_repeat = 1.1f;
            gen_inputs.quiet = false;
            
            generation_outputs outputs = gpttype_generate(gen_inputs);
            
            if (outputs.status == 1) {
                std::cout << "🤖 " << outputs.text << std::endl;
            } else {
                std::cout << "❌ Generation failed" << std::endl;
            }
        }
        
        std::cout << "👋 Goodbye!" << std::endl;
    } else {
        std::cout << "❌ Failed to load model. Result: " << static_cast<int>(result) << std::endl;
        return 1;
    }
    
    return 0;
}
