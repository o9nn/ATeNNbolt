#ifndef TAB_BAR_HPP
#define TAB_BAR_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include "bolt/core/thread_safety.hpp"

namespace bolt {

/**
 * @brief Represents a single editor tab
 */
struct EditorTab {
    std::string filePath;     // Full path to the file
    std::string displayName;  // Name shown in tab (usually filename)
    bool isDirty;             // Has unsaved changes
    bool isPinned;            // Tab is pinned (won't close easily)
    size_t id;                // Unique tab identifier
    
    EditorTab() : isDirty(false), isPinned(false), id(0) {}
    
    EditorTab(const std::string& path, size_t tabId) 
        : filePath(path), isDirty(false), isPinned(false), id(tabId) {
        // Extract filename from path for display
        size_t lastSlash = path.find_last_of("/\\");
        displayName = (lastSlash != std::string::npos) 
            ? path.substr(lastSlash + 1) 
            : path;
    }
    
    bool operator==(const EditorTab& other) const {
        return id == other.id;
    }
};

/**
 * @brief Manages a collection of editor tabs
 * 
 * The TabBar manages the list of open file tabs, handles tab switching,
 * closing, reordering, and provides callbacks for tab-related events.
 */
class TabBar {
private:
    ThreadSafe<std::vector<EditorTab>> tabs_;
    ThreadSafe<std::optional<size_t>> activeTabId_;
    size_t nextTabId_ = 1;
    
    // Callbacks
    std::function<void(size_t tabId)> onTabActivated_;
    std::function<void(size_t tabId)> onTabClosed_;
    std::function<void(size_t tabId)> onTabAdded_;
    std::function<void(bool isDirty)> onTabDirtyChanged_;

public:
    /**
     * @brief Add a new tab for a file
     * @param filePath Path to the file
     * @return ID of the created tab, or existing tab ID if file already open
     */
    size_t addTab(const std::string& filePath);
    
    /**
     * @brief Close a tab by ID
     * @param tabId ID of the tab to close
     * @return true if tab was closed, false if not found or pinned
     */
    bool closeTab(size_t tabId);
    
    /**
     * @brief Close a tab by file path
     * @param filePath Path to the file whose tab to close
     * @return true if tab was closed, false if not found or pinned
     */
    bool closeTabByPath(const std::string& filePath);
    
    /**
     * @brief Close all tabs except the specified one
     * @param exceptTabId ID of tab to keep open
     */
    void closeOtherTabs(size_t exceptTabId);
    
    /**
     * @brief Close all tabs (except pinned ones)
     */
    void closeAllTabs();
    
    /**
     * @brief Switch to a specific tab
     * @param tabId ID of the tab to activate
     * @return true if tab was activated, false if not found
     */
    bool activateTab(size_t tabId);
    
    /**
     * @brief Switch to tab by file path
     * @param filePath Path to the file whose tab to activate
     * @return true if tab was activated, false if not found
     */
    bool activateTabByPath(const std::string& filePath);
    
    /**
     * @brief Switch to next tab (right)
     */
    void activateNextTab();
    
    /**
     * @brief Switch to previous tab (left)
     */
    void activatePreviousTab();
    
    /**
     * @brief Get the currently active tab
     * @return Copy of active tab, or nullopt if no tabs
     */
    std::optional<EditorTab> getActiveTab() const;
    
    /**
     * @brief Get tab by ID
     * @param tabId ID of the tab
     * @return Copy of tab, or nullopt if not found
     */
    std::optional<EditorTab> getTab(size_t tabId) const;
    
    /**
     * @brief Get tab by file path
     * @param filePath Path to the file
     * @return Copy of tab, or nullopt if not found
     */
    std::optional<EditorTab> getTabByPath(const std::string& filePath) const;
    
    /**
     * @brief Get all tabs
     * @return Vector of all tabs
     */
    std::vector<EditorTab> getAllTabs() const;
    
    /**
     * @brief Get count of open tabs
     * @return Number of tabs
     */
    size_t getTabCount() const;
    
    /**
     * @brief Check if there are any tabs open
     * @return true if at least one tab exists
     */
    bool hasTabs() const;
    
    /**
     * @brief Mark a tab as dirty (has unsaved changes)
     * @param tabId ID of the tab
     * @param isDirty true if tab has unsaved changes
     */
    void setTabDirty(size_t tabId, bool isDirty);
    
    /**
     * @brief Mark a tab as dirty by file path
     * @param filePath Path to the file
     * @param isDirty true if tab has unsaved changes
     */
    void setTabDirtyByPath(const std::string& filePath, bool isDirty);
    
    /**
     * @brief Pin/unpin a tab
     * @param tabId ID of the tab
     * @param isPinned true to pin the tab
     */
    void setTabPinned(size_t tabId, bool isPinned);
    
    /**
     * @brief Reorder tabs by moving a tab to a new position
     * @param tabId ID of the tab to move
     * @param newIndex New position (0-based index)
     */
    void moveTab(size_t tabId, size_t newIndex);
    
    /**
     * @brief Find tab ID by file path
     * @param filePath Path to the file
     * @return Tab ID if found, 0 if not found
     */
    size_t findTabIdByPath(const std::string& filePath) const;
    
    // Callback setters
    void setOnTabActivated(std::function<void(size_t)> callback) { 
        onTabActivated_ = std::move(callback); 
    }
    
    void setOnTabClosed(std::function<void(size_t)> callback) { 
        onTabClosed_ = std::move(callback); 
    }
    
    void setOnTabAdded(std::function<void(size_t)> callback) { 
        onTabAdded_ = std::move(callback); 
    }
    
    void setOnTabDirtyChanged(std::function<void(bool)> callback) { 
        onTabDirtyChanged_ = std::move(callback); 
    }
    
    /**
     * @brief Singleton instance
     */
    static TabBar& getInstance() {
        static TabBar instance;
        return instance;
    }

private:
    TabBar() = default;
    
    // Helper to find tab index by ID
    std::optional<size_t> findTabIndexById(size_t tabId) const;
    std::optional<size_t> findTabIndexByPath(const std::string& filePath) const;
};

} // namespace bolt

#endif // TAB_BAR_HPP
