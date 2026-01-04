# Theme System Documentation

The Bolt C++ Theme System provides a comprehensive solution for managing visual themes in the IDE, including color schemes and font settings.

## Overview

The Theme System is implemented as a singleton class that manages:
- Color schemes for syntax highlighting and UI elements
- Font settings (family, size, line height)
- Built-in themes (dark and light)
- Custom theme creation and management
- Theme switching and persistence

## Basic Usage

### Including the Theme System

```cpp
#include "bolt/editor/theme_system.hpp"
```

### Getting the Theme System Instance

```cpp
auto& themeSystem = bolt::ThemeSystem::getInstance();
```

### Loading Default Themes

```cpp
themeSystem.loadDefaultThemes();
```

This loads two built-in themes:
- **Dark Theme**: Dark background with light text, similar to VS Code Dark
- **Light Theme**: Light background with dark text, similar to VS Code Light

### Setting a Theme

```cpp
themeSystem.setTheme("dark");   // Switch to dark theme
themeSystem.setTheme("light");  // Switch to light theme
```

### Getting Current Theme Information

```cpp
// Get current color scheme
auto colors = themeSystem.getCurrentColors();
std::cout << "Background: " << colors.background << std::endl;
std::cout << "Foreground: " << colors.foreground << std::endl;

// Get current font settings
auto fonts = themeSystem.getCurrentFontSettings();
std::cout << "Font: " << fonts.family << ", " << fonts.size << "px" << std::endl;
```

### Creating Custom Themes

```cpp
// Create a custom theme
bolt::Theme customTheme;
customTheme.colors.background = "#272822";  // Monokai-style background
customTheme.colors.foreground = "#F8F8F2";
customTheme.colors.keyword = "#F92672";
customTheme.colors.string = "#E6DB74";
customTheme.fontSettings.family = "Fira Code";
customTheme.fontSettings.size = 16;
customTheme.fontSettings.lineHeight = 1.4f;

// Add and activate the theme
themeSystem.addTheme("monokai", customTheme);
themeSystem.setTheme("monokai");
```

## Advanced Features

### Theme Management

```cpp
// Check if a theme exists
if (themeSystem.hasTheme("monokai")) {
    themeSystem.setTheme("monokai");
}

// Get list of available themes
auto themes = themeSystem.getAvailableThemes();
for (const auto& themeName : themes) {
    std::cout << "Available theme: " << themeName << std::endl;
}

// Get current theme name
std::string currentTheme = themeSystem.getCurrentThemeName();
```

### Error Handling

The theme system includes robust error handling:
- Setting an invalid theme name will keep the current theme unchanged
- Default fallback values are provided if no theme is set
- Proper initialization ensures the system is always in a valid state

## Color Scheme Structure

The `ColorScheme` struct includes colors for:

- `background`: Main editor background color
- `foreground`: Default text color
- `selection`: Text selection highlight color
- `lineNumbers`: Line number gutter color
- `comment`: Comment text color
- `keyword`: Language keyword color (if, class, void, etc.)
- `string`: String literal color
- `number`: Numeric literal color
- `type`: Type name color (int, string, custom classes)
- `function`: Function name color

## Font Settings Structure

The `FontSettings` struct includes:

- `family`: Font family name (e.g., "Consolas, monospace")
- `size`: Font size in pixels
- `lineHeight`: Line height multiplier (e.g., 1.5 for 150% line height)

## Integration Examples

### Syntax Highlighter Integration

```cpp
void applySyntaxHighlighting(const std::string& text) {
    auto& themeSystem = bolt::ThemeSystem::getInstance();
    auto colors = themeSystem.getCurrentColors();
    
    // Apply colors based on token type
    if (isKeyword(token)) {
        setColor(colors.keyword);
    } else if (isString(token)) {
        setColor(colors.string);
    } else if (isComment(token)) {
        setColor(colors.comment);
    }
    // ... etc
}
```

### Editor UI Integration

```cpp
void updateEditorAppearance() {
    auto& themeSystem = bolt::ThemeSystem::getInstance();
    auto colors = themeSystem.getCurrentColors();
    auto fonts = themeSystem.getCurrentFontSettings();
    
    // Update editor background and text colors
    setBackgroundColor(colors.background);
    setTextColor(colors.foreground);
    setSelectionColor(colors.selection);
    
    // Update font settings
    setFont(fonts.family, fonts.size);
    setLineHeight(fonts.lineHeight);
}
```

## Default Themes

### Dark Theme
- Background: `#1E1E1E` (dark gray)
- Foreground: `#D4D4D4` (light gray)
- Keywords: `#569CD6` (blue)
- Strings: `#CE9178` (orange)
- Comments: `#6A9955` (green)

### Light Theme
- Background: `#FFFFFF` (white)
- Foreground: `#000000` (black)
- Keywords: `#0000FF` (blue)
- Strings: `#A31515` (red)
- Comments: `#008000` (green)

## Best Practices

1. **Initialize Early**: Call `loadDefaultThemes()` during application startup
2. **Check Theme Existence**: Use `hasTheme()` before setting themes programmatically
3. **Handle Theme Changes**: Listen for theme changes to update UI components
4. **Consistent Colors**: Use the theme system colors throughout the application for consistency
5. **Fallback Handling**: The system provides sensible defaults, but always check for valid themes

## Testing

The theme system includes comprehensive tests covering:
- Singleton pattern functionality
- Default theme loading and switching
- Custom theme creation and management
- Error handling for invalid themes
- Font settings management
- Theme enumeration and validation

Run the theme system tests:
```bash
./test/bolt_theme_system_tests
```

Run the interactive demo:
```bash
./demo_theme_system
```

## File Structure

- `include/bolt/editor/theme_system.hpp` - Header file with class definitions
- `src/bolt/editor/theme_system.cpp` - Implementation file
- `test/test_theme_system.cpp` - Comprehensive test suite
- `demo_theme_system.cpp` - Interactive demonstration