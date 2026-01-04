#include "bolt/gui/dark_theme.hpp"

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#endif

namespace bolt {
namespace gui {

void ApplyBoltDarkTheme() {
#ifdef BOLT_HAVE_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Bolt.diy inspired dark theme colors
    // Background colors (dark slate/charcoal)
    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);  // #1a1a1f
    colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);  // #1e1e23
    colors[ImGuiCol_PopupBg]                = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);  // #33333d
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);  // #26262e
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    
    // Title bar (darker)
    colors[ImGuiCol_TitleBg]                = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);  // #141418
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.08f, 0.08f, 0.10f, 0.75f);
    
    // Menu bar
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    
    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    
    // Checkboxes and radio buttons
    colors[ImGuiCol_CheckMark]              = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);  // Blue accent
    
    // Sliders
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);
    
    // Buttons (primary action color - blue)
    colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.50f, 0.90f, 1.00f);  // #4080e6
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.60f, 1.00f, 1.00f);  // #5999ff
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.45f, 0.85f, 1.00f);  // #3373d9
    
    // Headers (for tree nodes, collapsing headers, etc.)
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.30f, 0.30f, 0.36f, 1.00f);
    
    // Separator
    colors[ImGuiCol_Separator]              = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.30f, 0.30f, 0.36f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.40f, 0.48f, 1.00f);
    
    // Resize grip
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.30f, 0.30f, 0.36f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.40f, 0.48f, 1.00f);
    
    // Tabs
    colors[ImGuiCol_Tab]                    = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.25f, 0.50f, 0.90f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.40f, 0.80f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.15f, 0.30f, 0.60f, 1.00f);
    
    // Docking (only if docking is enabled)
    #ifdef IMGUI_HAS_DOCK
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.25f, 0.50f, 0.90f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    #endif
    
    // Plot colors
    colors[ImGuiCol_PlotLines]              = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);
    
    // Table colors
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    
    // Text colors
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);  // Light gray
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.25f, 0.50f, 0.90f, 0.35f);
    
    // Drag and drop
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.40f, 0.70f, 1.00f, 0.90f);
    
    // Navigation
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    
    // Modal window dimming
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
    
    // Style adjustments for modern look
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
    
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
    
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(8.0f, 6.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 14.0f;
    style.GrabMinSize       = 10.0f;
#endif
}

void ApplyBoltLightTheme() {
#ifdef BOLT_HAVE_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Light theme colors (for optional light mode)
    colors[ImGuiCol_WindowBg]               = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.95f, 0.95f, 0.97f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.85f, 0.85f, 0.87f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);
    
    colors[ImGuiCol_TitleBg]                = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.88f, 0.88f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.92f, 0.92f, 0.94f, 0.75f);
    
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
    
    colors[ImGuiCol_Button]                 = ImVec4(0.25f, 0.50f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.60f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.45f, 0.85f, 1.00f);
    
    colors[ImGuiCol_Text]                   = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.52f, 1.00f);
    
    // Apply same style settings as dark theme
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
#endif
}

} // namespace gui
} // namespace bolt
