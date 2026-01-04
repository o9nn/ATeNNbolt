#include "bolt/editor/file_tree_manager.hpp"
#include <filesystem>
#include <algorithm>
#include <fstream>

namespace bolt {
namespace fs = std::filesystem;

void FileTreeManager::setRootDirectory(const std::string& path) {
    currentRootPath_.write([&path](std::string& current) {
        current = path;
    });
    
    if (fs::exists(path) && fs::is_directory(path)) {
        rootNode_ = std::make_shared<FileTreeNode>(
            fs::path(path).filename().string(),
            path,
            FileTreeNodeType::DIRECTORY
        );
        rootNode_->isExpanded = true;
        refreshTree();
    }
}

std::string FileTreeManager::getRootDirectory() const {
    return currentRootPath_.read([](const std::string& current) {
        return current;
    });
}

void FileTreeManager::refreshTree() {
    if (rootNode_) {
        scanDirectory(rootNode_);
    }
}

void FileTreeManager::refreshNode(std::shared_ptr<FileTreeNode> node) {
    if (node && node->isDirectory()) {
        scanDirectory(node);
    }
}

bool FileTreeManager::expandDirectory(const std::string& path) {
    auto node = findNode(path);
    if (node && node->isDirectory() && !node->isExpanded) {
        node->isExpanded = true;
        scanDirectory(node);
        notifyDirectoryExpanded(path);
        return true;
    }
    return false;
}

bool FileTreeManager::collapseDirectory(const std::string& path) {
    auto node = findNode(path);
    if (node && node->isDirectory() && node->isExpanded) {
        node->isExpanded = false;
        return true;
    }
    return false;
}

bool FileTreeManager::toggleDirectory(const std::string& path) {
    auto node = findNode(path);
    if (node && node->isDirectory()) {
        if (node->isExpanded) {
            return collapseDirectory(path);
        } else {
            return expandDirectory(path);
        }
    }
    return false;
}

void FileTreeManager::selectFile(const std::string& path) {
    clearSelection();
    auto node = findNode(path);
    if (node) {
        node->isSelected = true;
        notifyFileSelected(path);
    }
}

void FileTreeManager::clearSelection() {
    if (rootNode_) {
        clearSelectionRecursive(rootNode_);
    }
}

std::shared_ptr<FileTreeNode> FileTreeManager::getSelectedNode() const {
    std::vector<std::shared_ptr<FileTreeNode>> allNodes;
    if (rootNode_) {
        collectVisibleNodes(rootNode_, allNodes);
    }
    
    auto it = std::find_if(allNodes.begin(), allNodes.end(),
        [](const std::shared_ptr<FileTreeNode>& node) {
            return node->isSelected;
        });
    
    return (it != allNodes.end()) ? *it : nullptr;
}

std::shared_ptr<FileTreeNode> FileTreeManager::findNode(const std::string& path) const {
    if (!rootNode_) return nullptr;
    
    // Convert to canonical path for comparison
    fs::path searchPath = fs::path(path).lexically_normal();
    fs::path rootPath = fs::path(rootNode_->fullPath).lexically_normal();
    
    if (searchPath == rootPath) {
        return rootNode_;
    }
    
    // Check if path is under root
    auto relativePath = fs::relative(searchPath, rootPath);
    if (relativePath.begin() != relativePath.end() && *relativePath.begin() == "..") {
        return nullptr; // Path is not under root
    }
    
    // Traverse the tree following the relative path
    std::shared_ptr<FileTreeNode> current = rootNode_;
    for (const auto& part : relativePath) {
        if (part == ".") continue;
        
        auto child = current->findChild(part.string());
        if (!child) return nullptr;
        current = child;
    }
    
    return current;
}

std::vector<std::shared_ptr<FileTreeNode>> FileTreeManager::getVisibleNodes() const {
    std::vector<std::shared_ptr<FileTreeNode>> result;
    if (rootNode_) {
        collectVisibleNodes(rootNode_, result);
    }
    return result;
}

void FileTreeManager::setShowHiddenFiles(bool show) {
    showHiddenFiles_.write([show](bool& current) {
        current = show;
    });
    refreshTree();
}

bool FileTreeManager::getShowHiddenFiles() const {
    return showHiddenFiles_.read([](const bool& current) {
        return current;
    });
}

void FileTreeManager::setOnFileSelected(std::function<void(const std::string&)> callback) {
    onFileSelected_ = callback;
}

void FileTreeManager::setOnDirectoryExpanded(std::function<void(const std::string&)> callback) {
    onDirectoryExpanded_ = callback;
}

bool FileTreeManager::createFile(const std::string& parentPath, const std::string& fileName) {
    auto parentNode = findNode(parentPath);
    if (!parentNode || !parentNode->isDirectory()) return false;
    
    fs::path filePath = fs::path(parentPath) / fileName;
    
    try {
        std::ofstream file(filePath);
        if (file.is_open()) {
            file.close();
            refreshNode(parentNode);
            return true;
        }
    } catch (const std::exception&) {
        // File creation failed
    }
    
    return false;
}

bool FileTreeManager::createDirectory(const std::string& parentPath, const std::string& dirName) {
    auto parentNode = findNode(parentPath);
    if (!parentNode || !parentNode->isDirectory()) return false;
    
    fs::path dirPath = fs::path(parentPath) / dirName;
    
    try {
        if (fs::create_directory(dirPath)) {
            refreshNode(parentNode);
            return true;
        }
    } catch (const std::exception&) {
        // Directory creation failed
    }
    
    return false;
}

bool FileTreeManager::deleteNode(const std::string& path) {
    auto node = findNode(path);
    if (!node) return false;
    
    try {
        if (fs::remove_all(path) > 0) {
            auto parent = node->parent.lock();
            if (parent) {
                refreshNode(parent);
            }
            return true;
        }
    } catch (const std::exception&) {
        // Deletion failed
    }
    
    return false;
}

bool FileTreeManager::renameNode(const std::string& path, const std::string& newName) {
    auto node = findNode(path);
    if (!node) return false;
    
    fs::path oldPath(path);
    fs::path newPath = oldPath.parent_path() / newName;
    
    try {
        fs::rename(oldPath, newPath);
        auto parent = node->parent.lock();
        if (parent) {
            refreshNode(parent);
        }
        return true;
    } catch (const std::exception&) {
        // Rename failed
    }
    
    return false;
}

void FileTreeManager::scanDirectory(std::shared_ptr<FileTreeNode> dirNode) {
    if (!dirNode || !dirNode->isDirectory()) return;
    
    dirNode->children.clear();
    
    try {
        for (const auto& entry : fs::directory_iterator(dirNode->fullPath)) {
            if (!shouldShowFile(entry)) continue;
            
            auto fileName = entry.path().filename().string();
            auto fileType = entry.is_directory() ? FileTreeNodeType::DIRECTORY : FileTreeNodeType::FILE;
            
            auto child = std::make_shared<FileTreeNode>(fileName, entry.path().string(), fileType);
            
            // Set file metadata
            try {
                if (entry.is_regular_file()) {
                    child->size = fs::file_size(entry);
                }
                child->lastModified = fs::last_write_time(entry);
            } catch (const std::exception&) {
                // Metadata access failed, use defaults
            }
            
            dirNode->addChild(child);
        }
    } catch (const std::exception&) {
        // Directory scan failed
    }
}

bool FileTreeManager::shouldShowFile(const std::filesystem::directory_entry& entry) const {
    auto fileName = entry.path().filename().string();
    
    // Always show if hidden files are enabled
    auto showHidden = getShowHiddenFiles();
    if (showHidden) return true;
    
    // Hide files starting with dot (Unix hidden files)
    if (fileName.front() == '.') return false;
    
    return true;
}

void FileTreeManager::notifyFileSelected(const std::string& path) {
    if (onFileSelected_) {
        onFileSelected_(path);
    }
}

void FileTreeManager::notifyDirectoryExpanded(const std::string& path) {
    if (onDirectoryExpanded_) {
        onDirectoryExpanded_(path);
    }
}

void FileTreeManager::clearSelectionRecursive(std::shared_ptr<FileTreeNode> node) {
    if (!node) return;
    
    node->isSelected = false;
    for (auto& child : node->children) {
        clearSelectionRecursive(child);
    }
}

void FileTreeManager::collectVisibleNodes(std::shared_ptr<FileTreeNode> node, std::vector<std::shared_ptr<FileTreeNode>>& result) const {
    if (!node) return;
    
    result.push_back(node);
    
    if (node->isDirectory() && node->isExpanded) {
        for (const auto& child : node->children) {
            collectVisibleNodes(child, result);
        }
    }
}

} // namespace bolt