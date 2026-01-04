#include "bolt/gui/widget_framework.hpp"
#include "bolt/gui/widgets.hpp"
#include "bolt/gui/widget_registration.hpp"

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#endif

#include <iostream>
#include <memory>

using namespace bolt::gui;

class WidgetFrameworkDemo {
public:
    WidgetFrameworkDemo() : window_(nullptr) {}
    
    bool initialize() {
#ifdef BOLT_HAVE_IMGUI
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        // Create window
        window_ = glfwCreateWindow(1200, 800, "Bolt Widget Framework Demo", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // Enable vsync
        
        // Setup ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Initialize widget framework
        WidgetFrameworkInit::initialize();
        
        // Create demo widgets
        createDemoWidgets();
        
        return true;
#else
        std::cerr << "ImGui not available - widget framework demo disabled" << std::endl;
        return false;
#endif
    }
    
    void run() {
#ifdef BOLT_HAVE_IMGUI
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            
            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Update widgets
            static float lastTime = 0.0f;
            float currentTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            
            if (rootContainer_) {
                rootContainer_->update(deltaTime);
            }
            
            // Render demo
            renderDemo();
            
            // Render widgets
            if (rootContainer_) {
                rootContainer_->render();
            }
            
            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.12f, 0.12f, 0.14f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window_);
        }
#endif
    }
    
    void shutdown() {
#ifdef BOLT_HAVE_IMGUI
        WidgetFrameworkInit::shutdown();
        
        if (window_) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            
            glfwDestroyWindow(window_);
            glfwTerminate();
            window_ = nullptr;
        }
#endif
    }
    
private:
    GLFWwindow* window_;
    std::shared_ptr<WidgetContainer> rootContainer_;
    std::shared_ptr<TabContainer> tabContainer_;
    std::string outputText_;
    
    void createDemoWidgets() {
        // Create root container
        rootContainer_ = std::make_shared<WidgetContainer>("root", LayoutType::Vertical);
        rootContainer_->setBounds(Rect(0, 0, 1200, 800));
        rootContainer_->setSpacing(10.0f);
        
        // Apply theme
        auto& themeManager = ThemeManager::getInstance();
        themeManager.applyCurrentTheme(rootContainer_.get(), "WidgetContainer");
        
        // Create tab container for different widget categories
        tabContainer_ = std::make_shared<TabContainer>("main_tabs");
        tabContainer_->setBounds(Rect(10, 10, 1180, 780));
        themeManager.applyCurrentTheme(tabContainer_.get(), "TabContainer");
        
        // Create different tabs
        createBasicWidgetsTab();
        createInputWidgetsTab();
        createContainerWidgetsTab();
        createThemeDemo();
        
        rootContainer_->addChild(tabContainer_);
    }
    
    void createBasicWidgetsTab() {
        auto container = std::make_shared<WidgetContainer>("basic_widgets", LayoutType::Vertical);
        container->setSpacing(15.0f);
        
        // Buttons section
        auto buttonPanel = std::make_shared<Panel>("button_panel", "Buttons");
        buttonPanel->setSize(Size(0, 150));
        
        auto buttonContainer = std::make_shared<WidgetContainer>("buttons", LayoutType::Horizontal);
        buttonContainer->setSpacing(10.0f);
        
        // Standard button
        auto button1 = std::make_shared<Button>("btn1", "Click Me");
        button1->setOnClick([this]() {
            outputText_ += "Button 1 clicked!\n";
        });
        
        // Icon button
        auto button2 = std::make_shared<Button>("btn2", "Save");
        button2->setIcon("💾");
        button2->setOnClick([this]() {
            outputText_ += "Save button clicked!\n";
        });
        
        // Disabled button
        auto button3 = std::make_shared<Button>("btn3", "Disabled");
        button3->setEnabled(false);
        
        buttonContainer->addChild(button1);
        buttonContainer->addChild(button2);
        buttonContainer->addChild(button3);
        buttonPanel->addChild(buttonContainer);
        
        // Labels section
        auto labelPanel = std::make_shared<Panel>("label_panel", "Labels");
        labelPanel->setSize(Size(0, 100));
        
        auto label1 = std::make_shared<Label>("label1", "Left-aligned label");
        label1->setAlignment(Label::Alignment::Left);
        
        auto label2 = std::make_shared<Label>("label2", "Centered label");
        label2->setAlignment(Label::Alignment::Center);
        
        auto label3 = std::make_shared<Label>("label3", "Right-aligned label");
        label3->setAlignment(Label::Alignment::Right);
        
        labelPanel->addChild(label1);
        labelPanel->addChild(label2);
        labelPanel->addChild(label3);
        
        // Progress bar section
        auto progressPanel = std::make_shared<Panel>("progress_panel", "Progress Bars");
        progressPanel->setSize(Size(0, 100));
        
        auto progress1 = std::make_shared<ProgressBar>("progress1", 0.7f);
        progress1->setText("70% Complete");
        
        auto progress2 = std::make_shared<ProgressBar>("progress2", 0.0f);
        progress2->setIndeterminate(true);
        progress2->setText("Loading...");
        
        progressPanel->addChild(progress1);
        progressPanel->addChild(progress2);
        
        container->addChild(buttonPanel);
        container->addChild(labelPanel);
        container->addChild(progressPanel);
        
        tabContainer_->addTab("Basic Widgets", container, "🎛️");
    }
    
    void createInputWidgetsTab() {
        auto container = std::make_shared<WidgetContainer>("input_widgets", LayoutType::Vertical);
        container->setSpacing(15.0f);
        
        // Text input section
        auto textPanel = std::make_shared<Panel>("text_panel", "Text Input");
        textPanel->setSize(Size(0, 200));
        
        auto textInput = std::make_shared<TextInput>("text_input", "Enter your name...");
        textInput->setOnTextChanged([this](const std::string& text) {
            outputText_ += "Text changed: " + text + "\n";
        });
        
        auto passwordInput = std::make_shared<TextInput>("password_input", "Password");
        passwordInput->setPassword(true);
        
        auto textArea = std::make_shared<TextArea>("text_area", "Multi-line text...");
        textArea->setSize(Size(400, 80));
        textArea->setOnTextChanged([this](const std::string& text) {
            if (text.length() % 10 == 0) { // Only log every 10th character to avoid spam
                outputText_ += "TextArea changed (length: " + std::to_string(text.length()) + ")\n";
            }
        });
        
        textPanel->addChild(textInput);
        textPanel->addChild(passwordInput);
        textPanel->addChild(textArea);
        
        // Selection widgets section
        auto selectionPanel = std::make_shared<Panel>("selection_panel", "Selection Controls");
        selectionPanel->setSize(Size(0, 200));
        
        auto checkbox1 = std::make_shared<Checkbox>("cb1", "Enable notifications", true);
        checkbox1->setOnStateChanged([this](bool checked) {
            outputText_ += "Notifications " + std::string(checked ? "enabled" : "disabled") + "\n";
        });
        
        auto checkbox2 = std::make_shared<Checkbox>("cb2", "Dark mode", false);
        
        // Radio buttons
        auto radio1 = std::make_shared<RadioButton>("r1", "Option A", "group1", true);
        auto radio2 = std::make_shared<RadioButton>("r2", "Option B", "group1", false);
        auto radio3 = std::make_shared<RadioButton>("r3", "Option C", "group1", false);
        
        radio1->setOnStateChanged([this](bool selected) {
            if (selected) outputText_ += "Selected Option A\n";
        });
        radio2->setOnStateChanged([this](bool selected) {
            if (selected) outputText_ += "Selected Option B\n";
        });
        radio3->setOnStateChanged([this](bool selected) {
            if (selected) outputText_ += "Selected Option C\n";
        });
        
        // ComboBox
        auto combo = std::make_shared<ComboBox>("combo1");
        combo->addItem("Item 1");
        combo->addItem("Item 2");
        combo->addItem("Item 3");
        combo->setSelectedIndex(0);
        combo->setOnSelectionChanged([this](int index, const std::string& item) {
            outputText_ += "Selected: " + item + " (index " + std::to_string(index) + ")\n";
        });
        
        // Slider
        auto slider = std::make_shared<Slider>("slider1", 0.0f, 100.0f, 50.0f);
        slider->setOnValueChanged([this](float value) {
            outputText_ += "Slider value: " + std::to_string(static_cast<int>(value)) + "\n";
        });
        
        selectionPanel->addChild(checkbox1);
        selectionPanel->addChild(checkbox2);
        selectionPanel->addChild(radio1);
        selectionPanel->addChild(radio2);
        selectionPanel->addChild(radio3);
        selectionPanel->addChild(combo);
        selectionPanel->addChild(slider);
        
        container->addChild(textPanel);
        container->addChild(selectionPanel);
        
        tabContainer_->addTab("Input Widgets", container, "✏️");
    }
    
    void createContainerWidgetsTab() {
        auto container = std::make_shared<WidgetContainer>("container_widgets", LayoutType::Vertical);
        container->setSpacing(10.0f);
        
        // Splitter demo
        auto splitter = std::make_shared<Splitter>("splitter1", Splitter::Orientation::Horizontal);
        splitter->setSize(Size(800, 300));
        splitter->setSplitRatio(0.3f);
        
        // Left side - nested container
        auto leftContainer = std::make_shared<WidgetContainer>("left", LayoutType::Vertical);
        leftContainer->setSpacing(5.0f);
        
        auto label1 = std::make_shared<Label>("split_label1", "Left Panel");
        auto button1 = std::make_shared<Button>("split_btn1", "Left Button");
        button1->setOnClick([this]() {
            outputText_ += "Left button clicked!\n";
        });
        
        leftContainer->addChild(label1);
        leftContainer->addChild(button1);
        
        // Right side - another nested container
        auto rightContainer = std::make_shared<WidgetContainer>("right", LayoutType::Vertical);
        rightContainer->setSpacing(5.0f);
        
        auto label2 = std::make_shared<Label>("split_label2", "Right Panel");
        auto textInput = std::make_shared<TextInput>("split_input", "Type here...");
        
        rightContainer->addChild(label2);
        rightContainer->addChild(textInput);
        
        splitter->setFirstWidget(leftContainer);
        splitter->setSecondWidget(rightContainer);
        
        // Nested tabs
        auto nestedTabs = std::make_shared<TabContainer>("nested_tabs");
        nestedTabs->setSize(Size(800, 200));
        
        // Tab 1
        auto tab1Content = std::make_shared<WidgetContainer>("tab1", LayoutType::Horizontal);
        tab1Content->setSpacing(10.0f);
        
        auto tab1Button = std::make_shared<Button>("tab1_btn", "Tab 1 Button");
        auto tab1Label = std::make_shared<Label>("tab1_label", "This is tab 1 content");
        
        tab1Content->addChild(tab1Button);
        tab1Content->addChild(tab1Label);
        
        // Tab 2
        auto tab2Content = std::make_shared<WidgetContainer>("tab2", LayoutType::Vertical);
        tab2Content->setSpacing(5.0f);
        
        auto tab2Checkbox = std::make_shared<Checkbox>("tab2_cb", "Tab 2 Option", true);
        auto tab2Slider = std::make_shared<Slider>("tab2_slider", 0.0f, 10.0f, 5.0f);
        
        tab2Content->addChild(tab2Checkbox);
        tab2Content->addChild(tab2Slider);
        
        nestedTabs->addTab("First", tab1Content, "1️⃣");
        nestedTabs->addTab("Second", tab2Content, "2️⃣");
        
        container->addChild(splitter);
        container->addChild(nestedTabs);
        
        tabContainer_->addTab("Containers", container, "📦");
    }
    
    void createThemeDemo() {
        auto container = std::make_shared<WidgetContainer>("theme_demo", LayoutType::Vertical);
        container->setSpacing(10.0f);
        
        // Theme selector
        auto themePanel = std::make_shared<Panel>("theme_panel", "Theme Selection");
        themePanel->setSize(Size(0, 100));
        
        auto themeCombo = std::make_shared<ComboBox>("theme_combo");
        auto& themeManager = ThemeManager::getInstance();
        
        for (const auto& themeName : themeManager.getThemeNames()) {
            themeCombo->addItem(themeName);
        }
        
        themeCombo->setSelectedItem(themeManager.getCurrentTheme()->getName());
        themeCombo->setOnSelectionChanged([this](int index, const std::string& themeName) {
            auto& themeManager = ThemeManager::getInstance();
            themeManager.setCurrentTheme(themeName);
            
            // Reapply theme to all widgets
            applyCurrentThemeRecursively(rootContainer_.get());
            
            outputText_ += "Theme changed to: " + themeName + "\n";
        });
        
        themePanel->addChild(themeCombo);
        
        // Theme preview widgets
        auto previewPanel = std::make_shared<Panel>("preview_panel", "Theme Preview");
        previewPanel->setSize(Size(0, 400));
        
        auto previewContainer = std::make_shared<WidgetContainer>("preview", LayoutType::Vertical);
        previewContainer->setSpacing(10.0f);
        
        // Various widgets to show theme
        auto previewButton = std::make_shared<Button>("preview_btn", "Theme Preview Button");
        auto previewInput = std::make_shared<TextInput>("preview_input", "Sample text...");
        auto previewCheckbox = std::make_shared<Checkbox>("preview_cb", "Themed checkbox", true);
        auto previewProgress = std::make_shared<ProgressBar>("preview_progress", 0.6f);
        previewProgress->setText("60% themed");
        
        previewContainer->addChild(previewButton);
        previewContainer->addChild(previewInput);
        previewContainer->addChild(previewCheckbox);
        previewContainer->addChild(previewProgress);
        
        previewPanel->addChild(previewContainer);
        
        container->addChild(themePanel);
        container->addChild(previewPanel);
        
        tabContainer_->addTab("Themes", container, "🎨");
    }
    
    void applyCurrentThemeRecursively(Widget* widget) {
        if (!widget) return;
        
        auto& themeManager = ThemeManager::getInstance();
        
        // Apply theme based on widget type (simplified)
        std::string widgetType = "Widget"; // Default
        
        if (dynamic_cast<Button*>(widget)) widgetType = "Button";
        else if (dynamic_cast<TextInput*>(widget)) widgetType = "TextInput";
        else if (dynamic_cast<TextArea*>(widget)) widgetType = "TextArea";
        else if (dynamic_cast<Panel*>(widget)) widgetType = "Panel";
        else if (dynamic_cast<Label*>(widget)) widgetType = "Label";
        else if (dynamic_cast<WidgetContainer*>(widget)) widgetType = "WidgetContainer";
        
        themeManager.applyCurrentTheme(widget, widgetType);
        
        // Recursively apply to children
        for (const auto& child : widget->getChildren()) {
            applyCurrentThemeRecursively(child.get());
        }
    }
    
    void renderDemo() {
#ifdef BOLT_HAVE_IMGUI
        // Demo information window
        ImGui::Begin("Widget Framework Demo Info");
        
        ImGui::Text("Welcome to the Bolt Widget Framework Demo!");
        ImGui::Separator();
        
        ImGui::Text("Framework Status:");
        ImGui::BulletText("Framework Initialized: %s", WidgetFrameworkInit::isInitialized() ? "Yes" : "No");
        
        auto registeredTypes = WidgetRegistration::getRegisteredTypes();
        ImGui::BulletText("Registered Widget Types: %zu", registeredTypes.size());
        
        if (ImGui::TreeNode("Widget Types")) {
            for (const auto& type : registeredTypes) {
                ImGui::BulletText("%s", type.c_str());
            }
            ImGui::TreePop();
        }
        
        auto& themeManager = ThemeManager::getInstance();
        auto themeNames = themeManager.getThemeNames();
        ImGui::BulletText("Available Themes: %zu", themeNames.size());
        
        if (themeManager.getCurrentTheme()) {
            ImGui::BulletText("Current Theme: %s", themeManager.getCurrentTheme()->getName().c_str());
        }
        
        ImGui::Separator();
        ImGui::Text("Output Log:");
        
        // Output text area
        ImGui::BeginChild("output", ImVec2(0, 200), true);
        ImGui::TextUnformatted(outputText_.c_str());
        
        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
        
        if (ImGui::Button("Clear Output")) {
            outputText_.clear();
        }
        
        ImGui::End();
#endif
    }
};

int main() {
    std::cout << "🚀 Starting Bolt Widget Framework Demo..." << std::endl;
    
    WidgetFrameworkDemo demo;
    
    if (!demo.initialize()) {
        std::cerr << "❌ Failed to initialize demo" << std::endl;
        return -1;
    }
    
    std::cout << "✅ Demo initialized successfully" << std::endl;
    std::cout << "🎮 Running demo - close window to exit" << std::endl;
    
    demo.run();
    demo.shutdown();
    
    std::cout << "👋 Demo finished" << std::endl;
    return 0;
}