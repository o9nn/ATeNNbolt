#include "bolt/editor/file_tree_ui.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace bolt {

void FileTreeUI::render() {
    if (!isVisible()) return;
    
    std::string output;
    renderToString(output);
    std::cout << output;
}

void FileTreeUI::renderToString(std::string& output) {
    output.clear();
    
    auto rootNode = manager_.getRootNode();
    if (!rootNode) {
        output = "No directory loaded\n";
        return;
    }
    
    output += "File Tree: " + rootNode->fullPath + "\n";
    output += std::string(50, '-') + "\n";
    
    renderNode(rootNode, output, 0);
}

std::vector<std::string> FileTreeUI::renderToLines() const {
    std::vector<std::string> lines;
    
    auto rootNode = manager_.getRootNode();
    if (!rootNode) {
        lines.push_back("No directory loaded");
        return lines;
    }
    
    lines.push_back("File Tree: " + rootNode->fullPath);
    lines.push_back(std::string(50, '-'));
    
    renderNodeToLine(rootNode, lines, 0);
    
    return lines;
}

void FileTreeUI::setRenderOptions(const FileTreeRenderOptions& options) {
    options_ = options;
}

void FileTreeUI::setVisible(bool visible) {
    isVisible_.write([visible](bool& current) {
        current = visible;
    });
}

bool FileTreeUI::isVisible() const {
    return isVisible_.read([](const bool& current) {
        return current;
    });
}

void FileTreeUI::handleClick(size_t lineNumber) {
    std::string path = getPathFromLineNumber(lineNumber);
    if (path.empty()) return;
    
    auto node = manager_.findNode(path);
    if (!node) return;
    
    if (node->isDirectory()) {
        manager_.toggleDirectory(path);
    } else {
        manager_.selectFile(path);
    }
}

void FileTreeUI::handleDoubleClick(size_t lineNumber) {
    std::string path = getPathFromLineNumber(lineNumber);
    if (path.empty()) return;
    
    auto node = manager_.findNode(path);
    if (!node) return;
    
    if (node->isFile()) {
        manager_.selectFile(path);
        // File is now selected and callback should be triggered
    } else if (node->isDirectory()) {
        manager_.toggleDirectory(path);
    }
}

void FileTreeUI::handleKeyPress(const std::string& key) {
    auto selectedNode = manager_.getSelectedNode();
    if (!selectedNode) return;
    
    if (key == "Enter" || key == "Return") {
        if (selectedNode->isDirectory()) {
            manager_.toggleDirectory(selectedNode->fullPath);
        }
    } else if (key == "ArrowRight" || key == "Right") {
        if (selectedNode->isDirectory() && !selectedNode->isExpanded) {
            manager_.expandDirectory(selectedNode->fullPath);
        }
    } else if (key == "ArrowLeft" || key == "Left") {
        if (selectedNode->isDirectory() && selectedNode->isExpanded) {
            manager_.collapseDirectory(selectedNode->fullPath);
        }
    }
    // Add more keyboard navigation as needed
}

void FileTreeUI::showContextMenu(const std::string& path) {
    // Future implementation for context menu
    // Could show options like: New File, New Folder, Delete, Rename, etc.
}

void FileTreeUI::renderNode(std::shared_ptr<FileTreeNode> node, std::string& output, size_t depth) const {
    if (!node) return;
    
    std::string indent = getIndentation(depth);
    std::string prefix = getNodePrefix(node);
    std::string selection = node->isSelected ? "*" : " ";
    
    output += selection + indent + prefix + node->name;
    
    if (node->isFile() && options_.showIcons) {
        output += " (" + formatFileSize(node->size) + ")";
    }
    
    output += "\n";
    
    // Render children if directory is expanded
    if (node->isDirectory() && node->isExpanded) {
        for (const auto& child : node->children) {
            renderNode(child, output, depth + 1);
        }
    }
}

void FileTreeUI::renderNodeToLine(std::shared_ptr<FileTreeNode> node, std::vector<std::string>& lines, size_t depth) const {
    if (!node) return;
    
    std::string indent = getIndentation(depth);
    std::string prefix = getNodePrefix(node);
    std::string selection = node->isSelected ? "*" : " ";
    
    std::string line = selection + indent + prefix + node->name;
    
    if (node->isFile() && options_.showIcons) {
        line += " (" + formatFileSize(node->size) + ")";
    }
    
    lines.push_back(line);
    
    // Render children if directory is expanded
    if (node->isDirectory() && node->isExpanded) {
        for (const auto& child : node->children) {
            renderNodeToLine(child, lines, depth + 1);
        }
    }
}

std::string FileTreeUI::getIndentation(size_t depth) const {
    return std::string(depth * options_.indentSize, ' ');
}

std::string FileTreeUI::getNodePrefix(std::shared_ptr<FileTreeNode> node) const {
    if (!options_.showIcons) return "";
    
    if (node->isDirectory()) {
        if (node->isExpanded) {
            return std::string(1, options_.expandedIcon) + std::string(1, options_.directoryIcon) + " ";
        } else {
            return std::string(1, options_.collapsedIcon) + std::string(1, options_.directoryIcon) + " ";
        }
    } else {
        return std::string(1, options_.fileIcon) + " ";
    }
}

std::string FileTreeUI::formatFileSize(size_t size) const {
    const char* units[] = {"B", "KB", "MB", "GB"};
    size_t unitIndex = 0;
    double displaySize = static_cast<double>(size);
    
    while (displaySize >= 1024.0 && unitIndex < 3) {
        displaySize /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    if (unitIndex == 0) {
        oss << static_cast<size_t>(displaySize) << " " << units[unitIndex];
    } else {
        oss << std::fixed << std::setprecision(1) << displaySize << " " << units[unitIndex];
    }
    
    return oss.str();
}

std::string FileTreeUI::getPathFromLineNumber(size_t lineNumber) const {
    auto lines = renderToLines();
    
    // Account for header lines (title and separator)
    if (lineNumber < 2 || lineNumber >= lines.size()) {
        return "";
    }
    
    // Get the visible nodes
    auto visibleNodes = manager_.getVisibleNodes();
    size_t nodeIndex = lineNumber - 2; // Subtract header lines
    
    if (nodeIndex < visibleNodes.size()) {
        return visibleNodes[nodeIndex]->fullPath;
    }
    
    return "";
}

} // namespace bolt