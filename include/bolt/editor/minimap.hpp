#ifndef MINIMAP_HPP
#define MINIMAP_HPP

#include <string>
#include <vector>
#include <memory>

namespace bolt {

// Forward declarations
class MinimapRenderer;
class MinimapUI;

/**
 * Core minimap functionality for large files
 * Provides a miniaturized view of file content and navigation
 */
class Minimap {
public:
    struct MinimapConfig {
        size_t width = 120;           // Width in characters
        size_t height = 40;           // Height in lines
        size_t charWidth = 1;         // Characters per pixel column
        size_t lineHeight = 1;        // Lines per pixel row
        bool showScrollbar = true;    // Show scrollbar indicator
        bool showCurrentView = true;  // Highlight current viewport
    };

    struct ViewportInfo {
        size_t startLine = 0;         // First visible line in main editor
        size_t endLine = 0;           // Last visible line in main editor
        size_t totalLines = 0;        // Total lines in document
        size_t currentLine = 0;       // Current cursor line
    };

private:
    std::string documentPath_;
    std::vector<std::string> documentLines_;
    MinimapConfig config_;
    ViewportInfo viewport_;
    std::unique_ptr<MinimapRenderer> renderer_;
    bool enabled_ = true;

public:
    Minimap();
    explicit Minimap(const MinimapConfig& config);
    ~Minimap();

    // Document operations
    void setDocument(const std::string& filePath, const std::vector<std::string>& lines);
    void updateDocument(const std::vector<std::string>& lines);
    void clearDocument();
    const std::string& getDocumentPath() const { return documentPath_; }
    bool hasDocument() const { return !documentPath_.empty(); }

    // Viewport tracking
    void updateViewport(const ViewportInfo& viewport);
    const ViewportInfo& getViewport() const { return viewport_; }

    // Navigation
    size_t getLineFromPosition(size_t x, size_t y) const;
    std::pair<size_t, size_t> getPositionFromLine(size_t line) const;
    void scrollToLine(size_t line);

    // Configuration
    void setConfig(const MinimapConfig& config);
    const MinimapConfig& getConfig() const { return config_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    // Rendering
    std::vector<std::string> render() const;
    std::string renderLine(size_t minimapLine) const;

    // Statistics
    size_t getTotalLines() const { return documentLines_.size(); }
    size_t getVisibleLines() const;
    double getScrollPercentage() const;

private:
    void initializeRenderer();
    size_t calculateStartLine() const;
    size_t calculateEndLine() const;
    size_t calculateLinesPerMinimapLine() const;
};

} // namespace bolt

#endif