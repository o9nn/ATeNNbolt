
#include "bolt/editor/code_folding_visibility.hpp"

namespace bolt {

void CodeFoldingVisibilityManager::setLineVisibility(const std::string& filePath, size_t line, bool visible) {
    visibilityMap_[filePath][line] = visible;
}

bool CodeFoldingVisibilityManager::isLineVisible(const std::string& filePath, size_t line) const {
    auto fileIt = visibilityMap_.find(filePath);
    if (fileIt == visibilityMap_.end()) {
        return true; // Default visible if not managed
    }

    auto lineIt = fileIt->second.find(line);
    if (lineIt == fileIt->second.end()) {
        return true; // Default visible if not managed
    }

    return lineIt->second;
}

void CodeFoldingVisibilityManager::toggleFoldedSection(const std::string& filePath, size_t startLine, size_t endLine) {
    bool currentState = isLineVisible(filePath, startLine + 1);
    for (size_t line = startLine + 1; line <= endLine; ++line) {
        setLineVisibility(filePath, line, !currentState);
    }
}

void CodeFoldingVisibilityManager::showAllLines(const std::string& filePath) {
    visibilityMap_.erase(filePath);
}

void CodeFoldingVisibilityManager::updateFoldedSections(const std::string& filePath, const std::vector<FoldRange>& foldRanges) {
    for (const auto& range : foldRanges) {
        if (range.isFolded) {
            for (size_t line = range.startLine + 1; line <= range.endLine; ++line) {
                setLineVisibility(filePath, line, false);
            }
        }
    }
}

} // namespace bolt
