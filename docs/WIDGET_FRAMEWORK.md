# Bolt Widget Framework

The Bolt Widget Framework is a comprehensive GUI system built on top of ImGui, providing a modern, object-oriented approach to creating user interfaces in the Bolt IDE.

## Overview

The widget framework provides:
- **Hierarchical widget system** with parent-child relationships
- **Flexible layout management** with multiple layout types
- **Event-driven architecture** for user interactions
- **Theme system** for consistent styling
- **Widget factory** for dynamic widget creation
- **Comprehensive widget library** with common UI controls

## Architecture

### Core Components

#### Widget Base Class
The `Widget` class is the foundation of all UI components:

```cpp
#include "bolt/gui/widget_framework.hpp"

auto widget = std::make_shared<Widget>("my_widget");
widget->setSize(Size(200, 100));
widget->setPosition(Position(10, 20));
```

#### Widget Container
The `WidgetContainer` class manages layouts and child widgets:

```cpp
auto container = std::make_shared<WidgetContainer>("container", LayoutType::Vertical);
container->setSpacing(10.0f);
container->addChild(widget);
```

#### Layout Types
- `LayoutType::Vertical` - Stack widgets vertically
- `LayoutType::Horizontal` - Arrange widgets horizontally  
- `LayoutType::Grid` - Grid-based layout
- `LayoutType::Flex` - Flexible layout with grow/shrink
- `LayoutType::Absolute` - Manual positioning
- `LayoutType::None` - No automatic layout

### Widget Library

#### Input Widgets

**Button**
```cpp
auto button = std::make_shared<Button>("btn1", "Click Me");
button->setIcon("💾");
button->setOnClick([]() {
    std::cout << "Button clicked!" << std::endl;
});
```

**TextInput**
```cpp
auto textInput = std::make_shared<TextInput>("input1", "Enter text...");
textInput->setOnTextChanged([](const std::string& text) {
    std::cout << "Text: " << text << std::endl;
});
```

**Checkbox**
```cpp
auto checkbox = std::make_shared<Checkbox>("cb1", "Enable feature", true);
checkbox->setOnStateChanged([](bool checked) {
    std::cout << "Checked: " << checked << std::endl;
});
```

**ComboBox**
```cpp
auto combo = std::make_shared<ComboBox>("combo1");
combo->addItem("Option 1");
combo->addItem("Option 2");
combo->setOnSelectionChanged([](int index, const std::string& item) {
    std::cout << "Selected: " << item << std::endl;
});
```

**Slider**
```cpp
auto slider = std::make_shared<Slider>("slider1", 0.0f, 100.0f, 50.0f);
slider->setOnValueChanged([](float value) {
    std::cout << "Value: " << value << std::endl;
});
```

#### Display Widgets

**Label**
```cpp
auto label = std::make_shared<Label>("label1", "Hello World");
label->setAlignment(Label::Alignment::Center);
label->setWordWrap(true);
```

**ProgressBar**
```cpp
auto progress = std::make_shared<ProgressBar>("progress1", 0.7f);
progress->setText("70% Complete");
progress->setIndeterminate(false);
```

#### Container Widgets

**Panel**
```cpp
auto panel = std::make_shared<Panel>("panel1", "Settings");
panel->setCollapsible(true);
panel->setScrollbars(true);
```

**TabContainer**
```cpp
auto tabs = std::make_shared<TabContainer>("tabs1");
tabs->addTab("Tab 1", content1, "🔥", false);
tabs->addTab("Tab 2", content2, "⚡", true);
tabs->setOnTabChanged([](int index) {
    std::cout << "Active tab: " << index << std::endl;
});
```

**Splitter**
```cpp
auto splitter = std::make_shared<Splitter>("splitter1", Splitter::Orientation::Horizontal);
splitter->setSplitRatio(0.3f);
splitter->setFirstWidget(leftPanel);
splitter->setSecondWidget(rightPanel);
```

## Event System

Widgets use an event-driven architecture for handling user interactions:

```cpp
// Add event listener
widget->addEventListener(WidgetEventType::Click, [](const WidgetEvent& event) {
    std::cout << "Widget clicked!" << std::endl;
    
    // Access event data
    int x = event.getData<int>("mouseX", 0);
    int y = event.getData<int>("mouseY", 0);
});

// Fire custom events
WidgetEvent customEvent(WidgetEventType::Custom, widget.get());
customEvent.setData("message", std::string("Hello"));
widget->fireEvent(customEvent);
```

### Event Types
- `WidgetEventType::Click` - Mouse click
- `WidgetEventType::DoubleClick` - Double click
- `WidgetEventType::Hover` - Mouse hover
- `WidgetEventType::Focus` - Widget gained focus
- `WidgetEventType::TextChanged` - Text input changed
- `WidgetEventType::ValueChanged` - Value changed
- `WidgetEventType::Custom` - Custom events

## Theme System

The theme system provides consistent styling across all widgets:

### Using Themes

```cpp
// Get theme manager
auto& themeManager = ThemeManager::getInstance();

// Switch themes
themeManager.setCurrentTheme("Dark");
themeManager.setCurrentTheme("Light");
themeManager.setCurrentTheme("HighContrast");

// Apply theme to widget
themeManager.applyCurrentTheme(widget.get(), "Button");
```

### Creating Custom Themes

```cpp
auto customTheme = std::make_shared<WidgetTheme>("CustomTheme");

// Set default style
WidgetStyle defaultStyle;
defaultStyle.backgroundColor = ImVec4(0.2f, 0.2f, 0.3f, 1.0f);
defaultStyle.textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
customTheme->setDefaultStyle(defaultStyle);

// Set button-specific style
WidgetStyle buttonStyle = defaultStyle;
buttonStyle.hoverColor = ImVec4(0.4f, 0.4f, 0.6f, 1.0f);
customTheme->setWidgetStyle("Button", buttonStyle);

// Register theme
themeManager.addTheme(customTheme);
```

### Built-in Themes
- **Dark** - Modern dark theme with blue accents
- **Light** - Clean light theme  
- **HighContrast** - High contrast theme for accessibility

## Widget Factory

The widget factory enables dynamic widget creation:

```cpp
// Register custom widget type
WidgetFactory::getInstance().registerWidget<MyCustomWidget>("MyWidget");

// Create widgets by type
auto widget = WidgetFactory::getInstance().createWidget("Button", "btn1");
auto customWidget = WidgetFactory::getInstance().createWidget("MyWidget", "custom1");

// List registered types
auto types = WidgetFactory::getInstance().getRegisteredTypes();
```

## Initialization

Initialize the widget framework before use:

```cpp
#include "bolt/gui/widget_registration.hpp"

// Initialize framework
WidgetFrameworkInit::initialize();

// Create widgets...

// Shutdown when done
WidgetFrameworkInit::shutdown();
```

## Complete Example

```cpp
#include "bolt/gui/widget_framework.hpp"
#include "bolt/gui/widgets.hpp"
#include "bolt/gui/widget_registration.hpp"

int main() {
    // Initialize framework
    WidgetFrameworkInit::initialize();
    
    // Create root container
    auto root = std::make_shared<WidgetContainer>("root", LayoutType::Vertical);
    root->setBounds(Rect(0, 0, 800, 600));
    root->setSpacing(10.0f);
    
    // Create a panel
    auto panel = std::make_shared<Panel>("main_panel", "Demo Panel");
    panel->setSize(Size(780, 580));
    
    // Create input widgets
    auto button = std::make_shared<Button>("demo_button", "Click Me");
    auto textInput = std::make_shared<TextInput>("demo_input", "Type here...");
    auto checkbox = std::make_shared<Checkbox>("demo_check", "Enable feature", true);
    
    // Set up event handlers
    button->setOnClick([]() {
        std::cout << "Hello from widget framework!" << std::endl;
    });
    
    textInput->setOnTextChanged([](const std::string& text) {
        std::cout << "Text changed: " << text << std::endl;
    });
    
    // Build hierarchy
    panel->addChild(button);
    panel->addChild(textInput);
    panel->addChild(checkbox);
    root->addChild(panel);
    
    // Apply theme
    auto& themeManager = ThemeManager::getInstance();
    themeManager.applyCurrentTheme(root.get(), "WidgetContainer");
    
    // In your render loop:
    root->update(deltaTime);
    root->layout();
    root->render();
    
    // Cleanup
    WidgetFrameworkInit::shutdown();
    return 0;
}
```

## Integration with ImGui

The widget framework is built on top of ImGui and integrates seamlessly:

- Widgets automatically handle ImGui rendering
- Custom styling translates to ImGui style changes
- Event handling works with ImGui input system
- Layout system positions ImGui elements

## Best Practices

1. **Initialize the framework** before creating widgets
2. **Use smart pointers** for automatic memory management
3. **Set up proper hierarchy** with containers and children
4. **Apply themes consistently** across your application
5. **Handle events appropriately** for responsive UI
6. **Test layouts** with different window sizes
7. **Use appropriate widget types** for your use cases

## Advanced Features

### Custom Widget Properties

```cpp
// Set custom properties
widget->setProperty("userData", 42);
widget->setProperty("customColor", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

// Retrieve properties
int data = widget->getProperty<int>("userData", 0);
ImVec4 color = widget->getProperty<ImVec4>("customColor");
```

### Layout Parameters

```cpp
LayoutParameters params;
params.type = LayoutType::Grid;
params.columns = 3;
params.spacing = 5.0f;
params.fillAvailableSpace = true;
container->setLayoutParams(params);
```

### Widget States and Styling

```cpp
// Set different styles for different states
theme->setWidgetStyle("Button", "normal", normalStyle);
theme->setWidgetStyle("Button", "hovered", hoveredStyle);
theme->setWidgetStyle("Button", "pressed", pressedStyle);

// Apply state-specific style
theme->applyToWidget(button.get(), "Button", "hovered");
```

## Testing

The framework includes comprehensive tests covering:
- Widget creation and properties
- Event system functionality
- Layout management
- Theme application
- Factory registration
- Memory management

Run tests with:
```bash
cd build
make bolt_widget_framework_tests
./test/bolt_widget_framework_tests
```

## Demo Application

A comprehensive demo showcasing all widget types and features:

```bash
cd build
make demo_widget_framework
./demo_widget_framework
```

The demo includes:
- All widget types with interactive examples
- Theme switching
- Layout demonstrations
- Event handling examples
- Real-time property editing

## Future Enhancements

Planned features for future releases:
- Animation system for smooth transitions
- Drag and drop support
- Custom drawing capabilities
- Widget templates and serialization
- Performance optimizations
- Additional layout types
- Accessibility improvements

---

For more information, see the source code documentation and demo applications.