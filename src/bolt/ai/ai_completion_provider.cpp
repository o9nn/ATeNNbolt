#include "bolt/ai/ai_completion_provider.hpp"
#include "bolt/editor/code_completion.hpp"
#include <algorithm>
#include <sstream>

namespace bolt {

// Mock AI provider implementation for testing
std::vector<CompletionItem> MockAICompletionProvider::generateCompletions(
    const CodeContext& context,
    const std::string& prefix,
    size_t maxSuggestions
) {
    if (!initialized_) {
        return {};
    }
    
    std::vector<CompletionItem> suggestions;
    
    // Generate context-aware mock suggestions based on the prefix and file type
    if (context.language == "cpp" || context.language == "c++" || 
        (context.filePath.length() >= 4 && 
         (context.filePath.substr(context.filePath.length() - 4) == ".cpp" ||
          context.filePath.substr(context.filePath.length() - 4) == ".hpp" ||
          (context.filePath.length() >= 2 && context.filePath.substr(context.filePath.length() - 2) == ".h")))) {
        
        // C++ specific suggestions
        if (prefix == "std::") {
            suggestions = {
                {"vector", "std::vector<T>", "container", 95},
                {"string", "std::string", "type", 90},
                {"unique_ptr", "std::unique_ptr<T>", "smart_pointer", 85},
                {"shared_ptr", "std::shared_ptr<T>", "smart_pointer", 80},
                {"cout", "std::cout", "stream", 75}
            };
        } else if (prefix == "class" || prefix == "cla") {
            suggestions = {
                {"class MyClass", "class MyClass {\npublic:\n    MyClass();\n    ~MyClass();\n};", "snippet", 95}
            };
        } else if (prefix == "for" || prefix == "fo") {
            suggestions = {
                {"for loop", "for (size_t i = 0; i < n; ++i) {\n    \n}", "snippet", 90},
                {"for range", "for (const auto& item : container) {\n    \n}", "snippet", 85}
            };
        } else if (prefix == "if" || prefix == "i") {
            suggestions = {
                {"if statement", "if (condition) {\n    \n}", "snippet", 85}
            };
        } else if (prefix.empty() || prefix.length() == 1) {
            // General suggestions when typing starts
            suggestions = {
                {"namespace", "namespace " + prefix, "keyword", 70},
                {"include", "#include <" + prefix, "preprocessor", 65},
                {"auto", "auto " + prefix, "keyword", 60}
            };
        }
    } else {
        // Generic suggestions for other languages
        if (prefix == "function" || prefix == "func") {
            suggestions = {
                {"function", "function " + prefix + "() {\n    \n}", "snippet", 80}
            };
        } else if (prefix == "var" || prefix == "v") {
            suggestions = {
                {"variable", "var " + prefix, "keyword", 70}
            };
        }
    }
    
    // Add some intelligent context-based suggestions based on surrounding code
    std::string contextLine = "";
    if (context.cursorPosition < context.content.length()) {
        // Get the current line for context
        size_t lineStart = context.content.rfind('\n', context.cursorPosition);
        if (lineStart == std::string::npos) lineStart = 0;
        else lineStart++;
        
        size_t lineEnd = context.content.find('\n', context.cursorPosition);
        if (lineEnd == std::string::npos) lineEnd = context.content.length();
        
        contextLine = context.content.substr(lineStart, lineEnd - lineStart);
    }
    
    // Add context-specific suggestions
    if (contextLine.find("return") != std::string::npos && prefix.empty()) {
        suggestions.push_back({"nullptr", "nullptr", "literal", 85});
        suggestions.push_back({"true", "true", "literal", 80});
        suggestions.push_back({"false", "false", "literal", 80});
    }
    
    // Limit to requested number of suggestions
    if (suggestions.size() > maxSuggestions) {
        suggestions.resize(maxSuggestions);
    }
    
    return suggestions;
}

// AI Code Completion Engine implementation
AICodeCompletionEngine::AICodeCompletionEngine() 
    : provider_(std::make_unique<MockAICompletionProvider>())
    , baseEngine_(CodeCompletionEngine::getInstance()) {
    
    // Initialize the mock provider
    provider_->initialize();
}

void AICodeCompletionEngine::setProvider(std::unique_ptr<AICompletionProvider> provider) {
    if (provider_) {
        provider_->shutdown();
    }
    provider_ = std::move(provider);
    if (provider_) {
        provider_->initialize();
    }
}

std::vector<CompletionItem> AICodeCompletionEngine::getCompletions(
    const CodeContext& context,
    const std::string& prefix,
    size_t limit
) {
    std::vector<CompletionItem> results;
    
    // Get AI suggestions if enabled and provider is ready
    if (aiEnabled_ && provider_ && provider_->isReady()) {
        auto aiSuggestions = provider_->generateCompletions(context, prefix, limit);
        results.insert(results.end(), aiSuggestions.begin(), aiSuggestions.end());
    }
    
    // Get base engine suggestions as fallback or supplement
    auto baseSuggestions = baseEngine_.getCompletions(prefix, limit);
    results.insert(results.end(), baseSuggestions.begin(), baseSuggestions.end());
    
    // Sort by score and deduplicate
    std::sort(results.begin(), results.end(), 
        [](const CompletionItem& a, const CompletionItem& b) {
            return a.score > b.score;
        });
    
    // Remove duplicates based on label
    auto last = std::unique(results.begin(), results.end(),
        [](const CompletionItem& a, const CompletionItem& b) {
            return a.label == b.label;
        });
    results.erase(last, results.end());
    
    // Limit to requested number
    if (results.size() > limit) {
        results.resize(limit);
    }
    
    return results;
}

bool AICodeCompletionEngine::isAIReady() const {
    return provider_ && provider_->isReady();
}

} // namespace bolt