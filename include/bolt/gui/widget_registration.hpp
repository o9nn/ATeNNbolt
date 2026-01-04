#pragma once

#include "bolt/gui/widget_framework.hpp"
#include "bolt/gui/widgets.hpp"

namespace bolt {
namespace gui {

/**
 * Widget registration utility for registering all built-in widgets
 */
class WidgetRegistration {
public:
    /**
     * Register all built-in widget types with the widget factory
     */
    static void registerAllWidgets();
    
    /**
     * Register core widgets (Widget, WidgetContainer)
     */
    static void registerCoreWidgets();
    
    /**
     * Register basic input widgets (Button, TextInput, etc.)
     */
    static void registerInputWidgets();
    
    /**
     * Register display widgets (Label, ProgressBar, etc.)
     */
    static void registerDisplayWidgets();
    
    /**
     * Register container widgets (Panel, TabContainer, Splitter)
     */
    static void registerContainerWidgets();
    
    /**
     * Get list of all registered widget types
     */
    static std::vector<std::string> getRegisteredTypes();
    
    /**
     * Create a widget by type name with automatic registration
     */
    static std::shared_ptr<Widget> createWidget(const std::string& typeName, const std::string& id = "");
    
private:
    static bool registered_;
    static void ensureRegistered();
    
    // Allow WidgetFrameworkInit to reset registration state
    friend class WidgetFrameworkInit;
};

/**
 * Default theme definitions
 */
class DefaultThemes {
public:
    /**
     * Create and register all default themes
     */
    static void registerAllThemes();
    
    /**
     * Create the default dark theme
     */
    static std::shared_ptr<WidgetTheme> createDarkTheme();
    
    /**
     * Create the default light theme  
     */
    static std::shared_ptr<WidgetTheme> createLightTheme();
    
    /**
     * Create the high contrast theme
     */
    static std::shared_ptr<WidgetTheme> createHighContrastTheme();
    
    /**
     * Create a custom theme with specified base colors
     */
    static std::shared_ptr<WidgetTheme> createCustomTheme(
        const std::string& name,
        const ImVec4& backgroundColor,
        const ImVec4& textColor,
        const ImVec4& accentColor
    );
    
private:
    static WidgetStyle createBaseStyle(
        const ImVec4& backgroundColor,
        const ImVec4& textColor,
        const ImVec4& borderColor,
        const ImVec4& accentColor
    );
};

/**
 * Utility functions for widget framework initialization
 */
class WidgetFrameworkInit {
public:
    /**
     * Initialize the complete widget framework
     * Registers all widgets and themes
     */
    static void initialize();
    
    /**
     * Shutdown the widget framework
     * Cleans up resources
     */
    static void shutdown();
    
    /**
     * Check if the framework is initialized
     */
    static bool isInitialized();
    
private:
    static bool initialized_;
};

} // namespace gui
} // namespace bolt