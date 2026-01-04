#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <functional>

// Minimal includes for standalone testing
#include "bolt/ai/ai_code_generator.hpp"
#include "bolt/ai/ai_refactoring_engine.hpp"

using namespace bolt;

// Simple standalone test
int main() {
    std::cout << "🤖 Advanced AI Features - Standalone Test\n";
    std::cout << "==========================================\n\n";
    
    try {
        // Test code generation
        std::cout << "🏗️ Testing Code Generation:\n";
        
        auto codeGen = std::make_unique<TemplateCodeGenerator>();
        codeGen->initialize();
        
        if (codeGen->isReady()) {
            std::cout << "✅ Template Code Generator ready\n";
            
            // Generate a function
            CodeGenerationContext context;
            context.language = "cpp";
            
            auto result = codeGen->generateFunction("Calculate factorial of a number", context);
            std::cout << "\nGenerated Function:\n";
            std::cout << "-------------------\n";
            std::cout << result.code << "\n\n";
            std::cout << "Confidence: " << result.confidence << "\n";
        } else {
            std::cout << "❌ Code Generator not ready\n";
        }
        
        // Test refactoring suggestions
        std::cout << "\n🔧 Testing Refactoring Engine:\n";
        
        auto refactoringEngine = std::make_unique<TemplateRefactoringEngine>();
        refactoringEngine->initialize();
        
        if (refactoringEngine->isReady()) {
            std::cout << "✅ Refactoring Engine ready\n";
            
            const std::string sampleCode = R"(
void longFunction() {
    int x = 1;
    int y = 2;
    int z = 3;
    if (x > 0) {
        if (y > 0) {
            if (z > 0) {
                std::cout << "All positive" << std::endl;
            }
        }
    }
    for (int i = 0; i < 100; i++) {
        std::cout << "Processing " << i << std::endl;
        // More processing here...
    }
}
)";
            
            auto suggestions = refactoringEngine->generateSuggestions(sampleCode, "test.cpp");
            std::cout << "\nFound " << suggestions.size() << " refactoring suggestions:\n";
            
            for (const auto& suggestion : suggestions) {
                std::cout << "• " << suggestion.title << "\n";
                std::cout << "  Type: " << suggestion.refactoringType << "\n";
                std::cout << "  Confidence: " << suggestion.confidenceScore << "\n";
            }
        } else {
            std::cout << "❌ Refactoring Engine not ready\n";
        }
        
        std::cout << "\n✅ Standalone test completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}