#include <cassert>
#include <iostream>
#include <vector>
#include "bolt/editor/keyboard_shortcuts.hpp"

using namespace bolt;

void test_key_combination_parsing() {
    std::cout << "[KeyboardShortcuts] KeyCombinationParsing ... ";
    
    // Test basic key parsing
    auto combo1 = KeyCombination::fromString("Ctrl+S");
    assert(combo1.ctrl == true);
    assert(combo1.shift == false);
    assert(combo1.alt == false);
    assert(combo1.meta == false);
    assert(combo1.key == "S");
    assert(combo1.toString() == "Ctrl+S");
    
    // Test complex combination
    auto combo2 = KeyCombination::fromString("Ctrl+Shift+Alt+F1");
    assert(combo2.ctrl == true);
    assert(combo2.shift == true);
    assert(combo2.alt == true);
    assert(combo2.meta == false);
    assert(combo2.key == "F1");
    assert(combo2.toString() == "Ctrl+Shift+Alt+F1");
    
    // Test single key
    auto combo3 = KeyCombination::fromString("Escape");
    assert(combo3.ctrl == false);
    assert(combo3.shift == false);
    assert(combo3.alt == false);
    assert(combo3.meta == false);
    assert(combo3.key == "Escape");
    assert(combo3.toString() == "Escape");
    
    // Test validity
    assert(combo1.isValid());
    assert(combo2.isValid());
    assert(combo3.isValid());
    
    KeyCombination empty;
    assert(!empty.isValid());
    
    std::cout << "PASS" << std::endl;
}

void test_shortcut_registration() {
    std::cout << "[KeyboardShortcuts] ShortcutRegistration ... ";
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    bool executed = false;
    auto callback = [&executed]() { executed = true; };
    
    // Register a shortcut
    bool registered = shortcuts.registerShortcut("Ctrl+T", "test", callback, 
                                                ShortcutContext::Editor, "Test command");
    assert(registered);
    
    // Check if shortcut exists
    KeyCombination combo = KeyCombination::fromString("Ctrl+T");
    assert(shortcuts.hasShortcut(combo, ShortcutContext::Editor));
    
    // Execute shortcut
    bool result = shortcuts.executeShortcut("Ctrl+T", ShortcutContext::Editor);
    assert(result);
    assert(executed);
    
    // Unregister shortcut
    bool unregistered = shortcuts.unregisterShortcut("Ctrl+T", "test", ShortcutContext::Editor);
    assert(unregistered);
    assert(!shortcuts.hasShortcut(combo, ShortcutContext::Editor));
    
    std::cout << "PASS" << std::endl;
}

void test_context_management() {
    std::cout << "[KeyboardShortcuts] ContextManagement ... ";
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Test initial context
    assert(shortcuts.getActiveContext() == ShortcutContext::Global);
    
    // Set context
    shortcuts.setActiveContext(ShortcutContext::Editor);
    assert(shortcuts.getActiveContext() == ShortcutContext::Editor);
    
    // Test context stack
    shortcuts.pushContext(ShortcutContext::Search);
    assert(shortcuts.getActiveContext() == ShortcutContext::Search);
    
    shortcuts.popContext();
    assert(shortcuts.getActiveContext() == ShortcutContext::Editor);
    
    // Register shortcuts in different contexts
    bool executed1 = false, executed2 = false;
    shortcuts.registerShortcut("F1", "help1", [&executed1]() { executed1 = true; }, 
                              ShortcutContext::Editor, "Editor help");
    shortcuts.registerShortcut("F1", "help2", [&executed2]() { executed2 = true; }, 
                              ShortcutContext::Search, "Search help");
    
    // Execute in editor context
    shortcuts.setActiveContext(ShortcutContext::Editor);
    bool result1 = shortcuts.executeShortcut("F1", ShortcutContext::Editor);
    assert(result1 && executed1 && !executed2);
    
    // Execute in search context
    executed1 = executed2 = false;
    shortcuts.setActiveContext(ShortcutContext::Search);
    bool result2 = shortcuts.executeShortcut("F1", ShortcutContext::Search);
    assert(result2 && !executed1 && executed2);
    
    std::cout << "PASS" << std::endl;
}

void test_shortcut_discovery() {
    std::cout << "[KeyboardShortcuts] ShortcutDiscovery ... ";
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Register test shortcuts
    shortcuts.registerShortcut("Ctrl+K", "command1", []() {}, 
                              ShortcutContext::Editor, "Command 1");
    shortcuts.registerShortcut("Ctrl+L", "command2", []() {}, 
                              ShortcutContext::Editor, "Command 2");
    shortcuts.registerShortcut("Ctrl+K", "command3", []() {}, 
                              ShortcutContext::Search, "Command 3");
    
    // Test finding by context
    auto editorShortcuts = shortcuts.getShortcutsForContext(ShortcutContext::Editor);
    size_t editorCount = 0;
    for (const auto& info : editorShortcuts) {
        if (info.command == "command1" || info.command == "command2") {
            editorCount++;
        }
    }
    assert(editorCount >= 2);
    
    // Test finding by command
    auto command1Shortcuts = shortcuts.findShortcutsByCommand("command1");
    assert(!command1Shortcuts.empty());
    assert(command1Shortcuts[0].command == "command1");
    
    // Test finding by key
    auto ctrlKShortcuts = shortcuts.findShortcutsByKey("Ctrl+K");
    size_t ctrlKCount = 0;
    for (const auto& info : ctrlKShortcuts) {
        if (info.command == "command1" || info.command == "command3") {
            ctrlKCount++;
        }
    }
    assert(ctrlKCount >= 2);
    
    std::cout << "PASS" << std::endl;
}

void test_help_system() {
    std::cout << "[KeyboardShortcuts] HelpSystem ... ";
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Register test shortcut with description
    shortcuts.registerShortcut("Ctrl+H", "testHelp", []() {}, 
                              ShortcutContext::Editor, "Test help command");
    
    // Get help text
    std::string helpText = shortcuts.getHelpText(ShortcutContext::Editor);
    assert(!helpText.empty());
    assert(helpText.find("Test help command") != std::string::npos);
    
    // Get shortcut description
    std::string description = shortcuts.getShortcutDescription("Ctrl+H", ShortcutContext::Editor);
    assert(description == "Test help command");
    
    std::cout << "PASS" << std::endl;
}

void test_enable_disable() {
    std::cout << "[KeyboardShortcuts] EnableDisable ... ";
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    bool executed = false;
    shortcuts.registerShortcut("Ctrl+E", "testEnable", [&executed]() { executed = true; }, 
                              ShortcutContext::Global, "Test enable");
    
    // Test enabled state
    assert(shortcuts.isEnabled());
    bool result = shortcuts.executeShortcut("Ctrl+E", ShortcutContext::Global);
    assert(result && executed);
    
    // Test disabled state
    executed = false;
    shortcuts.setEnabled(false);
    assert(!shortcuts.isEnabled());
    result = shortcuts.executeShortcut("Ctrl+E", ShortcutContext::Global);
    assert(!result && !executed);
    
    // Re-enable
    shortcuts.setEnabled(true);
    result = shortcuts.executeShortcut("Ctrl+E", ShortcutContext::Global);
    assert(result && executed);
    
    std::cout << "PASS" << std::endl;
}

void test_conflict_detection() {
    std::cout << "[KeyboardShortcuts] ConflictDetection ... ";
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Register initial shortcut
    bool registered1 = shortcuts.registerShortcut("Ctrl+R", "command1", []() {}, 
                                                 ShortcutContext::Editor, "Command 1");
    assert(registered1);
    
    // Try to register conflicting shortcut (same key combination and context, different command)
    bool registered2 = shortcuts.registerShortcut("Ctrl+R", "command2", []() {}, 
                                                 ShortcutContext::Editor, "Command 2");
    assert(!registered2); // Should fail due to conflict
    
    // Register same key combination in different context should succeed
    bool registered3 = shortcuts.registerShortcut("Ctrl+R", "command3", []() {}, 
                                                 ShortcutContext::Search, "Command 3");
    assert(registered3);
    
    // Register same command with same key should succeed (overwrite)
    bool registered4 = shortcuts.registerShortcut("Ctrl+R", "command1", []() {}, 
                                                 ShortcutContext::Editor, "Updated Command 1");
    assert(registered4);
    
    std::cout << "PASS" << std::endl;
}

void test_default_shortcuts() {
    std::cout << "[KeyboardShortcuts] DefaultShortcuts ... ";
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Initialize default shortcuts
    shortcuts.initDefaultShortcuts();
    
    // Check if some default shortcuts exist
    auto globalShortcuts = shortcuts.getShortcutsForContext(ShortcutContext::Global);
    auto editorShortcuts = shortcuts.getShortcutsForContext(ShortcutContext::Editor);
    
    assert(!globalShortcuts.empty());
    assert(!editorShortcuts.empty());
    
    // Check for specific default shortcuts
    auto ctrlSShortcuts = shortcuts.findShortcutsByKey("Ctrl+S");
    bool foundSave = false;
    for (const auto& info : ctrlSShortcuts) {
        if (info.command == "save") {
            foundSave = true;
            break;
        }
    }
    assert(foundSave);
    
    auto ctrlZShortcuts = shortcuts.findShortcutsByKey("Ctrl+Z");
    bool foundUndo = false;
    for (const auto& info : ctrlZShortcuts) {
        if (info.command == "undo") {
            foundUndo = true;
            break;
        }
    }
    assert(foundUndo);
    
    std::cout << "PASS" << std::endl;
}

int main() {
    std::cout << "Running Keyboard Shortcuts Tests" << std::endl;
    std::cout << "=================================" << std::endl;
    
    try {
        test_key_combination_parsing();
        test_shortcut_registration();
        test_context_management();
        test_shortcut_discovery();
        test_help_system();
        test_enable_disable();
        test_conflict_detection();
        test_default_shortcuts();
        
        std::cout << "\n=== All Keyboard Shortcuts Tests Passed! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}