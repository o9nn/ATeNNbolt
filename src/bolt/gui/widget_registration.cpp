#include "bolt/gui/widget_registration.hpp"

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#endif

namespace bolt {
namespace gui {

// Static member initialization
bool WidgetRegistration::registered_ = false;
bool WidgetFrameworkInit::initialized_ = false;

// WidgetRegistration implementation
void WidgetRegistration::registerAllWidgets() {
    if (registered_) return;
    
    registerCoreWidgets();
    registerInputWidgets();
    registerDisplayWidgets();
    registerContainerWidgets();
    
    registered_ = true;
}

void WidgetRegistration::registerCoreWidgets() {
    auto& factory = WidgetFactory::getInstance();
    
    factory.registerWidget<Widget>("Widget");
    factory.registerWidget<WidgetContainer>("WidgetContainer");
}

void WidgetRegistration::registerInputWidgets() {
    auto& factory = WidgetFactory::getInstance();
    
    factory.registerWidget<Button>("Button");
    factory.registerWidget<TextInput>("TextInput");
    factory.registerWidget<TextArea>("TextArea");
    factory.registerWidget<Checkbox>("Checkbox");
    factory.registerWidget<RadioButton>("RadioButton");
    factory.registerWidget<ComboBox>("ComboBox");
    factory.registerWidget<Slider>("Slider");
}

void WidgetRegistration::registerDisplayWidgets() {
    auto& factory = WidgetFactory::getInstance();
    
    factory.registerWidget<Label>("Label");
    factory.registerWidget<ProgressBar>("ProgressBar");
}

void WidgetRegistration::registerContainerWidgets() {
    auto& factory = WidgetFactory::getInstance();
    
    factory.registerWidget<Panel>("Panel");
    factory.registerWidget<TabContainer>("TabContainer");
    factory.registerWidget<Splitter>("Splitter");
}

std::vector<std::string> WidgetRegistration::getRegisteredTypes() {
    ensureRegistered();
    return WidgetFactory::getInstance().getRegisteredTypes();
}

std::shared_ptr<Widget> WidgetRegistration::createWidget(const std::string& typeName, const std::string& id) {
    ensureRegistered();
    return WidgetFactory::getInstance().createWidget(typeName, id);
}

void WidgetRegistration::ensureRegistered() {
    if (!registered_) {
        registerAllWidgets();
    }
}

// DefaultThemes implementation
void DefaultThemes::registerAllThemes() {
    auto& themeManager = ThemeManager::getInstance();
    
    themeManager.addTheme(createDarkTheme());
    themeManager.addTheme(createLightTheme());
    themeManager.addTheme(createHighContrastTheme());
    
    // Set dark theme as default
    themeManager.setCurrentTheme("Dark");
}

std::shared_ptr<WidgetTheme> DefaultThemes::createDarkTheme() {
    auto theme = std::make_shared<WidgetTheme>("Dark");
    
#ifdef BOLT_HAVE_IMGUI
    // Base colors for dark theme
    ImVec4 backgroundColor(0.12f, 0.12f, 0.14f, 1.00f);
    ImVec4 textColor(0.90f, 0.90f, 0.90f, 1.00f);
    ImVec4 borderColor(0.25f, 0.25f, 0.27f, 0.50f);
    ImVec4 accentColor(0.26f, 0.59f, 0.98f, 1.00f);
    
    WidgetStyle baseStyle = createBaseStyle(backgroundColor, textColor, borderColor, accentColor);
    theme->setDefaultStyle(baseStyle);
    
    // Button specific styling
    WidgetStyle buttonStyle = baseStyle;
    buttonStyle.backgroundColor = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    buttonStyle.hoverColor = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    buttonStyle.activeColor = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    buttonStyle.cornerRadius = 3.0f;
    buttonStyle.padding = ImVec2(8.0f, 4.0f);
    theme->setWidgetStyle("Button", buttonStyle);
    
    // TextInput specific styling
    WidgetStyle textInputStyle = baseStyle;
    textInputStyle.backgroundColor = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    textInputStyle.borderColor = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    textInputStyle.cornerRadius = 2.0f;
    textInputStyle.padding = ImVec2(6.0f, 3.0f);
    theme->setWidgetStyle("TextInput", textInputStyle);
    theme->setWidgetStyle("TextArea", textInputStyle);
    
    // Panel specific styling
    WidgetStyle panelStyle = baseStyle;
    panelStyle.backgroundColor = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    panelStyle.borderColor = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    panelStyle.borderWidth = 1.0f;
    panelStyle.cornerRadius = 4.0f;
    panelStyle.padding = ImVec2(10.0f, 10.0f);
    theme->setWidgetStyle("Panel", panelStyle);
    
    // Label styling
    WidgetStyle labelStyle = baseStyle;
    labelStyle.padding = ImVec2(2.0f, 2.0f);
    theme->setWidgetStyle("Label", labelStyle);
    
#endif
    
    return theme;
}

std::shared_ptr<WidgetTheme> DefaultThemes::createLightTheme() {
    auto theme = std::make_shared<WidgetTheme>("Light");
    
#ifdef BOLT_HAVE_IMGUI
    // Base colors for light theme
    ImVec4 backgroundColor(0.95f, 0.95f, 0.95f, 1.00f);
    ImVec4 textColor(0.10f, 0.10f, 0.10f, 1.00f);
    ImVec4 borderColor(0.70f, 0.70f, 0.70f, 0.50f);
    ImVec4 accentColor(0.26f, 0.59f, 0.98f, 1.00f);
    
    WidgetStyle baseStyle = createBaseStyle(backgroundColor, textColor, borderColor, accentColor);
    theme->setDefaultStyle(baseStyle);
    
    // Button specific styling
    WidgetStyle buttonStyle = baseStyle;
    buttonStyle.backgroundColor = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    buttonStyle.hoverColor = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    buttonStyle.activeColor = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    buttonStyle.cornerRadius = 3.0f;
    buttonStyle.padding = ImVec2(8.0f, 4.0f);
    theme->setWidgetStyle("Button", buttonStyle);
    
    // TextInput specific styling
    WidgetStyle textInputStyle = baseStyle;
    textInputStyle.backgroundColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    textInputStyle.borderColor = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    textInputStyle.cornerRadius = 2.0f;
    textInputStyle.padding = ImVec2(6.0f, 3.0f);
    theme->setWidgetStyle("TextInput", textInputStyle);
    theme->setWidgetStyle("TextArea", textInputStyle);
    
    // Panel specific styling
    WidgetStyle panelStyle = baseStyle;
    panelStyle.backgroundColor = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    panelStyle.borderColor = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    panelStyle.borderWidth = 1.0f;
    panelStyle.cornerRadius = 4.0f;
    panelStyle.padding = ImVec2(10.0f, 10.0f);
    theme->setWidgetStyle("Panel", panelStyle);
    
    // Label styling
    WidgetStyle labelStyle = baseStyle;
    labelStyle.padding = ImVec2(2.0f, 2.0f);
    theme->setWidgetStyle("Label", labelStyle);
    
#endif
    
    return theme;
}

std::shared_ptr<WidgetTheme> DefaultThemes::createHighContrastTheme() {
    auto theme = std::make_shared<WidgetTheme>("HighContrast");
    
#ifdef BOLT_HAVE_IMGUI
    // Base colors for high contrast theme
    ImVec4 backgroundColor(0.00f, 0.00f, 0.00f, 1.00f);
    ImVec4 textColor(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 borderColor(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 accentColor(1.00f, 1.00f, 0.00f, 1.00f);
    
    WidgetStyle baseStyle = createBaseStyle(backgroundColor, textColor, borderColor, accentColor);
    theme->setDefaultStyle(baseStyle);
    
    // Button specific styling
    WidgetStyle buttonStyle = baseStyle;
    buttonStyle.backgroundColor = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    buttonStyle.hoverColor = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    buttonStyle.activeColor = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    buttonStyle.cornerRadius = 0.0f; // Sharp edges for high contrast
    buttonStyle.padding = ImVec2(10.0f, 6.0f);
    buttonStyle.borderWidth = 2.0f;
    theme->setWidgetStyle("Button", buttonStyle);
    
    // TextInput specific styling
    WidgetStyle textInputStyle = baseStyle;
    textInputStyle.backgroundColor = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    textInputStyle.borderColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    textInputStyle.cornerRadius = 0.0f;
    textInputStyle.padding = ImVec2(8.0f, 4.0f);
    textInputStyle.borderWidth = 2.0f;
    theme->setWidgetStyle("TextInput", textInputStyle);
    theme->setWidgetStyle("TextArea", textInputStyle);
    
    // Panel specific styling
    WidgetStyle panelStyle = baseStyle;
    panelStyle.backgroundColor = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    panelStyle.borderColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    panelStyle.borderWidth = 2.0f;
    panelStyle.cornerRadius = 0.0f;
    panelStyle.padding = ImVec2(15.0f, 15.0f);
    theme->setWidgetStyle("Panel", panelStyle);
    
    // Label styling
    WidgetStyle labelStyle = baseStyle;
    labelStyle.padding = ImVec2(4.0f, 4.0f);
    theme->setWidgetStyle("Label", labelStyle);
    
#endif
    
    return theme;
}

std::shared_ptr<WidgetTheme> DefaultThemes::createCustomTheme(
    const std::string& name,
    const ImVec4& backgroundColor,
    const ImVec4& textColor,
    const ImVec4& accentColor) {
    
    auto theme = std::make_shared<WidgetTheme>(name);
    
#ifdef BOLT_HAVE_IMGUI
    ImVec4 borderColor(
        (backgroundColor.x + textColor.x) * 0.5f,
        (backgroundColor.y + textColor.y) * 0.5f,
        (backgroundColor.z + textColor.z) * 0.5f,
        0.7f
    );
    
    WidgetStyle baseStyle = createBaseStyle(backgroundColor, textColor, borderColor, accentColor);
    theme->setDefaultStyle(baseStyle);
    
    // Create variations for different widget types
    WidgetStyle buttonStyle = baseStyle;
    buttonStyle.hoverColor = accentColor;
    buttonStyle.activeColor = ImVec4(
        accentColor.x * 0.8f,
        accentColor.y * 0.8f,
        accentColor.z * 0.8f,
        accentColor.w
    );
    theme->setWidgetStyle("Button", buttonStyle);
    
    WidgetStyle inputStyle = baseStyle;
    inputStyle.backgroundColor = ImVec4(
        backgroundColor.x * 0.9f,
        backgroundColor.y * 0.9f,
        backgroundColor.z * 0.9f,
        backgroundColor.w
    );
    theme->setWidgetStyle("TextInput", inputStyle);
    theme->setWidgetStyle("TextArea", inputStyle);
    
#endif
    
    return theme;
}

WidgetStyle DefaultThemes::createBaseStyle(
    const ImVec4& backgroundColor,
    const ImVec4& textColor,
    const ImVec4& borderColor,
    const ImVec4& accentColor) {
    
    WidgetStyle style;
    
#ifdef BOLT_HAVE_IMGUI
    style.backgroundColor = backgroundColor;
    style.textColor = textColor;
    style.borderColor = borderColor;
    style.hoverColor = accentColor;
    style.activeColor = ImVec4(
        accentColor.x * 0.7f,
        accentColor.y * 0.7f,
        accentColor.z * 0.7f,
        accentColor.w
    );
    
    style.borderWidth = 1.0f;
    style.cornerRadius = 3.0f;
    style.padding = ImVec2(8.0f, 4.0f);
    style.margin = ImVec2(2.0f, 2.0f);
    
    style.fontSize = 14.0f;
    style.animationDuration = 0.2f;
#endif
    
    return style;
}

// WidgetFrameworkInit implementation
void WidgetFrameworkInit::initialize() {
    if (initialized_) return;
    
    // Register all widget types
    WidgetRegistration::registerAllWidgets();
    
    // Register all default themes
    DefaultThemes::registerAllThemes();
    
    initialized_ = true;
}

void WidgetFrameworkInit::shutdown() {
    if (!initialized_) return;
    
    // Clean up theme manager
    auto& themeManager = ThemeManager::getInstance();
    for (const auto& themeName : themeManager.getThemeNames()) {
        themeManager.removeTheme(themeName);
    }
    
    initialized_ = false;
    WidgetRegistration::registered_ = false;
}

bool WidgetFrameworkInit::isInitialized() {
    return initialized_;
}

} // namespace gui
} // namespace bolt