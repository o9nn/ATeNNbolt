#ifndef FILE_TREE_NODE_HPP
#define FILE_TREE_NODE_HPP

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <filesystem>

namespace bolt {

enum class FileTreeNodeType {
    FILE,
    DIRECTORY
};

struct FileTreeNode : public std::enable_shared_from_this<FileTreeNode> {
    std::string name;
    std::string fullPath;
    FileTreeNodeType type;
    size_t size;  // in bytes, 0 for directories
    std::filesystem::file_time_type lastModified;
    
    bool isExpanded;  // for directories
    bool isSelected;
    
    // Hierarchy
    std::weak_ptr<FileTreeNode> parent;
    std::vector<std::shared_ptr<FileTreeNode>> children;
    
    FileTreeNode(const std::string& name, const std::string& fullPath, FileTreeNodeType type)
        : name(name), fullPath(fullPath), type(type), size(0), isExpanded(false), isSelected(false) {}
    
    bool isDirectory() const { return type == FileTreeNodeType::DIRECTORY; }
    bool isFile() const { return type == FileTreeNodeType::FILE; }
    
    // Helper methods
    std::shared_ptr<FileTreeNode> findChild(const std::string& name) const;
    void addChild(std::shared_ptr<FileTreeNode> child);
    void removeChild(const std::string& name);
    void sortChildren();
    
    // Get depth in tree (0 for root)
    size_t getDepth() const;
    
    // Get all descendant files
    std::vector<std::shared_ptr<FileTreeNode>> getAllFiles() const;
};

} // namespace bolt

#endif