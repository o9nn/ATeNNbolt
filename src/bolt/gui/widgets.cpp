#include "bolt/gui/widgets.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#endif

namespace bolt {
namespace gui {

// Static member initialization
std::unordered_map<std::string, std::vector<RadioButton*>> RadioButton::radioGroups_;

// Button implementation
Button::Button(const std::string& id, const std::string& text)
    : Widget(id), text_(text), pressed_(false), hovered_(false)
{
}

void Button::onRender() {
#ifdef BOLT_HAVE_IMGUI
    std::string displayText = icon_.empty() ? text_ : icon_ + " " + text_;
    
    if (ImGui::Button(displayText.c_str(), ImVec2(getBounds().size.width, getBounds().size.height))) {
        if (onClickCallback_) {
            onClickCallback_();
        }
        
        WidgetEvent event(WidgetEventType::Click, this);
        fireEvent(event);
    }
    
    hovered_ = ImGui::IsItemHovered();
    pressed_ = ImGui::IsItemActive();
#endif
}

void Button::onEvent(const WidgetEvent& event) {
    if (event.type == WidgetEventType::Click && onClickCallback_) {
        onClickCallback_();
    }
}

Size Button::calculatePreferredSize() const {
#ifdef BOLT_HAVE_IMGUI
    std::string displayText = icon_.empty() ? text_ : icon_ + " " + text_;
    ImVec2 textSize = ImGui::CalcTextSize(displayText.c_str());
    
    // Add padding
    return Size(textSize.x + 20.0f, textSize.y + 10.0f);
#else
    return Size(100.0f, 30.0f);
#endif
}

// Label implementation
Label::Label(const std::string& id, const std::string& text)
    : Widget(id), text_(text), alignment_(Alignment::Left), wordWrap_(false)
{
}

void Label::onRender() {
#ifdef BOLT_HAVE_IMGUI
    ImVec2 availableSize(getBounds().size.width, getBounds().size.height);
    
    if (wordWrap_) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + availableSize.x);
        ImGui::TextWrapped("%s", text_.c_str());
        ImGui::PopTextWrapPos();
    } else {
        // Handle alignment
        if (alignment_ != Alignment::Left) {
            ImVec2 textSize = ImGui::CalcTextSize(text_.c_str());
            float offsetX = 0.0f;
            
            if (alignment_ == Alignment::Center) {
                offsetX = (availableSize.x - textSize.x) * 0.5f;
            } else if (alignment_ == Alignment::Right) {
                offsetX = availableSize.x - textSize.x;
            }
            
            if (offsetX > 0.0f) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            }
        }
        
        ImGui::Text("%s", text_.c_str());
    }
#endif
}

Size Label::calculatePreferredSize() const {
#ifdef BOLT_HAVE_IMGUI
    if (wordWrap_) {
        // For word wrap, height depends on available width
        return Size(200.0f, 20.0f); // Default size, will be adjusted by layout
    } else {
        ImVec2 textSize = ImGui::CalcTextSize(text_.c_str());
        return Size(textSize.x, textSize.y);
    }
#else
    return Size(100.0f, 20.0f);
#endif
}

// TextInput implementation
TextInput::TextInput(const std::string& id, const std::string& placeholder)
    : Widget(id), placeholder_(placeholder), readOnly_(false), password_(false)
{
    textBuffer_[0] = '\0';
}

void TextInput::onRender() {
#ifdef BOLT_HAVE_IMGUI
    // Copy current text to buffer if it has changed
    if (text_ != textBuffer_) {
        strncpy(textBuffer_, text_.c_str(), sizeof(textBuffer_) - 1);
        textBuffer_[sizeof(textBuffer_) - 1] = '\0';
    }
    
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
    if (readOnly_) flags |= ImGuiInputTextFlags_ReadOnly;
    if (password_) flags |= ImGuiInputTextFlags_Password;
    
    bool textChanged = false;
    if (password_) {
        textChanged = ImGui::InputText(getImGuiId().c_str(), textBuffer_, sizeof(textBuffer_), flags);
    } else {
        textChanged = ImGui::InputTextWithHint(getImGuiId().c_str(), placeholder_.c_str(), 
                                              textBuffer_, sizeof(textBuffer_), flags);
    }
    
    if (textChanged) {
        text_ = textBuffer_;
        
        if (onTextChangedCallback_) {
            onTextChangedCallback_(text_);
        }
        
        WidgetEvent event(WidgetEventType::TextChanged, this);
        event.setData("text", text_);
        fireEvent(event);
    }
    
    // Check for Enter key
    if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (onEnterPressedCallback_) {
            onEnterPressedCallback_();
        }
        
        WidgetEvent event(WidgetEventType::KeyPress, this);
        event.setData("key", "Enter");
        fireEvent(event);
    }
#endif
}

Size TextInput::calculatePreferredSize() const {
    return Size(200.0f, 25.0f);
}

// TextArea implementation
TextArea::TextArea(const std::string& id, const std::string& placeholder)
    : Widget(id), placeholder_(placeholder), readOnly_(false)
{
    ensureBufferSize(1024);
}

void TextArea::setText(const std::string& text) {
    text_ = text;
    ensureBufferSize(text_.size() + 256);
    strncpy(textBuffer_.data(), text_.c_str(), textBuffer_.size() - 1);
    textBuffer_[textBuffer_.size() - 1] = '\0';
}

void TextArea::onRender() {
#ifdef BOLT_HAVE_IMGUI
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
    if (readOnly_) flags |= ImGuiInputTextFlags_ReadOnly;
    
    ImVec2 size(getBounds().size.width, getBounds().size.height);
    
    bool textChanged = ImGui::InputTextMultiline(getImGuiId().c_str(), textBuffer_.data(), 
                                                textBuffer_.size(), size, flags);
    
    if (textChanged) {
        text_ = textBuffer_.data();
        
        if (onTextChangedCallback_) {
            onTextChangedCallback_(text_);
        }
        
        WidgetEvent event(WidgetEventType::TextChanged, this);
        event.setData("text", text_);
        fireEvent(event);
    }
#endif
}

Size TextArea::calculatePreferredSize() const {
    return Size(300.0f, 100.0f);
}

void TextArea::ensureBufferSize(size_t minSize) {
    if (textBuffer_.size() < minSize) {
        size_t oldSize = textBuffer_.size();
        textBuffer_.resize(minSize);
        
        // Initialize new memory to zero
        std::fill(textBuffer_.begin() + oldSize, textBuffer_.end(), '\0');
    }
}

// Checkbox implementation
Checkbox::Checkbox(const std::string& id, const std::string& label, bool checked)
    : Widget(id), checked_(checked), label_(label)
{
}

void Checkbox::onRender() {
#ifdef BOLT_HAVE_IMGUI
    bool oldChecked = checked_;
    ImGui::Checkbox(label_.c_str(), &checked_);
    
    if (checked_ != oldChecked) {
        if (onStateChangedCallback_) {
            onStateChangedCallback_(checked_);
        }
        
        WidgetEvent event(WidgetEventType::ValueChanged, this);
        event.setData("checked", checked_);
        fireEvent(event);
    }
#endif
}

Size Checkbox::calculatePreferredSize() const {
#ifdef BOLT_HAVE_IMGUI
    ImVec2 textSize = ImGui::CalcTextSize(label_.c_str());
    return Size(textSize.x + 30.0f, std::max(textSize.y, 20.0f)); // Add space for checkbox
#else
    return Size(100.0f, 20.0f);
#endif
}

// RadioButton implementation
RadioButton::RadioButton(const std::string& id, const std::string& label, 
                        const std::string& group, bool selected)
    : Widget(id), selected_(selected), label_(label), group_(group)
{
    if (!group_.empty()) {
        radioGroups_[group_].push_back(this);
    }
}

void RadioButton::setSelected(bool selected) {
    if (selected_ != selected) {
        selected_ = selected;
        
        if (selected_) {
            deselectOthersInGroup();
        }
        
        if (onStateChangedCallback_) {
            onStateChangedCallback_(selected_);
        }
    }
}

void RadioButton::onRender() {
#ifdef BOLT_HAVE_IMGUI
    bool oldSelected = selected_;
    ImGui::RadioButton(label_.c_str(), &selected_);
    
    if (selected_ != oldSelected && selected_) {
        deselectOthersInGroup();
        
        if (onStateChangedCallback_) {
            onStateChangedCallback_(selected_);
        }
        
        WidgetEvent event(WidgetEventType::ValueChanged, this);
        event.setData("selected", selected_);
        fireEvent(event);
    }
#endif
}

Size RadioButton::calculatePreferredSize() const {
#ifdef BOLT_HAVE_IMGUI
    ImVec2 textSize = ImGui::CalcTextSize(label_.c_str());
    return Size(textSize.x + 30.0f, std::max(textSize.y, 20.0f));
#else
    return Size(100.0f, 20.0f);
#endif
}

void RadioButton::deselectOthersInGroup() {
    if (group_.empty()) return;
    
    auto it = radioGroups_.find(group_);
    if (it != radioGroups_.end()) {
        for (RadioButton* radio : it->second) {
            if (radio != this && radio->selected_) {
                radio->selected_ = false;
                if (radio->onStateChangedCallback_) {
                    radio->onStateChangedCallback_(false);
                }
            }
        }
    }
}

// ComboBox implementation
ComboBox::ComboBox(const std::string& id)
    : Widget(id), selectedIndex_(-1)
{
}

void ComboBox::addItem(const std::string& item, const std::any& userData) {
    items_.push_back(item);
    itemData_.push_back(userData);
    
    if (selectedIndex_ == -1) {
        selectedIndex_ = 0;
    }
}

void ComboBox::removeItem(const std::string& item) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        int index = static_cast<int>(it - items_.begin());
        items_.erase(it);
        itemData_.erase(itemData_.begin() + index);
        
        if (selectedIndex_ == index) {
            selectedIndex_ = items_.empty() ? -1 : std::min(selectedIndex_, static_cast<int>(items_.size()) - 1);
        } else if (selectedIndex_ > index) {
            selectedIndex_--;
        }
    }
}

void ComboBox::clearItems() {
    items_.clear();
    itemData_.clear();
    selectedIndex_ = -1;
}

void ComboBox::setSelectedIndex(int index) {
    if (index >= -1 && index < static_cast<int>(items_.size()) && index != selectedIndex_) {
        selectedIndex_ = index;
        
        if (onSelectionChangedCallback_) {
            onSelectionChangedCallback_(selectedIndex_, getSelectedItem());
        }
    }
}

void ComboBox::setSelectedItem(const std::string& item) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        setSelectedIndex(static_cast<int>(it - items_.begin()));
    }
}

const std::string& ComboBox::getSelectedItem() const {
    static const std::string empty;
    return (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) 
           ? items_[selectedIndex_] : empty;
}

void ComboBox::onRender() {
#ifdef BOLT_HAVE_IMGUI
    const char* currentItem = (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) 
                             ? items_[selectedIndex_].c_str() : "";
    
    if (ImGui::BeginCombo(getImGuiId().c_str(), currentItem)) {
        for (int i = 0; i < static_cast<int>(items_.size()); i++) {
            bool isSelected = (selectedIndex_ == i);
            if (ImGui::Selectable(items_[i].c_str(), isSelected)) {
                setSelectedIndex(i);
                
                WidgetEvent event(WidgetEventType::ValueChanged, this);
                event.setData("selectedIndex", selectedIndex_);
                event.setData("selectedItem", getSelectedItem());
                fireEvent(event);
            }
            
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
#endif
}

Size ComboBox::calculatePreferredSize() const {
    return Size(200.0f, 25.0f);
}

// Slider implementation
Slider::Slider(const std::string& id, float minValue, float maxValue, float value)
    : Widget(id), value_(value), minValue_(minValue), maxValue_(maxValue)
{
    value_ = std::clamp(value_, minValue_, maxValue_);
}

void Slider::setValue(float value) {
    float newValue = std::clamp(value, minValue_, maxValue_);
    if (newValue != value_) {
        value_ = newValue;
        
        if (onValueChangedCallback_) {
            onValueChangedCallback_(value_);
        }
    }
}

void Slider::onRender() {
#ifdef BOLT_HAVE_IMGUI
    float oldValue = value_;
    ImGui::SliderFloat(getImGuiId().c_str(), &value_, minValue_, maxValue_);
    
    if (value_ != oldValue) {
        if (onValueChangedCallback_) {
            onValueChangedCallback_(value_);
        }
        
        WidgetEvent event(WidgetEventType::ValueChanged, this);
        event.setData("value", value_);
        fireEvent(event);
    }
#endif
}

Size Slider::calculatePreferredSize() const {
    return Size(200.0f, 25.0f);
}

// ProgressBar implementation
ProgressBar::ProgressBar(const std::string& id, float progress)
    : Widget(id), progress_(std::clamp(progress, 0.0f, 1.0f)), indeterminate_(false), animationTime_(0.0f)
{
}

void ProgressBar::setProgress(float progress) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

void ProgressBar::onRender() {
#ifdef BOLT_HAVE_IMGUI
    ImVec2 size(getBounds().size.width, getBounds().size.height);
    
    if (indeterminate_) {
        // Animated indeterminate progress
        float animatedProgress = (sin(animationTime_ * 3.0f) + 1.0f) * 0.5f;
        ImGui::ProgressBar(animatedProgress, size, text_.empty() ? nullptr : text_.c_str());
    } else {
        ImGui::ProgressBar(progress_, size, text_.empty() ? nullptr : text_.c_str());
    }
#endif
}

void ProgressBar::onUpdate(float deltaTime) {
    if (indeterminate_) {
        animationTime_ += deltaTime;
    }
}

Size ProgressBar::calculatePreferredSize() const {
    return Size(200.0f, 25.0f);
}

// Panel implementation
Panel::Panel(const std::string& id, const std::string& title)
    : WidgetContainer(id), title_(title), collapsible_(false), collapsed_(false), scrollbars_(false)
{
}

void Panel::onRender() {
#ifdef BOLT_HAVE_IMGUI
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (scrollbars_) {
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
    } else {
        flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    }
    
    bool open = !collapsible_ || !collapsed_;
    
    if (collapsible_) {
        if (ImGui::CollapsingHeader(title_.c_str(), &open)) {
            // Content will be rendered by child widgets
        }
        collapsed_ = !open;
    } else {
        if (!title_.empty()) {
            ImGui::Text("%s", title_.c_str());
            ImGui::Separator();
        }
        
        ImGui::BeginChild(getImGuiId().c_str(), 
                         ImVec2(getBounds().size.width, getBounds().size.height), 
                         true, flags);
        
        // Child widgets will render themselves
        
        ImGui::EndChild();
    }
#endif
}

Size Panel::calculatePreferredSize() const {
    Size contentSize = WidgetContainer::calculatePreferredSize();
    
    // Add padding and title space
    float titleHeight = title_.empty() ? 0.0f : 25.0f;
    return Size(contentSize.width + 20.0f, contentSize.height + titleHeight + 20.0f);
}

// TabContainer implementation
TabContainer::TabContainer(const std::string& id)
    : WidgetContainer(id), activeTab_(-1)
{
}

void TabContainer::addTab(const std::string& title, std::shared_ptr<Widget> content, 
                         const std::string& icon, bool closable) {
    TabInfo tab(title, icon, closable);
    tab.content = content;
    tabs_.push_back(tab);
    
    if (content) {
        addChild(content);
        content->setVisible(false); // Initially hide all tabs
    }
    
    if (activeTab_ == -1) {
        setActiveTab(0);
    }
}

void TabContainer::removeTab(int index) {
    if (index >= 0 && index < static_cast<int>(tabs_.size())) {
        if (tabs_[index].content) {
            removeChild(tabs_[index].content);
        }
        
        tabs_.erase(tabs_.begin() + index);
        
        if (activeTab_ == index) {
            activeTab_ = tabs_.empty() ? -1 : std::min(activeTab_, static_cast<int>(tabs_.size()) - 1);
        } else if (activeTab_ > index) {
            activeTab_--;
        }
        
        // Update visibility
        for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
            if (tabs_[i].content) {
                tabs_[i].content->setVisible(i == activeTab_);
            }
        }
    }
}

void TabContainer::removeTab(const std::string& title) {
    for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
        if (tabs_[i].title == title) {
            removeTab(i);
            break;
        }
    }
}

void TabContainer::setActiveTab(int index) {
    if (index >= 0 && index < static_cast<int>(tabs_.size()) && index != activeTab_) {
        // Hide current tab
        if (activeTab_ >= 0 && activeTab_ < static_cast<int>(tabs_.size()) && tabs_[activeTab_].content) {
            tabs_[activeTab_].content->setVisible(false);
        }
        
        activeTab_ = index;
        
        // Show new tab
        if (tabs_[activeTab_].content) {
            tabs_[activeTab_].content->setVisible(true);
        }
        
        if (onTabChangedCallback_) {
            onTabChangedCallback_(activeTab_);
        }
    }
}

void TabContainer::onRender() {
#ifdef BOLT_HAVE_IMGUI
    if (tabs_.empty()) return;
    
    // Render tab bar
    if (ImGui::BeginTabBar(getImGuiId().c_str())) {
        for (int i = 0; i < static_cast<int>(tabs_.size()); ++i) {
            const auto& tab = tabs_[i];
            
            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            bool open = true;
            
            std::string tabLabel = tab.icon.empty() ? tab.title : tab.icon + " " + tab.title;
            
            if (ImGui::BeginTabItem(tabLabel.c_str(), tab.closable ? &open : nullptr, flags)) {
                if (activeTab_ != i) {
                    setActiveTab(i);
                }
                ImGui::EndTabItem();
            }
            
            if (tab.closable && !open) {
                if (onTabClosedCallback_) {
                    onTabClosedCallback_(i);
                }
                removeTab(i);
                break; // Avoid iterator invalidation
            }
        }
        ImGui::EndTabBar();
    }
    
    // The active tab content will be rendered by the base Widget::render()
#endif
}

Size TabContainer::calculatePreferredSize() const {
    Size contentSize = WidgetContainer::calculatePreferredSize();
    
    // Add space for tab bar
    return Size(contentSize.width, contentSize.height + 30.0f);
}

// Splitter implementation
Splitter::Splitter(const std::string& id, Orientation orientation)
    : WidgetContainer(id), orientation_(orientation), splitRatio_(0.5f), splitterThickness_(4.0f)
{
}

void Splitter::setSplitRatio(float ratio) {
    splitRatio_ = std::clamp(ratio, 0.1f, 0.9f);
}

void Splitter::setFirstWidget(std::shared_ptr<Widget> widget) {
    if (firstWidget_) {
        removeChild(firstWidget_);
    }
    firstWidget_ = widget;
    if (widget) {
        addChild(widget);
    }
}

void Splitter::setSecondWidget(std::shared_ptr<Widget> widget) {
    if (secondWidget_) {
        removeChild(secondWidget_);
    }
    secondWidget_ = widget;
    if (widget) {
        addChild(widget);
    }
}

void Splitter::onRender() {
#ifdef BOLT_HAVE_IMGUI
    const Rect& bounds = getBounds();
    
    if (orientation_ == Orientation::Horizontal) {
        float firstWidth = bounds.size.width * splitRatio_;
        float secondWidth = bounds.size.width - firstWidth - splitterThickness_;
        
        // First widget
        if (firstWidget_) {
            ImGui::BeginChild((getId() + "_first").c_str(), ImVec2(firstWidth, bounds.size.height), true);
            // Widget will render itself
            ImGui::EndChild();
        }
        
        ImGui::SameLine();
        
        // Splitter
        ImGui::Button("##splitter", ImVec2(splitterThickness_, bounds.size.height));
        if (ImGui::IsItemActive()) {
            float mouseX = ImGui::GetMousePos().x - bounds.position.x;
            setSplitRatio(mouseX / bounds.size.width);
        }
        
        ImGui::SameLine();
        
        // Second widget
        if (secondWidget_) {
            ImGui::BeginChild((getId() + "_second").c_str(), ImVec2(secondWidth, bounds.size.height), true);
            // Widget will render itself
            ImGui::EndChild();
        }
    } else {
        float firstHeight = bounds.size.height * splitRatio_;
        float secondHeight = bounds.size.height - firstHeight - splitterThickness_;
        
        // First widget
        if (firstWidget_) {
            ImGui::BeginChild((getId() + "_first").c_str(), ImVec2(bounds.size.width, firstHeight), true);
            // Widget will render itself
            ImGui::EndChild();
        }
        
        // Splitter
        ImGui::Button("##splitter", ImVec2(bounds.size.width, splitterThickness_));
        if (ImGui::IsItemActive()) {
            float mouseY = ImGui::GetMousePos().y - bounds.position.y;
            setSplitRatio(mouseY / bounds.size.height);
        }
        
        // Second widget
        if (secondWidget_) {
            ImGui::BeginChild((getId() + "_second").c_str(), ImVec2(bounds.size.width, secondHeight), true);
            // Widget will render itself
            ImGui::EndChild();
        }
    }
#endif
}

void Splitter::onLayout() {
    const Rect& bounds = getBounds();
    
    if (orientation_ == Orientation::Horizontal) {
        float firstWidth = bounds.size.width * splitRatio_;
        float secondWidth = bounds.size.width - firstWidth - splitterThickness_;
        
        if (firstWidget_) {
            firstWidget_->setBounds(Rect(bounds.position.x, bounds.position.y, firstWidth, bounds.size.height));
        }
        
        if (secondWidget_) {
            float secondX = bounds.position.x + firstWidth + splitterThickness_;
            secondWidget_->setBounds(Rect(secondX, bounds.position.y, secondWidth, bounds.size.height));
        }
    } else {
        float firstHeight = bounds.size.height * splitRatio_;
        float secondHeight = bounds.size.height - firstHeight - splitterThickness_;
        
        if (firstWidget_) {
            firstWidget_->setBounds(Rect(bounds.position.x, bounds.position.y, bounds.size.width, firstHeight));
        }
        
        if (secondWidget_) {
            float secondY = bounds.position.y + firstHeight + splitterThickness_;
            secondWidget_->setBounds(Rect(bounds.position.x, secondY, bounds.size.width, secondHeight));
        }
    }
}

Size Splitter::calculatePreferredSize() const {
    Size firstSize(0.0f, 0.0f);
    Size secondSize(0.0f, 0.0f);
    
    if (firstWidget_) {
        firstSize = firstWidget_->calculatePreferredSize();
    }
    
    if (secondWidget_) {
        secondSize = secondWidget_->calculatePreferredSize();
    }
    
    if (orientation_ == Orientation::Horizontal) {
        return Size(
            firstSize.width + secondSize.width + splitterThickness_,
            std::max(firstSize.height, secondSize.height)
        );
    } else {
        return Size(
            std::max(firstSize.width, secondSize.width),
            firstSize.height + secondSize.height + splitterThickness_
        );
    }
}

} // namespace gui
} // namespace bolt