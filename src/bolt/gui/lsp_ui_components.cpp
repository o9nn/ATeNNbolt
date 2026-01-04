/**
 * @file lsp_ui_components.cpp
 * @brief Implementation of ImGui UI components for LSP features
 */

#include "bolt/gui/lsp_ui_components.hpp"

#ifdef BOLT_HAVE_IMGUI
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace bolt {
namespace gui {

// ============================================================================
// LspCompletionPopup Implementation
// ============================================================================

LspCompletionPopup::LspCompletionPopup() = default;
LspCompletionPopup::~LspCompletionPopup() = default;

void LspCompletionPopup::render(lsp::CompletionPopupState& state, float editorPosX, 
                                 float editorPosY, float lineHeight) {
    if (!state.isVisible || state.items.empty()) return;
    
    // Position popup below the cursor
    ImGui::SetNextWindowPos(ImVec2(editorPosX, editorPosY + lineHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popupWidth_, 0), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_AlwaysAutoResize;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.18f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    
    if (ImGui::Begin("##CompletionPopup", nullptr, flags)) {
        // Filter items based on filter text
        std::vector<const lsp::EditorCompletionItem*> filteredItems;
        for (const auto& item : state.items) {
            if (state.filterText.empty() || 
                item.label.find(state.filterText) != std::string::npos) {
                filteredItems.push_back(&item);
            }
        }
        
        // Limit visible items
        size_t visibleCount = std::min(filteredItems.size(), static_cast<size_t>(maxVisibleItems_));
        
        // Calculate scroll position to keep selected item visible
        int scrollStart = 0;
        if (state.selectedIndex >= maxVisibleItems_) {
            scrollStart = state.selectedIndex - maxVisibleItems_ + 1;
        }
        
        for (size_t i = scrollStart; i < scrollStart + visibleCount && i < filteredItems.size(); ++i) {
            const auto* item = filteredItems[i];
            bool isSelected = (static_cast<int>(i) == state.selectedIndex);
            
            // Get kind icon and color
            const char* icon = getKindIcon(item->kind);
            float r, g, b;
            getKindColor(item->kind, r, g, b);
            
            // Highlight selected item
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.8f));
            }
            
            // Render item
            ImGui::PushID(static_cast<int>(i));
            
            if (ImGui::Selectable("##item", isSelected, 0, ImVec2(0, itemHeight_))) {
                state.selectedIndex = static_cast<int>(i);
                if (acceptCallback_) {
                    acceptCallback_(item->insertText);
                }
            }
            
            // Draw icon and label on same line
            ImGui::SameLine(8);
            ImGui::TextColored(ImVec4(r, g, b, 1.0f), "%s", icon);
            ImGui::SameLine(30);
            ImGui::Text("%s", item->label.c_str());
            
            // Show detail on the right if available
            if (!item->detail.empty()) {
                float detailX = popupWidth_ - ImGui::CalcTextSize(item->detail.c_str()).x - 20;
                ImGui::SameLine(detailX);
                ImGui::TextDisabled("%s", item->detail.c_str());
            }
            
            ImGui::PopID();
            
            if (isSelected) {
                ImGui::PopStyleColor(2);
            }
        }
        
        // Show scroll indicator if there are more items
        if (filteredItems.size() > static_cast<size_t>(maxVisibleItems_)) {
            ImGui::Separator();
            ImGui::TextDisabled("  %zu of %zu items", visibleCount, filteredItems.size());
        }
    }
    ImGui::End();
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

bool LspCompletionPopup::handleInput(lsp::CompletionPopupState& state) {
    if (!state.isVisible) return false;
    
    ImGuiIO& io = ImGui::GetIO();
    
    // Arrow keys for navigation
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        state.selectedIndex = (state.selectedIndex + 1) % static_cast<int>(state.items.size());
        return true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        state.selectedIndex = (state.selectedIndex - 1 + static_cast<int>(state.items.size())) 
                              % static_cast<int>(state.items.size());
        return true;
    }
    
    // Enter/Tab to accept
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        if (acceptCallback_ && !state.items.empty()) {
            acceptCallback_(state.items[state.selectedIndex].insertText);
        }
        state.isVisible = false;
        return true;
    }
    
    // Escape to dismiss
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        state.isVisible = false;
        return true;
    }
    
    return false;
}

const char* LspCompletionPopup::getKindIcon(const std::string& kind) {
    if (kind == "function" || kind == "method") return "ƒ";
    if (kind == "variable" || kind == "field") return "𝑥";
    if (kind == "class" || kind == "struct") return "◆";
    if (kind == "interface") return "◇";
    if (kind == "module") return "□";
    if (kind == "property") return "●";
    if (kind == "enum") return "∈";
    if (kind == "enum_member") return "∋";
    if (kind == "constant") return "π";
    if (kind == "keyword") return "⌘";
    if (kind == "snippet") return "✂";
    if (kind == "text") return "T";
    if (kind == "file") return "📄";
    if (kind == "folder") return "📁";
    if (kind == "type_parameter") return "τ";
    return "•";
}

void LspCompletionPopup::getKindColor(const std::string& kind, float& r, float& g, float& b) {
    if (kind == "function" || kind == "method") {
        r = 0.6f; g = 0.4f; b = 0.8f;  // Purple
    } else if (kind == "variable" || kind == "field") {
        r = 0.4f; g = 0.7f; b = 0.9f;  // Light blue
    } else if (kind == "class" || kind == "struct") {
        r = 0.9f; g = 0.7f; b = 0.3f;  // Orange
    } else if (kind == "interface") {
        r = 0.3f; g = 0.8f; b = 0.6f;  // Teal
    } else if (kind == "keyword") {
        r = 0.9f; g = 0.4f; b = 0.5f;  // Pink
    } else if (kind == "constant" || kind == "enum_member") {
        r = 0.4f; g = 0.6f; b = 0.9f;  // Blue
    } else if (kind == "snippet") {
        r = 0.7f; g = 0.7f; b = 0.7f;  // Gray
    } else {
        r = 0.8f; g = 0.8f; b = 0.8f;  // Default light gray
    }
}

// ============================================================================
// LspHoverTooltip Implementation
// ============================================================================

LspHoverTooltip::LspHoverTooltip() = default;
LspHoverTooltip::~LspHoverTooltip() = default;

void LspHoverTooltip::render(const lsp::HoverTooltipState& state, float mouseX, float mouseY) {
    if (!state.isVisible || state.info.content.empty()) return;
    
    // Position tooltip near the mouse
    ImGui::SetNextWindowPos(ImVec2(mouseX + 10, mouseY + 10), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(100, 0), ImVec2(maxWidth_, 400));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoFocusOnAppearing;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding_, padding_));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
    
    if (ImGui::Begin("##HoverTooltip", nullptr, flags)) {
        // Render content with word wrapping
        ImGui::PushTextWrapPos(maxWidth_ - padding_ * 2);
        
        // Parse and render markdown-like content
        const std::string& content = state.info.content;
        
        // Simple markdown rendering - handle code blocks
        size_t pos = 0;
        while (pos < content.length()) {
            // Check for code block
            if (content.substr(pos, 3) == "```") {
                size_t endPos = content.find("```", pos + 3);
                if (endPos != std::string::npos) {
                    // Extract code block
                    size_t codeStart = content.find('\n', pos + 3);
                    if (codeStart != std::string::npos && codeStart < endPos) {
                        std::string code = content.substr(codeStart + 1, endPos - codeStart - 1);
                        
                        // Render code with monospace styling
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 0.7f, 1.0f));
                        ImGui::TextUnformatted(code.c_str());
                        ImGui::PopStyleColor();
                    }
                    pos = endPos + 3;
                    continue;
                }
            }
            
            // Find next special marker or end
            size_t nextMarker = content.find("```", pos);
            if (nextMarker == std::string::npos) nextMarker = content.length();
            
            // Render regular text
            std::string text = content.substr(pos, nextMarker - pos);
            if (!text.empty()) {
                ImGui::TextUnformatted(text.c_str());
            }
            
            pos = nextMarker;
        }
        
        ImGui::PopTextWrapPos();
    }
    ImGui::End();
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

// ============================================================================
// LspDiagnosticsGutter Implementation
// ============================================================================

LspDiagnosticsGutter::LspDiagnosticsGutter() = default;
LspDiagnosticsGutter::~LspDiagnosticsGutter() = default;

void LspDiagnosticsGutter::render(const std::vector<lsp::EditorDiagnostic>& diagnostics,
                                   float gutterX, size_t firstVisibleLine, size_t lastVisibleLine,
                                   float lineHeight, float scrollY) {
    if (diagnostics.empty()) return;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    for (const auto& diag : diagnostics) {
        // Check if diagnostic is in visible range
        if (diag.startLine < firstVisibleLine || diag.startLine > lastVisibleLine) {
            continue;
        }
        
        // Calculate Y position
        float y = (diag.startLine - firstVisibleLine) * lineHeight - scrollY;
        
        // Get color based on severity
        float r, g, b;
        getSeverityColor(diag.severity, r, g, b);
        ImU32 color = IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255), 
                               static_cast<int>(b * 255), 255);
        
        // Draw marker
        float centerX = gutterX + gutterWidth_ / 2;
        float centerY = y + lineHeight / 2;
        float radius = 4.0f;
        
        if (diag.severity == lsp::DiagnosticSeverity::Error) {
            // Filled circle for errors
            drawList->AddCircleFilled(ImVec2(centerX, centerY), radius, color);
        } else if (diag.severity == lsp::DiagnosticSeverity::Warning) {
            // Triangle for warnings
            drawList->AddTriangleFilled(
                ImVec2(centerX, centerY - radius),
                ImVec2(centerX - radius, centerY + radius),
                ImVec2(centerX + radius, centerY + radius),
                color
            );
        } else {
            // Circle outline for info/hints
            drawList->AddCircle(ImVec2(centerX, centerY), radius, color, 12, 1.5f);
        }
    }
}

void LspDiagnosticsGutter::renderDiagnosticTooltip(const lsp::EditorDiagnostic& diagnostic, 
                                                    float mouseX, float mouseY) {
    ImGui::SetNextWindowPos(ImVec2(mouseX + 10, mouseY + 10), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_AlwaysAutoResize;
    
    // Color based on severity
    float r, g, b;
    getSeverityColor(diagnostic.severity, r, g, b);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.15f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(r, g, b, 0.8f));
    
    if (ImGui::Begin("##DiagnosticTooltip", nullptr, flags)) {
        // Severity icon and label
        const char* icon = getSeverityIcon(diagnostic.severity);
        ImGui::TextColored(ImVec4(r, g, b, 1.0f), "%s", icon);
        ImGui::SameLine();
        
        const char* severityLabel = "Info";
        switch (diagnostic.severity) {
            case lsp::DiagnosticSeverity::Error: severityLabel = "Error"; break;
            case lsp::DiagnosticSeverity::Warning: severityLabel = "Warning"; break;
            case lsp::DiagnosticSeverity::Information: severityLabel = "Info"; break;
            case lsp::DiagnosticSeverity::Hint: severityLabel = "Hint"; break;
        }
        ImGui::TextColored(ImVec4(r, g, b, 1.0f), "%s", severityLabel);
        
        if (!diagnostic.source.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", diagnostic.source.c_str());
        }
        
        ImGui::Separator();
        
        // Message
        ImGui::PushTextWrapPos(400);
        ImGui::TextUnformatted(diagnostic.message.c_str());
        ImGui::PopTextWrapPos();
        
        // Location
        ImGui::TextDisabled("Line %zu, Column %zu", diagnostic.startLine + 1, diagnostic.startColumn + 1);
    }
    ImGui::End();
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

const char* LspDiagnosticsGutter::getSeverityIcon(lsp::DiagnosticSeverity severity) {
    switch (severity) {
        case lsp::DiagnosticSeverity::Error: return "❌";
        case lsp::DiagnosticSeverity::Warning: return "⚠️";
        case lsp::DiagnosticSeverity::Information: return "ℹ️";
        case lsp::DiagnosticSeverity::Hint: return "💡";
        default: return "•";
    }
}

void LspDiagnosticsGutter::getSeverityColor(lsp::DiagnosticSeverity severity, float& r, float& g, float& b) {
    switch (severity) {
        case lsp::DiagnosticSeverity::Error:
            r = 0.9f; g = 0.3f; b = 0.3f;  // Red
            break;
        case lsp::DiagnosticSeverity::Warning:
            r = 0.9f; g = 0.7f; b = 0.2f;  // Yellow/Orange
            break;
        case lsp::DiagnosticSeverity::Information:
            r = 0.3f; g = 0.6f; b = 0.9f;  // Blue
            break;
        case lsp::DiagnosticSeverity::Hint:
            r = 0.5f; g = 0.8f; b = 0.5f;  // Green
            break;
        default:
            r = 0.7f; g = 0.7f; b = 0.7f;  // Gray
            break;
    }
}

// ============================================================================
// LspInlineDiagnostics Implementation
// ============================================================================

LspInlineDiagnostics::LspInlineDiagnostics() = default;
LspInlineDiagnostics::~LspInlineDiagnostics() = default;

void LspInlineDiagnostics::render(const std::vector<lsp::EditorDiagnostic>& diagnostics,
                                   float editorX, float editorY, size_t firstVisibleLine,
                                   float lineHeight, float charWidth, float scrollY) {
    if (diagnostics.empty()) return;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    for (const auto& diag : diagnostics) {
        // Check if diagnostic is in visible range
        if (diag.startLine < firstVisibleLine) continue;
        
        // Calculate position
        float y = editorY + (diag.startLine - firstVisibleLine) * lineHeight - scrollY + lineHeight - 2;
        float x1 = editorX + diag.startColumn * charWidth;
        float x2 = editorX + diag.endColumn * charWidth;
        
        // Ensure minimum width
        if (x2 - x1 < charWidth) {
            x2 = x1 + charWidth;
        }
        
        // Get color based on severity
        float r, g, b;
        LspDiagnosticsGutter::getSeverityColor(diag.severity, r, g, b);
        
        // Draw squiggly underline
        drawSquigglyLine(x1, y, x2, 2.0f, r, g, b, 0.8f);
    }
}

void LspInlineDiagnostics::drawSquigglyLine(float x1, float y, float x2, float amplitude,
                                             float r, float g, float b, float a) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 color = IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255),
                           static_cast<int>(b * 255), static_cast<int>(a * 255));
    
    float wavelength = 4.0f;
    float x = x1;
    
    while (x < x2) {
        float nextX = std::min(x + wavelength / 2, x2);
        float midY = y + (static_cast<int>((x - x1) / (wavelength / 2)) % 2 == 0 ? amplitude : -amplitude);
        
        drawList->AddLine(ImVec2(x, y), ImVec2((x + nextX) / 2, midY), color, 1.0f);
        drawList->AddLine(ImVec2((x + nextX) / 2, midY), ImVec2(nextX, y), color, 1.0f);
        
        x = nextX;
    }
}

// ============================================================================
// LspStatusBar Implementation
// ============================================================================

LspStatusBar::LspStatusBar() = default;
LspStatusBar::~LspStatusBar() = default;

void LspStatusBar::render(const lsp::LspEditorBridge& bridge, const std::string& currentFile) {
    if (currentFile.empty()) return;
    
    size_t errorCount = bridge.getErrorCount(currentFile);
    size_t warningCount = bridge.getWarningCount(currentFile);
    
    // Error count
    if (errorCount > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("❌ %zu", errorCount);
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    
    // Warning count
    if (warningCount > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
        ImGui::Text("⚠️ %zu", warningCount);
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }
    
    // Language server status
    std::string langId = lsp::LspEditorBridge::detectLanguageId(currentFile);
    if (bridge.isLanguageSupported(langId)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::Text("● LSP");
        ImGui::PopStyleColor();
    } else {
        ImGui::TextDisabled("○ LSP");
    }
}

} // namespace gui
} // namespace bolt

#else // BOLT_HAVE_IMGUI

// Stub implementations when ImGui is not available
namespace bolt {
namespace gui {

LspCompletionPopup::LspCompletionPopup() = default;
LspCompletionPopup::~LspCompletionPopup() = default;
void LspCompletionPopup::render(lsp::CompletionPopupState&, float, float, float) {}
bool LspCompletionPopup::handleInput(lsp::CompletionPopupState&) { return false; }
const char* LspCompletionPopup::getKindIcon(const std::string&) { return ""; }
void LspCompletionPopup::getKindColor(const std::string&, float& r, float& g, float& b) { r = g = b = 0.8f; }

LspHoverTooltip::LspHoverTooltip() = default;
LspHoverTooltip::~LspHoverTooltip() = default;
void LspHoverTooltip::render(const lsp::HoverTooltipState&, float, float) {}

LspDiagnosticsGutter::LspDiagnosticsGutter() = default;
LspDiagnosticsGutter::~LspDiagnosticsGutter() = default;
void LspDiagnosticsGutter::render(const std::vector<lsp::EditorDiagnostic>&, float, size_t, size_t, float, float) {}
void LspDiagnosticsGutter::renderDiagnosticTooltip(const lsp::EditorDiagnostic&, float, float) {}
const char* LspDiagnosticsGutter::getSeverityIcon(lsp::DiagnosticSeverity) { return ""; }
void LspDiagnosticsGutter::getSeverityColor(lsp::DiagnosticSeverity, float& r, float& g, float& b) { r = g = b = 0.7f; }

LspInlineDiagnostics::LspInlineDiagnostics() = default;
LspInlineDiagnostics::~LspInlineDiagnostics() = default;
void LspInlineDiagnostics::render(const std::vector<lsp::EditorDiagnostic>&, float, float, size_t, float, float, float) {}

LspStatusBar::LspStatusBar() = default;
LspStatusBar::~LspStatusBar() = default;
void LspStatusBar::render(const lsp::LspEditorBridge&, const std::string&) {}

} // namespace gui
} // namespace bolt

#endif // BOLT_HAVE_IMGUI
