#include "bolt/editor/minimap_ui.hpp"
#include <algorithm>
#include <sstream>

namespace bolt {

MinimapUI::MinimapUI(Minimap& minimap)
    : MinimapUI(minimap, UIConfig{}) {
}

MinimapUI::MinimapUI(Minimap& minimap, const UIConfig& config)
    : minimap_(minimap), config_(config) {
}

std::vector<std::string> MinimapUI::render() const {
    if (!visible_) {
        return {};
    }
    
    std::vector<std::string> result;
    
    // Add title if enabled
    if (config_.showTitle) {
        result.push_back(renderTitle());
    }
    
    // Get minimap content
    auto content = renderContent();
    result.insert(result.end(), content.begin(), content.end());
    
    // Add scroll indicator if enabled
    if (config_.showScrollIndicator) {
        std::string indicator = renderScrollIndicator();
        if (!indicator.empty()) {
            result.push_back(indicator);
        }
    }
    
    return result;
}

std::string MinimapUI::renderFrame() const {
    if (!config_.showBorder) {
        return "";
    }
    
    std::string frame;
    size_t width = minimap_.getConfig().width;
    
    // Top border
    frame += config_.cornerChar;
    frame += std::string(width, config_.horizontalChar);
    frame += config_.cornerChar;
    frame += '\n';
    
    // Bottom border (will be added by caller if needed)
    return frame;
}

std::vector<std::string> MinimapUI::renderContent() const {
    auto minimapLines = minimap_.render();
    
    if (!config_.showBorder) {
        return minimapLines;
    }
    
    // Add borders to each line
    std::vector<std::string> borderedLines;
    for (const auto& line : minimapLines) {
        std::string bordered = config_.borderChar + line + config_.borderChar;
        borderedLines.push_back(bordered);
    }
    
    // Add top border
    size_t width = minimap_.getConfig().width;
    std::string topBorder = config_.cornerChar + std::string(width, config_.horizontalChar) + config_.cornerChar;
    borderedLines.insert(borderedLines.begin(), topBorder);
    
    // Add bottom border
    std::string bottomBorder = config_.cornerChar + std::string(width, config_.horizontalChar) + config_.cornerChar;
    borderedLines.push_back(bottomBorder);
    
    return borderedLines;
}

std::string MinimapUI::renderTitle() const {
    if (!config_.showTitle) {
        return "";
    }
    
    size_t width = minimap_.getConfig().width;
    std::string title = config_.title;
    
    // Truncate title if too long
    if (title.length() > width) {
        title = title.substr(0, width - 3) + "...";
    }
    
    // Center the title
    size_t padding = (width - title.length()) / 2;
    std::string centeredTitle = std::string(padding, ' ') + title;
    
    // Pad to full width
    if (centeredTitle.length() < width) {
        centeredTitle += std::string(width - centeredTitle.length(), ' ');
    }
    
    if (config_.showBorder) {
        return config_.borderChar + centeredTitle + config_.borderChar;
    }
    
    return centeredTitle;
}

std::string MinimapUI::renderScrollIndicator() const {
    if (!config_.showScrollIndicator || !minimap_.hasDocument()) {
        return "";
    }
    
    auto viewport = minimap_.getViewport();
    size_t totalLines = minimap_.getTotalLines();
    
    if (totalLines == 0) {
        return "";
    }
    
    std::ostringstream oss;
    oss << "Line " << (viewport.currentLine + 1) << "/" << totalLines;
    
    double scrollPercent = minimap_.getScrollPercentage();
    oss << " (" << static_cast<int>(scrollPercent * 100) << "%)";
    
    std::string indicator = oss.str();
    size_t width = minimap_.getConfig().width;
    
    // Truncate if too long
    if (indicator.length() > width) {
        indicator = indicator.substr(0, width);
    }
    
    // Pad to width
    if (indicator.length() < width) {
        indicator += std::string(width - indicator.length(), ' ');
    }
    
    if (config_.showBorder) {
        return config_.borderChar + indicator + config_.borderChar;
    }
    
    return indicator;
}

bool MinimapUI::handleClick(size_t x, size_t y) {
    if (!visible_ || !isInsideMinimap(x, y)) {
        return false;
    }
    
    auto [localX, localY] = translateCoordinates(x, y);
    size_t targetLine = minimap_.getLineFromPosition(localX, localY);
    
    navigateToLine(targetLine);
    return true;
}

bool MinimapUI::handleScroll(int direction, size_t steps) {
    if (!visible_ || !minimap_.hasDocument()) {
        return false;
    }
    
    auto viewport = minimap_.getViewport();
    size_t currentLine = viewport.currentLine;
    
    if (direction > 0) {
        // Scroll down
        size_t newLine = std::min(currentLine + steps, minimap_.getTotalLines() - 1);
        navigateToLine(newLine);
    } else if (direction < 0) {
        // Scroll up
        size_t newLine = (currentLine >= steps) ? currentLine - steps : 0;
        navigateToLine(newLine);
    }
    
    return true;
}

bool MinimapUI::handleKeyPress(const std::string& key) {
    if (!visible_) {
        return false;
    }
    
    auto viewport = minimap_.getViewport();
    size_t currentLine = viewport.currentLine;
    size_t totalLines = minimap_.getTotalLines();
    
    if (key == "up" || key == "k") {
        if (currentLine > 0) {
            navigateToLine(currentLine - 1);
        }
        return true;
    } else if (key == "down" || key == "j") {
        if (currentLine < totalLines - 1) {
            navigateToLine(currentLine + 1);
        }
        return true;
    } else if (key == "home" || key == "g") {
        navigateToLine(0);
        return true;
    } else if (key == "end" || key == "G") {
        navigateToLine(totalLines - 1);
        return true;
    } else if (key == "pageup") {
        size_t jump = std::min(size_t{10}, currentLine);
        navigateToLine(currentLine - jump);
        return true;
    } else if (key == "pagedown") {
        size_t jump = std::min(size_t{10}, totalLines - currentLine - 1);
        navigateToLine(currentLine + jump);
        return true;
    }
    
    return false;
}

size_t MinimapUI::getRequiredWidth() const {
    size_t width = minimap_.getConfig().width;
    
    if (config_.showBorder) {
        width += 2; // Left and right borders
    }
    
    return width;
}

size_t MinimapUI::getRequiredHeight() const {
    size_t height = minimap_.getConfig().height;
    
    if (config_.showTitle) {
        height += 1;
    }
    
    if (config_.showBorder) {
        height += 2; // Top and bottom borders
    }
    
    if (config_.showScrollIndicator) {
        height += 1;
    }
    
    return height;
}

bool MinimapUI::fitsInArea(size_t width, size_t height) const {
    return getRequiredWidth() <= width && getRequiredHeight() <= height;
}

void MinimapUI::navigateToLine(size_t line) {
    if (navigationCallback_) {
        navigationCallback_(line);
    }
    
    minimap_.scrollToLine(line);
}

bool MinimapUI::isInsideMinimap(size_t x, size_t y) const {
    return x >= position_.x && x < position_.x + position_.width &&
           y >= position_.y && y < position_.y + position_.height;
}

std::pair<size_t, size_t> MinimapUI::translateCoordinates(size_t x, size_t y) const {
    size_t localX = (x >= position_.x) ? x - position_.x : 0;
    size_t localY = (y >= position_.y) ? y - position_.y : 0;
    
    // Adjust for borders and title
    if (config_.showBorder && localX > 0) {
        localX -= 1;
    }
    
    if (config_.showTitle && localY > 0) {
        localY -= 1;
    }
    
    if (config_.showBorder && localY > 0) {
        localY -= 1;
    }
    
    return {localX, localY};
}

std::string MinimapUI::addBorder(const std::string& content) const {
    if (!config_.showBorder) {
        return content;
    }
    
    return config_.borderChar + content + config_.borderChar;
}

} // namespace bolt