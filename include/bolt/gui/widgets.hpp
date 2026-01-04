#pragma once

#include "bolt/gui/widget_framework.hpp"
#include <functional>

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#endif

namespace bolt {
namespace gui {

/**
 * Button widget - clickable button with text/icon
 */
class Button : public Widget {
public:
    explicit Button(const std::string& id = "", const std::string& text = "Button");
    
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) { text_ = text; }
    
    const std::string& getIcon() const { return icon_; }
    void setIcon(const std::string& icon) { icon_ = icon; }
    
    void setOnClick(std::function<void()> callback) { onClickCallback_ = callback; }
    
    bool isPressed() const { return pressed_; }
    bool isHovered() const { return hovered_; }
    
protected:
    void onRender() override;
    void onEvent(const WidgetEvent& event) override;
    Size calculatePreferredSize() const override;
    
private:
    std::string text_;
    std::string icon_;
    bool pressed_;
    bool hovered_;
    std::function<void()> onClickCallback_;
};

/**
 * Label widget - non-interactive text display
 */
class Label : public Widget {
public:
    explicit Label(const std::string& id = "", const std::string& text = "");
    
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) { text_ = text; }
    
    enum class Alignment {
        Left,
        Center,
        Right
    };
    
    Alignment getAlignment() const { return alignment_; }
    void setAlignment(Alignment alignment) { alignment_ = alignment; }
    
    bool getWordWrap() const { return wordWrap_; }
    void setWordWrap(bool wrap) { wordWrap_ = wrap; }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    std::string text_;
    Alignment alignment_;
    bool wordWrap_;
};

/**
 * TextInput widget - single-line text input field
 */
class TextInput : public Widget {
public:
    explicit TextInput(const std::string& id = "", const std::string& placeholder = "");
    
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) { text_ = text; }
    
    const std::string& getPlaceholder() const { return placeholder_; }
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    
    bool isReadOnly() const { return readOnly_; }
    void setReadOnly(bool readOnly) { readOnly_ = readOnly; }
    
    bool isPassword() const { return password_; }
    void setPassword(bool password) { password_ = password; }
    
    void setOnTextChanged(std::function<void(const std::string&)> callback) { 
        onTextChangedCallback_ = callback; 
    }
    
    void setOnEnterPressed(std::function<void()> callback) { 
        onEnterPressedCallback_ = callback; 
    }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    std::string text_;
    std::string placeholder_;
    bool readOnly_;
    bool password_;
    char textBuffer_[1024];
    std::function<void(const std::string&)> onTextChangedCallback_;
    std::function<void()> onEnterPressedCallback_;
};

/**
 * TextArea widget - multi-line text input field
 */
class TextArea : public Widget {
public:
    explicit TextArea(const std::string& id = "", const std::string& placeholder = "");
    
    const std::string& getText() const { return text_; }
    void setText(const std::string& text);
    
    const std::string& getPlaceholder() const { return placeholder_; }
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    
    bool isReadOnly() const { return readOnly_; }
    void setReadOnly(bool readOnly) { readOnly_ = readOnly; }
    
    void setOnTextChanged(std::function<void(const std::string&)> callback) { 
        onTextChangedCallback_ = callback; 
    }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    std::string text_;
    std::string placeholder_;
    bool readOnly_;
    std::vector<char> textBuffer_;
    std::function<void(const std::string&)> onTextChangedCallback_;
    
    void ensureBufferSize(size_t minSize);
};

/**
 * Checkbox widget - boolean toggle with label
 */
class Checkbox : public Widget {
public:
    explicit Checkbox(const std::string& id = "", const std::string& label = "", bool checked = false);
    
    bool isChecked() const { return checked_; }
    void setChecked(bool checked) { checked_ = checked; }
    
    const std::string& getLabel() const { return label_; }
    void setLabel(const std::string& label) { label_ = label; }
    
    void setOnStateChanged(std::function<void(bool)> callback) { 
        onStateChangedCallback_ = callback; 
    }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    bool checked_;
    std::string label_;
    std::function<void(bool)> onStateChangedCallback_;
};

/**
 * RadioButton widget - mutually exclusive selection
 */
class RadioButton : public Widget {
public:
    explicit RadioButton(const std::string& id = "", const std::string& label = "", 
                        const std::string& group = "", bool selected = false);
    
    bool isSelected() const { return selected_; }
    void setSelected(bool selected);
    
    const std::string& getLabel() const { return label_; }
    void setLabel(const std::string& label) { label_ = label; }
    
    const std::string& getGroup() const { return group_; }
    void setGroup(const std::string& group) { group_ = group; }
    
    void setOnStateChanged(std::function<void(bool)> callback) { 
        onStateChangedCallback_ = callback; 
    }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    bool selected_;
    std::string label_;
    std::string group_;
    std::function<void(bool)> onStateChangedCallback_;
    
    void deselectOthersInGroup();
    static std::unordered_map<std::string, std::vector<RadioButton*>> radioGroups_;
};

/**
 * ComboBox widget - dropdown selection
 */
class ComboBox : public Widget {
public:
    explicit ComboBox(const std::string& id = "");
    
    void addItem(const std::string& item, const std::any& userData = {});
    void removeItem(const std::string& item);
    void clearItems();
    
    int getSelectedIndex() const { return selectedIndex_; }
    void setSelectedIndex(int index);
    
    const std::string& getSelectedItem() const;
    void setSelectedItem(const std::string& item);
    
    const std::vector<std::string>& getItems() const { return items_; }
    
    void setOnSelectionChanged(std::function<void(int, const std::string&)> callback) { 
        onSelectionChangedCallback_ = callback; 
    }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    std::vector<std::string> items_;
    std::vector<std::any> itemData_;
    int selectedIndex_;
    std::function<void(int, const std::string&)> onSelectionChangedCallback_;
};

/**
 * Slider widget - numeric value selection with draggable handle
 */
class Slider : public Widget {
public:
    explicit Slider(const std::string& id = "", float minValue = 0.0f, float maxValue = 100.0f, float value = 0.0f);
    
    float getValue() const { return value_; }
    void setValue(float value);
    
    float getMinValue() const { return minValue_; }
    void setMinValue(float minValue) { minValue_ = minValue; }
    
    float getMaxValue() const { return maxValue_; }
    void setMaxValue(float maxValue) { maxValue_ = maxValue; }
    
    void setOnValueChanged(std::function<void(float)> callback) { 
        onValueChangedCallback_ = callback; 
    }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    float value_;
    float minValue_;
    float maxValue_;
    std::function<void(float)> onValueChangedCallback_;
};

/**
 * ProgressBar widget - shows progress of an operation
 */
class ProgressBar : public Widget {
public:
    explicit ProgressBar(const std::string& id = "", float progress = 0.0f);
    
    float getProgress() const { return progress_; }
    void setProgress(float progress); // 0.0 to 1.0
    
    const std::string& getText() const { return text_; }
    void setText(const std::string& text) { text_ = text; }
    
    bool isIndeterminate() const { return indeterminate_; }
    void setIndeterminate(bool indeterminate) { indeterminate_ = indeterminate; }
    
protected:
    void onRender() override;
    void onUpdate(float deltaTime) override;
    Size calculatePreferredSize() const override;
    
private:
    float progress_;
    std::string text_;
    bool indeterminate_;
    float animationTime_;
};

/**
 * Panel widget - container with border and background
 */
class Panel : public WidgetContainer {
public:
    explicit Panel(const std::string& id = "", const std::string& title = "");
    
    const std::string& getTitle() const { return title_; }
    void setTitle(const std::string& title) { title_ = title; }
    
    bool isCollapsible() const { return collapsible_; }
    void setCollapsible(bool collapsible) { collapsible_ = collapsible; }
    
    bool isCollapsed() const { return collapsed_; }
    void setCollapsed(bool collapsed) { collapsed_ = collapsed; }
    
    bool hasScrollbars() const { return scrollbars_; }
    void setScrollbars(bool scrollbars) { scrollbars_ = scrollbars; }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    std::string title_;
    bool collapsible_;
    bool collapsed_;
    bool scrollbars_;
};

/**
 * TabContainer widget - tabbed interface
 */
class TabContainer : public WidgetContainer {
public:
    explicit TabContainer(const std::string& id = "");
    
    struct TabInfo {
        std::string title;
        std::string icon;
        bool closable;
        std::shared_ptr<Widget> content;
        
        TabInfo(const std::string& t = "", const std::string& i = "", bool c = false)
            : title(t), icon(i), closable(c) {}
    };
    
    void addTab(const std::string& title, std::shared_ptr<Widget> content, 
                const std::string& icon = "", bool closable = false);
    void removeTab(int index);
    void removeTab(const std::string& title);
    
    int getActiveTab() const { return activeTab_; }
    void setActiveTab(int index);
    
    const std::vector<TabInfo>& getTabs() const { return tabs_; }
    
    void setOnTabChanged(std::function<void(int)> callback) { 
        onTabChangedCallback_ = callback; 
    }
    
    void setOnTabClosed(std::function<void(int)> callback) { 
        onTabClosedCallback_ = callback; 
    }
    
protected:
    void onRender() override;
    Size calculatePreferredSize() const override;
    
private:
    std::vector<TabInfo> tabs_;
    int activeTab_;
    std::function<void(int)> onTabChangedCallback_;
    std::function<void(int)> onTabClosedCallback_;
};

/**
 * Splitter widget - resizable split container
 */
class Splitter : public WidgetContainer {
public:
    enum class Orientation {
        Horizontal,
        Vertical
    };
    
    explicit Splitter(const std::string& id = "", Orientation orientation = Orientation::Horizontal);
    
    Orientation getOrientation() const { return orientation_; }
    void setOrientation(Orientation orientation) { orientation_ = orientation; }
    
    float getSplitRatio() const { return splitRatio_; }
    void setSplitRatio(float ratio); // 0.0 to 1.0
    
    void setFirstWidget(std::shared_ptr<Widget> widget);
    void setSecondWidget(std::shared_ptr<Widget> widget);
    
    std::shared_ptr<Widget> getFirstWidget() const { return firstWidget_; }
    std::shared_ptr<Widget> getSecondWidget() const { return secondWidget_; }
    
protected:
    void onRender() override;
    void onLayout() override;
    Size calculatePreferredSize() const override;
    
private:
    Orientation orientation_;
    float splitRatio_;
    std::shared_ptr<Widget> firstWidget_;
    std::shared_ptr<Widget> secondWidget_;
    float splitterThickness_;
};

} // namespace gui
} // namespace bolt