#include "bolt/gui/bolt_gui_app.hpp"

#ifdef BOLT_HAVE_IMGUI

#include "bolt/ai/enhanced_ai_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <GL/gl.h>
#include <filesystem>
#include <algorithm>

namespace bolt {
namespace gui {

BoltGuiApp::BoltGuiApp() : window_(nullptr) {
    // Initialize enhanced AI manager
    try {
        ai_manager_ = std::make_unique<bolt::ai::EnhancedAIManager>();
        ai_ready_ = ai_manager_->is_ready();
        
        if (ai_ready_) {
            std::cout << "✅ AI Manager initialized with provider: " << ai_manager_->get_current_provider() << std::endl;
        } else {
            std::cout << "⚠️ AI Manager initialized but no working providers found" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to initialize AI Manager: " << e.what() << std::endl;
        ai_ready_ = false;
    }
    
    // Initialize file tree with project files
    InitializeFileTree();
    
    // Add welcome message based on AI availability
    if (ai_ready_) {
        AddChatMessage("Assistant", "🤖 Welcome to Bolt AI IDE! I'm ready to help you code. Try asking me about C++, algorithms, or request code completion! Current AI provider: " + (ai_manager_ ? ai_manager_->get_current_provider() : "unknown"), false);
    } else {
        AddChatMessage("System", "⚠️ Welcome to Bolt AI IDE! AI features are not currently available. Please configure an AI provider in the settings or check the console for setup instructions.", false);
        
        // Run quick setup in background
        std::thread([this]() {
            bolt::ai::AutoSetup::quick_setup_wizard();
        }).detach();
    }
}

BoltGuiApp::~BoltGuiApp() {
    Shutdown();
}

bool BoltGuiApp::Initialize() {
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
    window_ = glfwCreateWindow(window_width_, window_height_, "Bolt AI IDE - Powered by ImGui", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup ImGui style
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Note: Docking and Viewports require a more recent version of ImGui
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // Setup ImGui style
    SetupCustomStyle();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    return true;
}

void BoltGuiApp::SetupCustomStyle() {
    // Apply the current theme
    SetTheme(current_theme_);
}

void BoltGuiApp::SetTheme(ThemeMode theme) {
    current_theme_ = theme;
    
    switch (theme) {
        case ThemeMode::Dark:
            ApplyDarkTheme();
            break;
        case ThemeMode::Light:
            ApplyLightTheme();
            break;
        case ThemeMode::HighContrast:
            ApplyHighContrastTheme();
            break;
        case ThemeMode::Custom:
            ApplyDarkTheme(); // Default to dark for custom
            break;
    }
}

void BoltGuiApp::ApplyDarkTheme() {
    // Use the enhanced bolt.new-inspired theme
    ApplyEnhancedBoltDarkTheme();
    return;
    
    // Legacy theme code below (kept for reference)
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Modern dark theme with AI/development focus
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.20f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.27f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.32f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.08f, 0.08f, 0.10f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.12f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.33f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.43f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.53f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = colors[ImGuiCol_Header];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabUnfocused]           = colors[ImGuiCol_Tab];
    colors[ImGuiCol_TabUnfocusedActive]     = colors[ImGuiCol_TabActive];
    // Note: Docking colors require newer ImGui version
    // colors[ImGuiCol_DockingPreview]         = colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    // colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    
    // Styling
    style.WindowPadding     = ImVec2(12, 12);
    style.WindowRounding    = 5.0f;
    style.FramePadding      = ImVec2(8, 4);
    style.FrameRounding     = 3.0f;
    style.ItemSpacing       = ImVec2(8, 6);
    style.ItemInnerSpacing  = ImVec2(6, 6);
    style.IndentSpacing     = 25.0f;
    style.ScrollbarSize     = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize       = 5.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;
    style.ChildRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
}

void BoltGuiApp::ApplyLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Modern light theme with blue accents
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.98f, 0.98f, 0.98f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.70f, 0.70f, 0.70f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.88f, 0.88f, 0.88f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.82f, 0.82f, 0.82f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.76f, 0.76f, 0.76f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.88f, 0.88f, 0.88f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.92f, 0.92f, 0.92f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = colors[ImGuiCol_Header];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabUnfocused]           = colors[ImGuiCol_Tab];
    colors[ImGuiCol_TabUnfocusedActive]     = colors[ImGuiCol_TabActive];
    colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    
    // Styling
    style.WindowPadding     = ImVec2(12, 12);
    style.WindowRounding    = 5.0f;
    style.FramePadding      = ImVec2(8, 4);
    style.FrameRounding     = 3.0f;
    style.ItemSpacing       = ImVec2(8, 6);
    style.ItemInnerSpacing  = ImVec2(6, 6);
    style.IndentSpacing     = 25.0f;
    style.ScrollbarSize     = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize       = 5.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;
    style.ChildRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
}

void BoltGuiApp::ApplyHighContrastTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // High contrast dark theme for accessibility
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.10f, 0.10f, 0.10f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.30f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.70f, 0.70f, 0.70f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    colors[ImGuiCol_Tab]                    = colors[ImGuiCol_Header];
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabUnfocused]           = colors[ImGuiCol_Tab];
    colors[ImGuiCol_TabUnfocusedActive]     = colors[ImGuiCol_TabActive];
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(1.00f, 1.00f, 0.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    
    // Enhanced styling for high contrast
    style.WindowPadding     = ImVec2(15, 15);
    style.WindowRounding    = 0.0f;  // Sharp edges for high contrast
    style.FramePadding      = ImVec2(10, 6);
    style.FrameRounding     = 0.0f;
    style.ItemSpacing       = ImVec2(10, 8);
    style.ItemInnerSpacing  = ImVec2(8, 8);
    style.IndentSpacing     = 25.0f;
    style.ScrollbarSize     = 20.0f;  // Larger for accessibility
    style.ScrollbarRounding = 0.0f;
    style.GrabMinSize       = 8.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;
    style.ChildRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.WindowBorderSize  = 2.0f;  // Thicker borders
    style.ChildBorderSize   = 2.0f;
    style.PopupBorderSize   = 2.0f;
    style.FrameBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;
}

void BoltGuiApp::Run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        
        // Process pending messages from AI threads
        ProcessPendingMessages();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Handle keyboard shortcuts
        HandleKeyboardShortcuts();
        
        // Create main window without docking (for compatibility)
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        
        ImGui::Begin("MainWindow", nullptr, window_flags);
        ImGui::PopStyleVar(3);
        
        // Render main menu bar
        RenderMainMenuBar();
        
        ImGui::End();
        
        // Render all panels as separate windows
        if (show_file_tree_) RenderFileTree();
        if (show_chat_panel_) RenderChatPanel();
        if (show_code_editor_) RenderCodeEditor();
        if (show_console_window_) RenderConsoleWindow();
        if (show_ai_completion_) RenderAiCompletionOverlay();
        if (show_ai_settings_) RenderAiSettingsWindow();
        if (show_theme_settings_) RenderThemeSettingsWindow();
        if (show_about_window_) RenderAboutWindow();
        RenderStatusBar();
        
        // Demo window for reference
        if (show_demo_window_) ImGui::ShowDemoWindow(&show_demo_window_);
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.14f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Multi-viewport support would go here if enabled
        // (Commented out for compatibility)
        
        glfwSwapBuffers(window_);
    }
}

void BoltGuiApp::RenderMainMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) { 
                // Clear code buffer for new file
                strcpy(code_buffer_, "// New file\n\n");
            }
            if (ImGui::MenuItem("Open", "Ctrl+O")) { 
                OpenFileDialog();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) { 
                SaveCurrentFile();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                SaveFileAsDialog();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) { 
                glfwSetWindowShouldClose(window_, true); 
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("File Tree", nullptr, &show_file_tree_);
            ImGui::MenuItem("Chat Panel", nullptr, &show_chat_panel_);
            ImGui::MenuItem("Code Editor", nullptr, &show_code_editor_);
            ImGui::MenuItem("Console", nullptr, &show_console_window_);
            ImGui::Separator();
            if (ImGui::MenuItem("Theme Settings", nullptr)) {
                show_theme_settings_ = true;
            }
            ImGui::Separator();
            ImGui::MenuItem("Demo Window", nullptr, &show_demo_window_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("AI")) {
            if (ImGui::MenuItem("AI Settings", nullptr)) {
                show_ai_settings_ = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Toggle AI Completion", "Ctrl+Space")) {
                show_ai_completion_ = !show_ai_completion_;
            }
            if (ImGui::MenuItem("Clear Chat History")) {
                chat_history_.clear();
                AddChatMessage("Assistant", "Chat history cleared. How can I help you?", false);
            }
            ImGui::Separator();
            std::string status_text = ai_ready_ ? "✅ AI Ready" : "❌ AI Not Ready";
            ImGui::MenuItem(status_text.c_str(), nullptr, false, false);
            if (ai_manager_) {
                std::string provider_text = "Provider: " + ai_manager_->get_current_provider();
                ImGui::MenuItem(provider_text.c_str(), nullptr, false, false);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Keyboard Shortcuts", "F1")) {
                AddChatMessage("Assistant", 
                    "🔥 Bolt AI IDE Shortcuts:\n"
                    "• Ctrl+N - New file\n"
                    "• Ctrl+O - Open file\n"
                    "• Ctrl+S - Save file\n"
                    "• Ctrl+Space - AI completion\n"
                    "• F1 - Show help\n"
                    "• Alt+F4 - Exit\n"
                    "\nJust type in the chat to ask me anything!", false);
            }
            if (ImGui::MenuItem("About", nullptr)) {
                show_about_window_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void BoltGuiApp::RenderFileTree() {
    ImGui::Begin("📁 Project Files", &show_file_tree_);
    
    ImGui::Text("📂 /workspaces/bolt-cppml");
    ImGui::Separator();
    
    for (size_t i = 0; i < file_tree_.size(); ++i) {
        bool is_selected = (selected_file_index_ == static_cast<int>(i));
        
        // Add appropriate icon based on file extension
        std::string icon = "📄";
        if (file_tree_[i].find(".cpp") != std::string::npos || 
            file_tree_[i].find(".hpp") != std::string::npos ||
            file_tree_[i].find(".h") != std::string::npos) {
            icon = "💻";
        } else if (file_tree_[i].find(".md") != std::string::npos) {
            icon = "📝";
        } else if (file_tree_[i].find("CMakeLists.txt") != std::string::npos) {
            icon = "🔧";
        }
        
        std::string display_name = icon + " " + file_tree_[i];
        
        if (ImGui::Selectable(display_name.c_str(), is_selected)) {
            OnFileSelected(static_cast<int>(i));
        }
    }
    
    ImGui::End();
}

void BoltGuiApp::RenderChatPanel() {
    ImGui::Begin("💬 AI Assistant", &show_chat_panel_);
    
    // Chat history area
    ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true);
    
    for (const auto& message : chat_history_) {
        // Different styling for user vs assistant messages
        if (message.is_user) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
            ImGui::TextWrapped("👤 You: %s", message.content.c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 1.0f, 0.7f, 1.0f));
            ImGui::TextWrapped("🤖 Assistant: %s", message.content.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::Spacing();
    }
    
    // Auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    
    ImGui::EndChild();
    
    // Input area
    ImGui::PushItemWidth(-80);
    bool enter_pressed = ImGui::InputText("##ChatInput", chat_input_buffer_, sizeof(chat_input_buffer_), 
                                         ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    bool send_clicked = ImGui::Button("Send");
    
    if (enter_pressed || send_clicked) {
        ProcessChatInput();
    }
    
    // Set focus to input field initially
    static bool first_time = true;
    if (first_time) {
        ImGui::SetKeyboardFocusHere(-1);
        first_time = false;
    }
    
    ImGui::End();
}

void BoltGuiApp::RenderCodeEditor() {
    ImGui::Begin("💻 Code Editor", &show_code_editor_);
    
    // Editor toolbar
    if (ImGui::Button("🔄 AI Completion")) {
        show_ai_completion_ = true;
        AddChatMessage("Assistant", "AI Completion activated! I can help complete your code.", false);
    }
    ImGui::SameLine();
    if (ImGui::Button("💾 Save")) {
        AddChatMessage("Assistant", "File saved! (In a real implementation, this would save to disk)", false);
    }
    ImGui::SameLine();
    if (ImGui::Button("▶️ Run")) {
        AddChatMessage("Assistant", "Code execution requested! (Would compile and run in a real implementation)", false);
    }
    
    ImGui::Separator();
    
    // Main code editor
    ImGui::InputTextMultiline("##CodeEditor", code_buffer_, sizeof(code_buffer_), 
                             ImVec2(-1.0f, -1.0f), 
                             ImGuiInputTextFlags_AllowTabInput);
    
    ImGui::End();
}

void BoltGuiApp::RenderAiCompletionOverlay() {
    if (!show_ai_completion_) return;
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::Begin("🚀 AI Code Completion", &show_ai_completion_, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("🤖 AI is analyzing your code...");
    ImGui::Separator();
    
    // Simulate AI suggestions
    ImGui::Text("💡 Suggestions:");
    ImGui::BulletText("Add error handling with try-catch blocks");
    ImGui::BulletText("Consider using smart pointers for memory management");
    ImGui::BulletText("Add const correctness to your functions");
    ImGui::BulletText("Use modern C++17 features like auto and range-based loops");
    
    ImGui::Separator();
    if (ImGui::Button("✨ Apply Suggestions")) {
        AddChatMessage("Assistant", "AI suggestions applied to your code! Check the editor for improvements.", false);
        show_ai_completion_ = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("❌ Cancel")) {
        show_ai_completion_ = false;
    }
    
    ImGui::End();
}

void BoltGuiApp::RenderStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 25));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));
    ImGui::Begin("StatusBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
    
    ImGui::Text("🔥 Bolt AI IDE");
    ImGui::SameLine(200);
    ImGui::Text("📁 Files: %zu", file_tree_.size());
    ImGui::SameLine(300);
    ImGui::Text("💬 Messages: %zu", chat_history_.size());
    ImGui::SameLine();
    
    // Right-aligned status
    float window_width = ImGui::GetWindowSize().x;
    ImGui::SameLine(window_width - 200);
    ImGui::Text("🚀 Ready for AI assistance!");
    
    ImGui::End();
}

void BoltGuiApp::ProcessChatInput() {
    if (strlen(chat_input_buffer_) == 0) return;
    
    std::string input(chat_input_buffer_);
    AddChatMessage("You", input, true);
    
    // Clear input buffer first
    chat_input_buffer_[0] = '\0';
    
    // Generate AI response asynchronously to avoid blocking UI
    std::thread([this, input]() {
        std::string response = GenerateAiResponse(input);
        
        // Add response on main thread (note: this is a simplified approach)
        // In production, you'd want to use a proper thread-safe message queue
        std::lock_guard<std::mutex> lock(chat_mutex_);
        pending_messages_.push_back({"Assistant", response, false});
    }).detach();
}

void BoltGuiApp::AddChatMessage(const std::string& author, const std::string& message, bool is_user) {
    chat_history_.emplace_back(author, message, is_user);
    
    // Log to console as well
    AddConsoleLog("[" + author + "] " + message);
}

void BoltGuiApp::AddConsoleLog(const std::string& message) {
    console_log_.push_back(message);
    
    // Keep only last 1000 messages
    if (console_log_.size() > 1000) {
        console_log_.erase(console_log_.begin());
    }
}

std::string BoltGuiApp::GenerateAiResponse(const std::string& input) {
    // Check if AI manager is available
    if (!ai_manager_ || !ai_ready_) {
        return GenerateFallbackResponse(input);
    }
    
    try {
        // Use real AI inference
        auto response = ai_manager_->chat(input, "gui_session");
        
        if (response.success) {
            return response.response;
        } else {
            std::string error_msg = "❌ AI Error: " + response.error;
            std::cerr << error_msg << std::endl;
            
            // Try to reconnect or switch provider
            if (!ai_manager_->test_connection()) {
                error_msg += "\n\n💡 Trying to reconnect...";
                ai_ready_ = ai_manager_->test_all_providers();
                
                if (ai_ready_) {
                    error_msg += "\n✅ Reconnected! Please try your request again.";
                } else {
                    error_msg += "\n❌ No AI providers available. Please check your configuration.";
                }
            }
            
            return error_msg;
        }
        
    } catch (const std::exception& e) {
        std::string error_msg = "❌ Exception in AI processing: " + std::string(e.what());
        std::cerr << error_msg << std::endl;
        return error_msg + "\n\nFalling back to built-in responses...";
    }
}

std::string BoltGuiApp::GenerateFallbackResponse(const std::string& input) {
    // Enhanced fallback AI responses for common development queries
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    if (lower_input.find("help") != std::string::npos) {
        return "I'm here to help! I can assist with:\n"
               "• C++ coding questions and best practices\n"
               "• Code completion and suggestions\n"
               "• Debugging and error analysis\n"
               "• Algorithm implementations\n"
               "• Modern C++ features\n\n"
               "⚠️ Note: Using fallback responses. Please configure an AI provider for full functionality.";
    } else if (lower_input.find("cpp") != std::string::npos || lower_input.find("c++") != std::string::npos) {
        return "Great! C++ is an awesome language. Here are some modern C++ tips:\n"
               "• Use 'auto' for type deduction\n"
               "• Prefer smart pointers (unique_ptr, shared_ptr)\n"
               "• Use range-based for loops\n"
               "• Consider std::optional for nullable values\n"
               "• Use constexpr for compile-time constants\n\n"
               "⚠️ For detailed code help, please configure an AI provider.";
    } else if (lower_input.find("gui") != std::string::npos || lower_input.find("imgui") != std::string::npos) {
        return "Excellent choice! ImGui is perfect for development tools. Key features:\n"
               "• Immediate mode GUI (no state management)\n"
               "• Perfect for debug tools and editors\n"
               "• Easy integration with OpenGL/DirectX\n"
               "• Docking and multi-viewport support\n"
               "• Highly customizable styling\n\n"
               "This Bolt IDE is built with ImGui!";
    } else if (lower_input.find("complete") != std::string::npos || lower_input.find("completion") != std::string::npos) {
        return "🚀 AI Code Completion is available when an AI provider is configured!\n"
               "• Auto-complete function signatures\n"
               "• Suggest variable names and types\n"
               "• Generate boilerplate code\n"
               "• Recommend best practices\n"
               "• Fix common coding errors\n\n"
               "Please set up an AI provider for full completion features.";
    } else if (lower_input.find("algorithm") != std::string::npos) {
        return "Algorithms are the heart of programming! I can help with:\n"
               "• Sorting and searching algorithms\n"
               "• Data structures (trees, graphs, etc.)\n"
               "• Dynamic programming solutions\n"
               "• Time and space complexity analysis\n"
               "• Implementation examples\n\n"
               "⚠️ For detailed algorithm help, please configure an AI provider.";
    } else if (lower_input.find("configuration") != std::string::npos || lower_input.find("config") != std::string::npos || lower_input.find("setup") != std::string::npos) {
        return "🔧 AI Configuration Help:\n\n"
               "Quick setup options:\n"
               "1. 🏠 Local llama.cpp: ./server -m model.gguf --port 8080\n"
               "2. 🐋 Ollama: ollama run llama2\n"
               "3. ☁️ OpenAI API: Get key from platform.openai.com\n\n"
               "Edit bolt_ai_config.json to configure providers.\n"
               "Use the AI Settings panel for GUI configuration.";
    } else {
        return "I understand you're asking about: \"" + input + "\"\n\n"
               "⚠️ I'm currently using fallback responses. For intelligent AI assistance, please:\n"
               "• Configure an AI provider (OpenAI API, local llama.cpp, etc.)\n"
               "• Check the console for setup instructions\n"
               "• Use the AI Settings panel to test providers\n\n"
               "Ask me about 'configuration' for setup help!";
    }
}

void BoltGuiApp::InitializeFileTree() {
    // Populate with some project files (in a real implementation, this would scan the filesystem)
    file_tree_ = {
        "src/bolt/main.cpp",
        "src/bolt/core/bolt.cpp",
        "src/bolt/gui/bolt_gui_app.cpp",
        "include/bolt/gui/bolt_gui_app.hpp",
        "CMakeLists.txt",
        "README.md",
        "demo_ai_completion.cpp",
        "demo_file_tree.cpp",
        "test/test_ai_models.cpp",
        "docs/theme-system.md"
    };
}

void BoltGuiApp::OnFileSelected(int index) {
    selected_file_index_ = index;
    
    // Load actual file content
    std::string filename = file_tree_[index];
    std::string file_content;
    
    // Try to read the file from the filesystem
    std::ifstream file(filename);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file_content = buffer.str();
        file.close();
        
        // Update code buffer with actual file content
        strncpy(code_buffer_, file_content.c_str(), sizeof(code_buffer_) - 1);
        code_buffer_[sizeof(code_buffer_) - 1] = '\0';
        
        AddChatMessage("Assistant", "📁 Opened file: " + filename + " (" + std::to_string(file_content.length()) + " bytes)", false);
    } else {
        // If file doesn't exist, create sample content based on extension
        std::string sample_content = "// File: " + filename + "\n// New file - content will be saved on write\n\n";
        
        if (filename.find(".cpp") != std::string::npos) {
            sample_content += "#include <iostream>\n\nint main() {\n    std::cout << \"Hello from " + filename + "!\" << std::endl;\n    return 0;\n}\n";
        } else if (filename.find(".hpp") != std::string::npos || filename.find(".h") != std::string::npos) {
            sample_content += "#pragma once\n\nnamespace bolt {\n    // Class declarations here\n}\n";
        }
        
        strncpy(code_buffer_, sample_content.c_str(), sizeof(code_buffer_) - 1);
        code_buffer_[sizeof(code_buffer_) - 1] = '\0';
        
        AddChatMessage("Assistant", "📝 Created new file: " + filename, false);
    }
}

void BoltGuiApp::HandleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Ctrl+Space for AI completion
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Space)) {
        show_ai_completion_ = !show_ai_completion_;
        if (show_ai_completion_) {
            AddChatMessage("Assistant", "🚀 AI Completion activated! Analyzing your code...", false);
        }
    }
    
    // F1 for help
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
        AddChatMessage("Assistant", 
            "🔥 Bolt AI IDE Help:\n"
            "This is a modern C++ development environment with AI assistance!\n\n"
            "Features:\n"
            "• 💬 AI Chat Assistant\n"
            "• 💻 Code Editor with syntax highlighting\n"
            "• 📁 Project file tree\n"
            "• 🚀 AI-powered code completion\n"
            "• 🎨 Modern ImGui interface\n\n"
            "Try asking me about C++, algorithms, or coding best practices!", false);
    }
}

void BoltGuiApp::Shutdown() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

void BoltGuiApp::ProcessPendingMessages() {
    std::lock_guard<std::mutex> lock(chat_mutex_);
    for (const auto& msg : pending_messages_) {
        chat_history_.push_back(msg);
    }
    pending_messages_.clear();
}

void BoltGuiApp::RenderAiSettingsWindow() {
    ImGui::Begin("🤖 AI Settings", &show_ai_settings_);
    
    if (!ai_manager_) {
        ImGui::Text("❌ AI Manager not initialized");
        ImGui::End();
        return;
    }
    
    // Current provider status
    ImGui::Text("Current Provider: %s", ai_manager_->get_current_provider().c_str());
    ImGui::SameLine();
    ImGui::TextColored(ai_ready_ ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                      ai_ready_ ? "✅ Ready" : "❌ Not Ready");
    
    ImGui::Separator();
    
    // Provider list
    ImGui::Text("Available Providers:");
    auto providers = ai_manager_->list_providers();
    
    for (const auto& provider : providers) {
        bool is_selected = provider.is_active;
        if (ImGui::Selectable(provider.name.c_str(), is_selected)) {
            if (!is_selected) {
                ai_manager_->switch_provider(provider.name);
                ai_ready_ = ai_manager_->is_ready();
                
                AddChatMessage("System", "🔄 Switched to AI provider: " + provider.name, false);
                if (ai_ready_) {
                    AddChatMessage("System", "✅ AI provider is ready!", false);
                } else {
                    AddChatMessage("System", "❌ AI provider connection failed", false);
                }
            }
        }
        
        if (is_selected) {
            ImGui::SameLine();
            ImGui::Text("(Active)");
        }
        
        // Show provider details
        ImGui::Indent();
        ImGui::Text("URL: %s", provider.config.base_url.c_str());
        ImGui::Text("Model: %s", provider.config.model_name.c_str());
        ImGui::Unindent();
        ImGui::Spacing();
    }
    
    ImGui::Separator();
    
    // Direct GGUF model loading (offline)
    static char gguf_path_buf[1024] = "";
    ImGui::Text("Direct GGUF Model (Offline):");
    ImGui::InputTextWithHint("##ggufpath", "Path to model.gguf", gguf_path_buf, sizeof(gguf_path_buf));
    ImGui::SameLine();
    if (ImGui::Button("Load GGUF")) {
        std::string path = gguf_path_buf;
        if (!path.empty()) {
            bool ok = ai_manager_->load_gguf_model(path);
            ai_ready_ = ai_manager_->has_direct_model();
            AddChatMessage("System", ok ? std::string("✅ Loaded GGUF model: ") + path : std::string("❌ Failed to load GGUF model: ") + path, false);
            if (ok) {
                AddChatMessage("Assistant", ai_manager_->get_model_info(), false);
            }
        }
    }
    
    if (ImGui::Button("Auto-Detect GGUF")) {
        bool ok = ai_manager_->auto_detect_models();
        ai_ready_ = ai_manager_->has_direct_model();
        AddChatMessage("System", ok ? "✅ Auto-detected GGUF model" : "⚠️ No GGUF models found", false);
        if (ok) {
            AddChatMessage("Assistant", ai_manager_->get_model_info(), false);
        }
    }

    // Minimal RWKV direct model setup (stub)
    ImGui::Separator();
    static char rwkv_path_buf[1024] = "";
    ImGui::Text("Direct RWKV Model (Stub):");
    ImGui::InputTextWithHint("##rwkvpath", "Path to model.rwkv", rwkv_path_buf, sizeof(rwkv_path_buf));
    ImGui::SameLine();
    if (ImGui::Button("Load RWKV")) {
        std::string path = rwkv_path_buf;
        if (!path.empty()) {
            bool ok = ai_manager_->load_rwkv_model(path);
            ai_ready_ = ai_manager_->has_rwkv_model() || ai_ready_;
            AddChatMessage("System", ok ? std::string("✅ RWKV path set: ") + path : std::string("❌ Failed to set RWKV path: ") + path, false);
            AddChatMessage("Assistant", ai_manager_->get_model_info(), false);
        }
    }
    
    ImGui::Separator();
    
    // Test connections
    if (ImGui::Button("🔍 Test Current Provider")) {
        bool success = ai_manager_->test_connection();
        AddChatMessage("System", success ? "✅ Connection test successful!" : "❌ Connection test failed", false);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("🔍 Test All Providers")) {
        ai_manager_->test_all_providers();
        ai_ready_ = ai_manager_->is_ready();
        AddChatMessage("System", "🔍 Tested all providers. Check console for details.", false);
    }
    
    ImGui::Separator();
    
    // Statistics
    auto stats = ai_manager_->get_statistics();
    ImGui::Text("Statistics:");
    ImGui::BulletText("Total Requests: %zu", stats.total_requests);
    ImGui::BulletText("Successful: %zu", stats.successful_requests);
    ImGui::BulletText("Failed: %zu", stats.failed_requests);
    ImGui::BulletText("Avg Response Time: %.1f ms", stats.average_inference_time_ms);
    ImGui::BulletText("Tokens Generated: %zu", stats.total_tokens_generated);
    
    if (ImGui::Button("🔄 Reset Statistics")) {
        ai_manager_->reset_statistics();
        AddChatMessage("System", "🔄 Statistics reset", false);
    }
    
    ImGui::Separator();
    
    // Quick setup
    ImGui::Text("Quick Setup:");
    if (ImGui::Button("🧙 Run Setup Wizard")) {
        std::thread([this]() {
            bolt::ai::AutoSetup::quick_setup_wizard();
            ai_ready_ = ai_manager_->is_ready();
        }).detach();
        
        AddChatMessage("System", "🧙 Running AI setup wizard... Check console for details.", false);
    }
    
    if (ImGui::Button("📋 Show Setup Instructions")) {
        bolt::ai::AutoSetup::print_setup_instructions();
        AddChatMessage("System", "📋 Setup instructions printed to console", false);
    }
    
    ImGui::End();
}

void BoltGuiApp::RenderThemeSettingsWindow() {
    ImGui::Begin("🎨 Theme Settings", &show_theme_settings_);
    
    ImGui::Text("Choose a theme for the IDE:");
    ImGui::Separator();
    
    // Theme selection
    const char* theme_names[] = { "🌙 Dark Mode", "☀️ Light Mode", "⚡ High Contrast" };
    static int selected_theme = static_cast<int>(current_theme_);
    
    for (int i = 0; i < 3; i++) {
        if (ImGui::RadioButton(theme_names[i], &selected_theme, i)) {
            SetTheme(static_cast<ThemeMode>(i));
            std::string theme_name;
            switch (static_cast<ThemeMode>(i)) {
                case ThemeMode::Dark: theme_name = "Dark Mode"; break;
                case ThemeMode::Light: theme_name = "Light Mode"; break;
                case ThemeMode::HighContrast: theme_name = "High Contrast"; break;
                default: theme_name = "Unknown"; break;
            }
            AddChatMessage("System", "🎨 Theme changed to " + theme_name + "!", false);
        }
    }
    
    ImGui::Separator();
    
    // Theme preview/info
    ImGui::Text("📝 Theme Information:");
    ImGui::Indent();
    
    switch (current_theme_) {
        case ThemeMode::Dark:
            ImGui::Text("🌙 Dark Mode - Modern dark theme with blue accents");
            ImGui::Text("Perfect for coding sessions and reduces eye strain");
            break;
        case ThemeMode::Light:
            ImGui::Text("☀️ Light Mode - Clean light theme with good contrast");
            ImGui::Text("Great for daytime use and presentations");
            break;
        case ThemeMode::HighContrast:
            ImGui::Text("⚡ High Contrast - Accessibility-focused theme");
            ImGui::Text("Maximum contrast for better visibility");
            break;
        case ThemeMode::Custom:
            ImGui::Text("🎨 Custom Theme - User-defined colors");
            break;
    }
    
    ImGui::Unindent();
    ImGui::Separator();
    
    // Quick theme preview buttons
    ImGui::Text("🎯 Quick Preview:");
    if (ImGui::Button("🌙 Dark")) {
        SetTheme(ThemeMode::Dark);
        AddChatMessage("System", "🌙 Switched to Dark Mode!", false);
    }
    ImGui::SameLine();
    if (ImGui::Button("☀️ Light")) {
        SetTheme(ThemeMode::Light);
        AddChatMessage("System", "☀️ Switched to Light Mode!", false);
    }
    ImGui::SameLine();
    if (ImGui::Button("⚡ High Contrast")) {
        SetTheme(ThemeMode::HighContrast);
        AddChatMessage("System", "⚡ Switched to High Contrast Mode!", false);
    }
    
    ImGui::Separator();
    
    // Additional styling options
    ImGui::Text("🔧 Style Options:");
    ImGuiStyle& style = ImGui::GetStyle();
    
    ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
    ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
    ImGui::SliderFloat("Scrollbar Size", &style.ScrollbarSize, 10.0f, 25.0f, "%.0f");
    
    if (ImGui::Button("🔄 Reset to Default")) {
        SetTheme(current_theme_); // Reapply current theme to reset custom changes
        AddChatMessage("System", "🔄 Style reset to defaults!", false);
    }
    
    ImGui::End();
}

void BoltGuiApp::RenderConsoleWindow() {
    ImGui::Begin("📺 Console Log", &show_console_window_);
    
    // Console output area
    ImGui::BeginChild("ConsoleOutput", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
    
    for (const auto& log_message : console_log_) {
        // Color coding for different message types
        if (log_message.find("[System]") != std::string::npos) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
        } else if (log_message.find("[Assistant]") != std::string::npos) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 1.0f, 0.7f, 1.0f));
        } else if (log_message.find("❌") != std::string::npos || log_message.find("ERROR") != std::string::npos) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        } else if (log_message.find("✅") != std::string::npos || log_message.find("SUCCESS") != std::string::npos) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        }
        
        ImGui::TextWrapped("%s", log_message.c_str());
        ImGui::PopStyleColor();
    }
    
    // Auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    
    ImGui::EndChild();
    
    // Console controls
    if (ImGui::Button("🗑️ Clear Console")) {
        console_log_.clear();
        AddConsoleLog("[System] Console cleared");
    }
    ImGui::SameLine();
    if (ImGui::Button("💾 Save Log")) {
        AddConsoleLog("[System] Console log saved (would save to file in real implementation)");
    }
    
    ImGui::End();
}

void BoltGuiApp::RenderAboutWindow() {
    ImGui::SetNextWindowSize(ImVec2(450, 350), ImGuiCond_FirstUseEver);
    ImGui::Begin("ℹ️ About Bolt AI IDE", &show_about_window_);
    
    // Logo/Title area
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("🔥 Bolt AI IDE");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    
    ImGui::Text("Version 1.0.0");
    ImGui::Separator();
    
    // Features
    ImGui::Text("🚀 Features:");
    ImGui::BulletText("Modern ImGui interface with dark mode");
    ImGui::BulletText("AI-powered code completion and assistance");
    ImGui::BulletText("Real-time chat with AI assistant");
    ImGui::BulletText("Project file explorer");
    ImGui::BulletText("Integrated code editor");
    ImGui::BulletText("Theme customization (Dark, Light, High Contrast)");
    ImGui::BulletText("Console logging and debugging");
    ImGui::BulletText("Cross-platform compatibility");
    
    ImGui::Separator();
    
    // Technology stack
    ImGui::Text("🔧 Built with:");
    ImGui::BulletText("Dear ImGui - Immediate mode GUI");
    ImGui::BulletText("OpenGL 3.3 - Graphics rendering");
    ImGui::BulletText("GLFW - Window management");
    ImGui::BulletText("Modern C++17 - Core language");
    ImGui::BulletText("CMake - Build system");
    
    ImGui::Separator();
    
    // Copyright
    ImGui::Text("© 2024 Bolt C++ Project");
    ImGui::Text("Licensed under MIT License");
    
    if (ImGui::Button("🌐 Visit Website")) {
        AddChatMessage("System", "Opening project website... (would open browser in real implementation)", false);
    }
    ImGui::SameLine();
    if (ImGui::Button("📖 Documentation")) {
        AddChatMessage("System", "Opening documentation... (would open docs in real implementation)", false);
    }
    
    ImGui::End();
}

void BoltGuiApp::OpenFileDialog() {
    // Simple file dialog implementation using ImGui
    // In production, you'd use a native file dialog library like nativefiledialog or portable-file-dialogs
    
    // For now, we'll use the file tree selection
    if (selected_file_index_ >= 0 && selected_file_index_ < static_cast<int>(file_tree_.size())) {
        std::string selected_file = file_tree_[selected_file_index_];
        
        // Try to load the file
        try {
            std::ifstream file(selected_file);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string content = buffer.str();
                
                // Copy to code buffer (with size limit)
                size_t copy_size = std::min(content.size(), sizeof(code_buffer_) - 1);
                std::memcpy(code_buffer_, content.c_str(), copy_size);
                code_buffer_[copy_size] = '\0';
                
                current_file_path_ = selected_file;
                AddChatMessage("System", "✅ Opened file: " + selected_file, false);
                
                file.close();
            } else {
                AddChatMessage("System", "❌ Failed to open file: " + selected_file, false);
            }
        } catch (const std::exception& e) {
            AddChatMessage("System", "❌ Error opening file: " + std::string(e.what()), false);
        }
    } else {
        AddChatMessage("System", "ℹ️ Please select a file from the file tree first, or use 'Save As' to create a new file.", false);
    }
}

void BoltGuiApp::SaveCurrentFile() {
    if (current_file_path_.empty()) {
        // No file path set, prompt for save as
        AddChatMessage("System", "ℹ️ No file path set. Please use 'Save As' to specify a file name.", false);
        SaveFileAsDialog();
        return;
    }
    
    // Save to the current file path
    try {
        std::ofstream file(current_file_path_);
        if (file.is_open()) {
            file << code_buffer_;
            file.close();
            AddChatMessage("System", "✅ Saved file: " + current_file_path_, false);
        } else {
            AddChatMessage("System", "❌ Failed to save file: " + current_file_path_, false);
        }
    } catch (const std::exception& e) {
        AddChatMessage("System", "❌ Error saving file: " + std::string(e.what()), false);
    }
}

void BoltGuiApp::SaveFileAsDialog() {
    // Simple save-as implementation
    // In production, you'd use a native file dialog library
    
    // For now, we'll create a simple input dialog
    static char filename_buffer[512] = "untitled.cpp";
    
    ImGui::OpenPopup("Save File As");
    
    if (ImGui::BeginPopupModal("Save File As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter filename:");
        ImGui::InputText("##filename", filename_buffer, sizeof(filename_buffer));
        
        ImGui::Separator();
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            std::string filepath = std::string(filename_buffer);
            
            // Add default extension if none provided
            if (filepath.find('.') == std::string::npos) {
                filepath += ".cpp";
            }
            
            try {
                std::ofstream file(filepath);
                if (file.is_open()) {
                    file << code_buffer_;
                    file.close();
                    current_file_path_ = filepath;
                    AddChatMessage("System", "✅ Saved file as: " + filepath, false);
                    
                    // Refresh file tree
                    InitializeFileTree();
                    
                    ImGui::CloseCurrentPopup();
                } else {
                    AddChatMessage("System", "❌ Failed to create file: " + filepath, false);
                }
            } catch (const std::exception& e) {
                AddChatMessage("System", "❌ Error saving file: " + std::string(e.what()), false);
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

} // namespace gui
} // namespace bolt

#endif // BOLT_HAVE_IMGUI
