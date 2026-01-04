#include "bolt/ai/ai_completion_provider.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include <iostream>
#include <iomanip>

using namespace bolt;

void demonstrateAICompletion() {
    std::cout << "=== AI Code Completion Integration Demo ===\n\n";
    
    // Get the integrated editor instance
    IntegratedEditor& editor = IntegratedEditor::getInstance();
    
    std::cout << "1. Checking AI Completion Status:\n";
    std::cout << "   AI Completion Ready: " << (editor.isAICompletionReady() ? "YES" : "NO") << "\n";
    std::cout << "   AI Completion Enabled: " << (editor.isAICompletionEnabled() ? "YES" : "NO") << "\n";
    std::cout << "   Code Completion Active: " << (editor.isCodeCompletionActive() ? "YES" : "NO") << "\n\n";
    
    // Create a test C++ file
    std::cout << "2. Opening test C++ file with content:\n";
    std::string testFilePath = "demo.cpp";
    std::string testContent = R"(#include <iostream>
#include <vector>
#include <string>

int main() {
    std::
    return 0;
})";
    
    std::cout << "   File: " << testFilePath << "\n";
    std::cout << "   Content:\n";
    std::cout << testContent << "\n\n";
    
    editor.openDocument(testFilePath, testContent);
    
    // Test AI completions for std::
    std::cout << "3. Getting AI completions for 'std::':\n";
    auto completions = editor.getAICompletions(testFilePath, "std::", 5);
    
    if (!completions.empty()) {
        std::cout << "   Found " << completions.size() << " completions:\n";
        for (size_t i = 0; i < completions.size(); ++i) {
            std::cout << "   " << (i + 1) << ". " << std::left << std::setw(15) << completions[i].label
                      << " (" << completions[i].kind << ", score: " << completions[i].score << ")\n";
            if (!completions[i].detail.empty()) {
                std::cout << "      " << completions[i].detail << "\n";
            }
        }
    } else {
        std::cout << "   No completions found.\n";
    }
    std::cout << "\n";
    
    // Test triggering completion
    std::cout << "4. Triggering code completion:\n";
    editor.triggerCodeCompletion(testFilePath, "std::");
    std::cout << "   Code Completion Active: " << (editor.isCodeCompletionActive() ? "YES" : "NO") << "\n";
    
    if (editor.isCodeCompletionActive()) {
        auto selected = editor.getSelectedCompletion();
        std::cout << "   Selected: " << selected.label << "\n";
        
        std::cout << "   Testing navigation...\n";
        editor.selectNextCompletion();
        selected = editor.getSelectedCompletion();
        std::cout << "   After next: " << selected.label << "\n";
        
        editor.selectPreviousCompletion();
        selected = editor.getSelectedCompletion();
        std::cout << "   After previous: " << selected.label << "\n";
    }
    std::cout << "\n";
    
    // Test different prefixes
    std::cout << "5. Testing different completion contexts:\n";
    
    // Test for 'class' keyword
    auto classCompletions = editor.getAICompletions(testFilePath, "class", 3);
    std::cout << "   'class' prefix: " << classCompletions.size() << " completions\n";
    for (const auto& comp : classCompletions) {
        std::cout << "      " << comp.label << "\n";
    }
    
    // Test for 'for' keyword
    auto forCompletions = editor.getAICompletions(testFilePath, "for", 3);
    std::cout << "   'for' prefix: " << forCompletions.size() << " completions\n";
    for (const auto& comp : forCompletions) {
        std::cout << "      " << comp.label << "\n";
    }
    std::cout << "\n";
    
    // Test AI enable/disable
    std::cout << "6. Testing AI enable/disable:\n";
    editor.setAICompletionEnabled(false);
    std::cout << "   AI Disabled - Completions for 'std::': " 
              << editor.getAICompletions(testFilePath, "std::", 3).size() << "\n";
    
    editor.setAICompletionEnabled(true);
    std::cout << "   AI Re-enabled - Completions for 'std::': " 
              << editor.getAICompletions(testFilePath, "std::", 3).size() << "\n";
    std::cout << "\n";
    
    // Clean up
    if (editor.isCodeCompletionActive()) {
        editor.cancelCompletion();
    }
    
    std::cout << "7. Final state:\n";
    std::cout << "   Code Completion Active: " << (editor.isCodeCompletionActive() ? "YES" : "NO") << "\n";
    std::cout << "   AI Completion Enabled: " << (editor.isAICompletionEnabled() ? "YES" : "NO") << "\n";
    
    std::cout << "\n=== Demo Complete ===\n";
}

void demonstrateAIProviderDirectly() {
    std::cout << "\n=== Direct AI Provider Demo ===\n\n";
    
    // Test the AI provider directly
    MockAICompletionProvider provider;
    
    std::cout << "1. Initializing AI Provider:\n";
    std::cout << "   Ready before init: " << (provider.isReady() ? "YES" : "NO") << "\n";
    provider.initialize();
    std::cout << "   Ready after init: " << (provider.isReady() ? "YES" : "NO") << "\n\n";
    
    // Test C++ context
    std::cout << "2. Testing C++ completion context:\n";
    CodeContext cppContext;
    cppContext.filePath = "test.cpp";
    cppContext.content = "int main() {\n    std::\n    return 0;\n}";
    cppContext.cursorPosition = 20;
    cppContext.line = 1;
    cppContext.column = 9;
    cppContext.language = "cpp";
    
    auto completions = provider.generateCompletions(cppContext, "std::", 5);
    std::cout << "   Generated " << completions.size() << " completions for 'std::':\n";
    for (const auto& comp : completions) {
        std::cout << "      " << comp.label << " (" << comp.kind << ")\n";
    }
    std::cout << "\n";
    
    // Test different context
    std::cout << "3. Testing different contexts:\n";
    cppContext.content = "class MyClass {\n    \n};";
    cppContext.cursorPosition = 16;
    
    completions = provider.generateCompletions(cppContext, "for", 3);
    std::cout << "   'for' in class context: " << completions.size() << " completions\n";
    
    completions = provider.generateCompletions(cppContext, "", 3);
    std::cout << "   Empty prefix: " << completions.size() << " completions\n";
    
    provider.shutdown();
    std::cout << "   Ready after shutdown: " << (provider.isReady() ? "YES" : "NO") << "\n";
    
    std::cout << "\n=== AI Provider Demo Complete ===\n";
}

int main() {
    std::cout << "Bolt C++ AI Code Completion Integration Demo\n";
    std::cout << "============================================\n\n";
    
    try {
        demonstrateAICompletion();
        demonstrateAIProviderDirectly();
        
        std::cout << "\n✅ All demonstrations completed successfully!\n";
        std::cout << "\nAI Code Completion has been successfully integrated with the editor.\n";
        std::cout << "Features available:\n";
        std::cout << "  - AI-powered code completions with context awareness\n";
        std::cout << "  - Language-specific suggestions (C++, Python, JavaScript)\n";
        std::cout << "  - Keyboard shortcuts (Ctrl+Space to trigger)\n";
        std::cout << "  - Navigation through suggestions (Up/Down arrows)\n";
        std::cout << "  - Accept (Enter) and Cancel (Escape) operations\n";
        std::cout << "  - Enable/disable AI completions\n";
        std::cout << "  - Mock AI provider ready for real AI integration\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error during demonstration: " << e.what() << "\n";
        return 1;
    }
}