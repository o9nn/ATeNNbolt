#include "bolt/ai/llama_completion_provider.hpp"
#include "bolt/ai/ai_code_generator.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "=== AI-Powered Code Completion Demo ===\n\n";
    
    // 1. Initialize the Llama completion provider
    std::cout << "Initializing AI completion provider...\n";
    auto llama_provider = std::make_unique<bolt::LlamaCompletionProvider>();
    
    if (!llama_provider->initialize()) {
        std::cerr << "❌ Failed to initialize Llama provider.\n";
        std::cerr << "Make sure a GGUF model is available in ./models/\n";
        return 1;
    }
    
    std::cout << "✅ AI provider initialized\n";
    std::cout << "Model: " << llama_provider->getModelInfo() << "\n\n";
    
    // 2. Test code completion scenarios
    std::cout << "=== Test 1: Function Completion ===\n";
    
    bolt::CodeContext context1;
    context1.language = "cpp";
    context1.filePath = "example.cpp";
    context1.content = R"(#include <iostream>
#include <vector>

// Function to calculate the sum of a vector
int calculateSum(const std::vector<int>& numbers) {
    int sum = 0;
    for (const auto& num : numbers) {
        sum += num;
    }
    return sum;
}

// Function to calculate the average
)";
    context1.cursorPosition = context1.content.length();
    
    auto completions1 = llama_provider->generateCompletions(context1, "", 3);
    
    std::cout << "Code before cursor:\n" << context1.content << "█\n\n";
    std::cout << "AI Suggestions:\n";
    for (size_t i = 0; i < completions1.size(); ++i) {
        std::cout << (i + 1) << ". " << completions1[i].label << "\n";
        std::cout << "   Type: " << static_cast<int>(completions1[i].kind) << "\n\n";
    }
    
    // 3. Test class member completion
    std::cout << "\n=== Test 2: Class Member Completion ===\n";
    
    bolt::CodeContext context2;
    context2.language = "cpp";
    context2.content = R"(class DataProcessor {
private:
    std::vector<int> data_;
    
public:
    DataProcessor() {}
    
    void addData(int value) {
        data_.push_back(value);
    }
    
    // Add a method to get the data size
)";
    context2.cursorPosition = context2.content.length();
    
    auto completions2 = llama_provider->generateCompletions(context2, "", 3);
    
    std::cout << "Code before cursor:\n" << context2.content << "█\n\n";
    std::cout << "AI Suggestions:\n";
    for (size_t i = 0; i < completions2.size(); ++i) {
        std::cout << (i + 1) << ". " << completions2[i].label << "\n\n";
    }
    
    // 4. Test with AIEnhancedCodeGenerator
    std::cout << "\n=== Test 3: Full Code Generation ===\n";
    
    auto code_generator = std::make_unique<bolt::AIEnhancedCodeGenerator>();
    code_generator->setAIProvider(std::move(llama_provider));
    
    bolt::CodeGenerationContext gen_context;
    gen_context.language = "cpp";
    gen_context.projectContext = "A simple data processing application";
    
    auto generated_func = code_generator->generateFunction(
        "a function that finds the maximum element in a vector",
        gen_context
    );
    
    std::cout << "Generated Function:\n";
    std::cout << "Description: " << generated_func.description << "\n";
    std::cout << "Code:\n" << generated_func.code << "\n";
    std::cout << "Confidence: " << generated_func.confidence << "\n";
    
    std::cout << "\n=== Demo Complete ===\n";
    
    return 0;
}
