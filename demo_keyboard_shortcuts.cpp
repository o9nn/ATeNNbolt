#include <iostream>
#include <string>
#include "bolt/editor/keyboard_shortcuts.hpp"
#include "bolt/editor/integrated_editor.hpp"

using namespace bolt;

void demonstrateKeyParsing() {
    std::cout << "\n=== Key Combination Parsing ===" << std::endl;
    
    // Demonstrate various key combinations
    std::vector<std::string> testKeys = {
        "Ctrl+S", "Ctrl+Shift+Z", "Alt+F4", "Ctrl+Alt+Delete", 
        "F1", "Escape", "Meta+A", "Ctrl+Shift+Alt+T"
    };
    
    for (const auto& keyStr : testKeys) {
        auto combo = KeyCombination::fromString(keyStr);
        std::cout << "Input: " << keyStr << " -> Parsed: " << combo.toString() 
                  << " (Valid: " << (combo.isValid() ? "Yes" : "No") << ")" << std::endl;
    }
}

void demonstrateContextualShortcuts() {
    std::cout << "\n=== Contextual Shortcuts ===" << std::endl;
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Register shortcuts in different contexts
    shortcuts.registerShortcut("F1", "showHelp", []() {
        std::cout << "  -> Showing global help" << std::endl;
    }, ShortcutContext::Global, "Show global help");
    
    shortcuts.registerShortcut("F1", "showEditorHelp", []() {
        std::cout << "  -> Showing editor-specific help" << std::endl;
    }, ShortcutContext::Editor, "Show editor help");
    
    shortcuts.registerShortcut("F1", "showSearchHelp", []() {
        std::cout << "  -> Showing search help" << std::endl;
    }, ShortcutContext::Search, "Show search help");
    
    // Demonstrate context switching
    std::cout << "Testing F1 in different contexts:" << std::endl;
    
    std::cout << "Global context: ";
    shortcuts.executeShortcut("F1", ShortcutContext::Global);
    
    std::cout << "Editor context: ";
    shortcuts.executeShortcut("F1", ShortcutContext::Editor);
    
    std::cout << "Search context: ";
    shortcuts.executeShortcut("F1", ShortcutContext::Search);
}

void demonstrateShortcutDiscovery() {
    std::cout << "\n=== Shortcut Discovery ===" << std::endl;
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Initialize default shortcuts
    shortcuts.initDefaultShortcuts();
    
    // Show all editor shortcuts
    auto editorShortcuts = shortcuts.getShortcutsForContext(ShortcutContext::Editor);
    std::cout << "Editor shortcuts (" << editorShortcuts.size() << " total):" << std::endl;
    
    size_t count = 0;
    for (const auto& info : editorShortcuts) {
        if (count < 5) { // Show first 5 for brevity
            std::cout << "  " << info.keyCombination.toString() 
                      << " -> " << info.command;
            if (!info.description.empty()) {
                std::cout << " (" << info.description << ")";
            }
            std::cout << std::endl;
        }
        count++;
    }
    if (editorShortcuts.size() > 5) {
        std::cout << "  ... and " << (editorShortcuts.size() - 5) << " more" << std::endl;
    }
    
    // Show shortcuts for a specific key
    auto ctrlShortcuts = shortcuts.findShortcutsByKey("Ctrl+S");
    std::cout << "\nShortcuts using Ctrl+S:" << std::endl;
    for (const auto& info : ctrlShortcuts) {
        std::cout << "  Context: " << (info.context == ShortcutContext::Global ? "Global" : 
                                      info.context == ShortcutContext::Editor ? "Editor" : "Other")
                  << ", Command: " << info.command
                  << ", Description: " << info.description << std::endl;
    }
}

void demonstrateHelpSystem() {
    std::cout << "\n=== Help System ===" << std::endl;
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Get help for global context
    std::string globalHelp = shortcuts.getHelpText(ShortcutContext::Global);
    std::cout << "Sample of Global shortcuts help:" << std::endl;
    
    // Show first few lines of help
    std::istringstream iss(globalHelp);
    std::string line;
    int lineCount = 0;
    while (std::getline(iss, line) && lineCount < 8) {
        std::cout << line << std::endl;
        lineCount++;
    }
    std::cout << "..." << std::endl;
}

void demonstrateIntegratedEditorShortcuts() {
    std::cout << "\n=== Integrated Editor Shortcuts ===" << std::endl;
    
    IntegratedEditor& editor = IntegratedEditor::getInstance();
    
    // Initialize shortcuts through the editor
    editor.initializeKeyboardShortcuts();
    
    std::cout << "Testing multi-cursor shortcuts through IntegratedEditor:" << std::endl;
    
    // Clear any existing cursors
    editor.clearExtraCursors();
    editor.addCursorAtPosition(0, 0);
    std::cout << "Initial cursors: " << editor.getCursorCount() << std::endl;
    
    // Test Ctrl+D (add cursor at next occurrence)
    std::cout << "Executing Ctrl+D (add cursor at next occurrence)..." << std::endl;
    bool result = editor.handleKeyboardShortcut("Ctrl+D", "addCursorAtNextOccurrence");
    std::cout << "  Result: " << (result ? "Success" : "Failed") 
              << ", Cursors: " << editor.getCursorCount() << std::endl;
    
    // Test Ctrl+Alt+Down (add cursor below)
    std::cout << "Executing Ctrl+Alt+Down (add cursor below)..." << std::endl;
    result = editor.handleKeyboardShortcut("Ctrl+Alt+Down", "addCursorBelow");
    std::cout << "  Result: " << (result ? "Success" : "Failed") 
              << ", Cursors: " << editor.getCursorCount() << std::endl;
    
    // Test Escape (clear extra cursors)
    std::cout << "Executing Escape (clear extra cursors)..." << std::endl;
    result = editor.handleKeyboardShortcut("Escape", "clearExtraCursors");
    std::cout << "  Result: " << (result ? "Success" : "Failed") 
              << ", Cursors: " << editor.getCursorCount() << std::endl;
}

void demonstrateConflictPrevention() {
    std::cout << "\n=== Conflict Prevention ===" << std::endl;
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Try to register a conflicting shortcut
    std::cout << "Attempting to register Ctrl+S for 'saveAs' in Global context..." << std::endl;
    bool result1 = shortcuts.registerShortcut("Ctrl+S", "saveAs", []() {
        std::cout << "Save As executed" << std::endl;
    }, ShortcutContext::Global, "Save file as");
    
    std::cout << "  Registration result: " << (result1 ? "Success" : "Failed (conflict detected)") << std::endl;
    
    // Register same shortcut in different context
    std::cout << "Attempting to register Ctrl+S for 'saveAs' in Editor context..." << std::endl;
    bool result2 = shortcuts.registerShortcut("Ctrl+S", "saveAs", []() {
        std::cout << "Editor Save As executed" << std::endl;
    }, ShortcutContext::Editor, "Save file as");
    
    std::cout << "  Registration result: " << (result2 ? "Success" : "Failed (conflict detected)") << std::endl;
}

void demonstrateEnableDisable() {
    std::cout << "\n=== Enable/Disable Shortcuts ===" << std::endl;
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Register a test shortcut
    shortcuts.registerShortcut("Ctrl+T", "testCommand", []() {
        std::cout << "  -> Test command executed" << std::endl;
    }, ShortcutContext::Global, "Test command");
    
    std::cout << "Shortcuts enabled: " << (shortcuts.isEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "Executing Ctrl+T: ";
    shortcuts.executeShortcut("Ctrl+T", ShortcutContext::Global);
    
    // Disable shortcuts
    shortcuts.setEnabled(false);
    std::cout << "Shortcuts enabled: " << (shortcuts.isEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "Executing Ctrl+T: ";
    bool result = shortcuts.executeShortcut("Ctrl+T", ShortcutContext::Global);
    if (!result) {
        std::cout << "  -> Shortcut disabled, not executed" << std::endl;
    }
    
    // Re-enable shortcuts
    shortcuts.setEnabled(true);
    std::cout << "Shortcuts enabled: " << (shortcuts.isEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "Executing Ctrl+T: ";
    shortcuts.executeShortcut("Ctrl+T", ShortcutContext::Global);
}

int main() {
    std::cout << "Bolt C++ Keyboard Shortcuts System Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        demonstrateKeyParsing();
        demonstrateContextualShortcuts();
        demonstrateShortcutDiscovery();
        demonstrateHelpSystem();
        demonstrateIntegratedEditorShortcuts();
        demonstrateConflictPrevention();
        demonstrateEnableDisable();
        
        std::cout << "\n=== Keyboard Shortcuts System Demo Completed! ===" << std::endl;
        std::cout << "The enhanced keyboard shortcuts system provides:" << std::endl;
        std::cout << "✓ Robust key combination parsing" << std::endl;
        std::cout << "✓ Context-aware shortcut execution" << std::endl;
        std::cout << "✓ Conflict detection and prevention" << std::endl;
        std::cout << "✓ Comprehensive help and discovery system" << std::endl;
        std::cout << "✓ Enable/disable functionality" << std::endl;
        std::cout << "✓ Integration with editor components" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}