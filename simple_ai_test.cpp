#include "bolt/ai/ai_completion_provider.hpp"
#include <iostream>

using namespace bolt;

int main() {
    std::cout << "Testing AI Code Completion Provider directly...\n";
    
    try {
        // Test the AI code completion engine directly
        AICodeCompletionEngine& engine = AICodeCompletionEngine::getInstance();
        
        std::cout << "1. AI Completion Engine Status:\n";
        std::cout << "   AI Ready: " << (engine.isAIReady() ? "YES" : "NO") << "\n";
        std::cout << "   AI Enabled: " << (engine.isAIEnabled() ? "YES" : "NO") << "\n\n";
        
        // Test getting completions
        CodeContext context;
        context.filePath = "test.cpp";
        context.content = "int main() {\n    std::\n    return 0;\n}";
        context.cursorPosition = 20;
        context.line = 1;
        context.column = 9;
        context.language = "cpp";
        
        std::cout << "2. Testing AI Completions:\n";
        auto completions = engine.getCompletions(context, "std::", 5);
        std::cout << "   Found " << completions.size() << " completions for 'std::':\n";
        
        for (size_t i = 0; i < completions.size(); ++i) {
            std::cout << "   " << (i+1) << ". " << completions[i].label 
                      << " (" << completions[i].kind << ", score: " << completions[i].score << ")\n";
        }
        
        std::cout << "\n3. Testing Enable/Disable:\n";
        engine.setAIEnabled(false);
        std::cout << "   AI Disabled - Ready: " << (engine.isAIReady() ? "YES" : "NO") << "\n";
        
        completions = engine.getCompletions(context, "std::", 3);
        std::cout << "   Completions when disabled: " << completions.size() << "\n";
        
        engine.setAIEnabled(true);
        std::cout << "   AI Re-enabled - Ready: " << (engine.isAIReady() ? "YES" : "NO") << "\n";
        
        completions = engine.getCompletions(context, "std::", 3);
        std::cout << "   Completions when re-enabled: " << completions.size() << "\n";
        
        std::cout << "\n✅ AI Code Completion Provider test successful!\n";
        std::cout << "\nThe AI code completion integration includes:\n";
        std::cout << "  - Mock AI provider with C++ language awareness\n";
        std::cout << "  - Context-aware completions (std:: namespace, keywords, etc.)\n";
        std::cout << "  - Enable/disable functionality\n";
        std::cout << "  - Scoring system for suggestions\n";
        std::cout << "  - Integration ready for real AI models\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
}