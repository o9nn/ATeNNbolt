#include "bolt/editor/file_tree_manager.hpp"
#include "bolt/editor/file_tree_ui.hpp"
#include <iostream>
#include <string>
#include <filesystem>

using namespace bolt;

void demonstrateFileTree() {
    std::cout << "Bolt C++ File Tree Navigation Component Demo\n";
    std::cout << "============================================\n\n";

    // Get current directory as the demo directory
    std::string currentDir = std::filesystem::current_path().string();
    std::cout << "Scanning directory: " << currentDir << "\n\n";

    // Initialize the file tree manager
    auto& manager = FileTreeManager::getInstance();
    
    // Set up callbacks to demonstrate integration
    manager.setOnFileSelected([](const std::string& path) {
        std::cout << ">>> FILE SELECTED: " << path << std::endl;
    });
    
    manager.setOnDirectoryExpanded([](const std::string& path) {
        std::cout << ">>> DIRECTORY EXPANDED: " << path << std::endl;
    });

    // Set the root directory to current directory
    manager.setRootDirectory(currentDir);

    // Create UI component
    FileTreeUI ui;
    
    // Configure rendering options
    FileTreeRenderOptions options;
    options.showIcons = true;
    options.indentSize = 2;
    options.expandedIcon = 'v';
    options.collapsedIcon = '>';
    options.fileIcon = '-';
    options.directoryIcon = '/';
    ui.setRenderOptions(options);

    // Display the initial tree
    std::cout << "Initial File Tree:\n";
    ui.render();
    std::cout << "\n";

    // Demonstrate selecting a file
    auto rootNode = manager.getRootNode();
    if (rootNode && !rootNode->children.empty()) {
        // Find the first file in the tree
        auto firstFile = rootNode->children[0];
        for (const auto& child : rootNode->children) {
            if (child->isFile()) {
                firstFile = child;
                break;
            }
        }
        
        std::cout << "Selecting file: " << firstFile->name << "\n";
        manager.selectFile(firstFile->fullPath);
        ui.render();
        std::cout << "\n";
    }

    // Demonstrate directory expansion/collapse
    auto visibleNodes = manager.getVisibleNodes();
    for (const auto& node : visibleNodes) {
        if (node->isDirectory() && !node->isExpanded) {
            std::cout << "Expanding directory: " << node->name << "\n";
            manager.expandDirectory(node->fullPath);
            ui.render();
            std::cout << "\n";
            
            std::cout << "Collapsing directory: " << node->name << "\n";
            manager.collapseDirectory(node->fullPath);
            ui.render();
            std::cout << "\n";
            break; // Just demo with first expandable directory
        }
    }

    // Show hidden files toggle
    std::cout << "Toggling hidden files display:\n";
    manager.setShowHiddenFiles(true);
    std::cout << "With hidden files:\n";
    ui.render();
    std::cout << "\n";

    manager.setShowHiddenFiles(false);
    std::cout << "Without hidden files:\n";
    ui.render();
    std::cout << "\n";

    // Demonstrate line-based navigation (simulating UI clicks)
    auto lines = ui.renderToLines();
    std::cout << "File tree as lines (for UI integration):\n";
    for (size_t i = 0; i < lines.size() && i < 10; ++i) {
        std::cout << "Line " << i << ": " << lines[i] << std::endl;
    }
    std::cout << "... (showing first 10 lines)\n\n";

    std::cout << "Demo completed!\n";
    std::cout << "The file tree component is ready for integration with the editor.\n";
}

int main() {
    try {
        demonstrateFileTree();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}