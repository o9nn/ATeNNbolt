#include "bolt/test_framework.hpp"
#include "bolt/editor/file_tree_manager.hpp"
#include "bolt/editor/file_tree_ui.hpp"
#include <filesystem>
#include <fstream>

using namespace bolt::test;

// ===== File Tree Node Tests =====

BOLT_TEST(FileTree, NodeCreation) {
    auto node = std::make_shared<bolt::FileTreeNode>("test.txt", "/tmp/test.txt", bolt::FileTreeNodeType::FILE);
    BOLT_ASSERT_EQ("test.txt", node->name);
    BOLT_ASSERT_EQ("/tmp/test.txt", node->fullPath);
    BOLT_ASSERT(node->isFile());
    BOLT_ASSERT(!node->isDirectory());
    BOLT_ASSERT(!node->isSelected);
}

BOLT_TEST(FileTree, NodeHierarchy) {
    auto parent = std::make_shared<bolt::FileTreeNode>("parent", "/tmp/parent", bolt::FileTreeNodeType::DIRECTORY);
    auto child = std::make_shared<bolt::FileTreeNode>("child.txt", "/tmp/parent/child.txt", bolt::FileTreeNodeType::FILE);
    
    parent->addChild(child);
    
    BOLT_ASSERT_EQ(1, parent->children.size());
    BOLT_ASSERT_EQ(child.get(), parent->children[0].get());
    BOLT_ASSERT_EQ(parent.get(), child->parent.lock().get());
}

// ===== File Tree Manager Tests =====

BOLT_TEST(FileTree, ManagerSingleton) {
    auto& manager1 = bolt::FileTreeManager::getInstance();
    auto& manager2 = bolt::FileTreeManager::getInstance();
    BOLT_ASSERT_EQ(&manager1, &manager2);
}

BOLT_TEST(FileTree, ManagerSettings) {
    auto& manager = bolt::FileTreeManager::getInstance();
    BOLT_ASSERT(!manager.getShowHiddenFiles());
    
    manager.setShowHiddenFiles(true);
    BOLT_ASSERT(manager.getShowHiddenFiles());
    
    manager.setShowHiddenFiles(false);
    BOLT_ASSERT(!manager.getShowHiddenFiles());
}

BOLT_TEST(FileTree, DirectoryScanning) {
    // Create a temporary test directory structure
    std::string testDir = "/tmp/bolt_test_filetree_scan";
    std::filesystem::create_directories(testDir);
    std::filesystem::create_directories(testDir + "/subdir");
    
    // Create test files
    std::ofstream(testDir + "/file1.txt") << "test content";
    std::ofstream(testDir + "/file2.cpp") << "// C++ code";
    std::ofstream(testDir + "/subdir/nested.txt") << "nested content";
    
    auto& manager = bolt::FileTreeManager::getInstance();
    manager.setRootDirectory(testDir);
    
    BOLT_ASSERT_EQ(testDir, manager.getRootDirectory());
    auto rootNode = manager.getRootNode();
    BOLT_ASSERT(rootNode != nullptr);
    BOLT_ASSERT(rootNode->isDirectory());
    BOLT_ASSERT(rootNode->isExpanded);
    
    // Should have at least the files and subdirectory we created
    BOLT_ASSERT(rootNode->children.size() >= 3);
    
    // Cleanup
    std::filesystem::remove_all(testDir);
}

// ===== File Tree UI Tests =====

BOLT_TEST(FileTree, UIBasics) {
    bolt::FileTreeUI ui;
    BOLT_ASSERT(ui.isVisible());
    
    ui.setVisible(false);
    BOLT_ASSERT(!ui.isVisible());
    
    ui.setVisible(true);
    BOLT_ASSERT(ui.isVisible());
}

BOLT_TEST(FileTree, UIRendering) {
    // Create a small test directory
    std::string testDir = "/tmp/bolt_test_ui_render";
    std::filesystem::create_directories(testDir);
    std::ofstream(testDir + "/test.txt") << "test";
    
    auto& manager = bolt::FileTreeManager::getInstance();
    manager.setRootDirectory(testDir);
    
    bolt::FileTreeUI ui;
    auto lines = ui.renderToLines();
    BOLT_ASSERT(lines.size() >= 2); // At least header and one file
    
    std::string output;
    ui.renderToString(output);
    BOLT_ASSERT(!output.empty());
    
    // Cleanup
    std::filesystem::remove_all(testDir);
}

// ===== File Operations Tests =====

BOLT_TEST(FileTree, FileSelection) {
    std::string testDir = "/tmp/bolt_test_selection";
    std::filesystem::create_directories(testDir);
    std::ofstream(testDir + "/select_me.txt") << "content";
    
    auto& manager = bolt::FileTreeManager::getInstance();
    manager.setRootDirectory(testDir);
    
    std::string selectedPath = testDir + "/select_me.txt";
    manager.selectFile(selectedPath);
    
    auto selectedNode = manager.getSelectedNode();
    BOLT_ASSERT(selectedNode != nullptr);
    BOLT_ASSERT_EQ(selectedPath, selectedNode->fullPath);
    BOLT_ASSERT(selectedNode->isSelected);
    
    manager.clearSelection();
    selectedNode = manager.getSelectedNode();
    BOLT_ASSERT(selectedNode == nullptr);
    
    // Cleanup
    std::filesystem::remove_all(testDir);
}

BOLT_TEST(FileTree, DirectoryExpansion) {
    std::string testDir = "/tmp/bolt_test_expansion";
    std::filesystem::create_directories(testDir + "/expand_test");
    std::ofstream(testDir + "/expand_test/hidden.txt") << "content";
    
    auto& manager = bolt::FileTreeManager::getInstance();
    manager.setRootDirectory(testDir);
    
    auto rootNode = manager.getRootNode();
    BOLT_ASSERT(rootNode != nullptr);
    BOLT_ASSERT(rootNode->isExpanded);
    
    // Find the subdirectory
    auto subdirPath = testDir + "/expand_test";
    auto subdirNode = manager.findNode(subdirPath);
    BOLT_ASSERT(subdirNode != nullptr);
    BOLT_ASSERT(!subdirNode->isExpanded); // Should start collapsed
    
    manager.expandDirectory(subdirPath);
    BOLT_ASSERT(subdirNode->isExpanded);
    
    manager.collapseDirectory(subdirPath);
    BOLT_ASSERT(!subdirNode->isExpanded);
    
    // Cleanup
    std::filesystem::remove_all(testDir);
}