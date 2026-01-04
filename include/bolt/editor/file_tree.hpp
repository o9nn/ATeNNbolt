#ifndef FILE_TREE_HPP
#define FILE_TREE_HPP

// Main file tree navigation component header
// This includes all file tree functionality for the editor

#include "file_tree_node.hpp"
#include "file_tree_manager.hpp"
#include "file_tree_ui.hpp"

namespace bolt {

// Convenience function to get the file tree manager instance
inline FileTreeManager& getFileTreeManager() {
    return FileTreeManager::getInstance();
}

// Convenience function to create a file tree UI
inline std::unique_ptr<FileTreeUI> createFileTreeUI() {
    return std::make_unique<FileTreeUI>();
}

} // namespace bolt

#endif