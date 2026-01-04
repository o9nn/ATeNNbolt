#include <iostream>
#include "bolt/editor/keyboard_shortcuts.hpp"

using namespace bolt;

int main() {
    std::cout << "Testing Keyboard Shortcuts Configuration" << std::endl;
    std::cout << "========================================" << std::endl;
    
    KeyboardShortcuts& shortcuts = KeyboardShortcuts::getInstance();
    
    // Test loading from config file
    std::cout << "Loading shortcuts from configuration file..." << std::endl;
    bool loaded = shortcuts.loadShortcutsFromFile("../default_shortcuts.conf");
    std::cout << "Load result: " << (loaded ? "Success" : "Failed") << std::endl;
    
    if (loaded) {
        // Show some loaded shortcuts
        auto globalShortcuts = shortcuts.getShortcutsForContext(ShortcutContext::Global);
        std::cout << "\nLoaded Global shortcuts:" << std::endl;
        for (size_t i = 0; i < std::min(size_t(5), globalShortcuts.size()); ++i) {
            const auto& info = globalShortcuts[i];
            std::cout << "  " << info.keyCombination.toString() 
                      << " -> " << info.command
                      << " (" << info.description << ")" << std::endl;
        }
        
        // Test saving to a new file
        std::cout << "\nSaving shortcuts to test_shortcuts.conf..." << std::endl;
        bool saved = shortcuts.saveShortcutsToFile("test_shortcuts.conf");
        std::cout << "Save result: " << (saved ? "Success" : "Failed") << std::endl;
    }
    
    return 0;
}