#pragma once

#include <string>
#include <memory>
#include <vector>
#include "../drawkern/ai_integration.hpp"

namespace bolt {
namespace ai {

// Direct GGUF file loader and inference engine
class DirectGGUFInference {
public:
    DirectGGUFInference();
    ~DirectGGUFInference();

    // Load a GGUF model from file
    bool load_model(const std::string& model_path);
    
    // Generate text completion
    drawkern::AIInferenceResponse generate_text(
        const std::string& prompt, 
        int max_tokens = 100,
        float temperature = 0.7f
    );
    
    // Chat-specific inference with conversation context
    drawkern::AIInferenceResponse chat(
        const std::string& message,
        const std::vector<std::string>& conversation_history = {}
    );
    
    // Check if model is loaded
    bool is_loaded() const { return model_loaded_; }
    
    // Get model info
    std::string get_model_info() const;
    
    // Fallback responses for when no model is available
    drawkern::AIInferenceResponse get_fallback_response(const std::string& input);

private:
    struct ModelData;
    std::unique_ptr<ModelData> model_data_;
    bool model_loaded_;
    std::string model_path_;
    
    // Internal text generation
    std::string generate_internal(const std::string& prompt, int max_tokens, float temperature);
    
    // Conversation formatting
    std::string format_chat_prompt(const std::string& message, const std::vector<std::string>& history);
    
    // Smart fallback responses
    std::string get_smart_fallback(const std::string& input);
    std::string get_coding_help(const std::string& input);
    std::string get_algorithm_help(const std::string& input);
};

// Factory for creating GGUF inference engines
class DirectGGUFFactory {
public:
    // Create from model file
    static std::unique_ptr<DirectGGUFInference> create_from_file(const std::string& model_path);
    
    // Create with auto-detection of available models
    static std::unique_ptr<DirectGGUFInference> create_auto_detect();
    
    // Find available GGUF models in common locations
    static std::vector<std::string> find_available_models();
    
    // Download a tiny model for testing (if needed)
    static bool download_test_model(const std::string& output_path);
};

} // namespace ai
} // namespace bolt
