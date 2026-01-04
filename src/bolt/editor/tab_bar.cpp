#include "bolt/editor/tab_bar.hpp"
#include <algorithm>

namespace bolt {

size_t TabBar::addTab(const std::string& filePath) {
    // Check if tab already exists for this file
    size_t existingTabId = findTabIdByPath(filePath);
    if (existingTabId != 0) {
        // Tab exists, just activate it
        activateTab(existingTabId);
        return existingTabId;
    }
    
    // Create new tab
    size_t newTabId = nextTabId_++;
    EditorTab newTab(filePath, newTabId);
    
    tabs_.write([&](auto& tabs) {
        tabs.push_back(newTab);
    });
    
    // Activate the new tab
    activeTabId_.write([&](auto& activeId) {
        activeId = newTabId;
    });
    
    // Notify listeners
    if (onTabAdded_) {
        onTabAdded_(newTabId);
    }
    if (onTabActivated_) {
        onTabActivated_(newTabId);
    }
    
    return newTabId;
}

bool TabBar::closeTab(size_t tabId) {
    bool closed = false;
    
    tabs_.write([&](auto& tabs) {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [tabId](const EditorTab& tab) { return tab.id == tabId; });
        
        if (it != tabs.end()) {
            // Don't close pinned tabs
            if (it->isPinned) {
                return;
            }
            
            bool wasActive = false;
            activeTabId_.read([&](const auto& activeId) {
                wasActive = activeId.has_value() && activeId.value() == tabId;
            });
            
            // Remove the tab
            size_t removedIndex = std::distance(tabs.begin(), it);
            tabs.erase(it);
            closed = true;
            
            // If we closed the active tab, activate another one
            if (wasActive && !tabs.empty()) {
                // Activate the tab to the left, or the first tab if we removed the first
                size_t newActiveIndex = (removedIndex > 0) ? removedIndex - 1 : 0;
                if (newActiveIndex < tabs.size()) {
                    activeTabId_.write([&](auto& activeId) {
                        activeId = tabs[newActiveIndex].id;
                    });
                    if (onTabActivated_) {
                        onTabActivated_(tabs[newActiveIndex].id);
                    }
                }
            } else if (tabs.empty()) {
                activeTabId_.write([](auto& activeId) {
                    activeId = std::nullopt;
                });
            }
        }
    });
    
    if (closed && onTabClosed_) {
        onTabClosed_(tabId);
    }
    
    return closed;
}

bool TabBar::closeTabByPath(const std::string& filePath) {
    size_t tabId = findTabIdByPath(filePath);
    if (tabId != 0) {
        return closeTab(tabId);
    }
    return false;
}

void TabBar::closeOtherTabs(size_t exceptTabId) {
    std::vector<size_t> tabsToClose;
    
    tabs_.read([&](const auto& tabs) {
        for (const auto& tab : tabs) {
            if (tab.id != exceptTabId && !tab.isPinned) {
                tabsToClose.push_back(tab.id);
            }
        }
    });
    
    for (size_t tabId : tabsToClose) {
        closeTab(tabId);
    }
}

void TabBar::closeAllTabs() {
    std::vector<size_t> tabsToClose;
    
    tabs_.read([&](const auto& tabs) {
        for (const auto& tab : tabs) {
            if (!tab.isPinned) {
                tabsToClose.push_back(tab.id);
            }
        }
    });
    
    for (size_t tabId : tabsToClose) {
        closeTab(tabId);
    }
}

bool TabBar::activateTab(size_t tabId) {
    bool found = false;
    
    tabs_.read([&](const auto& tabs) {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [tabId](const EditorTab& tab) { return tab.id == tabId; });
        
        if (it != tabs.end()) {
            found = true;
        }
    });
    
    if (found) {
        activeTabId_.write([&](auto& activeId) {
            activeId = tabId;
        });
        
        if (onTabActivated_) {
            onTabActivated_(tabId);
        }
    }
    
    return found;
}

bool TabBar::activateTabByPath(const std::string& filePath) {
    size_t tabId = findTabIdByPath(filePath);
    if (tabId != 0) {
        return activateTab(tabId);
    }
    return false;
}

void TabBar::activateNextTab() {
    tabs_.read([&](const auto& tabs) {
        if (tabs.empty()) return;
        
        activeTabId_.write([&](auto& activeId) {
            if (!activeId.has_value()) {
                activeId = tabs[0].id;
                if (onTabActivated_) {
                    onTabActivated_(tabs[0].id);
                }
                return;
            }
            
            // Find current tab index
            auto it = std::find_if(tabs.begin(), tabs.end(),
                [&activeId](const EditorTab& tab) { return tab.id == activeId.value(); });
            
            if (it != tabs.end()) {
                // Move to next tab (wrap around)
                ++it;
                if (it == tabs.end()) {
                    it = tabs.begin();
                }
                activeId = it->id;
                if (onTabActivated_) {
                    onTabActivated_(it->id);
                }
            }
        });
    });
}

void TabBar::activatePreviousTab() {
    tabs_.read([&](const auto& tabs) {
        if (tabs.empty()) return;
        
        activeTabId_.write([&](auto& activeId) {
            if (!activeId.has_value()) {
                activeId = tabs[0].id;
                if (onTabActivated_) {
                    onTabActivated_(tabs[0].id);
                }
                return;
            }
            
            // Find current tab index
            auto it = std::find_if(tabs.begin(), tabs.end(),
                [&activeId](const EditorTab& tab) { return tab.id == activeId.value(); });
            
            if (it != tabs.end()) {
                // Move to previous tab (wrap around)
                if (it == tabs.begin()) {
                    it = tabs.end();
                }
                --it;
                activeId = it->id;
                if (onTabActivated_) {
                    onTabActivated_(it->id);
                }
            }
        });
    });
}

std::optional<EditorTab> TabBar::getActiveTab() const {
    std::optional<EditorTab> result;
    
    activeTabId_.read([&](const auto& activeId) {
        if (activeId.has_value()) {
            result = getTab(activeId.value());
        }
    });
    
    return result;
}

std::optional<EditorTab> TabBar::getTab(size_t tabId) const {
    return tabs_.read([&](const auto& tabs) -> std::optional<EditorTab> {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [tabId](const EditorTab& tab) { return tab.id == tabId; });
        
        if (it != tabs.end()) {
            return *it;  // Return copy
        }
        return std::nullopt;
    });
}

std::optional<EditorTab> TabBar::getTabByPath(const std::string& filePath) const {
    return tabs_.read([&](const auto& tabs) -> std::optional<EditorTab> {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [&filePath](const EditorTab& tab) { return tab.filePath == filePath; });
        
        if (it != tabs.end()) {
            return *it;  // Return copy
        }
        return std::nullopt;
    });
}

std::vector<EditorTab> TabBar::getAllTabs() const {
    return tabs_.read([](const auto& tabs) {
        return tabs;
    });
}

size_t TabBar::getTabCount() const {
    return tabs_.read([](const auto& tabs) {
        return tabs.size();
    });
}

bool TabBar::hasTabs() const {
    return getTabCount() > 0;
}

void TabBar::setTabDirty(size_t tabId, bool isDirty) {
    tabs_.write([&](auto& tabs) {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [tabId](const EditorTab& tab) { return tab.id == tabId; });
        
        if (it != tabs.end()) {
            it->isDirty = isDirty;
        }
    });
    
    if (onTabDirtyChanged_) {
        onTabDirtyChanged_(isDirty);
    }
}

void TabBar::setTabDirtyByPath(const std::string& filePath, bool isDirty) {
    size_t tabId = findTabIdByPath(filePath);
    if (tabId != 0) {
        setTabDirty(tabId, isDirty);
    }
}

void TabBar::setTabPinned(size_t tabId, bool isPinned) {
    tabs_.write([&](auto& tabs) {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [tabId](const EditorTab& tab) { return tab.id == tabId; });
        
        if (it != tabs.end()) {
            it->isPinned = isPinned;
        }
    });
}

void TabBar::moveTab(size_t tabId, size_t newIndex) {
    tabs_.write([&](auto& tabs) {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [tabId](const EditorTab& tab) { return tab.id == tabId; });
        
        if (it != tabs.end() && newIndex < tabs.size()) {
            EditorTab tab = *it;
            tabs.erase(it);
            tabs.insert(tabs.begin() + newIndex, tab);
        }
    });
}

size_t TabBar::findTabIdByPath(const std::string& filePath) const {
    return tabs_.read([&](const auto& tabs) {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [&filePath](const EditorTab& tab) { return tab.filePath == filePath; });
        
        return (it != tabs.end()) ? it->id : 0;
    });
}

std::optional<size_t> TabBar::findTabIndexById(size_t tabId) const {
    return tabs_.read([&](const auto& tabs) -> std::optional<size_t> {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [tabId](const EditorTab& tab) { return tab.id == tabId; });
        
        if (it != tabs.end()) {
            return std::distance(tabs.begin(), it);
        }
        return std::nullopt;
    });
}

std::optional<size_t> TabBar::findTabIndexByPath(const std::string& filePath) const {
    return tabs_.read([&](const auto& tabs) -> std::optional<size_t> {
        auto it = std::find_if(tabs.begin(), tabs.end(),
            [&filePath](const EditorTab& tab) { return tab.filePath == filePath; });
        
        if (it != tabs.end()) {
            return std::distance(tabs.begin(), it);
        }
        return std::nullopt;
    });
}

} // namespace bolt
