#ifndef LLAMA_COMPLETION_PROVIDER_HPP
#define LLAMA_COMPLETION_PROVIDER_HPP

#include "bolt/ai/ai_completion_provider.hpp"
#include "bolt/ai/direct_gguf_inference.hpp"
#include <memory>
#include <string>

namespace bolt {

/**
 * AI completion provider powered by llama.cpp
 * Provides intelligent code completions using GGUF language models
 */
class LlamaCompletionProvider : public AICompletionProvider {
private:
    std::unique_ptr<ai::DirectGGUFInference> inference_engine_;
    bool initialized_ = false;
    std::string model_path_;
    
    // Configuration
    float temperature_ = 0.3f;  // Lower temperature for more deterministic code
    int max_tokens_ = 50;       // Max tokens per completion
    
    // Helper methods
    std::string buildCompletionPrompt(const CodeContext& context, const std::string& prefix) const;
    std::vector<CompletionItem> parseCompletionResponse(const std::string& response) const;
    CompletionItemKind inferCompletionKind(const std::string& completion) const;

public:
    LlamaCompletionProvider();
    explicit LlamaCompletionProvider(const std::string& model_path);
    ~LlamaCompletionProvider() override = default;
    
    // AICompletionProvider interface
    std::vector<CompletionItem> generateCompletions(
        const CodeContext& context,
        const std::string& prefix,
        size_t maxSuggestions = 10
    ) override;
    
    bool isReady() const override { return initialized_; }
    bool initialize() override;
    void shutdown() override;
    
    // Configuration methods
    void setTemperature(float temp) { temperature_ = temp; }
    void setMaxTokens(int tokens) { max_tokens_ = tokens; }
    void setModelPath(const std::string& path) { model_path_ = path; }
    
    // Model info
    std::string getModelInfo() const;
};

} // namespace bolt

#endif // LLAMA_COMPLETION_PROVIDER_HPP
