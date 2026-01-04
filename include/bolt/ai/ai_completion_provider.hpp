#ifndef AI_COMPLETION_PROVIDER_HPP
#define AI_COMPLETION_PROVIDER_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "bolt/editor/code_completion.hpp"

namespace bolt {

struct CodeContext {
    std::string filePath;
    std::string content;
    size_t cursorPosition;
    size_t line;
    size_t column;
    std::string language;
};

class AICompletionProvider {
public:
    virtual ~AICompletionProvider() = default;
    
    // Generate AI-powered code completions
    virtual std::vector<CompletionItem> generateCompletions(
        const CodeContext& context,
        const std::string& prefix,
        size_t maxSuggestions = 10
    ) = 0;
    
    // Check if the provider is ready/initialized
    virtual bool isReady() const = 0;
    
    // Initialize the provider (load models, etc.)
    virtual bool initialize() = 0;
    
    // Shutdown and cleanup
    virtual void shutdown() = 0;
};

// Mock implementation for testing/demo purposes
class MockAICompletionProvider : public AICompletionProvider {
private:
    bool initialized_ = false;
    
public:
    std::vector<CompletionItem> generateCompletions(
        const CodeContext& context,
        const std::string& prefix,
        size_t maxSuggestions = 10
    ) override;
    
    bool isReady() const override { return initialized_; }
    bool initialize() override { initialized_ = true; return true; }
    void shutdown() override { initialized_ = false; }
};

// AI-powered code completion engine that integrates with existing CodeCompletionEngine
class AICodeCompletionEngine {
private:
    std::unique_ptr<AICompletionProvider> provider_;
    CodeCompletionEngine& baseEngine_;
    bool aiEnabled_ = true;
    
public:
    AICodeCompletionEngine();
    
    // Set the AI provider
    void setProvider(std::unique_ptr<AICompletionProvider> provider);
    
    // Get completions with AI integration
    std::vector<CompletionItem> getCompletions(
        const CodeContext& context,
        const std::string& prefix,
        size_t limit = 10
    );
    
    // Enable/disable AI completions
    void setAIEnabled(bool enabled) { aiEnabled_ = enabled; }
    bool isAIEnabled() const { return aiEnabled_; }
    
    // Check if AI provider is ready
    bool isAIReady() const;
    
    // Singleton access
    static AICodeCompletionEngine& getInstance() {
        static AICodeCompletionEngine instance;
        return instance;
    }
};

} // namespace bolt

#endif