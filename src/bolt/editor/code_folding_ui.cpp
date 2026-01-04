#include "bolt/editor/code_folding_ui.hpp"

namespace bolt {

CodeFoldingUI::CodeFoldingUI() : visible_(true), foldingManager_(std::make_shared<CodeFoldingManager>()) {}

void CodeFoldingUI::toggleVisibility() {
    visible_ = !visible_;
}

void CodeFoldingUI::setVisibility(bool visible) {
    visible_ = visible;
}

bool CodeFoldingUI::isVisible() const {
    return visible_;
}

void CodeFoldingUI::renderFoldingIndicator(size_t lineNumber, bool isFolded) {
    if (!visible_) return;

    // Render folding indicator icon based on fold state
    std::string indicator = isFolded ? "[+]" : "[-]";
    // Implementation for actual UI rendering would go here
}

void CodeFoldingUI::handleFoldingClick(size_t lineNumber) {
    if (!visible_) return;

    // In a real implementation, this would emit an event or signal
    // emit("fold-click", lineNumber);
}

CodeFoldingUI::FoldingGutter CodeFoldingUI::createGutter() {
    return {
        true,     // isVisible
        14,       // width in pixels
        "▶",      // iconCollapsed
        "▼"       // iconExpanded
    };
}

void CodeFoldingUI::renderFoldingMarkers(const std::string& filePath) {
    auto ranges = CodeFoldingManager::getInstance().getFoldingRanges(filePath);
    for (const auto& range : ranges) {
        if (range.isFolded) {
            // Render collapsed marker
            renderMarker(range.startLine, range.endLine, true);
        } else {
            // Render expanded marker
            renderMarker(range.startLine, range.endLine, false);
        }
    }
}

void CodeFoldingUI::renderMarker(size_t startLine, size_t endLine, bool isCollapsed) {
    auto gutter = createGutter();
    if (!gutter.isVisible) return;

    // Position calculation for the marker
    int xPos = 0;
    int yPos = static_cast<int>(startLine * lineHeight);

    // Render the appropriate icon
    const std::string& icon = isCollapsed ? gutter.iconCollapsed : gutter.iconExpanded;
    drawIcon(xPos, yPos, icon);
}

void CodeFoldingUI::drawIcon(int x, int y, const std::string& icon) {
    // Implementation would connect to the actual rendering system
    // This is a placeholder for the actual drawing implementation
}

} // namespace bolt