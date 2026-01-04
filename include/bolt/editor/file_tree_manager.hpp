#ifndef FILE_TREE_MANAGER_HPP
#define FILE_TREE_MANAGER_HPP

#include "file_tree_node.hpp"
#include "bolt/core/thread_safety.hpp"
#include <string>
#include <memory>
#include <functional>
#include <filesystem>

namespace bolt {

class FileTreeManager {
private:
    std::shared_ptr<FileTreeNode> rootNode_;
    ThreadSafe<std::string> currentRootPath_;
    ThreadSafe<bool> showHiddenFiles_{false};
    std::function<void(const std::string&)> onFileSelected_;
    std::function<void(const std::string&)> onDirectoryExpanded_;
    
public:
    static FileTreeManager& getInstance() {
        static FileTreeManager instance;
        return instance;
    }
    
    // Root directory management
    void setRootDirectory(const std::string& path);
    std::string getRootDirectory() const;
    std::shared_ptr<FileTreeNode> getRootNode() const { return rootNode_; }
    
    // Tree operations
    void refreshTree();
    void refreshNode(std::shared_ptr<FileTreeNode> node);
    bool expandDirectory(const std::string& path);
    bool collapseDirectory(const std::string& path);
    bool toggleDirectory(const std::string& path);
    
    // Selection
    void selectFile(const std::string& path);
    void clearSelection();
    std::shared_ptr<FileTreeNode> getSelectedNode() const;
    
    // Navigation
    std::shared_ptr<FileTreeNode> findNode(const std::string& path) const;
    std::vector<std::shared_ptr<FileTreeNode>> getVisibleNodes() const;
    
    // Settings
    void setShowHiddenFiles(bool show);
    bool getShowHiddenFiles() const;
    
    // Callbacks
    void setOnFileSelected(std::function<void(const std::string&)> callback);
    void setOnDirectoryExpanded(std::function<void(const std::string&)> callback);
    
    // File operations
    bool createFile(const std::string& parentPath, const std::string& fileName);
    bool createDirectory(const std::string& parentPath, const std::string& dirName);
    bool deleteNode(const std::string& path);
    bool renameNode(const std::string& path, const std::string& newName);
    
private:
    FileTreeManager() = default;
    void buildTreeFromPath(const std::string& path, std::shared_ptr<FileTreeNode> parent);
    void scanDirectory(std::shared_ptr<FileTreeNode> dirNode);
    bool shouldShowFile(const std::filesystem::directory_entry& entry) const;
    void notifyFileSelected(const std::string& path);
    void notifyDirectoryExpanded(const std::string& path);
    void clearSelectionRecursive(std::shared_ptr<FileTreeNode> node);
    void collectVisibleNodes(std::shared_ptr<FileTreeNode> node, std::vector<std::shared_ptr<FileTreeNode>>& result) const;
};

} // namespace bolt

#endif