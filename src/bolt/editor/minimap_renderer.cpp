#include "bolt/editor/minimap_renderer.hpp"
#include <algorithm>
#include <cmath>

namespace bolt {

MinimapRenderer::MinimapRenderer() : MinimapRenderer(RenderConfig{}) {
}

MinimapRenderer::MinimapRenderer(const RenderConfig& config) 
    : config_(config) {
}

std::vector<std::string> MinimapRenderer::render(const RenderContext& context) const {
    std::vector<std::string> result;
    result.reserve(config_.height);
    
    for (size_t minimapLine = 0; minimapLine < config_.height; ++minimapLine) {
        result.push_back(renderLine(context, minimapLine));
    }
    
    return result;
}

std::string MinimapRenderer::renderLine(const RenderContext& context, size_t minimapLine) const {
    if (context.documentLines.empty()) {
        return std::string(config_.width, config_.emptyChar);
    }
    
    std::string line(config_.width, config_.emptyChar);
    
    // Calculate which document lines this minimap line represents
    size_t totalDocumentLines = context.endLine - context.startLine;
    if (totalDocumentLines == 0) {
        return line;
    }
    
    size_t linesPerMinimapLine = std::max(size_t{1}, totalDocumentLines / config_.height);
    size_t docLineStart = context.startLine + (minimapLine * linesPerMinimapLine);
    size_t docLineEnd = std::min(docLineStart + linesPerMinimapLine, context.endLine);
    
    // Analyze the content of these document lines
    char representation = analyzeLinesRange(context.documentLines, docLineStart, docLineEnd);
    
    // Fill the minimap line with the representation character
    std::fill(line.begin(), line.end(), representation);
    
    // Add viewport overlay
    line = addViewportOverlay(line, minimapLine, context);
    
    return line;
}

char MinimapRenderer::analyzeDocumentLine(const std::string& line) const {
    if (line.empty()) {
        return config_.emptyChar;
    }
    
    size_t density = calculateContentDensity(line);
    return selectRepresentationChar(density);
}

char MinimapRenderer::analyzeLinesRange(const std::vector<std::string>& lines, size_t start, size_t end) const {
    if (start >= lines.size() || start >= end) {
        return config_.emptyChar;
    }
    
    size_t totalDensity = 0;
    size_t lineCount = 0;
    
    for (size_t i = start; i < std::min(end, lines.size()); ++i) {
        totalDensity += calculateContentDensity(lines[i]);
        lineCount++;
    }
    
    if (lineCount == 0) {
        return config_.emptyChar;
    }
    
    size_t averageDensity = totalDensity / lineCount;
    return selectRepresentationChar(averageDensity);
}

std::string MinimapRenderer::addViewportOverlay(const std::string& line, size_t minimapLine, 
                                                const RenderContext& context) const {
    std::string result = line;
    
    // Check if this minimap line intersects with the current viewport
    if (isViewportLine(minimapLine, context)) {
        // Replace some characters with viewport indicator
        for (size_t i = 0; i < result.length(); i += 4) {
            result[i] = config_.viewportChar;
        }
    }
    
    // Highlight current line
    if (isCurrentLine(minimapLine, context)) {
        if (!result.empty()) {
            result[0] = config_.currentLineChar;
            if (result.length() > 1) {
                result[result.length() - 1] = config_.currentLineChar;
            }
        }
    }
    
    return result;
}

bool MinimapRenderer::isViewportLine(size_t minimapLine, const RenderContext& context) const {
    size_t documentLine = mapDocumentLineToMinimap(context.viewportStart, 
                                                   context.endLine - context.startLine, 
                                                   config_.height);
    size_t endDocumentLine = mapDocumentLineToMinimap(context.viewportEnd,
                                                      context.endLine - context.startLine,
                                                      config_.height);
    
    return minimapLine >= documentLine && minimapLine <= endDocumentLine;
}

bool MinimapRenderer::isCurrentLine(size_t minimapLine, const RenderContext& context) const {
    if (context.currentLine < context.startLine || context.currentLine >= context.endLine) {
        return false;
    }
    
    size_t relativeLine = context.currentLine - context.startLine;
    size_t mappedLine = mapDocumentLineToMinimap(relativeLine,
                                                 context.endLine - context.startLine,
                                                 config_.height);
    
    return minimapLine == mappedLine;
}

size_t MinimapRenderer::mapDocumentLineToMinimap(size_t documentLine, size_t totalLines, size_t minimapHeight) const {
    if (totalLines == 0 || minimapHeight == 0) {
        return 0;
    }
    
    // Map document line to minimap line proportionally
    double ratio = static_cast<double>(documentLine) / static_cast<double>(totalLines);
    size_t minimapLine = static_cast<size_t>(ratio * minimapHeight);
    
    return std::min(minimapLine, minimapHeight - 1);
}

size_t MinimapRenderer::calculateContentDensity(const std::string& line) const {
    if (line.empty()) {
        return 0;
    }
    
    size_t nonSpaceChars = 0;
    for (char c : line) {
        if (c != ' ' && c != '\t') {
            nonSpaceChars++;
        }
    }
    
    // Return density as percentage (0-100)
    return (nonSpaceChars * 100) / line.length();
}

char MinimapRenderer::selectRepresentationChar(size_t density) const {
    if (density == 0) {
        return config_.emptyChar;
    } else if (density < 30) {
        return '~';  // Light shade
    } else if (density < 70) {
        return ':';  // Medium shade
    } else {
        return config_.fillChar;  // Dense content
    }
}

} // namespace bolt