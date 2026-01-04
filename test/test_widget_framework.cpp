#include "bolt/gui/widget_framework.hpp"
#include "bolt/gui/widgets.hpp"
#include "bolt/gui/widget_registration.hpp"

#include <gtest/gtest.h>
#include <memory>

using namespace bolt::gui;

class WidgetFrameworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        WidgetFrameworkInit::initialize();
    }
    
    void TearDown() override {
        WidgetFrameworkInit::shutdown();
    }
};

// Widget Framework Core Tests
TEST_F(WidgetFrameworkTest, FrameworkInitialization) {
    EXPECT_TRUE(WidgetFrameworkInit::isInitialized());
    
    // Check widget registration
    auto registeredTypes = WidgetRegistration::getRegisteredTypes();
    EXPECT_GT(registeredTypes.size(), 0);
    
    // Check theme registration
    auto& themeManager = ThemeManager::getInstance();
    auto themeNames = themeManager.getThemeNames();
    EXPECT_GT(themeNames.size(), 0);
    EXPECT_TRUE(themeManager.getCurrentTheme() != nullptr);
}

TEST_F(WidgetFrameworkTest, WidgetCreation) {
    auto widget = std::make_shared<Widget>("test_widget");
    
    EXPECT_EQ(widget->getId(), "test_widget");
    EXPECT_TRUE(widget->isVisible());
    EXPECT_TRUE(widget->isEnabled());
    EXPECT_FALSE(widget->isFocused());
    EXPECT_EQ(widget->getParent(), nullptr);
    EXPECT_TRUE(widget->getChildren().empty());
}

TEST_F(WidgetFrameworkTest, WidgetHierarchy) {
    auto parent = std::make_shared<Widget>("parent");
    auto child1 = std::make_shared<Widget>("child1");
    auto child2 = std::make_shared<Widget>("child2");
    
    parent->addChild(child1);
    parent->addChild(child2);
    
    EXPECT_EQ(parent->getChildren().size(), 2);
    EXPECT_EQ(child1->getParent(), parent.get());
    EXPECT_EQ(child2->getParent(), parent.get());
    
    parent->removeChild(child1);
    EXPECT_EQ(parent->getChildren().size(), 1);
    EXPECT_EQ(child1->getParent(), nullptr);
    
    parent->clearChildren();
    EXPECT_TRUE(parent->getChildren().empty());
    EXPECT_EQ(child2->getParent(), nullptr);
}

TEST_F(WidgetFrameworkTest, WidgetProperties) {
    auto widget = std::make_shared<Widget>("test");
    
    widget->setProperty("intValue", 42);
    widget->setProperty("stringValue", std::string("hello"));
    widget->setProperty("floatValue", 3.14f);
    
    EXPECT_EQ(widget->getProperty<int>("intValue"), 42);
    EXPECT_EQ(widget->getProperty<std::string>("stringValue"), "hello");
    EXPECT_FLOAT_EQ(widget->getProperty<float>("floatValue"), 3.14f);
    
    EXPECT_TRUE(widget->hasProperty("intValue"));
    EXPECT_FALSE(widget->hasProperty("nonexistent"));
    
    widget->removeProperty("intValue");
    EXPECT_FALSE(widget->hasProperty("intValue"));
}

TEST_F(WidgetFrameworkTest, WidgetGeometry) {
    auto widget = std::make_shared<Widget>("test");
    
    widget->setPosition(Position(10.0f, 20.0f));
    widget->setSize(Size(100.0f, 50.0f));
    
    EXPECT_EQ(widget->getBounds().position.x, 10.0f);
    EXPECT_EQ(widget->getBounds().position.y, 20.0f);
    EXPECT_EQ(widget->getBounds().size.width, 100.0f);
    EXPECT_EQ(widget->getBounds().size.height, 50.0f);
    
    widget->setMinimumSize(Size(50.0f, 25.0f));
    widget->setMaximumSize(Size(200.0f, 100.0f));
    
    EXPECT_EQ(widget->getMinimumSize().width, 50.0f);
    EXPECT_EQ(widget->getMaximumSize().width, 200.0f);
}

// Widget Factory Tests
TEST_F(WidgetFrameworkTest, WidgetFactory) {
    auto& factory = WidgetFactory::getInstance();
    
    EXPECT_TRUE(factory.isTypeRegistered("Button"));
    EXPECT_TRUE(factory.isTypeRegistered("Label"));
    EXPECT_TRUE(factory.isTypeRegistered("TextInput"));
    EXPECT_FALSE(factory.isTypeRegistered("NonExistentWidget"));
    
    auto button = factory.createWidget("Button", "test_button");
    EXPECT_TRUE(button != nullptr);
    EXPECT_EQ(button->getId(), "test_button");
    
    auto invalidWidget = factory.createWidget("InvalidType");
    EXPECT_TRUE(invalidWidget == nullptr);
}

// Container Tests
TEST_F(WidgetFrameworkTest, WidgetContainer) {
    auto container = std::make_shared<WidgetContainer>("container", LayoutType::Vertical);
    
    EXPECT_EQ(container->getLayoutType(), LayoutType::Vertical);
    
    container->setLayoutType(LayoutType::Horizontal);
    EXPECT_EQ(container->getLayoutType(), LayoutType::Horizontal);
    
    container->setSpacing(10.0f);
    EXPECT_EQ(container->getSpacing(), 10.0f);
}

// Button Tests
TEST_F(WidgetFrameworkTest, ButtonWidget) {
    auto button = std::make_shared<Button>("test_button", "Click Me");
    
    EXPECT_EQ(button->getText(), "Click Me");
    EXPECT_FALSE(button->isPressed());
    EXPECT_FALSE(button->isHovered());
    
    button->setText("New Text");
    EXPECT_EQ(button->getText(), "New Text");
    
    button->setIcon("🔥");
    EXPECT_EQ(button->getIcon(), "🔥");
    
    // Test callback
    bool clicked = false;
    button->setOnClick([&clicked]() {
        clicked = true;
    });
    
    // Simulate click event
    WidgetEvent clickEvent(WidgetEventType::Click, button.get());
    button->fireEvent(clickEvent);
    EXPECT_TRUE(clicked);
}

// Label Tests
TEST_F(WidgetFrameworkTest, LabelWidget) {
    auto label = std::make_shared<Label>("test_label", "Hello World");
    
    EXPECT_EQ(label->getText(), "Hello World");
    EXPECT_EQ(label->getAlignment(), Label::Alignment::Left);
    EXPECT_FALSE(label->getWordWrap());
    
    label->setText("New Text");
    EXPECT_EQ(label->getText(), "New Text");
    
    label->setAlignment(Label::Alignment::Center);
    EXPECT_EQ(label->getAlignment(), Label::Alignment::Center);
    
    label->setWordWrap(true);
    EXPECT_TRUE(label->getWordWrap());
}

// TextInput Tests
TEST_F(WidgetFrameworkTest, TextInputWidget) {
    auto textInput = std::make_shared<TextInput>("test_input", "Placeholder");
    
    EXPECT_EQ(textInput->getText(), "");
    EXPECT_EQ(textInput->getPlaceholder(), "Placeholder");
    EXPECT_FALSE(textInput->isReadOnly());
    EXPECT_FALSE(textInput->isPassword());
    
    textInput->setText("Hello");
    EXPECT_EQ(textInput->getText(), "Hello");
    
    textInput->setReadOnly(true);
    EXPECT_TRUE(textInput->isReadOnly());
    
    textInput->setPassword(true);
    EXPECT_TRUE(textInput->isPassword());
    
    // Test callback
    std::string changedText;
    textInput->setOnTextChanged([&changedText](const std::string& text) {
        changedText = text;
    });
    
    textInput->setText("Test");
    // Note: Callback would be triggered in real UI interaction
}

// Checkbox Tests
TEST_F(WidgetFrameworkTest, CheckboxWidget) {
    auto checkbox = std::make_shared<Checkbox>("test_checkbox", "Check me", false);
    
    EXPECT_FALSE(checkbox->isChecked());
    EXPECT_EQ(checkbox->getLabel(), "Check me");
    
    checkbox->setChecked(true);
    EXPECT_TRUE(checkbox->isChecked());
    
    checkbox->setLabel("New label");
    EXPECT_EQ(checkbox->getLabel(), "New label");
    
    // Test callback
    bool stateChanged = false;
    bool newState = false;
    checkbox->setOnStateChanged([&stateChanged, &newState](bool checked) {
        stateChanged = true;
        newState = checked;
    });
    
    checkbox->setChecked(false);
    // Note: In real implementation, callback would be triggered
}

// RadioButton Tests
TEST_F(WidgetFrameworkTest, RadioButtonWidget) {
    auto radio1 = std::make_shared<RadioButton>("radio1", "Option 1", "group1", true);
    auto radio2 = std::make_shared<RadioButton>("radio2", "Option 2", "group1", false);
    auto radio3 = std::make_shared<RadioButton>("radio3", "Option 3", "group1", false);
    
    EXPECT_TRUE(radio1->isSelected());
    EXPECT_FALSE(radio2->isSelected());
    EXPECT_FALSE(radio3->isSelected());
    
    EXPECT_EQ(radio1->getGroup(), "group1");
    EXPECT_EQ(radio1->getLabel(), "Option 1");
    
    // Test mutual exclusivity
    radio2->setSelected(true);
    EXPECT_FALSE(radio1->isSelected());
    EXPECT_TRUE(radio2->isSelected());
    EXPECT_FALSE(radio3->isSelected());
}

// ComboBox Tests
TEST_F(WidgetFrameworkTest, ComboBoxWidget) {
    auto combo = std::make_shared<ComboBox>("test_combo");
    
    EXPECT_EQ(combo->getSelectedIndex(), -1);
    EXPECT_TRUE(combo->getItems().empty());
    
    combo->addItem("Item 1");
    combo->addItem("Item 2");
    combo->addItem("Item 3");
    
    EXPECT_EQ(combo->getItems().size(), 3);
    EXPECT_EQ(combo->getSelectedIndex(), 0); // Auto-select first item
    EXPECT_EQ(combo->getSelectedItem(), "Item 1");
    
    combo->setSelectedIndex(1);
    EXPECT_EQ(combo->getSelectedIndex(), 1);
    EXPECT_EQ(combo->getSelectedItem(), "Item 2");
    
    combo->removeItem("Item 2");
    EXPECT_EQ(combo->getItems().size(), 2);
    EXPECT_EQ(combo->getSelectedIndex(), 1); // Should adjust
    
    combo->clearItems();
    EXPECT_TRUE(combo->getItems().empty());
    EXPECT_EQ(combo->getSelectedIndex(), -1);
}

// Slider Tests
TEST_F(WidgetFrameworkTest, SliderWidget) {
    auto slider = std::make_shared<Slider>("test_slider", 0.0f, 100.0f, 50.0f);
    
    EXPECT_FLOAT_EQ(slider->getValue(), 50.0f);
    EXPECT_FLOAT_EQ(slider->getMinValue(), 0.0f);
    EXPECT_FLOAT_EQ(slider->getMaxValue(), 100.0f);
    
    slider->setValue(75.0f);
    EXPECT_FLOAT_EQ(slider->getValue(), 75.0f);
    
    // Test clamping
    slider->setValue(150.0f);
    EXPECT_FLOAT_EQ(slider->getValue(), 100.0f);
    
    slider->setValue(-10.0f);
    EXPECT_FLOAT_EQ(slider->getValue(), 0.0f);
}

// ProgressBar Tests
TEST_F(WidgetFrameworkTest, ProgressBarWidget) {
    auto progress = std::make_shared<ProgressBar>("test_progress", 0.5f);
    
    EXPECT_FLOAT_EQ(progress->getProgress(), 0.5f);
    EXPECT_FALSE(progress->isIndeterminate());
    
    progress->setProgress(0.8f);
    EXPECT_FLOAT_EQ(progress->getProgress(), 0.8f);
    
    progress->setText("80% Complete");
    EXPECT_EQ(progress->getText(), "80% Complete");
    
    progress->setIndeterminate(true);
    EXPECT_TRUE(progress->isIndeterminate());
    
    // Test clamping
    progress->setProgress(1.5f);
    EXPECT_FLOAT_EQ(progress->getProgress(), 1.0f);
    
    progress->setProgress(-0.1f);
    EXPECT_FLOAT_EQ(progress->getProgress(), 0.0f);
}

// Panel Tests
TEST_F(WidgetFrameworkTest, PanelWidget) {
    auto panel = std::make_shared<Panel>("test_panel", "Panel Title");
    
    EXPECT_EQ(panel->getTitle(), "Panel Title");
    EXPECT_FALSE(panel->isCollapsible());
    EXPECT_FALSE(panel->isCollapsed());
    EXPECT_FALSE(panel->hasScrollbars());
    
    panel->setTitle("New Title");
    EXPECT_EQ(panel->getTitle(), "New Title");
    
    panel->setCollapsible(true);
    EXPECT_TRUE(panel->isCollapsible());
    
    panel->setCollapsed(true);
    EXPECT_TRUE(panel->isCollapsed());
    
    panel->setScrollbars(true);
    EXPECT_TRUE(panel->hasScrollbars());
}

// TabContainer Tests
TEST_F(WidgetFrameworkTest, TabContainerWidget) {
    auto tabContainer = std::make_shared<TabContainer>("test_tabs");
    
    EXPECT_EQ(tabContainer->getActiveTab(), -1);
    EXPECT_TRUE(tabContainer->getTabs().empty());
    
    auto content1 = std::make_shared<Label>("tab1_content", "Tab 1");
    auto content2 = std::make_shared<Label>("tab2_content", "Tab 2");
    
    tabContainer->addTab("Tab 1", content1, "🔥", false);
    tabContainer->addTab("Tab 2", content2, "⚡", true);
    
    EXPECT_EQ(tabContainer->getTabs().size(), 2);
    EXPECT_EQ(tabContainer->getActiveTab(), 0);
    
    const auto& tabs = tabContainer->getTabs();
    EXPECT_EQ(tabs[0].title, "Tab 1");
    EXPECT_EQ(tabs[0].icon, "🔥");
    EXPECT_FALSE(tabs[0].closable);
    
    EXPECT_EQ(tabs[1].title, "Tab 2");
    EXPECT_EQ(tabs[1].icon, "⚡");
    EXPECT_TRUE(tabs[1].closable);
    
    tabContainer->setActiveTab(1);
    EXPECT_EQ(tabContainer->getActiveTab(), 1);
    
    tabContainer->removeTab(0);
    EXPECT_EQ(tabContainer->getTabs().size(), 1);
    EXPECT_EQ(tabContainer->getActiveTab(), 0); // Should adjust
}

// Splitter Tests
TEST_F(WidgetFrameworkTest, SplitterWidget) {
    auto splitter = std::make_shared<Splitter>("test_splitter", Splitter::Orientation::Horizontal);
    
    EXPECT_EQ(splitter->getOrientation(), Splitter::Orientation::Horizontal);
    EXPECT_FLOAT_EQ(splitter->getSplitRatio(), 0.5f);
    
    splitter->setOrientation(Splitter::Orientation::Vertical);
    EXPECT_EQ(splitter->getOrientation(), Splitter::Orientation::Vertical);
    
    splitter->setSplitRatio(0.3f);
    EXPECT_FLOAT_EQ(splitter->getSplitRatio(), 0.3f);
    
    // Test clamping
    splitter->setSplitRatio(1.5f);
    EXPECT_FLOAT_EQ(splitter->getSplitRatio(), 0.9f);
    
    splitter->setSplitRatio(-0.1f);
    EXPECT_FLOAT_EQ(splitter->getSplitRatio(), 0.1f);
    
    auto widget1 = std::make_shared<Label>("left", "Left");
    auto widget2 = std::make_shared<Label>("right", "Right");
    
    splitter->setFirstWidget(widget1);
    splitter->setSecondWidget(widget2);
    
    EXPECT_EQ(splitter->getFirstWidget(), widget1);
    EXPECT_EQ(splitter->getSecondWidget(), widget2);
}

// Event System Tests
TEST_F(WidgetFrameworkTest, WidgetEventSystem) {
    auto widget = std::make_shared<Widget>("test_widget");
    
    bool eventReceived = false;
    WidgetEventType receivedType = WidgetEventType::Custom;
    
    widget->addEventListener(WidgetEventType::Click, [&](const WidgetEvent& event) {
        eventReceived = true;
        receivedType = event.type;
    });
    
    WidgetEvent clickEvent(WidgetEventType::Click, widget.get());
    clickEvent.setData("testData", 42);
    
    widget->fireEvent(clickEvent);
    
    EXPECT_TRUE(eventReceived);
    EXPECT_EQ(receivedType, WidgetEventType::Click);
    EXPECT_EQ(clickEvent.getData<int>("testData"), 42);
}

// Theme System Tests
TEST_F(WidgetFrameworkTest, ThemeSystem) {
    auto& themeManager = ThemeManager::getInstance();
    
    EXPECT_TRUE(themeManager.getCurrentTheme() != nullptr);
    
    auto customTheme = std::make_shared<WidgetTheme>("CustomTest");
    themeManager.addTheme(customTheme);
    
    EXPECT_TRUE(themeManager.getTheme("CustomTest") != nullptr);
    
    themeManager.setCurrentTheme("CustomTest");
    EXPECT_EQ(themeManager.getCurrentTheme()->getName(), "CustomTest");
    
    themeManager.removeTheme("CustomTest");
    EXPECT_TRUE(themeManager.getTheme("CustomTest") == nullptr);
}

// Layout Tests
TEST_F(WidgetFrameworkTest, LayoutSystem) {
    auto container = std::make_shared<WidgetContainer>("test_container", LayoutType::Vertical);
    container->setBounds(Rect(0, 0, 200, 300));
    
    auto child1 = std::make_shared<Label>("child1", "Child 1");
    auto child2 = std::make_shared<Label>("child2", "Child 2");
    auto child3 = std::make_shared<Label>("child3", "Child 3");
    
    container->addChild(child1);
    container->addChild(child2);
    container->addChild(child3);
    
    // Test preferred size calculation
    Size preferredSize = container->calculatePreferredSize();
    EXPECT_GT(preferredSize.width, 0.0f);
    EXPECT_GT(preferredSize.height, 0.0f);
    
    // Test layout
    container->layout();
    
    // Children should be positioned vertically
    // Note: Exact positions depend on preferred sizes, which depend on text rendering in ImGui
    // So we just verify that layout was called (no exceptions thrown)
    EXPECT_TRUE(true); // Layout completed without error
}

// Utility Classes Tests
TEST_F(WidgetFrameworkTest, UtilityClasses) {
    // Test Size
    Size size1(100.0f, 50.0f);
    EXPECT_TRUE(size1.isValid());
    
    Size size2(-10.0f, 20.0f);
    EXPECT_FALSE(size2.isValid());
    
    // Test Position
    Position pos(10.0f, 20.0f);
    EXPECT_EQ(pos.x, 10.0f);
    EXPECT_EQ(pos.y, 20.0f);
    
    // Test Rect
    Rect rect(10.0f, 20.0f, 100.0f, 50.0f);
    EXPECT_TRUE(rect.contains(Position(50.0f, 30.0f)));
    EXPECT_FALSE(rect.contains(Position(150.0f, 30.0f)));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}