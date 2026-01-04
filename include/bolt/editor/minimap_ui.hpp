#ifndef MINIMAP_UI_HPP
#define MINIMAP_UI_HPP

#include "minimap.hpp"
#include <string>
#include <vector>
#include <functional>

namespace bolt {

/**
 * UI integration for minimap component
 * Handles user interaction and display integration
 */
class MinimapUI {
public:
    using NavigationCallback = std::function<void(size_t line)>;
    using ClickHandler = std::function<void(size_t x, size_t y)>;

    struct UIConfig {
        bool showBorder = true;
        bool showTitle = true;
        bool showScrollIndicator = true;
        std::string title = "Minimap";
        char borderChar = '|';
        char cornerChar = '+';
        char horizontalChar = '-';
    };

    struct Position {
        size_t x = 0;
        size_t y = 0;
        size_t width = 0;
        size_t height = 0;
    };

private:
    Minimap& minimap_;
    UIConfig config_;
    Position position_;
    NavigationCallback navigationCallback_;
    bool visible_ = true;

public:
    explicit MinimapUI(Minimap& minimap);
    MinimapUI(Minimap& minimap, const UIConfig& config);

    // Configuration
    void setConfig(const UIConfig& config) { config_ = config; }
    const UIConfig& getConfig() const { return config_; }
    void setPosition(const Position& position) { position_ = position; }
    const Position& getPosition() const { return position_; }

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Callbacks
    void setNavigationCallback(const NavigationCallback& callback) { navigationCallback_ = callback; }

    // Rendering
    std::vector<std::string> render() const;
    std::string renderFrame() const;
    std::vector<std::string> renderContent() const;
    std::string renderTitle() const;
    std::string renderScrollIndicator() const;

    // User interaction
    bool handleClick(size_t x, size_t y);
    bool handleScroll(int direction, size_t steps = 1);
    bool handleKeyPress(const std::string& key);

    // Layout
    size_t getRequiredWidth() const;
    size_t getRequiredHeight() const;
    bool fitsInArea(size_t width, size_t height) const;

private:
    void navigateToLine(size_t line);
    bool isInsideMinimap(size_t x, size_t y) const;
    std::pair<size_t, size_t> translateCoordinates(size_t x, size_t y) const;
    std::string addBorder(const std::string& content) const;
};

} // namespace bolt

#endif