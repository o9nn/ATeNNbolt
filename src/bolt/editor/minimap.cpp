#include "bolt/editor/minimap.hpp"
#include "bolt/editor/minimap_renderer.hpp"
#include <algorithm>
#include <cmath>

namespace bolt {

Minimap::Minimap() : Minimap(MinimapConfig{}) {
}

Minimap::Minimap(const MinimapConfig& config) 
    : config_(config) {
    initializeRenderer();
}

Minimap::~Minimap() = default;

void Minimap::setDocument(const std::string& filePath, const std::vector<std::string>& lines) {
    documentPath_ = filePath;
    documentLines_ = lines;
    
    // Update viewport to show full document initially
    viewport_.totalLines = lines.size();
    viewport_.startLine = 0;
    viewport_.endLine = std::min(config_.height, lines.size());
    viewport_.currentLine = 0;
}

void Minimap::updateDocument(const std::vector<std::string>& lines) {
    documentLines_ = lines;
    viewport_.totalLines = lines.size();
    
    // Adjust viewport if document changed
    if (viewport_.endLine > lines.size()) {
        viewport_.endLine = lines.size();
    }
}

void Minimap::clearDocument() {
    documentPath_.clear();
    documentLines_.clear();
    viewport_ = ViewportInfo{};
}

void Minimap::updateViewport(const ViewportInfo& viewport) {
    viewport_ = viewport;
    
    // Ensure viewport is within document bounds
    if (!documentLines_.empty()) {
        viewport_.totalLines = documentLines_.size();
        viewport_.startLine = std::min(viewport_.startLine, documentLines_.size() - 1);
        viewport_.endLine = std::min(viewport_.endLine, documentLines_.size());
        viewport_.currentLine = std::min(viewport_.currentLine, documentLines_.size() - 1);
    }
}

size_t Minimap::getLineFromPosition(size_t x, size_t y) const {
    if (documentLines_.empty()) {
        return 0;
    }
    
    // Calculate which document line corresponds to this minimap position
    size_t linesPerMinimapLine = calculateLinesPerMinimapLine();
    size_t startLine = calculateStartLine();
    
    size_t documentLine = startLine + (y * linesPerMinimapLine);
    return std::min(documentLine, documentLines_.size() - 1);
}

std::pair<size_t, size_t> Minimap::getPositionFromLine(size_t line) const {
    if (documentLines_.empty()) {
        return {0, 0};
    }
    
    size_t linesPerMinimapLine = calculateLinesPerMinimapLine();
    size_t startLine = calculateStartLine();
    
    if (line < startLine) {
        return {0, 0};
    }
    
    size_t minimapY = (line - startLine) / linesPerMinimapLine;
    minimapY = std::min(minimapY, config_.height - 1);
    
    return {0, minimapY};
}

void Minimap::scrollToLine(size_t line) {
    if (documentLines_.empty()) {
        return;
    }
    
    // Update viewport to center on the requested line
    size_t visibleLines = getVisibleLines();
    size_t halfVisible = visibleLines / 2;
    
    if (line >= halfVisible) {
        viewport_.startLine = line - halfVisible;
    } else {
        viewport_.startLine = 0;
    }
    
    viewport_.endLine = std::min(viewport_.startLine + visibleLines, documentLines_.size());
    viewport_.currentLine = line;
}

void Minimap::setConfig(const MinimapConfig& config) {
    config_ = config;
    initializeRenderer();
}

std::vector<std::string> Minimap::render() const {
    if (!enabled_ || documentLines_.empty()) {
        return std::vector<std::string>(config_.height, std::string(config_.width, ' '));
    }
    
    return renderer_->render({
        documentLines_,
        calculateStartLine(),
        calculateEndLine(),
        viewport_.currentLine,
        viewport_.startLine,
        viewport_.endLine
    });
}

std::string Minimap::renderLine(size_t minimapLine) const {
    if (!enabled_ || documentLines_.empty() || minimapLine >= config_.height) {
        return std::string(config_.width, ' ');
    }
    
    return renderer_->renderLine({
        documentLines_,
        calculateStartLine(),
        calculateEndLine(),
        viewport_.currentLine,
        viewport_.startLine,
        viewport_.endLine
    }, minimapLine);
}

size_t Minimap::getVisibleLines() const {
    if (documentLines_.empty()) {
        return 0;
    }
    
    return std::min(config_.height * calculateLinesPerMinimapLine(), documentLines_.size());
}

double Minimap::getScrollPercentage() const {
    if (documentLines_.empty() || documentLines_.size() <= getVisibleLines()) {
        return 0.0;
    }
    
    return static_cast<double>(viewport_.startLine) / 
           static_cast<double>(documentLines_.size() - getVisibleLines());
}

void Minimap::initializeRenderer() {
    MinimapRenderer::RenderConfig renderConfig;
    renderConfig.width = config_.width;
    renderConfig.height = config_.height;
    
    renderer_ = std::make_unique<MinimapRenderer>(renderConfig);
}

size_t Minimap::calculateStartLine() const {
    if (documentLines_.empty()) {
        return 0;
    }
    
    // Show content around the current viewport
    size_t totalMinimapLines = config_.height;
    size_t linesPerMinimapLine = calculateLinesPerMinimapLine();
    size_t totalDocumentLines = documentLines_.size();
    
    // If document fits in minimap, start from beginning
    if (totalDocumentLines <= totalMinimapLines * linesPerMinimapLine) {
        return 0;
    }
    
    // Center minimap around current viewport
    size_t viewportCenter = (viewport_.startLine + viewport_.endLine) / 2;
    size_t minimapCoverage = totalMinimapLines * linesPerMinimapLine;
    
    if (viewportCenter >= minimapCoverage / 2) {
        size_t start = viewportCenter - minimapCoverage / 2;
        return std::min(start, totalDocumentLines - minimapCoverage);
    }
    
    return 0;
}

size_t Minimap::calculateEndLine() const {
    size_t startLine = calculateStartLine();
    size_t linesPerMinimapLine = calculateLinesPerMinimapLine();
    size_t endLine = startLine + (config_.height * linesPerMinimapLine);
    
    return std::min(endLine, documentLines_.size());
}

size_t Minimap::calculateLinesPerMinimapLine() const {
    if (documentLines_.empty()) {
        return 1;
    }
    
    // Calculate how many document lines should be represented by each minimap line
    size_t ratio = std::max(size_t{1}, documentLines_.size() / config_.height);
    return ratio;
}

} // namespace bolt