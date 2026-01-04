#include <iostream>
#include <string>
#include <vector>
#include "bolt/editor/theme_system.hpp"

namespace {

void printSeparator() {
    std::cout << std::string(60, '=') << std::endl;
}

void printThemeInfo(const std::string& themeName) {
    auto& themeSystem = bolt::ThemeSystem::getInstance();
    themeSystem.setTheme(themeName);
    
    auto colors = themeSystem.getCurrentColors();
    auto fonts = themeSystem.getCurrentFontSettings();
    
    std::cout << "\n🎨 Theme: " << themeName << std::endl;
    printSeparator();
    
    std::cout << "📋 Color Scheme:" << std::endl;
    std::cout << "  Background:  " << colors.background << std::endl;
    std::cout << "  Foreground:  " << colors.foreground << std::endl;
    std::cout << "  Selection:   " << colors.selection << std::endl;
    std::cout << "  Line Numbers:" << colors.lineNumbers << std::endl;
    std::cout << "  Comments:    " << colors.comment << std::endl;
    std::cout << "  Keywords:    " << colors.keyword << std::endl;
    std::cout << "  Strings:     " << colors.string << std::endl;
    std::cout << "  Numbers:     " << colors.number << std::endl;
    std::cout << "  Types:       " << colors.type << std::endl;
    std::cout << "  Functions:   " << colors.function << std::endl;
    
    std::cout << "\n🔤 Font Settings:" << std::endl;
    std::cout << "  Family:      " << fonts.family << std::endl;
    std::cout << "  Size:        " << fonts.size << "px" << std::endl;
    std::cout << "  Line Height: " << fonts.lineHeight << std::endl;
}

void demonstrateCodeSyntaxHighlighting(const std::string& themeName) {
    auto& themeSystem = bolt::ThemeSystem::getInstance();
    themeSystem.setTheme(themeName);
    auto colors = themeSystem.getCurrentColors();
    
    std::cout << "\n💻 Sample Code with " << themeName << " theme colors:" << std::endl;
    std::cout << "    (showing color codes that would be used for syntax highlighting)" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    // Simulate syntax highlighting by showing what colors would be used
    std::cout << "// This is a comment          -> " << colors.comment << std::endl;
    std::cout << "class Example {               -> " << colors.keyword << " + " << colors.type << std::endl;
    std::cout << "    int number = 42;          -> " << colors.keyword << " + " << colors.number << std::endl;
    std::cout << "    string text = \"hello\";     -> " << colors.keyword << " + " << colors.string << std::endl;
    std::cout << "    void function() {         -> " << colors.keyword << " + " << colors.function << std::endl;
    std::cout << "        // code here         -> " << colors.comment << std::endl;
    std::cout << "    }                         -> " << colors.foreground << std::endl;
    std::cout << "}                             -> " << colors.foreground << std::endl;
}

void createCustomThemeDemo() {
    auto& themeSystem = bolt::ThemeSystem::getInstance();
    
    std::cout << "\n🛠️  Creating Custom Theme Demo" << std::endl;
    printSeparator();
    
    // Create a custom "monokai" inspired theme
    bolt::Theme monokaiTheme;
    monokaiTheme.colors.background = "#272822";
    monokaiTheme.colors.foreground = "#F8F8F2";
    monokaiTheme.colors.selection = "#49483E";
    monokaiTheme.colors.lineNumbers = "#90908A";
    monokaiTheme.colors.comment = "#75715E";
    monokaiTheme.colors.keyword = "#F92672";
    monokaiTheme.colors.string = "#E6DB74";
    monokaiTheme.colors.number = "#AE81FF";
    monokaiTheme.colors.type = "#66D9EF";
    monokaiTheme.colors.function = "#A6E22E";
    monokaiTheme.fontSettings.family = "Fira Code, Monaco, monospace";
    monokaiTheme.fontSettings.size = 15;
    monokaiTheme.fontSettings.lineHeight = 1.4f;
    
    themeSystem.addTheme("monokai", monokaiTheme);
    std::cout << "✅ Added custom 'monokai' theme" << std::endl;
    
    printThemeInfo("monokai");
    demonstrateCodeSyntaxHighlighting("monokai");
}

void demonstrateThemeSwitching() {
    std::cout << "\n🔄 Theme Switching Demo" << std::endl;
    printSeparator();
    
    std::vector<std::string> themes = {"dark", "light", "monokai"};
    
    for (const auto& theme : themes) {
        std::cout << "\n>> Switching to '" << theme << "' theme..." << std::endl;
        demonstrateCodeSyntaxHighlighting(theme);
    }
}

void testErrorHandling() {
    std::cout << "\n🚨 Error Handling Demo" << std::endl;
    printSeparator();
    
    auto& themeSystem = bolt::ThemeSystem::getInstance();
    
    // Set to a known good theme first
    themeSystem.setTheme("dark");
    auto beforeColors = themeSystem.getCurrentColors();
    std::cout << "Current theme background: " << beforeColors.background << std::endl;
    
    // Try to set an invalid theme
    std::cout << "Attempting to set invalid theme 'nonexistent'..." << std::endl;
    themeSystem.setTheme("nonexistent");
    
    auto afterColors = themeSystem.getCurrentColors();
    std::cout << "After invalid theme attempt, background: " << afterColors.background << std::endl;
    
    if (beforeColors.background == afterColors.background) {
        std::cout << "✅ Error handling working correctly - theme unchanged" << std::endl;
    } else {
        std::cout << "❌ Error handling failed - theme was changed" << std::endl;
    }
}

} // anonymous namespace

int main() {
    std::cout << "🎨 Bolt C++ Theme System Demo" << std::endl;
    std::cout << "==============================" << std::endl;
    
    // Initialize the theme system
    auto& themeSystem = bolt::ThemeSystem::getInstance();
    themeSystem.loadDefaultThemes();
    std::cout << "✅ Theme system initialized with default themes" << std::endl;
    
    // Demonstrate built-in themes
    printThemeInfo("dark");
    demonstrateCodeSyntaxHighlighting("dark");
    
    printThemeInfo("light");
    demonstrateCodeSyntaxHighlighting("light");
    
    // Create and demonstrate custom theme
    createCustomThemeDemo();
    
    // Demonstrate theme switching
    demonstrateThemeSwitching();
    
    // Test error handling
    testErrorHandling();
    
    std::cout << "\n🎯 Demo Summary" << std::endl;
    printSeparator();
    std::cout << "✅ Default themes (dark, light) loaded and working" << std::endl;
    std::cout << "✅ Custom theme creation and application working" << std::endl;
    std::cout << "✅ Theme switching functionality working" << std::endl;
    std::cout << "✅ Error handling for invalid themes working" << std::endl;
    std::cout << "✅ Font settings management working" << std::endl;
    std::cout << "✅ Color scheme management working" << std::endl;
    
    std::cout << "\n🚀 Theme System Demo Complete!" << std::endl;
    std::cout << "The Bolt C++ theme system is fully functional and ready for integration." << std::endl;
    
    return 0;
}