#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

namespace bolt {
namespace drawkern {

// Forward declarations for integration
class DISVM;
class StyxServer; 
class YaccGrammarSystem;
class DISProgram;
struct VMGlyph;
struct AIWorkbenchGlyph;

// DrawKern AI Integration - Complete GGML/RWKV model integration for AI inference

struct AIModelConfig {
    std::string model_type;      // "ggml", "rwkv", "llama"
    std::string model_path;      // Path to model file
    std::string model_name;      // Human-readable name
    
    // Model parameters
    int32_t context_length = 2048;
    int32_t vocab_size = 50277;
    int32_t n_layers = 12;
    int32_t n_head = 12;
    int32_t n_embd = 768;
    
    // Inference parameters
    float temperature = 0.8f;
    float top_p = 0.95f;
    int32_t top_k = 40;
    int32_t max_tokens = 256;
    
    // Hardware settings
    int32_t n_threads = 4;
    bool use_mmap = true;
    bool use_mlock = false;
    bool use_gpu = false;
    
    // DrawKern specific
    bool enable_chat = true;
    bool enable_completion = true;
    bool enable_analysis = true;
};

struct AIInferenceRequest {
    std::string prompt;
    std::string context;
    std::string task_type;  // "chat", "completion", "analysis"
    
    // Override parameters for this request
    std::map<std::string, float> parameters;
    
    // DrawKern integration
    std::string vm_id;      // Requesting VM
    std::string session_id; // Session for context
};

struct AIInferenceResponse {
    std::string response;
    std::string error;
    bool success = false;
    
    // Metadata
    float inference_time_ms = 0.0f;
    int32_t tokens_generated = 0;
    int32_t tokens_processed = 0;
    
    // DrawKern integration
    std::string vm_id;
    std::string session_id;
};

// Base class for AI models
class AIModel {
public:
    AIModel(const AIModelConfig& config);
    virtual ~AIModel();
    
    // Model lifecycle
    virtual bool load() = 0;
    virtual void unload() = 0;
    virtual bool is_loaded() const = 0;
    
    // Inference
    virtual AIInferenceResponse generate(const AIInferenceRequest& request) = 0;
    virtual AIInferenceResponse chat(const std::string& message, const std::string& context = "") = 0;
    virtual AIInferenceResponse complete_code(const std::string& code, const std::string& language = "cpp") = 0;
    
    // Model info
    virtual std::string get_model_info() const = 0;
    virtual std::vector<std::string> get_capabilities() const = 0;
    
    const AIModelConfig& get_config() const { return config_; }
    
protected:
    AIModelConfig config_;
    bool loaded_ = false;
};

// GGML-based model implementation
class GGMLModel : public AIModel {
public:
    GGMLModel(const AIModelConfig& config);
    ~GGMLModel();
    
    bool load() override;
    void unload() override;
    bool is_loaded() const override { return loaded_; }
    
    AIInferenceResponse generate(const AIInferenceRequest& request) override;
    AIInferenceResponse chat(const std::string& message, const std::string& context = "") override;
    AIInferenceResponse complete_code(const std::string& code, const std::string& language = "cpp") override;
    
    std::string get_model_info() const override;
    std::vector<std::string> get_capabilities() const override;
    
private:
    void* ggml_context_ = nullptr;  // ggml_context pointer
    void* model_data_ = nullptr;    // Model-specific data
    
    std::string tokenize_and_generate(const std::string& input, const std::map<std::string, float>& params);
    std::vector<int32_t> tokenize(const std::string& text);
    std::string detokenize(const std::vector<int32_t>& tokens);
};

// RWKV model implementation
class RWKVModel : public AIModel {
public:
    RWKVModel(const AIModelConfig& config);
    ~RWKVModel();
    
    bool load() override;
    void unload() override;
    bool is_loaded() const override { return loaded_; }
    
    AIInferenceResponse generate(const AIInferenceRequest& request) override;
    AIInferenceResponse chat(const std::string& message, const std::string& context = "") override;
    AIInferenceResponse complete_code(const std::string& code, const std::string& language = "cpp") override;
    
    std::string get_model_info() const override;
    std::vector<std::string> get_capabilities() const override;
    
private:
    void* rwkv_context_ = nullptr;   // RWKV context
    void* rwkv_state_ = nullptr;     // RWKV state
    
    std::string generate_with_rwkv(const std::string& input, const std::map<std::string, float>& params);
    void reset_state();
    void update_state(const std::vector<int32_t>& tokens);
};

// AI Model Manager for DrawKern
class DrawKernAIManager {
public:
    DrawKernAIManager();
    ~DrawKernAIManager();
    
    // Model management
    bool load_model(const std::string& model_id, const AIModelConfig& config);
    bool unload_model(const std::string& model_id);
    bool is_model_loaded(const std::string& model_id) const;
    
    // Inference
    AIInferenceResponse generate(const std::string& model_id, const AIInferenceRequest& request);
    AIInferenceResponse chat(const std::string& model_id, const std::string& message, const std::string& session_id = "");
    AIInferenceResponse complete_code(const std::string& model_id, const std::string& code, const std::string& language = "cpp");
    
    // DrawKern integration
    void register_vm_ai_handler(const std::string& vm_id, std::function<void(const AIInferenceResponse&)> handler);
    void broadcast_to_vm(const std::string& vm_id, const AIInferenceResponse& response);
    
    // Session management
    void create_session(const std::string& session_id, const std::string& model_id);
    void destroy_session(const std::string& session_id);
    std::vector<std::string> list_sessions() const;
    
    // Model registry
    std::vector<std::string> list_models() const;
    std::vector<std::string> list_available_model_types() const;
    AIModelConfig get_default_config(const std::string& model_type);
    
    // Statistics
    struct ModelStats {
        std::string model_id;
        size_t total_requests = 0;
        size_t successful_requests = 0;
        float average_inference_time_ms = 0.0f;
        size_t total_tokens_generated = 0;
    };
    
    std::vector<ModelStats> get_model_statistics() const;
    
private:
    std::map<std::string, std::unique_ptr<AIModel>> models_;
    std::map<std::string, std::string> sessions_;  // session_id -> model_id
    std::map<std::string, std::vector<std::string>> session_history_;
    std::map<std::string, std::function<void(const AIInferenceResponse&)>> vm_handlers_;
    std::map<std::string, ModelStats> model_stats_;
    
    std::unique_ptr<AIModel> create_model(const AIModelConfig& config);
    void update_stats(const std::string& model_id, const AIInferenceResponse& response);
};

// Factory for creating pre-configured AI models
class AIModelFactory {
public:
    // Standard model configurations
    static AIModelConfig create_rwkv_config(const std::string& model_path, const std::string& size = "14b");
    static AIModelConfig create_llama_config(const std::string& model_path, const std::string& size = "7b");
    static AIModelConfig create_code_model_config(const std::string& model_path);
    static AIModelConfig create_chat_model_config(const std::string& model_path);
    
    // DrawKern optimized configurations
    static AIModelConfig create_drawkern_workbench_config(const std::string& model_path);
    static AIModelConfig create_lightweight_config(const std::string& model_path);  // For resource-constrained VMs
    static AIModelConfig create_high_performance_config(const std::string& model_path);  // For powerful servers
    
    // Validate model file and suggest configuration
    static AIModelConfig analyze_model_file(const std::string& model_path);
    static bool validate_model_config(const AIModelConfig& config);
    
private:
    static std::map<std::string, std::string> detect_model_info(const std::string& model_path);
};

// AI-powered code analysis for DrawKern
class DrawKernCodeAnalyzer {
public:
    DrawKernCodeAnalyzer(DrawKernAIManager& ai_manager, const std::string& model_id);
    ~DrawKernCodeAnalyzer();
    
    // Code analysis capabilities
    struct AnalysisResult {
        std::vector<std::string> suggestions;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::string explanation;
        float confidence = 0.0f;
    };
    
    AnalysisResult analyze_code(const std::string& code, const std::string& language = "cpp");
    AnalysisResult suggest_completion(const std::string& partial_code, const std::string& language = "cpp");
    AnalysisResult explain_error(const std::string& error_message, const std::string& code_context);
    AnalysisResult suggest_refactoring(const std::string& code, const std::string& language = "cpp");
    
    // DrawKern specific analysis
    AnalysisResult analyze_vm_glyph(const std::string& glyph_description);
    AnalysisResult analyze_limbo_code(const std::string& limbo_code);
    AnalysisResult suggest_vm_optimization(const std::string& vm_config);
    
private:
    DrawKernAIManager& ai_manager_;
    std::string model_id_;
    
    AIInferenceRequest create_analysis_request(const std::string& prompt, const std::string& code);
    AnalysisResult parse_analysis_response(const AIInferenceResponse& response);
};

// Integration with DrawKern components
namespace Integration {
    
    // Connect AI to DIS VMs
    void setup_dis_vm_ai_integration(DISVM& vm, DrawKernAIManager& ai_manager, const std::string& model_id);
    
    // Connect AI to Styx protocol
    void setup_styx_ai_integration(StyxServer& server, DrawKernAIManager& ai_manager);
    
    // Connect AI to Yacc grammar system
    void setup_yacc_ai_integration(YaccGrammarSystem& grammar, DrawKernAIManager& ai_manager);
    
    // Full DrawKern AI workbench setup
    struct AIWorkbenchConfig {
        std::string workbench_id;
        std::vector<std::string> model_ids;
        std::string styx_address;
        std::string dis_vm_spec;
        bool enable_code_analysis = true;
        bool enable_chat = true;
        bool enable_completion = true;
    };
    
    bool setup_ai_workbench(const AIWorkbenchConfig& config, DrawKernAIManager& ai_manager);
}

} // namespace drawkern
} // namespace bolt