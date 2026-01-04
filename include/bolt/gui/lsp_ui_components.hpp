/**
 * @file lsp_ui_components.hpp
 * @brief ImGui UI components for LSP features (completion, hover, diagnostics)
 */

#ifndef BOLT_LSP_UI_COMPONENTS_HPP
#define BOLT_LSP_UI_COMPONENTS_HPP

#include "bolt/editor/lsp_editor_bridge.hpp"
#include <string>
#include <functional>

// Forward declare ImGui types to avoid header dependency
struct ImVec2;
struct ImVec4;

namespace bolt {
namespace gui {

/**
 * @brief UI component for rendering LSP code completion popup
 */
class LspCompletionPopup {
public:
    using AcceptCallback = std::function<void(const std::string& insertText)>;
    
    LspCompletionPopup();
    ~LspCompletionPopup();
    
    /**
     * @brief Render the completion popup
     * @param state Current completion state from LspEditorBridge
     * @param editorPosX X position of the cursor in the editor
     * @param editorPosY Y position of the cursor in the editor
     * @param lineHeight Height of a line in the editor
     */
    void render(lsp::CompletionPopupState& state, float editorPosX, float editorPosY, float lineHeight);
    
    /**
     * @brief Set callback for when a completion is accepted
     */
    void setAcceptCallback(AcceptCallback callback) { acceptCallback_ = callback; }
    
    /**
     * @brief Handle keyboard input for the popup
     * @return true if input was handled
     */
    bool handleInput(lsp::CompletionPopupState& state);
    
    /**
     * @brief Get the icon for a completion kind
     */
    static const char* getKindIcon(const std::string& kind);
    
    /**
     * @brief Get the color for a completion kind
     */
    static void getKindColor(const std::string& kind, float& r, float& g, float& b);
    
private:
    AcceptCallback acceptCallback_;
    int maxVisibleItems_ = 10;
    float popupWidth_ = 400.0f;
    float itemHeight_ = 24.0f;
};

/**
 * @brief UI component for rendering LSP hover tooltip
 */
class LspHoverTooltip {
public:
    LspHoverTooltip();
    ~LspHoverTooltip();
    
    /**
     * @brief Render the hover tooltip
     * @param state Current hover state from LspEditorBridge
     * @param mouseX Current mouse X position
     * @param mouseY Current mouse Y position
     */
    void render(const lsp::HoverTooltipState& state, float mouseX, float mouseY);
    
    /**
     * @brief Set maximum width for the tooltip
     */
    void setMaxWidth(float width) { maxWidth_ = width; }
    
private:
    float maxWidth_ = 500.0f;
    float padding_ = 8.0f;
};

/**
 * @brief UI component for rendering LSP diagnostics in the editor gutter
 */
class LspDiagnosticsGutter {
public:
    LspDiagnosticsGutter();
    ~LspDiagnosticsGutter();
    
    /**
     * @brief Render diagnostic markers in the gutter
     * @param diagnostics List of diagnostics for the current file
     * @param gutterX X position of the gutter
     * @param firstVisibleLine First visible line in the editor
     * @param lastVisibleLine Last visible line in the editor
     * @param lineHeight Height of each line
     * @param scrollY Current scroll position
     */
    void render(const std::vector<lsp::EditorDiagnostic>& diagnostics,
                float gutterX, size_t firstVisibleLine, size_t lastVisibleLine,
                float lineHeight, float scrollY);
    
    /**
     * @brief Render diagnostic tooltip when hovering over a marker
     * @param diagnostic The diagnostic to show
     * @param mouseX Mouse X position
     * @param mouseY Mouse Y position
     */
    void renderDiagnosticTooltip(const lsp::EditorDiagnostic& diagnostic, float mouseX, float mouseY);
    
    /**
     * @brief Get icon for diagnostic severity
     */
    static const char* getSeverityIcon(lsp::DiagnosticSeverity severity);
    
    /**
     * @brief Get color for diagnostic severity
     */
    static void getSeverityColor(lsp::DiagnosticSeverity severity, float& r, float& g, float& b);
    
    /**
     * @brief Set gutter width
     */
    void setGutterWidth(float width) { gutterWidth_ = width; }
    
private:
    float gutterWidth_ = 20.0f;
};

/**
 * @brief UI component for rendering inline diagnostics (squiggly underlines)
 */
class LspInlineDiagnostics {
public:
    LspInlineDiagnostics();
    ~LspInlineDiagnostics();
    
    /**
     * @brief Render inline diagnostic decorations (underlines)
     * @param diagnostics List of diagnostics for the current file
     * @param editorX X position of the editor content area
     * @param editorY Y position of the editor content area
     * @param firstVisibleLine First visible line
     * @param lineHeight Height of each line
     * @param charWidth Width of each character (monospace)
     * @param scrollY Current scroll position
     */
    void render(const std::vector<lsp::EditorDiagnostic>& diagnostics,
                float editorX, float editorY, size_t firstVisibleLine,
                float lineHeight, float charWidth, float scrollY);
    
private:
    void drawSquigglyLine(float x1, float y, float x2, float amplitude, 
                          float r, float g, float b, float a);
};

/**
 * @brief Status bar component showing LSP status and diagnostic summary
 */
class LspStatusBar {
public:
    LspStatusBar();
    ~LspStatusBar();
    
    /**
     * @brief Render the LSP status section in the status bar
     * @param bridge The LSP editor bridge
     * @param currentFile Currently open file path
     */
    void render(const lsp::LspEditorBridge& bridge, const std::string& currentFile);
    
private:
    float sectionWidth_ = 200.0f;
};

} // namespace gui
} // namespace bolt

#endif // BOLT_LSP_UI_COMPONENTS_HPP
