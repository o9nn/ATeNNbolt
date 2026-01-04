
#include "bolt/editor/theme_system.hpp"
#include <map>
#include <string>
#include <vector>
#include <algorithm>

namespace bolt {

ThemeSystem& ThemeSystem::getInstance() {
    static ThemeSystem instance;
    return instance;
}

void ThemeSystem::initializeDefaults() {
    // Initialize default color scheme (fallback)
    defaultColorScheme_.background = "#1E1E1E";
    defaultColorScheme_.foreground = "#D4D4D4";
    defaultColorScheme_.selection = "#264F78";
    defaultColorScheme_.lineNumbers = "#858585";
    defaultColorScheme_.comment = "#6A9955";
    defaultColorScheme_.keyword = "#569CD6";
    defaultColorScheme_.string = "#CE9178";
    defaultColorScheme_.number = "#B5CEA8";
    defaultColorScheme_.type = "#4EC9B0";
    defaultColorScheme_.function = "#DCDCAA";
    
    // Initialize default font settings
    defaultFontSettings_.family = "Consolas, 'Courier New', monospace";
    defaultFontSettings_.size = 14;
    defaultFontSettings_.lineHeight = 1.5f;
}

void ThemeSystem::setTheme(const std::string& themeName) {
    if (themes_.find(themeName) != themes_.end()) {
        currentTheme_ = themeName;
    }
}

void ThemeSystem::addTheme(const std::string& themeName, const Theme& theme) {
    themes_[themeName] = theme;
}

ColorScheme ThemeSystem::getCurrentColors() const {
    if (themes_.find(currentTheme_) != themes_.end()) {
        return themes_.at(currentTheme_).colors;
    }
    return defaultColorScheme_;
}

FontSettings ThemeSystem::getCurrentFontSettings() const {
    if (themes_.find(currentTheme_) != themes_.end()) {
        return themes_.at(currentTheme_).fontSettings;
    }
    return defaultFontSettings_;
}

void ThemeSystem::loadDefaultThemes() {
    // Dark theme
    Theme darkTheme;
    darkTheme.colors.background = "#1E1E1E";
    darkTheme.colors.foreground = "#D4D4D4";
    darkTheme.colors.selection = "#264F78";
    darkTheme.colors.lineNumbers = "#858585";
    darkTheme.colors.comment = "#6A9955";
    darkTheme.colors.keyword = "#569CD6";
    darkTheme.colors.string = "#CE9178";
    darkTheme.colors.number = "#B5CEA8";
    darkTheme.colors.type = "#4EC9B0";
    darkTheme.colors.function = "#DCDCAA";
    darkTheme.fontSettings.family = "Consolas, 'Courier New', monospace";
    darkTheme.fontSettings.size = 14;
    darkTheme.fontSettings.lineHeight = 1.5;
    
    // Light theme
    Theme lightTheme;
    lightTheme.colors.background = "#FFFFFF";
    lightTheme.colors.foreground = "#000000";
    lightTheme.colors.selection = "#ADD6FF";
    lightTheme.colors.lineNumbers = "#237893";
    lightTheme.colors.comment = "#008000";
    lightTheme.colors.keyword = "#0000FF";
    lightTheme.colors.string = "#A31515";
    lightTheme.colors.number = "#098658";
    lightTheme.colors.type = "#267F99";
    lightTheme.colors.function = "#795E26";
    lightTheme.fontSettings.family = "Consolas, 'Courier New', monospace";
    lightTheme.fontSettings.size = 14;
    lightTheme.fontSettings.lineHeight = 1.5;

    addTheme("dark", darkTheme);
    addTheme("light", lightTheme);
    setTheme("dark"); // Set default theme
}

std::vector<std::string> ThemeSystem::getAvailableThemes() const {
    std::vector<std::string> themeNames;
    for (const auto& pair : themes_) {
        themeNames.push_back(pair.first);
    }
    std::sort(themeNames.begin(), themeNames.end());
    return themeNames;
}

std::string ThemeSystem::getCurrentThemeName() const {
    return currentTheme_;
}

bool ThemeSystem::hasTheme(const std::string& themeName) const {
    return themes_.find(themeName) != themes_.end();
}

} // namespace bolt
