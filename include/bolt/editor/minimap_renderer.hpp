#ifndef MINIMAP_RENDERER_HPP
#define MINIMAP_RENDERER_HPP

#include <string>
#include <vector>

namespace bolt {

/**
 * Handles rendering logic for the minimap
 * Converts full document content into miniaturized representation
 */
class MinimapRenderer {
public:
    struct RenderConfig {
        size_t width = 120;
        size_t height = 40;
        char fillChar = '#';          // Character for filled areas
        char emptyChar = ' ';         // Character for empty areas
        char viewportChar = '*';      // Character for viewport indicator
        char currentLineChar = '|';   // Character for current line
    };

    struct RenderContext {
        const std::vector<std::string>& documentLines;
        size_t startLine;
        size_t endLine;
        size_t currentLine;
        size_t viewportStart;
        size_t viewportEnd;
    };

private:
    RenderConfig config_;

public:
    MinimapRenderer();
    explicit MinimapRenderer(const RenderConfig& config);

    // Configuration
    void setConfig(const RenderConfig& config) { config_ = config; }
    const RenderConfig& getConfig() const { return config_; }

    // Core rendering
    std::vector<std::string> render(const RenderContext& context) const;
    std::string renderLine(const RenderContext& context, size_t minimapLine) const;

    // Line analysis
    char analyzeDocumentLine(const std::string& line) const;
    char analyzeLinesRange(const std::vector<std::string>& lines, size_t start, size_t end) const;

    // Viewport rendering
    std::string addViewportOverlay(const std::string& line, size_t minimapLine, 
                                   const RenderContext& context) const;
    bool isViewportLine(size_t minimapLine, const RenderContext& context) const;
    bool isCurrentLine(size_t minimapLine, const RenderContext& context) const;

private:
    size_t mapDocumentLineToMinimap(size_t documentLine, size_t totalLines, size_t minimapHeight) const;
    size_t calculateContentDensity(const std::string& line) const;
    char selectRepresentationChar(size_t density) const;
};

} // namespace bolt

#endif