#ifndef BOLT_THEME_SYSTEM_HPP
#define BOLT_THEME_SYSTEM_HPP

#include <map>
#include <string>
#include <vector>

namespace bolt {

struct ColorScheme {
    std::string background;
    std::string foreground;
    std::string selection;
    std::string lineNumbers;
    std::string comment;
    std::string keyword;
    std::string string;
    std::string number;
    std::string type;
    std::string function;
};

struct FontSettings {
    std::string family;
    int size;
    float lineHeight;
};

struct Theme {
    ColorScheme colors;
    FontSettings fontSettings;
};

class ThemeSystem {
public:
    static ThemeSystem& getInstance();

    void setTheme(const std::string& themeName);
    void addTheme(const std::string& themeName, const Theme& theme);
    ColorScheme getCurrentColors() const;
    FontSettings getCurrentFontSettings() const;
    void loadDefaultThemes();
    
    // Additional functionality
    std::vector<std::string> getAvailableThemes() const;
    std::string getCurrentThemeName() const;
    bool hasTheme(const std::string& themeName) const;

private:
    ThemeSystem() {
        initializeDefaults();
    }
    
    void initializeDefaults();
    std::map<std::string, Theme> themes_;
    std::string currentTheme_;
    ColorScheme defaultColorScheme_;
    FontSettings defaultFontSettings_;
};

} // namespace bolt

#endif // BOLT_THEME_SYSTEM_HPP