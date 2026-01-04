
#include "bolt/editor/keyboard_shortcuts.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cctype>
#include <iomanip>
#include <iostream>

namespace bolt {

// KeyCombination implementation
bool KeyCombination::operator<(const KeyCombination& other) const {
    if (ctrl != other.ctrl) return ctrl < other.ctrl;
    if (shift != other.shift) return shift < other.shift;
    if (alt != other.alt) return alt < other.alt;
    if (meta != other.meta) return meta < other.meta;
    return key < other.key;
}

std::string KeyCombination::toString() const {
    std::string result;
    
    if (ctrl) result += "Ctrl+";
    if (shift) result += "Shift+";
    if (alt) result += "Alt+";
    if (meta) result += "Meta+";
    
    result += key;
    return result;
}

KeyCombination KeyCombination::fromString(const std::string& keyString) {
    KeyCombination combo;
    std::string remaining = keyString;
    
    // Parse modifiers
    while (true) {
        if (remaining.find("Ctrl+") == 0) {
            combo.ctrl = true;
            remaining = remaining.substr(5);
        } else if (remaining.find("Shift+") == 0) {
            combo.shift = true;
            remaining = remaining.substr(6);
        } else if (remaining.find("Alt+") == 0) {
            combo.alt = true;
            remaining = remaining.substr(4);
        } else if (remaining.find("Meta+") == 0) {
            combo.meta = true;
            remaining = remaining.substr(5);
        } else {
            break;
        }
    }
    
    combo.key = remaining;
    return combo;
}

bool KeyCombination::isValid() const {
    return !key.empty();
}

// KeyboardShortcuts implementation
KeyboardShortcuts& KeyboardShortcuts::getInstance() {
    static KeyboardShortcuts instance;
    return instance;
}

bool KeyboardShortcuts::registerShortcut(const std::string& keyString, const std::string& command,
                                        ShortcutCallback callback, ShortcutContext context,
                                        const std::string& description) {
    KeyCombination combo = KeyCombination::fromString(keyString);
    return registerShortcut(combo, command, callback, context, description);
}

bool KeyboardShortcuts::registerShortcut(const KeyCombination& keyCombination, const std::string& command,
                                        ShortcutCallback callback, ShortcutContext context,
                                        const std::string& description) {
    if (!keyCombination.isValid() || command.empty() || !callback) {
        return false;
    }
    
    // Check for conflicts
    if (isKeyConflict(keyCombination, context, command)) {
        return false;
    }
    
    ShortcutInfo info{keyCombination, command, description, context, callback};
    shortcuts_[info] = callback;
    return true;
}

bool KeyboardShortcuts::unregisterShortcut(const std::string& keyString, const std::string& command,
                                          ShortcutContext context) {
    KeyCombination combo = KeyCombination::fromString(keyString);
    return unregisterShortcut(combo, command, context);
}

bool KeyboardShortcuts::unregisterShortcut(const KeyCombination& keyCombination, const std::string& command,
                                          ShortcutContext context) {
    ShortcutInfo info{keyCombination, command, "", context, nullptr};
    auto it = shortcuts_.find(info);
    if (it != shortcuts_.end()) {
        shortcuts_.erase(it);
        return true;
    }
    return false;
}

bool KeyboardShortcuts::executeShortcut(const std::string& keyString, ShortcutContext context) {
    KeyCombination combo = KeyCombination::fromString(keyString);
    return executeShortcut(combo, context);
}

bool KeyboardShortcuts::executeShortcut(const KeyCombination& keyCombination, ShortcutContext context) {
    if (!enabled_) {
        return false;
    }
    
    // Try to find shortcut in current context first
    for (const auto& [info, callback] : shortcuts_) {
        if (info.keyCombination.ctrl == keyCombination.ctrl &&
            info.keyCombination.shift == keyCombination.shift &&
            info.keyCombination.alt == keyCombination.alt &&
            info.keyCombination.meta == keyCombination.meta &&
            info.keyCombination.key == keyCombination.key &&
            info.context == context) {
            callback();
            return true;
        }
    }
    
    // If not found in specific context, try global context
    if (context != ShortcutContext::Global) {
        return executeShortcut(keyCombination, ShortcutContext::Global);
    }
    
    return false;
}

void KeyboardShortcuts::setActiveContext(ShortcutContext context) {
    activeContext_ = context;
}

ShortcutContext KeyboardShortcuts::getActiveContext() const {
    return activeContext_;
}

void KeyboardShortcuts::pushContext(ShortcutContext context) {
    contextStack_.push_back(activeContext_);
    activeContext_ = context;
}

void KeyboardShortcuts::popContext() {
    if (!contextStack_.empty()) {
        activeContext_ = contextStack_.back();
        contextStack_.pop_back();
    }
}

std::vector<ShortcutInfo> KeyboardShortcuts::getShortcutsForContext(ShortcutContext context) const {
    std::vector<ShortcutInfo> result;
    for (const auto& [info, callback] : shortcuts_) {
        if (info.context == context) {
            result.push_back(info);
        }
    }
    return result;
}

std::vector<ShortcutInfo> KeyboardShortcuts::getAllShortcuts() const {
    std::vector<ShortcutInfo> result;
    for (const auto& [info, callback] : shortcuts_) {
        result.push_back(info);
    }
    return result;
}

std::vector<ShortcutInfo> KeyboardShortcuts::findShortcutsByCommand(const std::string& command) const {
    std::vector<ShortcutInfo> result;
    for (const auto& [info, callback] : shortcuts_) {
        if (info.command == command) {
            result.push_back(info);
        }
    }
    return result;
}

std::vector<ShortcutInfo> KeyboardShortcuts::findShortcutsByKey(const std::string& keyString) const {
    KeyCombination combo = KeyCombination::fromString(keyString);
    std::vector<ShortcutInfo> result;
    for (const auto& [info, callback] : shortcuts_) {
        if (info.keyCombination.ctrl == combo.ctrl &&
            info.keyCombination.shift == combo.shift &&
            info.keyCombination.alt == combo.alt &&
            info.keyCombination.meta == combo.meta &&
            info.keyCombination.key == combo.key) {
            result.push_back(info);
        }
    }
    return result;
}

bool KeyboardShortcuts::hasShortcut(const KeyCombination& keyCombination, ShortcutContext context) const {
    for (const auto& [info, callback] : shortcuts_) {
        if (info.keyCombination.ctrl == keyCombination.ctrl &&
            info.keyCombination.shift == keyCombination.shift &&
            info.keyCombination.alt == keyCombination.alt &&
            info.keyCombination.meta == keyCombination.meta &&
            info.keyCombination.key == keyCombination.key &&
            info.context == context) {
            return true;
        }
    }
    return false;
}

std::string KeyboardShortcuts::getHelpText(ShortcutContext context) const {
    std::ostringstream oss;
    oss << "Keyboard Shortcuts - " << contextToString(context) << ":\n";
    oss << std::string(50, '=') << "\n\n";
    
    auto shortcuts = getShortcutsForContext(context);
    std::sort(shortcuts.begin(), shortcuts.end(), [](const ShortcutInfo& a, const ShortcutInfo& b) {
        return a.keyCombination.toString() < b.keyCombination.toString();
    });
    
    for (const auto& info : shortcuts) {
        oss << std::left << std::setw(25) << info.keyCombination.toString()
            << std::setw(30) << info.command
            << info.description << "\n";
    }
    
    return oss.str();
}

std::string KeyboardShortcuts::getShortcutDescription(const std::string& keyString, ShortcutContext context) const {
    auto shortcuts = findShortcutsByKey(keyString);
    for (const auto& info : shortcuts) {
        if (info.context == context) {
            return info.description;
        }
    }
    return "";
}

void KeyboardShortcuts::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool KeyboardShortcuts::isEnabled() const {
    return enabled_;
}

void KeyboardShortcuts::initDefaultShortcuts() {
    initGlobalShortcuts();
    initEditorShortcuts();
    initFileTreeShortcuts();
    initSearchShortcuts();
}

void KeyboardShortcuts::initGlobalShortcuts() {
    // File operations
    registerShortcut("Ctrl+S", "save", []() { /* Save current file */ }, 
                    ShortcutContext::Global, "Save current file");
    registerShortcut("Ctrl+O", "open", []() { /* Open file dialog */ }, 
                    ShortcutContext::Global, "Open file");
    registerShortcut("Ctrl+N", "new", []() { /* Create new file */ }, 
                    ShortcutContext::Global, "Create new file");
    registerShortcut("Ctrl+Shift+S", "saveAs", []() { /* Save as dialog */ }, 
                    ShortcutContext::Global, "Save file as");
    registerShortcut("Ctrl+W", "close", []() { /* Close current file */ }, 
                    ShortcutContext::Global, "Close current file");
    registerShortcut("Ctrl+Shift+T", "reopenTab", []() { /* Reopen last closed tab */ }, 
                    ShortcutContext::Global, "Reopen last closed tab");
}

void KeyboardShortcuts::initEditorShortcuts() {
    // Edit operations
    registerShortcut("Ctrl+Z", "undo", []() { /* Undo last action */ }, 
                    ShortcutContext::Editor, "Undo");
    registerShortcut("Ctrl+Y", "redo", []() { /* Redo last action */ }, 
                    ShortcutContext::Editor, "Redo");
    registerShortcut("Ctrl+Shift+Z", "redo", []() { /* Alternative redo */ }, 
                    ShortcutContext::Editor, "Redo (alternative)");
    registerShortcut("Ctrl+X", "cut", []() { /* Cut selection */ }, 
                    ShortcutContext::Editor, "Cut");
    registerShortcut("Ctrl+C", "copy", []() { /* Copy selection */ }, 
                    ShortcutContext::Editor, "Copy");
    registerShortcut("Ctrl+V", "paste", []() { /* Paste from clipboard */ }, 
                    ShortcutContext::Editor, "Paste");
    registerShortcut("Ctrl+A", "selectAll", []() { /* Select all text */ }, 
                    ShortcutContext::Editor, "Select all");
    
    // Multi-cursor operations
    registerShortcut("Ctrl+D", "addCursorAtNextOccurrence", []() { /* Add cursor at next occurrence */ }, 
                    ShortcutContext::Editor, "Add cursor at next occurrence");
    registerShortcut("Ctrl+Shift+L", "selectAllOccurrences", []() { /* Select all occurrences */ }, 
                    ShortcutContext::Editor, "Select all occurrences");
    registerShortcut("Ctrl+Alt+Up", "addCursorAbove", []() { /* Add cursor above */ }, 
                    ShortcutContext::Editor, "Add cursor above");
    registerShortcut("Ctrl+Alt+Down", "addCursorBelow", []() { /* Add cursor below */ }, 
                    ShortcutContext::Editor, "Add cursor below");
    registerShortcut("Escape", "clearExtraCursors", []() { /* Clear extra cursors */ }, 
                    ShortcutContext::Editor, "Clear extra cursors");
    
    // Code folding
    registerShortcut("Ctrl+Shift+[", "foldRegion", []() { /* Fold current region */ }, 
                    ShortcutContext::Editor, "Fold region");
    registerShortcut("Ctrl+Shift+]", "unfoldRegion", []() { /* Unfold current region */ }, 
                    ShortcutContext::Editor, "Unfold region");
    registerShortcut("Ctrl+K Ctrl+0", "foldAll", []() { /* Fold all regions */ }, 
                    ShortcutContext::Editor, "Fold all");
    registerShortcut("Ctrl+K Ctrl+J", "unfoldAll", []() { /* Unfold all regions */ }, 
                    ShortcutContext::Editor, "Unfold all");
}

void KeyboardShortcuts::initFileTreeShortcuts() {
    registerShortcut("Ctrl+Shift+E", "toggleFileTree", []() { /* Toggle file tree visibility */ }, 
                    ShortcutContext::FileTree, "Toggle file tree");
    registerShortcut("F2", "rename", []() { /* Rename selected file */ }, 
                    ShortcutContext::FileTree, "Rename file");
    registerShortcut("Delete", "delete", []() { /* Delete selected file */ }, 
                    ShortcutContext::FileTree, "Delete file");
    registerShortcut("Ctrl+Shift+N", "newFile", []() { /* Create new file */ }, 
                    ShortcutContext::FileTree, "New file");
    registerShortcut("Ctrl+Shift+Alt+N", "newFolder", []() { /* Create new folder */ }, 
                    ShortcutContext::FileTree, "New folder");
}

void KeyboardShortcuts::initSearchShortcuts() {
    registerShortcut("Ctrl+F", "find", []() { /* Open find dialog */ }, 
                    ShortcutContext::Search, "Find");
    registerShortcut("Ctrl+H", "replace", []() { /* Open find/replace dialog */ }, 
                    ShortcutContext::Search, "Find and replace");
    registerShortcut("F3", "findNext", []() { /* Find next occurrence */ }, 
                    ShortcutContext::Search, "Find next");
    registerShortcut("Shift+F3", "findPrevious", []() { /* Find previous occurrence */ }, 
                    ShortcutContext::Search, "Find previous");
    registerShortcut("Ctrl+G", "goto", []() { /* Go to line */ }, 
                    ShortcutContext::Search, "Go to line");
    registerShortcut("Ctrl+Shift+F", "findInFiles", []() { /* Find in files */ }, 
                    ShortcutContext::Search, "Find in files");
}

void KeyboardShortcuts::initDebuggingShortcuts() {
    registerShortcut("F5", "startDebugging", []() { /* Start debugging */ }, 
                    ShortcutContext::Debugging, "Start debugging");
    registerShortcut("Ctrl+F5", "runWithoutDebugging", []() { /* Run without debugging */ }, 
                    ShortcutContext::Debugging, "Run without debugging");
    registerShortcut("Shift+F5", "stopDebugging", []() { /* Stop debugging */ }, 
                    ShortcutContext::Debugging, "Stop debugging");
    registerShortcut("F9", "toggleBreakpoint", []() { /* Toggle breakpoint */ }, 
                    ShortcutContext::Debugging, "Toggle breakpoint");
    registerShortcut("F10", "stepOver", []() { /* Step over */ }, 
                    ShortcutContext::Debugging, "Step over");
    registerShortcut("F11", "stepInto", []() { /* Step into */ }, 
                    ShortcutContext::Debugging, "Step into");
    registerShortcut("Shift+F11", "stepOut", []() { /* Step out */ }, 
                    ShortcutContext::Debugging, "Step out");
}

bool KeyboardShortcuts::isKeyConflict(const KeyCombination& keyCombination, ShortcutContext context,
                                     const std::string& command) const {
    for (const auto& [info, callback] : shortcuts_) {
        if (info.keyCombination.ctrl == keyCombination.ctrl &&
            info.keyCombination.shift == keyCombination.shift &&
            info.keyCombination.alt == keyCombination.alt &&
            info.keyCombination.meta == keyCombination.meta &&
            info.keyCombination.key == keyCombination.key &&
            info.context == context &&
            info.command != command) {
            return true;
        }
    }
    return false;
}

bool KeyboardShortcuts::loadShortcutsFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse line format: key_combination|command|context|description
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        
        while (std::getline(ss, part, '|')) {
            parts.push_back(part);
        }
        
        if (parts.size() < 3) {
            std::cerr << "Warning: Invalid line " << lineNumber << " in " << filePath << std::endl;
            continue;
        }
        
        std::string keyString = parts[0];
        std::string command = parts[1];
        std::string contextStr = parts[2];
        std::string description = parts.size() > 3 ? parts[3] : "";
        
        ShortcutContext context = stringToContext(contextStr);
        
        // Register shortcut with empty callback (will be overridden by actual implementation)
        registerShortcut(keyString, command, []() {
            // Placeholder callback - will be replaced by actual implementations
        }, context, description);
    }
    
    return true;
}

bool KeyboardShortcuts::saveShortcutsToFile(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# Bolt C++ Keyboard Shortcuts Configuration\n";
    file << "# Format: key_combination|command|context|description\n\n";
    
    // Group shortcuts by context
    std::map<ShortcutContext, std::vector<ShortcutInfo>> groupedShortcuts;
    for (const auto& [info, callback] : shortcuts_) {
        groupedShortcuts[info.context].push_back(info);
    }
    
    for (const auto& [context, shortcuts] : groupedShortcuts) {
        file << "# " << contextToString(context) << " shortcuts\n";
        
        for (const auto& info : shortcuts) {
            file << info.keyCombination.toString() << "|"
                 << info.command << "|"
                 << contextToString(info.context) << "|"
                 << info.description << "\n";
        }
        file << "\n";
    }
    
    return true;
}

void KeyboardShortcuts::resetToDefaults() {
    shortcuts_.clear();
    initDefaultShortcuts();
}

std::string KeyboardShortcuts::contextToString(ShortcutContext context) const {
    switch (context) {
        case ShortcutContext::Global: return "Global";
        case ShortcutContext::Editor: return "Editor";
        case ShortcutContext::FileTree: return "File Tree";
        case ShortcutContext::Search: return "Search";
        case ShortcutContext::Debugging: return "Debugging";
        default: return "Unknown";
    }
}

ShortcutContext KeyboardShortcuts::stringToContext(const std::string& contextStr) const {
    if (contextStr == "Global") return ShortcutContext::Global;
    if (contextStr == "Editor") return ShortcutContext::Editor;
    if (contextStr == "File Tree") return ShortcutContext::FileTree;
    if (contextStr == "Search") return ShortcutContext::Search;
    if (contextStr == "Debugging") return ShortcutContext::Debugging;
    return ShortcutContext::Global;
}

} // namespace bolt
