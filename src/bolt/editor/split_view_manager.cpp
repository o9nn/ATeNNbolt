#include "bolt/editor/split_view_manager.hpp"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace bolt {

SplitViewManager::SplitViewManager() {
    // Create initial default pane
    std::string initialPaneId = createPane();
    activePaneId_.write([&](std::string& activeId) {
        activeId = initialPaneId;
    });
}

std::string SplitViewManager::createHorizontalSplit(const std::string& sourcePaneId) {
    std::string sourceId = sourcePaneId.empty() ? getActivePaneId() : sourcePaneId;
    
    std::string newPaneId;
    EditorPane::PanePosition newPanePosition;
    
    newPaneId = withPanes([&](auto& panes) -> std::string {
        auto it = panes.find(sourceId);
        if (it == panes.end()) {
            return "";  // Source pane not found
        }
        
        EditorPane* sourcePane = it->second.get();
        EditorPane::PanePosition sourcePos = sourcePane->getPosition();
        
        // Calculate new positions for horizontal split (left/right)
        EditorPane::PanePosition leftPos = sourcePos;
        leftPos.width = sourcePos.width / 2;
        
        EditorPane::PanePosition rightPos = sourcePos;
        rightPos.x = sourcePos.x + leftPos.width;
        rightPos.width = sourcePos.width - leftPos.width;
        
        // Validate sizes
        if (!validatePaneSize(leftPos.width, leftPos.height) || 
            !validatePaneSize(rightPos.width, rightPos.height)) {
            return "";  // Split would create too small panes
        }
        
        // Update source pane position
        sourcePane->setPosition(leftPos);
        
        // Create new pane
        std::string newPaneIdLocal = generatePaneId();
        auto newPane = std::make_unique<EditorPane>(newPaneIdLocal);
        newPane->setPosition(rightPos);
        
        // Copy document from source if it has one
        if (sourcePane->hasDocument()) {
            newPane->openDocument(sourcePane->getDocumentPath());
        }
        
        panes[newPaneIdLocal] = std::move(newPane);
        
        // Store position for later use
        newPanePosition = rightPos;
        
        return newPaneIdLocal;
    });
    
    if (!newPaneId.empty()) {
        // Update layout and focus outside of withPanes to avoid deadlock
        addPaneToLayout(newPaneId, newPanePosition);
        focusPane(newPaneId);
        notifyPaneOperation(PaneOperation::Create, newPaneId);
    }
    
    return newPaneId;
}

std::string SplitViewManager::createVerticalSplit(const std::string& sourcePaneId) {
    std::string sourceId = sourcePaneId.empty() ? getActivePaneId() : sourcePaneId;
    
    std::string newPaneId;
    EditorPane::PanePosition newPanePosition;
    
    newPaneId = withPanes([&](auto& panes) -> std::string {
        auto it = panes.find(sourceId);
        if (it == panes.end()) {
            return "";  // Source pane not found
        }
        
        EditorPane* sourcePane = it->second.get();
        EditorPane::PanePosition sourcePos = sourcePane->getPosition();
        
        // Calculate new positions for vertical split (top/bottom)
        EditorPane::PanePosition topPos = sourcePos;
        topPos.height = sourcePos.height / 2;
        
        EditorPane::PanePosition bottomPos = sourcePos;
        bottomPos.y = sourcePos.y + topPos.height;
        bottomPos.height = sourcePos.height - topPos.height;
        
        // Validate sizes
        if (!validatePaneSize(topPos.width, topPos.height) || 
            !validatePaneSize(bottomPos.width, bottomPos.height)) {
            return "";  // Split would create too small panes
        }
        
        // Update source pane position
        sourcePane->setPosition(topPos);
        
        // Create new pane
        std::string newPaneIdLocal = generatePaneId();
        auto newPane = std::make_unique<EditorPane>(newPaneIdLocal);
        newPane->setPosition(bottomPos);
        
        // Copy document from source if it has one
        if (sourcePane->hasDocument()) {
            newPane->openDocument(sourcePane->getDocumentPath());
        }
        
        panes[newPaneIdLocal] = std::move(newPane);
        
        // Store position for later use
        newPanePosition = bottomPos;
        
        return newPaneIdLocal;
    });
    
    if (!newPaneId.empty()) {
        // Update layout and focus outside of withPanes to avoid deadlock
        addPaneToLayout(newPaneId, newPanePosition);
        focusPane(newPaneId);
        notifyPaneOperation(PaneOperation::Create, newPaneId);
    }
    
    return newPaneId;
}

bool SplitViewManager::closePane(const std::string& paneId) {
    // Check validity first (avoid nested lock in withPanes)
    bool paneExists = withPanes([&](const auto& panes) -> bool {
        return panes.find(paneId) != panes.end();
    });
    
    if (!paneExists) {
        return false;
    }
    
    // Get active pane ID outside of the withPanes call to avoid deadlock
    std::string activeId = getActivePaneId();
    std::string newActivePaneId;
    
    bool success = withPanes([&](auto& panes) -> bool {
        // Don't allow closing the last pane
        if (panes.size() <= 1) {
            return false;
        }
        
        auto it = panes.find(paneId);
        if (it == panes.end()) {
            return false;
        }
        
        // If closing the active pane, find next available pane
        if (activeId == paneId) {
            for (const auto& pair : panes) {
                if (pair.first != paneId) {
                    newActivePaneId = pair.first;
                    break;
                }
            }
        }
        
        // Remove the pane
        panes.erase(it);
        
        return true;
    });
    
    if (success) {
        // Update focus if needed (outside of panes lock)
        if (!newActivePaneId.empty()) {
            focusPane(newActivePaneId);
        }
        
        // Remove from layout
        removePaneFromLayout(paneId);
        
        // Redistribute remaining panes
        redistributeLayout();
        
        notifyPaneOperation(PaneOperation::Close, paneId);
    }
    
    return success;
}

bool SplitViewManager::focusPane(const std::string& paneId) {
    // Get current active pane ID outside of the withPanes call to avoid deadlock
    std::string oldActiveId = getActivePaneId();
    
    bool success = withPanes([&](auto& panes) -> bool {
        auto it = panes.find(paneId);
        if (it == panes.end()) {
            return false;
        }
        
        // Remove focus from current active pane
        if (!oldActiveId.empty() && oldActiveId != paneId) {
            auto oldIt = panes.find(oldActiveId);
            if (oldIt != panes.end()) {
                oldIt->second->setFocus(false);
            }
        }
        
        // Set focus to new pane
        it->second->setFocus(true);
        
        return true;
    });
    
    if (success) {
        // Update active pane ID outside of the panes lock
        activePaneId_.write([&](std::string& activeId) {
            activeId = paneId;
        });
        
        notifyPaneOperation(PaneOperation::Focus, paneId);
    }
    
    return success;
}

std::string SplitViewManager::createPane() {
    std::string paneId = generatePaneId();
    
    auto updatePanes = [&](std::map<std::string, std::unique_ptr<EditorPane>>& panes) {
        auto newPane = std::make_unique<EditorPane>(paneId);
        
        // Set default position based on existing panes
        EditorPane::PanePosition pos{0, 0, 800, 600};
        if (!panes.empty()) {
            // Position new pane relative to existing ones
            pos = calculateSplitPosition(pos, SplitDirection::Horizontal);
        }
        
        newPane->setPosition(pos);
        panes[paneId] = std::move(newPane);
        
        addPaneToLayout(paneId, pos);
    };
    
    panes_.write(updatePanes);
    
    notifyPaneOperation(PaneOperation::Create, paneId);
    return paneId;
}

EditorPane* SplitViewManager::getPane(const std::string& paneId) {
    return withPanes([&](auto& panes) -> EditorPane* {
        auto it = panes.find(paneId);
        return (it != panes.end()) ? it->second.get() : nullptr;
    });
}

const EditorPane* SplitViewManager::getPane(const std::string& paneId) const {
    return withPanes([&](const auto& panes) -> const EditorPane* {
        auto it = panes.find(paneId);
        return (it != panes.end()) ? it->second.get() : nullptr;
    });
}

EditorPane* SplitViewManager::getActivePane() {
    std::string activeId = getActivePaneId();
    return activeId.empty() ? nullptr : getPane(activeId);
}

const EditorPane* SplitViewManager::getActivePane() const {
    std::string activeId = getActivePaneId();
    return activeId.empty() ? nullptr : getPane(activeId);
}

std::string SplitViewManager::getActivePaneId() const {
    return activePaneId_.read([](const std::string& activeId) {
        return activeId;
    });
}

void SplitViewManager::setContainerSize(int width, int height) {
    withPanes([&](auto& panes) {
        if (panes.size() == 1) {
            // Single pane takes full container
            auto& pane = panes.begin()->second;
            EditorPane::PanePosition pos{0, 0, width, height};
            pane->setPosition(pos);
        } else {
            // Redistribute multiple panes
            redistributeLayout();
        }
    });
}

void SplitViewManager::resizePane(const std::string& paneId, int width, int height) {
    withPanes([&](auto& panes) {
        auto it = panes.find(paneId);
        if (it != panes.end()) {
            EditorPane::PanePosition pos = it->second->getPosition();
            pos.width = width;
            pos.height = height;
            enforceSizeConstraints(pos);
            it->second->setPosition(pos);
            
            notifyPaneOperation(PaneOperation::Resize, paneId);
        }
    });
}

void SplitViewManager::redistributeLayout() {
    withPanes([&](auto& panes) {
        if (panes.empty()) {
            return;
        }
        
        // Simple redistribution: arrange panes in a grid
        size_t paneCount = panes.size();
        int cols = static_cast<int>(std::ceil(std::sqrt(paneCount)));
        int rows = static_cast<int>(std::ceil(static_cast<double>(paneCount) / cols));
        
        int containerWidth = 800;  // Default container size
        int containerHeight = 600;
        
        // Get container size from first pane if available
        if (!panes.empty()) {
            const auto& firstPane = panes.begin()->second;
            const auto& pos = firstPane->getPosition();
            containerWidth = pos.width * cols;
            containerHeight = pos.height * rows;
        }
        
        int paneWidth = containerWidth / cols;
        int paneHeight = containerHeight / rows;
        
        int index = 0;
        for (auto& pair : panes) {
            int col = index % cols;
            int row = index / cols;
            
            EditorPane::PanePosition pos{
                col * paneWidth,
                row * paneHeight,
                paneWidth,
                paneHeight
            };
            
            enforceSizeConstraints(pos);
            pair.second->setPosition(pos);
            index++;
        }
    });
}

std::vector<std::string> SplitViewManager::getAllPaneIds() const {
    return withPanes([](const auto& panes) -> std::vector<std::string> {
        std::vector<std::string> ids;
        ids.reserve(panes.size());
        for (const auto& pair : panes) {
            ids.push_back(pair.first);
        }
        return ids;
    });
}

size_t SplitViewManager::getPaneCount() const {
    return withPanes([](const auto& panes) -> size_t {
        return panes.size();
    });
}

void SplitViewManager::openDocumentInPane(const std::string& paneId, const std::string& filePath) {
    bool success = withPanes([&](auto& panes) -> bool {
        auto it = panes.find(paneId);
        if (it != panes.end()) {
            it->second->openDocument(filePath);
            return true;
        }
        return false;
    });
    
    if (success) {
        // Focus the pane with the new document (outside of withPanes to avoid deadlock)
        focusPane(paneId);
    }
}

void SplitViewManager::openDocumentInNewPane(const std::string& filePath, SplitDirection direction) {
    std::string newPaneId;
    if (direction == SplitDirection::Horizontal) {
        newPaneId = createHorizontalSplit();
    } else {
        newPaneId = createVerticalSplit();
    }
    
    if (!newPaneId.empty()) {
        openDocumentInPane(newPaneId, filePath);
    }
}

std::vector<std::string> SplitViewManager::getOpenDocuments() const {
    return withPanes([](const auto& panes) -> std::vector<std::string> {
        std::vector<std::string> documents;
        for (const auto& pair : panes) {
            if (pair.second->hasDocument()) {
                documents.push_back(pair.second->getDocumentPath());
            }
        }
        return documents;
    });
}

std::string SplitViewManager::findPaneWithDocument(const std::string& filePath) const {
    return withPanes([&](const auto& panes) -> std::string {
        for (const auto& pair : panes) {
            if (pair.second->hasDocument() && pair.second->getDocumentPath() == filePath) {
                return pair.first;
            }
        }
        return "";
    });
}

bool SplitViewManager::isEnabled() const {
    return isEnabled_.read([](bool enabled) { return enabled; });
}

void SplitViewManager::setEnabled(bool enabled) {
    isEnabled_.write([enabled](bool& currentEnabled) {
        currentEnabled = enabled;
    });
    
    if (!enabled) {
        collapseAllSplits();
    }
}

bool SplitViewManager::hasSplits() const {
    return getPaneCount() > 1;
}

void SplitViewManager::collapseAllSplits() {
    auto updatePanes = [&](std::map<std::string, std::unique_ptr<EditorPane>>& panes) {
        if (panes.size() <= 1) {
            return;
        }
        
        // Keep only the active pane
        std::string activeId = getActivePaneId();
        if (activeId.empty() && !panes.empty()) {
            activeId = panes.begin()->first;
        }
        
        // Remove all panes except the active one
        auto it = panes.begin();
        while (it != panes.end()) {
            if (it->first != activeId) {
                it = panes.erase(it);
            } else {
                ++it;
            }
        }
        
        // Resize remaining pane to full size
        if (!panes.empty()) {
            EditorPane::PanePosition fullSize{0, 0, 800, 600};
            panes.begin()->second->setPosition(fullSize);
        }
    };
    
    panes_.write(updatePanes);
    
    // Clear layouts
    layouts_.write([](auto& layouts) {
        layouts.clear();
    });
}

std::string SplitViewManager::getNextPane(const std::string& currentPaneId, bool forward) const {
    return withPanes([&](const auto& panes) -> std::string {
        if (panes.size() <= 1) {
            return currentPaneId;
        }
        
        std::vector<std::string> paneIds;
        for (const auto& pair : panes) {
            paneIds.push_back(pair.first);
        }
        
        auto it = std::find(paneIds.begin(), paneIds.end(), currentPaneId);
        if (it == paneIds.end()) {
            return paneIds.empty() ? "" : paneIds[0];
        }
        
        if (forward) {
            ++it;
            if (it == paneIds.end()) {
                it = paneIds.begin();
            }
        } else {
            if (it == paneIds.begin()) {
                it = paneIds.end() - 1;
            } else {
                --it;
            }
        }
        
        return *it;
    });
}

bool SplitViewManager::navigateToNextPane() {
    std::string currentId = getActivePaneId();
    std::string nextId = getNextPane(currentId, true);
    return !nextId.empty() && nextId != currentId && focusPane(nextId);
}

bool SplitViewManager::navigateToPreviousPane() {
    std::string currentId = getActivePaneId();
    std::string prevId = getNextPane(currentId, false);
    return !prevId.empty() && prevId != currentId && focusPane(prevId);
}

void SplitViewManager::setOnPaneOperation(PaneOperationCallback callback) {
    onPaneOperation_ = callback;
}

std::string SplitViewManager::serializeLayout() const {
    // Simple serialization - in practice would use JSON or similar
    std::ostringstream oss;
    withPanes([&](const auto& panes) {
        for (const auto& pair : panes) {
            const auto& pos = pair.second->getPosition();
            oss << pair.first << ":" << pos.x << "," << pos.y << "," << pos.width << "," << pos.height;
            if (pair.second->hasDocument()) {
                oss << ":" << pair.second->getDocumentPath();
            }
            oss << ";";
        }
    });
    return oss.str();
}

bool SplitViewManager::restoreLayout(const std::string& layoutData) {
    // Basic layout restoration - would be more robust in practice
    clearAllPanes();
    
    std::istringstream iss(layoutData);
    std::string paneData;
    
    while (std::getline(iss, paneData, ';')) {
        if (paneData.empty()) continue;
        
        // Parse pane data (simplified)
        size_t colonPos = paneData.find(':');
        if (colonPos == std::string::npos) continue;
        
        std::string paneId = paneData.substr(0, colonPos);
        // Would parse position and document path here
        
        createPane();  // Simplified - just create panes
    }
    
    return true;
}

void SplitViewManager::clearAllPanes() {
    auto clearPanes = [](std::map<std::string, std::unique_ptr<EditorPane>>& panes) {
        panes.clear();
    };
    
    panes_.write(clearPanes);
    
    layouts_.write([](auto& layouts) {
        layouts.clear();
    });
    
    activePaneId_.write([](std::string& activeId) {
        activeId = "";
    });
}

// Private methods

void SplitViewManager::calculatePanePositions() {
    redistributeLayout();
}

void SplitViewManager::updateLayout(const std::string& modifiedPaneId) {
    // Update layout when a pane is modified
    calculatePanePositions();
}

EditorPane::PanePosition SplitViewManager::calculateSplitPosition(
    const EditorPane::PanePosition& sourcePosition, 
    SplitDirection direction, 
    float splitRatio) {
    
    EditorPane::PanePosition newPosition = sourcePosition;
    
    if (direction == SplitDirection::Horizontal) {
        int splitWidth = static_cast<int>(sourcePosition.width * splitRatio);
        newPosition.x = sourcePosition.x + splitWidth;
        newPosition.width = sourcePosition.width - splitWidth;
    } else {
        int splitHeight = static_cast<int>(sourcePosition.height * splitRatio);
        newPosition.y = sourcePosition.y + splitHeight;
        newPosition.height = sourcePosition.height - splitHeight;
    }
    
    enforceSizeConstraints(newPosition);
    return newPosition;
}

std::string SplitViewManager::generatePaneId() {
    return nextPaneId_.write([](int& nextId) -> std::string {
        return "pane_" + std::to_string(nextId++);
    });
}

void SplitViewManager::notifyPaneOperation(PaneOperation op, const std::string& paneId) {
    if (onPaneOperation_) {
        onPaneOperation_(op, paneId);
    }
}

bool SplitViewManager::isValidPaneId(const std::string& paneId) const {
    return withPanes([&](const auto& panes) -> bool {
        return panes.find(paneId) != panes.end();
    });
}

void SplitViewManager::removePaneFromLayout(const std::string& paneId) {
    layouts_.write([&](auto& layouts) {
        layouts.erase(
            std::remove_if(layouts.begin(), layouts.end(),
                [&paneId](const SplitLayout& layout) {
                    return layout.paneId == paneId;
                }),
            layouts.end()
        );
    });
}

void SplitViewManager::addPaneToLayout(const std::string& paneId, const EditorPane::PanePosition& position) {
    layouts_.write([&](auto& layouts) {
        SplitLayout layout;
        layout.paneId = paneId;
        layout.position = position;
        layouts.push_back(layout);
    });
}

bool SplitViewManager::validatePaneSize(int width, int height) const {
    return width >= MIN_PANE_WIDTH && height >= MIN_PANE_HEIGHT;
}

void SplitViewManager::enforceSizeConstraints(EditorPane::PanePosition& position) const {
    if (position.width < MIN_PANE_WIDTH) {
        position.width = MIN_PANE_WIDTH;
    }
    if (position.height < MIN_PANE_HEIGHT) {
        position.height = MIN_PANE_HEIGHT;
    }
}

// Template method implementations
template<typename Func>
auto SplitViewManager::withPanes(Func&& func) const -> decltype(func(std::declval<const std::map<std::string, std::unique_ptr<EditorPane>>&>())) {
    return panes_.read(std::forward<Func>(func));
}

template<typename Func>
auto SplitViewManager::withPanes(Func&& func) -> decltype(func(std::declval<std::map<std::string, std::unique_ptr<EditorPane>>&>())) {
    return panes_.write(std::forward<Func>(func));
}

} // namespace bolt