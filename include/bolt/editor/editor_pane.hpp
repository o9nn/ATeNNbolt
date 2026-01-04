#ifndef EDITOR_PANE_HPP
#define EDITOR_PANE_HPP

#include "bolt/core/editor_store.hpp"
#include "bolt/editor/minimap.hpp"
#include "bolt/editor/minimap_ui.hpp"
#include <string>
#include <memory>

namespace bolt {

// Forward declarations
class CodeFoldingManager;
class CursorManager;

/**
 * Represents a single editor pane within a split view
 * Each pane can display a different document and maintain its own state
 */
class EditorPane {
public:
    struct PanePosition {
        int x, y;      // Position within the parent container
        int width, height;  // Dimensions of the pane
    };

    struct PaneState {
        std::string documentPath;
        size_t cursorLine = 0;
        size_t cursorColumn = 0;
        size_t scrollLine = 0;
        size_t scrollColumn = 0;
        bool hasFocus = false;
        bool isVisible = true;
    };

private:
    std::string paneId_;
    PanePosition position_;
    PaneState state_;
    EditorStore& editorStore_;
    
    // Minimap integration
    std::unique_ptr<Minimap> minimap_;
    std::unique_ptr<MinimapUI> minimapUI_;
    bool minimapEnabled_ = true;

public:
    explicit EditorPane(const std::string& paneId);
    ~EditorPane() = default;

    // Basic pane operations
    const std::string& getId() const { return paneId_; }
    void setPosition(const PanePosition& position);
    const PanePosition& getPosition() const { return position_; }
    
    // Document operations
    void openDocument(const std::string& filePath);
    void closeDocument();
    const std::string& getDocumentPath() const { return state_.documentPath; }
    bool hasDocument() const { return !state_.documentPath.empty(); }
    
    // State management
    void setFocus(bool focus);
    bool hasFocus() const { return state_.hasFocus; }
    void setVisible(bool visible);
    bool isVisible() const { return state_.isVisible; }
    
    // Cursor and scroll operations
    void setCursorPosition(size_t line, size_t column);
    void getCursorPosition(size_t& line, size_t& column) const;
    void setScrollPosition(size_t line, size_t column);
    void getScrollPosition(size_t& line, size_t& column) const;
    
    // Content operations
    std::string getDocumentContent() const;
    void updateDocumentContent(const std::string& content);
    
    // Rendering and display
    void render();
    void refresh();
    
    // Pane state
    const PaneState& getState() const { return state_; }
    void restoreState(const PaneState& state);
    
    // Minimap operations
    void setMinimapEnabled(bool enabled);
    bool isMinimapEnabled() const { return minimapEnabled_; }
    Minimap* getMinimap() const { return minimap_.get(); }
    MinimapUI* getMinimapUI() const { return minimapUI_.get(); }
    void updateMinimap();
    std::vector<std::string> renderMinimap() const;

private:
    void syncWithEditorStore();
    void updateEditorStoreDocument();
    void initializeMinimap();
    void updateMinimapViewport();
    std::vector<std::string> getDocumentLines() const;
};

} // namespace bolt

#endif