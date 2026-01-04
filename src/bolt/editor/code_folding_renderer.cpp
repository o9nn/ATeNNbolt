
#include "bolt/editor/code_folding.hpp"
#include <memory>
#include <string>
#include <vector>

namespace bolt {

class CodeFoldingRenderer {
private:
    std::shared_ptr<CodeFoldingManager> foldingManager_;
    std::shared_ptr<CodeFoldingUI> foldingUI_;
    bool showFoldingMarkers_{true};

public:
    CodeFoldingRenderer() {
        foldingManager_ = std::make_shared<CodeFoldingManager>();
        foldingUI_ = std::make_shared<CodeFoldingUI>();
    }

    void render(const std::string& filePath, size_t viewportStartLine, size_t viewportEndLine) {
        if (!showFoldingMarkers_) return;

        auto ranges = foldingManager_->getFoldingRanges(filePath);
        for (const auto& range : ranges) {
            // Only render markers that are within the viewport
            if (range.startLine >= viewportStartLine && range.startLine <= viewportEndLine) {
                renderFoldingMarker(range);
            }
        }
    }

    void handleClick(const std::string& filePath, size_t line) {
        foldingManager_->handleClick(filePath, line);
        // Trigger re-render after state change
        invalidateView();
    }

    void setShowFoldingMarkers(bool show) {
        showFoldingMarkers_ = show;
        invalidateView();
    }

private:
    void renderFoldingMarker(const FoldRange& range) {
        auto gutter = foldingUI_->createGutter();
        // Calculate marker position based on line number
        int xPos = 0;
        int yPos = range.startLine * lineHeight;
        
        // Render appropriate icon based on fold state
        const std::string& icon = range.isFolded ? gutter.iconCollapsed : gutter.iconExpanded;
        drawIcon(xPos, yPos, icon);
    }

    void drawIcon(int x, int y, const std::string& icon) {
        // Implementation would connect to actual rendering system
    }

    void invalidateView() {
        // Signal the editor to refresh the view
    }

    static constexpr int lineHeight = 20; // Default line height in pixels
};

} // namespace bolt
