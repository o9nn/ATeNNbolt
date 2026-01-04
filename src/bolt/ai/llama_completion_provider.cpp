#include "bolt/ai/llama_completion_provider.hpp"
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace bolt {

LlamaCompletionProvider::LlamaCompletionProvider() 
    : inference_engine_(std::make_unique<ai::DirectGGUFInference>()) {
}

LlamaCompletionProvider::LlamaCompletionProvider(const std::string& model_path)
    : inference_engine_(std::make_unique<ai::DirectGGUFInference>())
    , model_path_(model_path) {
}

bool LlamaCompletionProvider::initialize() {
    if (initialized_) {
        return true;
    }
    
    // If no model path specified, try to auto-detect
    if (model_path_.empty()) {
        std::vector<std::string> search_paths = {
            "./models/codellama.gguf",
            "./models/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf",
            "/workspaces/bolt-cppml/models/TinyLlama-1.1B-Chat-v1.0-GGUF/tinyllama-1.1b-chat-v1.0.Q3_K_M.gguf"
        };
        
        for (const auto& path : search_paths) {
            if (std::filesystem::exists(path)) {
                model_path_ = path;
                break;
            }
        }
    }
    
    if (model_path_.empty()) {
        return false;
    }
    
    bool loaded = inference_engine_->load_model(model_path_);
    initialized_ = loaded;
    return loaded;
}

void LlamaCompletionProvider::shutdown() {
    initialized_ = false;
    inference_engine_.reset();
}

std::string LlamaCompletionProvider::buildCompletionPrompt(
    const CodeContext& context, 
    const std::string& prefix) const {
    
    std::ostringstream prompt;
    
    // System instruction
    prompt << "You are an expert C++ code completion assistant. ";
    prompt << "Complete the following code with the most appropriate continuation. ";
    prompt << "Only provide the completion, no explanations.\n\n";
    
    // Context information
    if (!context.filePath.empty()) {
        prompt << "File: " << context.filePath << "\n";
    }
    if (!context.language.empty()) {
        prompt << "Language: " << context.language << "\n";
    }
    
    // Code context
    prompt << "```cpp\n";
    
    // Get surrounding context (up to 500 chars before cursor)
    size_t context_start = (context.cursorPosition > 500) 
        ? context.cursorPosition - 500 
        : 0;
    
    std::string before_cursor = context.content.substr(
        context_start, 
        context.cursorPosition - context_start
    );
    
    prompt << before_cursor;
    prompt << "█"; // Cursor marker
    prompt << "\n```\n\n";
    prompt << "Complete the code at the cursor (█):\n";
    
    return prompt.str();
}

std::vector<CompletionItem> LlamaCompletionProvider::parseCompletionResponse(
    const std::string& response) const {
    
    std::vector<CompletionItem> items;
    
    if (response.empty()) {
        return items;
    }
    
    // Clean up the response
    std::string cleaned = response;
    
    // Remove markdown code blocks if present
    size_t code_start = cleaned.find("```");
    if (code_start != std::string::npos) {
        size_t code_end = cleaned.find("```", code_start + 3);
        if (code_end != std::string::npos) {
            cleaned = cleaned.substr(code_start + 3, code_end - code_start - 3);
            // Remove language identifier
            size_t newline = cleaned.find('\n');
            if (newline != std::string::npos) {
                cleaned = cleaned.substr(newline + 1);
            }
        }
    }
    
    // Trim whitespace
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    // Create completion item
    CompletionItem item;
    item.label = cleaned;
    item.detail = "AI Suggestion (llama.cpp)";
    item.kind = inferCompletionKind(cleaned);
    item.insertText = cleaned;
    
    items.push_back(item);
    
    return items;
}

CompletionItemKind LlamaCompletionProvider::inferCompletionKind(
    const std::string& completion) const {
    
    // Simple heuristics to determine completion type
    if (completion.find("class ") != std::string::npos) {
        return CompletionItemKind::CLASS;
    }
    if (completion.find("struct ") != std::string::npos) {
        return CompletionItemKind::STRUCT;
    }
    if (completion.find("(") != std::string::npos && 
        completion.find(")") != std::string::npos) {
        return CompletionItemKind::FUNCTION;
    }
    if (completion.find("::") != std::string::npos) {
        return CompletionItemKind::METHOD;
    }
    
    return CompletionItemKind::TEXT;
}

std::vector<CompletionItem> LlamaCompletionProvider::generateCompletions(
    const CodeContext& context,
    const std::string& prefix,
    size_t maxSuggestions) {
    
    if (!isReady()) {
        return {};
    }
    
    // Build the prompt
    std::string prompt = buildCompletionPrompt(context, prefix);
    
    // Generate completion using the inference engine
    auto response = inference_engine_->generate_text(prompt, max_tokens_, temperature_);
    
    if (!response.success) {
        return {};
    }
    
    // Parse and return completions
    return parseCompletionResponse(response.response);
}

std::string LlamaCompletionProvider::getModelInfo() const {
    if (!initialized_ || !inference_engine_) {
        return "Not initialized";
    }
    return inference_engine_->get_model_info();
}

} // namespace bolt
