#include "bolt/editor/file_tree_node.hpp"
#include <algorithm>

namespace bolt {

std::shared_ptr<FileTreeNode> FileTreeNode::findChild(const std::string& name) const {
    auto it = std::find_if(children.begin(), children.end(), 
        [&name](const std::shared_ptr<FileTreeNode>& child) {
            return child->name == name;
        });
    return (it != children.end()) ? *it : nullptr;
}

void FileTreeNode::addChild(std::shared_ptr<FileTreeNode> child) {
    if (child) {
        child->parent = shared_from_this();
        children.push_back(child);
        sortChildren();
    }
}

void FileTreeNode::removeChild(const std::string& name) {
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [&name](const std::shared_ptr<FileTreeNode>& child) {
                return child->name == name;
            }),
        children.end()
    );
}

void FileTreeNode::sortChildren() {
    std::sort(children.begin(), children.end(),
        [](const std::shared_ptr<FileTreeNode>& a, const std::shared_ptr<FileTreeNode>& b) {
            // Directories first, then files
            if (a->isDirectory() && b->isFile()) return true;
            if (a->isFile() && b->isDirectory()) return false;
            // Within same type, sort alphabetically
            return a->name < b->name;
        });
}

size_t FileTreeNode::getDepth() const {
    size_t depth = 0;
    auto current = parent.lock();
    while (current) {
        depth++;
        current = current->parent.lock();
    }
    return depth;
}

std::vector<std::shared_ptr<FileTreeNode>> FileTreeNode::getAllFiles() const {
    std::vector<std::shared_ptr<FileTreeNode>> files;
    
    if (isFile()) {
        files.push_back(std::const_pointer_cast<FileTreeNode>(shared_from_this()));
    }
    
    for (const auto& child : children) {
        auto childFiles = child->getAllFiles();
        files.insert(files.end(), childFiles.begin(), childFiles.end());
    }
    
    return files;
}

} // namespace bolt