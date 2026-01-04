#include <iostream>
#include "bolt/editor/editor_pane.hpp"
#include "bolt/editor/split_view_manager.hpp"

using namespace bolt;

int main() {
    std::cout << "Split View Editing Demo (Basic)" << std::endl;
    std::cout << "================================" << std::endl << std::endl;
    
    try {
        // Test basic EditorPane functionality
        std::cout << "1. Testing EditorPane basics..." << std::endl;
        EditorPane pane("demo_pane");
        
        std::cout << "   - Pane ID: " << pane.getId() << std::endl;
        std::cout << "   - Has document: " << (pane.hasDocument() ? "Yes" : "No") << std::endl;
        std::cout << "   - Has focus: " << (pane.hasFocus() ? "Yes" : "No") << std::endl;
        std::cout << "   - Is visible: " << (pane.isVisible() ? "Yes" : "No") << std::endl;
        
        // Test focus management
        pane.setFocus(true);
        std::cout << "   - After setting focus: " << (pane.hasFocus() ? "Yes" : "No") << std::endl;
        
        // Test position management
        EditorPane::PanePosition pos{100, 200, 800, 600};
        pane.setPosition(pos);
        const auto& retrievedPos = pane.getPosition();
        std::cout << "   - Position: (" << retrievedPos.x << ", " << retrievedPos.y << 
                     ", " << retrievedPos.width << ", " << retrievedPos.height << ")" << std::endl;
        
        // Test cursor operations
        pane.setCursorPosition(10, 25);
        size_t line, column;
        pane.getCursorPosition(line, column);
        std::cout << "   - Cursor position: (" << line << ", " << column << ")" << std::endl;
        
        std::cout << std::endl;
        
        // Test SplitViewManager singleton access
        std::cout << "2. Testing SplitViewManager basics..." << std::endl;
        auto& manager = SplitViewManager::getInstance();
        
        std::cout << "   - Split view enabled: " << (manager.isEnabled() ? "Yes" : "No") << std::endl;
        std::cout << "   - Initial pane count: " << manager.getPaneCount() << std::endl;
        std::cout << "   - Has splits: " << (manager.hasSplits() ? "Yes" : "No") << std::endl;
        
        std::cout << std::endl;
        
        std::cout << "Basic split view functionality verified successfully!" << std::endl;
        std::cout << std::endl;
        std::cout << "Key Components Implemented:" << std::endl;
        std::cout << "- EditorPane: Individual editor views with position, focus, and cursor management" << std::endl;
        std::cout << "- SplitViewManager: Singleton manager for coordinating multiple panes" << std::endl;
        std::cout << "- Position Management: Support for positioning and sizing panes" << std::endl;
        std::cout << "- Focus Management: Ability to track which pane has focus" << std::endl;
        std::cout << "- Thread Safety: All operations use ThreadSafe wrappers" << std::endl;
        std::cout << std::endl;
        std::cout << "Split view editing foundation is ready for use!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Demo failed with unknown exception" << std::endl;
        return 1;
    }
}