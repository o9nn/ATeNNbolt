#ifndef BOLT_KEYBOARD_SHORTCUTS_HPP
#define BOLT_KEYBOARD_SHORTCUTS_HPP

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <unordered_set>

namespace bolt {

using ShortcutCallback = std::function<void()>;

enum class ShortcutContext {
    Global,
    Editor,
    FileTree,
    Search,
    Debugging
};

struct KeyCombination {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool meta = false;  // Command key on Mac, Windows key on PC
    std::string key;    // The main key (e.g., "A", "F1", "Enter", "Escape")
    
    bool operator<(const KeyCombination& other) const;
    std::string toString() const;
    static KeyCombination fromString(const std::string& keyString);
    bool isValid() const;
};

struct ShortcutInfo {
    KeyCombination keyCombination;
    std::string command;
    std::string description;
    ShortcutContext context;
    ShortcutCallback callback;
    
    bool operator<(const ShortcutInfo& other) const {
        if (context != other.context) return context < other.context;
        if (keyCombination < other.keyCombination) return true;
        if (other.keyCombination < keyCombination) return false;
        return command < other.command;
    }
};

class KeyboardShortcuts {
public:
    static KeyboardShortcuts& getInstance();

    // Core shortcut management
    bool registerShortcut(const std::string& keyString, const std::string& command, 
                         ShortcutCallback callback, ShortcutContext context = ShortcutContext::Global,
                         const std::string& description = "");
    bool registerShortcut(const KeyCombination& keyCombination, const std::string& command,
                         ShortcutCallback callback, ShortcutContext context = ShortcutContext::Global,
                         const std::string& description = "");
    bool unregisterShortcut(const std::string& keyString, const std::string& command, 
                           ShortcutContext context = ShortcutContext::Global);
    bool unregisterShortcut(const KeyCombination& keyCombination, const std::string& command,
                           ShortcutContext context = ShortcutContext::Global);
    
    // Shortcut execution
    bool executeShortcut(const std::string& keyString, ShortcutContext context = ShortcutContext::Global);
    bool executeShortcut(const KeyCombination& keyCombination, ShortcutContext context = ShortcutContext::Global);
    
    // Context management
    void setActiveContext(ShortcutContext context);
    ShortcutContext getActiveContext() const;
    void pushContext(ShortcutContext context);
    void popContext();
    
    // Query and discovery
    std::vector<ShortcutInfo> getShortcutsForContext(ShortcutContext context) const;
    std::vector<ShortcutInfo> getAllShortcuts() const;
    std::vector<ShortcutInfo> findShortcutsByCommand(const std::string& command) const;
    std::vector<ShortcutInfo> findShortcutsByKey(const std::string& keyString) const;
    bool hasShortcut(const KeyCombination& keyCombination, ShortcutContext context) const;
    
    // Configuration
    bool loadShortcutsFromFile(const std::string& filePath);
    bool saveShortcutsToFile(const std::string& filePath) const;
    void resetToDefaults();
    
    // Help and documentation
    std::string getHelpText(ShortcutContext context = ShortcutContext::Global) const;
    std::string getShortcutDescription(const std::string& keyString, ShortcutContext context = ShortcutContext::Global) const;
    
    // Enable/disable shortcuts
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Initialize default shortcuts
    void initDefaultShortcuts();

private:
    KeyboardShortcuts() = default;
    
    std::map<ShortcutInfo, ShortcutCallback> shortcuts_;
    std::vector<ShortcutContext> contextStack_;
    ShortcutContext activeContext_ = ShortcutContext::Global;
    bool enabled_ = true;
    
    // Helper methods
    void initGlobalShortcuts();
    void initEditorShortcuts();
    void initFileTreeShortcuts();
    void initSearchShortcuts();
    void initDebuggingShortcuts();
    
    bool isKeyConflict(const KeyCombination& keyCombination, ShortcutContext context, 
                      const std::string& command) const;
    std::string contextToString(ShortcutContext context) const;
    ShortcutContext stringToContext(const std::string& contextStr) const;
};

} // namespace bolt

#endif // BOLT_KEYBOARD_SHORTCUTS_HPP