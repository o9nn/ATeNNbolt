#include "bolt/editor/code_folding_manager.hpp"

namespace bolt {

void CodeFoldingManager::updateFoldingRanges(const std::string& filePath, const std::string& content) {
    bool enabled = foldingEnabled_.read([](const bool& value) { return value; });
    if (!enabled) return;
    
    auto ranges = CodeFoldingDetector::detectFoldableRanges(content);
    foldingRanges_[filePath] = ranges;
}

void CodeFoldingManager::toggleFold(const std::string& filePath, size_t line) {
    auto& ranges = foldingRanges_[filePath];
    for (auto& range : ranges) {
        if (line >= range.startLine && line <= range.endLine) {
            range.isFolded = !range.isFolded;
            visibilityManager_.toggleFoldedSection(filePath, range.startLine, range.endLine);
            break;
        }
    }
    if (onFoldingChanged_) {
        onFoldingChanged_();
    }
}

std::vector<FoldRange> CodeFoldingManager::getFoldingRanges(const std::string& filePath) const {
    auto it = foldingRanges_.find(filePath);
    return it != foldingRanges_.end() ? it->second : std::vector<FoldRange>{};
}

void CodeFoldingManager::setFoldingEnabled(bool enabled) {
    foldingEnabled_.write([enabled](bool& value) {
        value = enabled;
        return value;
    });
    if (enabled) {
        refreshFoldingRanges();
    }
}

bool CodeFoldingManager::isFoldingEnabled() const {
    return foldingEnabled_.read([](const bool& value) { return value; });
}

void CodeFoldingManager::handleClick(const std::string& filePath, size_t lineNumber) {
    bool enabled = foldingEnabled_.read([](const bool& value) { return value; });
    if (!enabled) return;

    toggleFold(filePath, lineNumber);
}

void CodeFoldingManager::setOnFoldingChanged(std::function<void()> callback) {
    onFoldingChanged_ = std::move(callback);
}

void CodeFoldingManager::refreshFoldingRanges() {
    // Trigger a refresh of folding ranges for all open files
    for (const auto& [filePath, ranges] : foldingRanges_) {
        // In a real implementation, we would need to get cached content
        // For now, we'll leave this as a placeholder
        (void)filePath;  // Mark as intentionally unused
        (void)ranges;    // Mark as intentionally unused
    }
}

} // namespace bolt
