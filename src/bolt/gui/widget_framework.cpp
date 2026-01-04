#include "bolt/gui/widget_framework.hpp"

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#endif

#include <algorithm>
#include <iostream>
#include <cfloat>

namespace bolt {
namespace gui {

// WidgetStyle implementation
void WidgetStyle::merge(const WidgetStyle& other) {
    if (other.backgroundColor) backgroundColor = other.backgroundColor;
    if (other.borderColor) borderColor = other.borderColor;
    if (other.textColor) textColor = other.textColor;
    if (other.hoverColor) hoverColor = other.hoverColor;
    if (other.activeColor) activeColor = other.activeColor;
    if (other.borderWidth) borderWidth = other.borderWidth;
    if (other.cornerRadius) cornerRadius = other.cornerRadius;
    if (other.padding) padding = other.padding;
    if (other.margin) margin = other.margin;
    if (other.fontSize) fontSize = other.fontSize;
    if (other.fontFamily) fontFamily = other.fontFamily;
    if (other.animationDuration) animationDuration = other.animationDuration;
}

void WidgetStyle::applyToImGui() const {
#ifdef BOLT_HAVE_IMGUI
    if (backgroundColor) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, *backgroundColor);
    }
    if (borderColor) {
        ImGui::PushStyleColor(ImGuiCol_Border, *borderColor);
    }
    if (textColor) {
        ImGui::PushStyleColor(ImGuiCol_Text, *textColor);
    }
    if (hoverColor) {
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, *hoverColor);
    }
    if (activeColor) {
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, *activeColor);
    }
    if (borderWidth) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, *borderWidth);
    }
    if (cornerRadius) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, *cornerRadius);
    }
    if (padding) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, *padding);
    }
#endif
}

void WidgetStyle::restoreImGui() const {
#ifdef BOLT_HAVE_IMGUI
    int colorCount = 0;
    int styleCount = 0;
    
    if (backgroundColor) colorCount++;
    if (borderColor) colorCount++;
    if (textColor) colorCount++;
    if (hoverColor) colorCount++;
    if (activeColor) colorCount++;
    
    if (borderWidth) styleCount++;
    if (cornerRadius) styleCount++;
    if (padding) styleCount++;
    
    if (colorCount > 0) ImGui::PopStyleColor(colorCount);
    if (styleCount > 0) ImGui::PopStyleVar(styleCount);
#endif
}

// Widget implementation
Widget::Widget(const std::string& id)
    : id_(id.empty() ? "widget_" + std::to_string(reinterpret_cast<uintptr_t>(this)) : id)
    , parent_(nullptr)
    , visible_(true)
    , enabled_(true)
    , focused_(false)
    , minimumSize_(0.0f, 0.0f)
    , maximumSize_(FLT_MAX, FLT_MAX)
    , imguiStylePushCount_(0)
{
}

void Widget::addChild(std::shared_ptr<Widget> child) {
    if (!child) return;
    
    // Remove from previous parent
    if (child->parent_) {
        child->parent_->removeChild(child);
    }
    
    // Add to this widget
    children_.push_back(child);
    child->parent_ = this;
}

void Widget::removeChild(std::shared_ptr<Widget> child) {
    if (!child) return;
    
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

void Widget::removeChild(const std::string& childId) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [&childId](const std::shared_ptr<Widget>& child) {
            return child->getId() == childId;
        });
    
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

void Widget::clearChildren() {
    for (auto& child : children_) {
        child->parent_ = nullptr;
    }
    children_.clear();
}

void Widget::addEventListener(WidgetEventType type, WidgetEventHandler handler) {
    eventHandlers_[type].push_back(handler);
}

void Widget::removeEventListener(WidgetEventType type) {
    eventHandlers_.erase(type);
}

bool Widget::fireEvent(const WidgetEvent& event) {
    auto it = eventHandlers_.find(event.type);
    if (it != eventHandlers_.end()) {
        for (const auto& handler : it->second) {
            handler(event);
            if (event.handled) {
                return true;
            }
        }
    }
    
    // Call virtual onEvent handler
    const_cast<Widget*>(this)->onEvent(event);
    
    // Bubble event to parent if not handled
    if (!event.handled && parent_) {
        return parent_->fireEvent(event);
    }
    
    return event.handled;
}

void Widget::render() {
    if (!visible_) return;
    
    pushImGuiStyle();
    
    onRender();
    
    // Render children
    for (auto& child : children_) {
        if (child && child->isVisible()) {
            child->render();
        }
    }
    
    popImGuiStyle();
}

void Widget::update(float deltaTime) {
    if (!visible_) return;
    
    onUpdate(deltaTime);
    
    // Update children
    for (auto& child : children_) {
        if (child && child->isVisible()) {
            child->update(deltaTime);
        }
    }
}

Size Widget::calculatePreferredSize() const {
    // Default implementation: return minimum size or current size
    if (bounds_.size.isValid()) {
        return Size(
            std::max(bounds_.size.width, minimumSize_.width),
            std::max(bounds_.size.height, minimumSize_.height)
        );
    }
    return minimumSize_;
}

void Widget::layout() {
    onLayout();
    
    // Layout children
    for (auto& child : children_) {
        if (child && child->isVisible()) {
            child->layout();
        }
    }
}

void Widget::pushImGuiStyle() const {
#ifdef BOLT_HAVE_IMGUI
    imguiStylePushCount_ = 0;
    style_.applyToImGui();
#endif
}

void Widget::popImGuiStyle() const {
#ifdef BOLT_HAVE_IMGUI
    style_.restoreImGui();
    imguiStylePushCount_ = 0;
#endif
}

std::string Widget::getImGuiId() const {
    return "##" + id_;
}

// WidgetContainer implementation
WidgetContainer::WidgetContainer(const std::string& id, LayoutType layoutType)
    : Widget(id)
{
    LayoutParameters params;
    params.type = layoutType;
    params.spacing = 5.0f;
    setLayoutParams(params);
}

void WidgetContainer::setLayoutType(LayoutType type) {
    LayoutParameters params = getLayoutParams();
    params.type = type;
    setLayoutParams(params);
}

void WidgetContainer::setSpacing(float spacing) {
    LayoutParameters params = getLayoutParams();
    params.spacing = spacing;
    setLayoutParams(params);
}

void WidgetContainer::onRender() {
#ifdef BOLT_HAVE_IMGUI
    // Container-specific rendering
    ImGui::BeginChild(getImGuiId().c_str(), 
                     ImVec2(getBounds().size.width, getBounds().size.height), 
                     true);
    
    // The actual layout and child rendering is handled by the base Widget::render()
    
    ImGui::EndChild();
#endif
}

void WidgetContainer::onLayout() {
    const auto& params = getLayoutParams();
    
    switch (params.type) {
        case LayoutType::Vertical:
            layoutVertical();
            break;
        case LayoutType::Horizontal:
            layoutHorizontal();
            break;
        case LayoutType::Grid:
            layoutGrid();
            break;
        case LayoutType::Flex:
            layoutFlex();
            break;
        case LayoutType::Absolute:
            // Children maintain their absolute positions
            break;
        case LayoutType::None:
        default:
            // No automatic layout
            break;
    }
}

Size WidgetContainer::calculatePreferredSize() const {
    const auto& params = getLayoutParams();
    const auto& children = getChildren();
    
    if (children.empty()) {
        return Widget::calculatePreferredSize();
    }
    
    Size totalSize(0.0f, 0.0f);
    
    switch (params.type) {
        case LayoutType::Vertical: {
            float maxWidth = 0.0f;
            float totalHeight = 0.0f;
            
            for (const auto& child : children) {
                if (!child || !child->isVisible()) continue;
                
                Size childSize = child->calculatePreferredSize();
                maxWidth = std::max(maxWidth, childSize.width);
                totalHeight += childSize.height;
                
                if (totalHeight > 0.0f) {
                    totalHeight += params.spacing;
                }
            }
            
            totalSize = Size(maxWidth, totalHeight);
            break;
        }
        
        case LayoutType::Horizontal: {
            float totalWidth = 0.0f;
            float maxHeight = 0.0f;
            
            for (const auto& child : children) {
                if (!child || !child->isVisible()) continue;
                
                Size childSize = child->calculatePreferredSize();
                totalWidth += childSize.width;
                maxHeight = std::max(maxHeight, childSize.height);
                
                if (totalWidth > 0.0f) {
                    totalWidth += params.spacing;
                }
            }
            
            totalSize = Size(totalWidth, maxHeight);
            break;
        }
        
        case LayoutType::Grid: {
            int visibleChildren = 0;
            for (const auto& child : children) {
                if (child && child->isVisible()) visibleChildren++;
            }
            
            if (visibleChildren > 0) {
                int cols = std::max(1, params.columns);
                int rows = (visibleChildren + cols - 1) / cols;
                
                float cellWidth = 0.0f;
                float cellHeight = 0.0f;
                
                for (const auto& child : children) {
                    if (!child || !child->isVisible()) continue;
                    
                    Size childSize = child->calculatePreferredSize();
                    cellWidth = std::max(cellWidth, childSize.width);
                    cellHeight = std::max(cellHeight, childSize.height);
                }
                
                totalSize = Size(
                    cols * cellWidth + (cols - 1) * params.spacing,
                    rows * cellHeight + (rows - 1) * params.spacing
                );
            }
            break;
        }
        
        default:
            totalSize = Widget::calculatePreferredSize();
            break;
    }
    
    // Ensure we meet minimum size requirements
    return Size(
        std::max(totalSize.width, getMinimumSize().width),
        std::max(totalSize.height, getMinimumSize().height)
    );
}

void WidgetContainer::layoutVertical() {
    const auto& params = getLayoutParams();
    const auto& children = getChildren();
    const Rect& bounds = getBounds();
    
    float currentY = bounds.position.y;
    
    for (auto& child : children) {
        if (!child || !child->isVisible()) continue;
        
        Size childSize = child->calculatePreferredSize();
        
        // Respect child's preferred width or use container width
        float childWidth = params.fillAvailableSpace ? bounds.size.width : childSize.width;
        childWidth = std::min(childWidth, bounds.size.width);
        
        child->setPosition(Position(bounds.position.x, currentY));
        child->setSize(Size(childWidth, childSize.height));
        
        currentY += childSize.height + params.spacing;
    }
}

void WidgetContainer::layoutHorizontal() {
    const auto& params = getLayoutParams();
    const auto& children = getChildren();
    const Rect& bounds = getBounds();
    
    float currentX = bounds.position.x;
    
    for (auto& child : children) {
        if (!child || !child->isVisible()) continue;
        
        Size childSize = child->calculatePreferredSize();
        
        // Respect child's preferred height or use container height
        float childHeight = params.fillAvailableSpace ? bounds.size.height : childSize.height;
        childHeight = std::min(childHeight, bounds.size.height);
        
        child->setPosition(Position(currentX, bounds.position.y));
        child->setSize(Size(childSize.width, childHeight));
        
        currentX += childSize.width + params.spacing;
    }
}

void WidgetContainer::layoutGrid() {
    const auto& params = getLayoutParams();
    const auto& children = getChildren();
    const Rect& bounds = getBounds();
    
    int cols = std::max(1, params.columns);
    int visibleChildren = 0;
    for (const auto& child : children) {
        if (child && child->isVisible()) visibleChildren++;
    }
    
    if (visibleChildren == 0) return;
    
    int rows = (visibleChildren + cols - 1) / cols;
    float cellWidth = (bounds.size.width - (cols - 1) * params.spacing) / cols;
    float cellHeight = (bounds.size.height - (rows - 1) * params.spacing) / rows;
    
    int index = 0;
    for (auto& child : children) {
        if (!child || !child->isVisible()) continue;
        
        int row = index / cols;
        int col = index % cols;
        
        float x = bounds.position.x + col * (cellWidth + params.spacing);
        float y = bounds.position.y + row * (cellHeight + params.spacing);
        
        child->setPosition(Position(x, y));
        child->setSize(Size(cellWidth, cellHeight));
        
        index++;
    }
}

void WidgetContainer::layoutFlex() {
    // Simplified flex layout implementation
    const auto& params = getLayoutParams();
    const auto& children = getChildren();
    const Rect& bounds = getBounds();
    
    // For now, implement as horizontal layout with flex grow
    float totalFlexGrow = 0.0f;
    float totalFixedWidth = 0.0f;
    
    for (const auto& child : children) {
        if (!child || !child->isVisible()) continue;
        
        float flexGrow = child->getProperty<float>("flexGrow", 0.0f);
        if (flexGrow > 0.0f) {
            totalFlexGrow += flexGrow;
        } else {
            totalFixedWidth += child->calculatePreferredSize().width;
        }
    }
    
    float availableSpace = bounds.size.width - totalFixedWidth - (children.size() - 1) * params.spacing;
    float currentX = bounds.position.x;
    
    for (auto& child : children) {
        if (!child || !child->isVisible()) continue;
        
        float flexGrow = child->getProperty<float>("flexGrow", 0.0f);
        Size childSize = child->calculatePreferredSize();
        
        float childWidth;
        if (flexGrow > 0.0f && totalFlexGrow > 0.0f) {
            childWidth = (availableSpace * flexGrow) / totalFlexGrow;
        } else {
            childWidth = childSize.width;
        }
        
        child->setPosition(Position(currentX, bounds.position.y));
        child->setSize(Size(childWidth, bounds.size.height));
        
        currentX += childWidth + params.spacing;
    }
}

// WidgetFactory implementation
WidgetFactory& WidgetFactory::getInstance() {
    static WidgetFactory instance;
    return instance;
}

std::shared_ptr<Widget> WidgetFactory::createWidget(const std::string& typeName, const std::string& id) {
    auto it = creators_.find(typeName);
    if (it != creators_.end()) {
        return it->second(id);
    }
    
    std::cerr << "Warning: Unknown widget type '" << typeName << "'" << std::endl;
    return nullptr;
}

std::vector<std::string> WidgetFactory::getRegisteredTypes() const {
    std::vector<std::string> types;
    types.reserve(creators_.size());
    
    for (const auto& pair : creators_) {
        types.push_back(pair.first);
    }
    
    return types;
}

bool WidgetFactory::isTypeRegistered(const std::string& typeName) const {
    return creators_.find(typeName) != creators_.end();
}

// WidgetTheme implementation
void WidgetTheme::setWidgetStyle(const std::string& widgetType, const WidgetStyle& style) {
    widgetStyles_[widgetType] = style;
}

WidgetStyle WidgetTheme::getWidgetStyle(const std::string& widgetType) const {
    auto it = widgetStyles_.find(widgetType);
    if (it != widgetStyles_.end()) {
        WidgetStyle combinedStyle = defaultStyle_;
        combinedStyle.merge(it->second);
        return combinedStyle;
    }
    return defaultStyle_;
}

void WidgetTheme::setWidgetStyle(const std::string& widgetType, const std::string& state, const WidgetStyle& style) {
    stateStyles_[widgetType][state] = style;
}

WidgetStyle WidgetTheme::getWidgetStyle(const std::string& widgetType, const std::string& state) const {
    WidgetStyle baseStyle = getWidgetStyle(widgetType);
    
    auto typeIt = stateStyles_.find(widgetType);
    if (typeIt != stateStyles_.end()) {
        auto stateIt = typeIt->second.find(state);
        if (stateIt != typeIt->second.end()) {
            baseStyle.merge(stateIt->second);
        }
    }
    
    return baseStyle;
}

void WidgetTheme::applyToWidget(Widget* widget, const std::string& widgetType, const std::string& state) const {
    if (!widget) return;
    
    WidgetStyle style = getWidgetStyle(widgetType, state);
    widget->setStyle(style);
}

// ThemeManager implementation
ThemeManager& ThemeManager::getInstance() {
    static ThemeManager instance;
    return instance;
}

void ThemeManager::addTheme(std::shared_ptr<WidgetTheme> theme) {
    if (theme) {
        themes_[theme->getName()] = theme;
        
        // Set as current theme if it's the first one
        if (!currentTheme_) {
            currentTheme_ = theme;
        }
    }
}

void ThemeManager::removeTheme(const std::string& themeName) {
    auto it = themes_.find(themeName);
    if (it != themes_.end()) {
        if (currentTheme_ == it->second) {
            // Set to first available theme or nullptr
            currentTheme_ = themes_.empty() ? nullptr : themes_.begin()->second;
        }
        themes_.erase(it);
    }
}

std::shared_ptr<WidgetTheme> ThemeManager::getTheme(const std::string& themeName) const {
    auto it = themes_.find(themeName);
    return (it != themes_.end()) ? it->second : nullptr;
}

void ThemeManager::setCurrentTheme(const std::string& themeName) {
    auto theme = getTheme(themeName);
    if (theme) {
        currentTheme_ = theme;
    }
}

void ThemeManager::setCurrentTheme(std::shared_ptr<WidgetTheme> theme) {
    if (theme) {
        currentTheme_ = theme;
        // Ensure theme is registered
        themes_[theme->getName()] = theme;
    }
}

std::vector<std::string> ThemeManager::getThemeNames() const {
    std::vector<std::string> names;
    names.reserve(themes_.size());
    
    for (const auto& pair : themes_) {
        names.push_back(pair.first);
    }
    
    return names;
}

void ThemeManager::applyCurrentTheme(Widget* widget, const std::string& widgetType, const std::string& state) const {
    if (currentTheme_) {
        currentTheme_->applyToWidget(widget, widgetType, state);
    }
}

} // namespace gui
} // namespace bolt