#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <any>
#include <optional>
#include <typeindex>

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#else
// Fallback types when ImGui is not available
struct ImVec2 {
    float x, y;
    ImVec2() : x(0.0f), y(0.0f) {}
    ImVec2(float x_, float y_) : x(x_), y(y_) {}
};

struct ImVec4 {
    float x, y, z, w;
    ImVec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    ImVec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};
#endif

namespace bolt {
namespace gui {

// Forward declarations
class Widget;
class WidgetContainer;
class WidgetFactory;
class WidgetTheme;
class WidgetEvent;

/**
 * Widget sizing and positioning types
 */
struct Size {
    float width = 0.0f;
    float height = 0.0f;
    
    Size() = default;
    Size(float w, float h) : width(w), height(h) {}
    
    bool isValid() const { return width >= 0.0f && height >= 0.0f; }
};

struct Position {
    float x = 0.0f;
    float y = 0.0f;
    
    Position() = default;
    Position(float x_pos, float y_pos) : x(x_pos), y(y_pos) {}
};

struct Rect {
    Position position;
    Size size;
    
    Rect() = default;
    Rect(const Position& pos, const Size& sz) : position(pos), size(sz) {}
    Rect(float x, float y, float w, float h) : position(x, y), size(w, h) {}
    
    bool contains(const Position& point) const {
        return point.x >= position.x && 
               point.x <= position.x + size.width &&
               point.y >= position.y && 
               point.y <= position.y + size.height;
    }
};

/**
 * Widget styling and theming
 */
struct WidgetStyle {
    // Colors
    std::optional<ImVec4> backgroundColor;
    std::optional<ImVec4> borderColor;
    std::optional<ImVec4> textColor;
    std::optional<ImVec4> hoverColor;
    std::optional<ImVec4> activeColor;
    
    // Dimensions
    std::optional<float> borderWidth;
    std::optional<float> cornerRadius;
    std::optional<ImVec2> padding;
    std::optional<ImVec2> margin;
    
    // Typography
    std::optional<float> fontSize;
    std::optional<std::string> fontFamily;
    
    // Animation
    std::optional<float> animationDuration;
    
    void merge(const WidgetStyle& other);
    void applyToImGui() const;
    void restoreImGui() const;
};

/**
 * Widget event system
 */
enum class WidgetEventType {
    Click,
    DoubleClick,
    RightClick,
    Hover,
    Leave,
    Focus,
    Blur,
    KeyPress,
    KeyRelease,
    TextChanged,
    ValueChanged,
    Resize,
    Move,
    Custom
};

class WidgetEvent {
public:
    WidgetEventType type;
    Widget* source;
    std::unordered_map<std::string, std::any> data;
    bool handled = false;
    
    WidgetEvent(WidgetEventType eventType, Widget* src) 
        : type(eventType), source(src) {}
    
    template<typename T>
    void setData(const std::string& key, const T& value) {
        data[key] = value;
    }
    
    template<typename T>
    T getData(const std::string& key, const T& defaultValue = T{}) const {
        auto it = data.find(key);
        if (it != data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
};

using WidgetEventHandler = std::function<void(const WidgetEvent&)>;

/**
 * Layout management
 */
enum class LayoutType {
    None,
    Vertical,
    Horizontal,
    Grid,
    Absolute,
    Flex
};

struct LayoutParameters {
    LayoutType type = LayoutType::None;
    float spacing = 0.0f;
    ImVec2 alignment = ImVec2(0.0f, 0.0f); // 0.0 = left/top, 1.0 = right/bottom
    bool fillAvailableSpace = false;
    
    // Grid-specific
    int columns = 1;
    int rows = 1;
    
    // Flex-specific
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
};

/**
 * Core widget base class
 */
class Widget {
public:
    Widget(const std::string& id = "");
    virtual ~Widget() = default;
    
    // Identity and hierarchy
    const std::string& getId() const { return id_; }
    void setId(const std::string& id) { id_ = id; }
    
    Widget* getParent() const { return parent_; }
    void setParent(Widget* parent) { parent_ = parent; }
    
    const std::vector<std::shared_ptr<Widget>>& getChildren() const { return children_; }
    void addChild(std::shared_ptr<Widget> child);
    void removeChild(std::shared_ptr<Widget> child);
    void removeChild(const std::string& childId);
    void clearChildren();
    
    // Visibility and state
    bool isVisible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }
    void show() { setVisible(true); }
    void hide() { setVisible(false); }
    
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    void enable() { setEnabled(true); }
    void disable() { setEnabled(false); }
    
    bool isFocused() const { return focused_; }
    void setFocused(bool focused) { focused_ = focused; }
    
    // Geometry
    const Rect& getBounds() const { return bounds_; }
    void setBounds(const Rect& bounds) { bounds_ = bounds; }
    void setPosition(const Position& pos) { bounds_.position = pos; }
    void setSize(const Size& size) { bounds_.size = size; }
    
    Size getMinimumSize() const { return minimumSize_; }
    void setMinimumSize(const Size& size) { minimumSize_ = size; }
    
    Size getMaximumSize() const { return maximumSize_; }
    void setMaximumSize(const Size& size) { maximumSize_ = size; }
    
    // Styling
    const WidgetStyle& getStyle() const { return style_; }
    void setStyle(const WidgetStyle& style) { style_ = style; }
    void mergeStyle(const WidgetStyle& style) { style_.merge(style); }
    
    // Layout
    const LayoutParameters& getLayoutParams() const { return layoutParams_; }
    void setLayoutParams(const LayoutParameters& params) { layoutParams_ = params; }
    
    // Event handling
    void addEventListener(WidgetEventType type, WidgetEventHandler handler);
    void removeEventListener(WidgetEventType type);
    bool fireEvent(const WidgetEvent& event);
    
    // Rendering and updates
    virtual void render();
    virtual void update(float deltaTime);
    virtual Size calculatePreferredSize() const;
    virtual void layout();
    
    // Custom properties
    template<typename T>
    void setProperty(const std::string& name, const T& value) {
        properties_[name] = value;
    }
    
    template<typename T>
    T getProperty(const std::string& name, const T& defaultValue = T{}) const {
        auto it = properties_.find(name);
        if (it != properties_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    bool hasProperty(const std::string& name) const {
        return properties_.find(name) != properties_.end();
    }
    
    void removeProperty(const std::string& name) {
        properties_.erase(name);
    }
    
protected:
    // Override these for custom widget behavior
    virtual void onRender() {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onLayout() {}
    virtual void onStyleChanged() {}
    virtual void onEvent(const WidgetEvent& event) {}
    
    // Helper methods for ImGui integration
    void pushImGuiStyle() const;
    void popImGuiStyle() const;
    std::string getImGuiId() const;
    
private:
    std::string id_;
    Widget* parent_;
    std::vector<std::shared_ptr<Widget>> children_;
    
    bool visible_;
    bool enabled_;
    bool focused_;
    
    Rect bounds_;
    Size minimumSize_;
    Size maximumSize_;
    
    WidgetStyle style_;
    LayoutParameters layoutParams_;
    
    std::unordered_map<WidgetEventType, std::vector<WidgetEventHandler>> eventHandlers_;
    std::unordered_map<std::string, std::any> properties_;
    
    mutable int imguiStylePushCount_;
};

/**
 * Container widget for laying out child widgets
 */
class WidgetContainer : public Widget {
public:
    WidgetContainer(const std::string& id = "", LayoutType layoutType = LayoutType::Vertical);
    
    void setLayoutType(LayoutType type);
    LayoutType getLayoutType() const { return getLayoutParams().type; }
    
    void setSpacing(float spacing);
    float getSpacing() const { return getLayoutParams().spacing; }
    
protected:
    void onRender() override;
    void onLayout() override;
    Size calculatePreferredSize() const override;
    
private:
    void layoutVertical();
    void layoutHorizontal();
    void layoutGrid();
    void layoutFlex();
};

/**
 * Widget factory for creating widgets by type
 */
class WidgetFactory {
public:
    using WidgetCreator = std::function<std::shared_ptr<Widget>(const std::string&)>;
    
    static WidgetFactory& getInstance();
    
    template<typename T>
    void registerWidget(const std::string& typeName) {
        creators_[typeName] = [](const std::string& id) -> std::shared_ptr<Widget> {
            return std::make_shared<T>(id);
        };
    }
    
    std::shared_ptr<Widget> createWidget(const std::string& typeName, const std::string& id = "");
    
    std::vector<std::string> getRegisteredTypes() const;
    bool isTypeRegistered(const std::string& typeName) const;
    
private:
    WidgetFactory() = default;
    std::unordered_map<std::string, WidgetCreator> creators_;
};

/**
 * Theme system for consistent widget styling
 */
class WidgetTheme {
public:
    WidgetTheme(const std::string& name) : name_(name) {}
    
    const std::string& getName() const { return name_; }
    
    void setDefaultStyle(const WidgetStyle& style) { defaultStyle_ = style; }
    const WidgetStyle& getDefaultStyle() const { return defaultStyle_; }
    
    void setWidgetStyle(const std::string& widgetType, const WidgetStyle& style);
    WidgetStyle getWidgetStyle(const std::string& widgetType) const;
    
    void setWidgetStyle(const std::string& widgetType, const std::string& state, const WidgetStyle& style);
    WidgetStyle getWidgetStyle(const std::string& widgetType, const std::string& state) const;
    
    void applyToWidget(Widget* widget, const std::string& widgetType = "", const std::string& state = "default") const;
    
private:
    std::string name_;
    WidgetStyle defaultStyle_;
    std::unordered_map<std::string, WidgetStyle> widgetStyles_;
    std::unordered_map<std::string, std::unordered_map<std::string, WidgetStyle>> stateStyles_;
};

/**
 * Theme manager for managing multiple themes
 */
class ThemeManager {
public:
    static ThemeManager& getInstance();
    
    void addTheme(std::shared_ptr<WidgetTheme> theme);
    void removeTheme(const std::string& themeName);
    
    std::shared_ptr<WidgetTheme> getTheme(const std::string& themeName) const;
    std::shared_ptr<WidgetTheme> getCurrentTheme() const { return currentTheme_; }
    
    void setCurrentTheme(const std::string& themeName);
    void setCurrentTheme(std::shared_ptr<WidgetTheme> theme);
    
    std::vector<std::string> getThemeNames() const;
    
    void applyCurrentTheme(Widget* widget, const std::string& widgetType = "", const std::string& state = "default") const;
    
private:
    ThemeManager() = default;
    std::unordered_map<std::string, std::shared_ptr<WidgetTheme>> themes_;
    std::shared_ptr<WidgetTheme> currentTheme_;
};

} // namespace gui
} // namespace bolt