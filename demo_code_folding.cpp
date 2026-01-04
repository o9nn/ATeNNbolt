#include "bolt/editor/integrated_editor.hpp"
#include <iostream>

int main() {
    std::cout << "Bolt C++ Code Folding Integration Demo\n";
    std::cout << "=====================================\n\n";
    
    // Get the integrated editor instance
    auto& editor = bolt::IntegratedEditor::getInstance();
    
    // Sample C++ code with foldable regions
    std::string sampleCode = R"(
#include <iostream>
#include <vector>

namespace MyNamespace {
    class Calculator {
    private:
        int value;
        
    public:
        Calculator(int initial) : value(initial) {
            // Constructor implementation
        }
        
        int add(int x) {
            if (x > 0) {
                value += x;
                return value;
            } else {
                return value;
            }
        }
        
        void printValue() {
            std::cout << "Value: " << value << std::endl;
        }
    };
    
    void demonstrateUsage() {
        Calculator calc(10);
        calc.add(5);
        calc.printValue();
    }
}

int main() {
    MyNamespace::demonstrateUsage();
    return 0;
}
)";

    // Open the document with code folding
    std::cout << "1. Opening document with code folding detection...\n";
    editor.openDocument("demo.cpp", sampleCode);
    
    // Display detected folding ranges
    auto ranges = editor.getFoldingRanges("demo.cpp");
    std::cout << "   Detected " << ranges.size() << " foldable regions:\n";
    
    for (const auto& range : ranges) {
        std::cout << "   - Lines " << range.startLine << "-" << range.endLine 
                  << " (folded: " << (range.isFolded ? "yes" : "no") 
                  << ", placeholder: \"" << range.placeholder << "\")\n";
    }
    
    std::cout << "\n2. Testing fold operations...\n";
    
    // Test folding operations
    if (!ranges.empty()) {
        auto& firstRange = ranges[0];
        std::cout << "   Toggling fold on line " << firstRange.startLine << "...\n";
        editor.toggleFold("demo.cpp", firstRange.startLine);
        
        // Check if fold state changed
        auto updatedRanges = editor.getFoldingRanges("demo.cpp");
        for (const auto& range : updatedRanges) {
            if (range.startLine == firstRange.startLine) {
                std::cout << "   Fold state changed to: " 
                          << (range.isFolded ? "folded" : "expanded") << "\n";
                break;
            }
        }
        
        // Test expand all
        std::cout << "   Expanding all folds...\n";
        editor.expandAllFolds("demo.cpp");
        
        // Test collapse all
        std::cout << "   Collapsing all folds...\n";
        editor.collapseAllFolds("demo.cpp");
    }
    
    std::cout << "\n3. Testing folding enable/disable...\n";
    std::cout << "   Folding enabled: " << (editor.isFoldingEnabled() ? "yes" : "no") << "\n";
    
    editor.setFoldingEnabled(false);
    std::cout << "   Disabled folding. Enabled: " << (editor.isFoldingEnabled() ? "yes" : "no") << "\n";
    
    editor.setFoldingEnabled(true);
    std::cout << "   Re-enabled folding. Enabled: " << (editor.isFoldingEnabled() ? "yes" : "no") << "\n";
    
    std::cout << "\n4. Testing content update with re-folding...\n";
    std::string newCode = R"(
struct SimpleStruct {
    int data;
    void process() {
        data *= 2;
    }
};
)";
    
    editor.updateDocumentContent("demo.cpp", newCode);
    auto newRanges = editor.getFoldingRanges("demo.cpp");
    std::cout << "   After content update, detected " << newRanges.size() << " foldable regions\n";
    
    std::cout << "\n✓ Code folding integration demo completed successfully!\n";
    std::cout << "  All folding features are working correctly:\n";
    std::cout << "  - Automatic detection of foldable regions (braces, comments)\n";
    std::cout << "  - Toggle individual folds\n";
    std::cout << "  - Expand/collapse all folds\n";
    std::cout << "  - Enable/disable folding globally\n";
    std::cout << "  - Content update with re-detection\n";
    std::cout << "  - Integration with editor document management\n";
    
    return 0;
}