#ifndef FILE_TREE_UI_HPP
#define FILE_TREE_UI_HPP

#include "file_tree_node.hpp"
#include "file_tree_manager.hpp"
#include <string>
#include <vector>
#include <memory>

namespace bolt {

struct FileTreeRenderOptions {
    bool showIcons;
    int indentSize;
    char expandedIcon;
    char collapsedIcon;
    char fileIcon;
    char directoryIcon;
    
    FileTreeRenderOptions() 
        : showIcons(true)
        , indentSize(2)
        , expandedIcon('v')
        , collapsedIcon('>')
        , fileIcon('-')
        , directoryIcon('/')
    {}
};

class FileTreeUI {
private:
    FileTreeManager& manager_;
    FileTreeRenderOptions options_;
    ThreadSafe<bool> isVisible_{true};
    
public:
    FileTreeUI() : manager_(FileTreeManager::getInstance()) {}
    
    // Rendering
    void render();
    void renderToString(std::string& output);
    std::vector<std::string> renderToLines() const;
    
    // UI Settings
    void setRenderOptions(const FileTreeRenderOptions& options);
    FileTreeRenderOptions getRenderOptions() const { return options_; }
    
    // Visibility
    void setVisible(bool visible);
    bool isVisible() const;
    
    // Interaction handling
    void handleClick(size_t lineNumber);
    void handleDoubleClick(size_t lineNumber);
    void handleKeyPress(const std::string& key);
    
    // Context menu (future expansion)
    void showContextMenu(const std::string& path);
    
private:
    void renderNode(std::shared_ptr<FileTreeNode> node, std::string& output, size_t depth) const;
    void renderNodeToLine(std::shared_ptr<FileTreeNode> node, std::vector<std::string>& lines, size_t depth) const;
    std::string getIndentation(size_t depth) const;
    std::string getNodePrefix(std::shared_ptr<FileTreeNode> node) const;
    std::string formatFileSize(size_t size) const;
    
    // Convert line number to node path
    std::string getPathFromLineNumber(size_t lineNumber) const;
};

} // namespace bolt

#endif