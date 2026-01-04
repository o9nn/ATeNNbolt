#include <iostream>
#include <string>
#include "bolt/editor/cursor_manager.hpp"
#include "bolt/editor/integrated_editor.hpp"

using namespace bolt;

void demonstrateBasicMultiCursor() {
    std::cout << "\n=== Basic Multi-Cursor Operations ===" << std::endl;
    
    CursorManager& manager = CursorManager::getInstance();
    manager.clearCursors();
    
    // Add multiple cursors
    std::cout << "Adding cursors at positions (1,5), (3,10), (5,15)..." << std::endl;
    manager.addCursor(1, 5);
    manager.addCursor(3, 10);
    manager.addCursor(5, 15);
    
    std::cout << "Total cursors: " << manager.getCursorCount() << std::endl;
    
    auto cursors = manager.getCursors();
    for (size_t i = 0; i < cursors.size(); ++i) {
        std::cout << "Cursor " << i << ": line " << cursors[i].line 
                  << ", column " << cursors[i].column << std::endl;
    }
}

void demonstrateTextOperations() {
    std::cout << "\n=== Multi-Cursor Text Operations ===" << std::endl;
    
    CursorManager& manager = CursorManager::getInstance();
    manager.clearCursors();
    
    // Add cursors for demonstration
    manager.addCursor(0, 0);
    manager.addCursor(1, 5);
    
    std::cout << "Initial cursor positions:" << std::endl;
    auto cursors = manager.getCursors();
    for (size_t i = 0; i < cursors.size(); ++i) {
        std::cout << "  Cursor " << i << ": (" << cursors[i].line 
                  << ", " << cursors[i].column << ")" << std::endl;
    }
    
    // Simulate text insertion
    std::cout << "\nInserting text 'Hello' at all cursor positions..." << std::endl;
    manager.insertTextAtCursors("Hello");
    
    cursors = manager.getCursors();
    std::cout << "Cursors after insertion:" << std::endl;
    for (size_t i = 0; i < cursors.size(); ++i) {
        std::cout << "  Cursor " << i << ": (" << cursors[i].line 
                  << ", " << cursors[i].column << ")" << std::endl;
    }
}

void demonstrateSelectionOperations() {
    std::cout << "\n=== Multi-Cursor Selection Operations ===" << std::endl;
    
    CursorManager& manager = CursorManager::getInstance();
    manager.clearCursors();
    
    // Add cursor with selection
    Selection selection{2, 5, 2, 10};
    manager.addCursorWithSelection(2, 5, selection);
    
    auto cursors = manager.getCursors();
    std::cout << "Cursor with selection:" << std::endl;
    std::cout << "  Position: (" << cursors[0].line << ", " << cursors[0].column << ")" << std::endl;
    std::cout << "  Selection: (" << cursors[0].selection.startLine << ", " 
              << cursors[0].selection.startColumn << ") to (" 
              << cursors[0].selection.endLine << ", " << cursors[0].selection.endColumn << ")" << std::endl;
    std::cout << "  Has selection: " << (cursors[0].selection.hasSelection() ? "Yes" : "No") << std::endl;
}

void demonstrateMovementOperations() {
    std::cout << "\n=== Multi-Cursor Movement Operations ===" << std::endl;
    
    CursorManager& manager = CursorManager::getInstance();
    manager.clearCursors();
    
    // Add multiple cursors
    manager.addCursor(5, 10);
    manager.addCursor(8, 15);
    
    std::cout << "Initial cursor positions:" << std::endl;
    auto cursors = manager.getCursors();
    for (size_t i = 0; i < cursors.size(); ++i) {
        std::cout << "  Cursor " << i << ": (" << cursors[i].line 
                  << ", " << cursors[i].column << ")" << std::endl;
    }
    
    // Move all cursors
    std::cout << "\nMoving all cursors by (+2 lines, +3 columns)..." << std::endl;
    manager.moveCursors(2, 3);
    
    cursors = manager.getCursors();
    std::cout << "Cursors after movement:" << std::endl;
    for (size_t i = 0; i < cursors.size(); ++i) {
        std::cout << "  Cursor " << i << ": (" << cursors[i].line 
                  << ", " << cursors[i].column << ")" << std::endl;
    }
}

void demonstrateIntegratedEditorOperations() {
    std::cout << "\n=== Integrated Editor Multi-Cursor Operations ===" << std::endl;
    
    IntegratedEditor& editor = IntegratedEditor::getInstance();
    
    // Clear any existing cursors
    editor.clearExtraCursors();
    
    // Add cursors through the editor
    std::cout << "Adding cursors through IntegratedEditor..." << std::endl;
    editor.addCursorAtPosition(0, 0);
    editor.addCursorAtPosition(2, 5);
    editor.addCursorAtPosition(4, 10);
    
    std::cout << "Total cursors in editor: " << editor.getCursorCount() << std::endl;
    
    auto cursors = editor.getAllCursors();
    for (size_t i = 0; i < cursors.size(); ++i) {
        std::cout << "  Editor cursor " << i << ": (" << cursors[i].line 
                  << ", " << cursors[i].column << ")" << std::endl;
    }
    
    // Test keyboard shortcut simulation
    std::cout << "\nSimulating 'Ctrl+D' (add cursor at next occurrence)..." << std::endl;
    bool shortcutHandled = editor.handleKeyboardShortcut("Ctrl+D", "addCursorAtNextOccurrence");
    std::cout << "Shortcut handled: " << (shortcutHandled ? "Yes" : "No") << std::endl;
    std::cout << "Cursors after shortcut: " << editor.getCursorCount() << std::endl;
}

void demonstrateFindOperations() {
    std::cout << "\n=== Find and Multi-Cursor Operations ===" << std::endl;
    
    CursorManager& manager = CursorManager::getInstance();
    manager.clearCursors();
    
    // Start with one cursor
    manager.addCursor(0, 0);
    std::cout << "Starting with " << manager.getCursorCount() << " cursor" << std::endl;
    
    // Find next occurrence (simplified demonstration)
    std::cout << "Finding next occurrence of 'example'..." << std::endl;
    manager.findNextOccurrence("example", "This is example content with multiple example words");
    std::cout << "Cursors after find next: " << manager.getCursorCount() << std::endl;
    
    // Select all occurrences
    std::cout << "Selecting all occurrences..." << std::endl;
    manager.selectAllOccurrences("example", "This is example content with multiple example words");
    std::cout << "Cursors after select all: " << manager.getCursorCount() << std::endl;
}

int main() {
    std::cout << "Bolt C++ Multi-Cursor Editing Demo" << std::endl;
    std::cout << "===================================" << std::endl;
    
    try {
        demonstrateBasicMultiCursor();
        demonstrateTextOperations();
        demonstrateSelectionOperations();
        demonstrateMovementOperations();
        demonstrateIntegratedEditorOperations();
        demonstrateFindOperations();
        
        std::cout << "\n=== Demo completed successfully! ===" << std::endl;
        std::cout << "Multi-cursor editing support has been successfully implemented." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}