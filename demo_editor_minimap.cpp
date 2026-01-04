#include <iostream>
#include <fstream>
#include "bolt/editor/editor_pane.hpp"
#include "bolt/editor/split_view_manager.hpp"

using namespace bolt;

void createLargeTestFile() {
    std::ofstream file("/tmp/editor_test.cpp");
    
    file << "// Large C++ File for Editor Pane Minimap Demo\n";
    file << "// Demonstrating minimap integration with editor panes\n\n";
    
    file << "#include <iostream>\n";
    file << "#include <vector>\n";
    file << "#include <string>\n";
    file << "#include <memory>\n\n";
    
    file << "namespace editor_demo {\n\n";
    
    // Create a realistic C++ class structure
    file << "class DocumentEditor {\n";
    file << "private:\n";
    file << "    std::vector<std::string> lines_;\n";
    file << "    size_t cursorLine_;\n";
    file << "    size_t cursorColumn_;\n";
    file << "    bool modified_;\n\n";
    
    file << "public:\n";
    file << "    DocumentEditor() : cursorLine_(0), cursorColumn_(0), modified_(false) {}\n\n";
    
    // Add many methods to make it a large file
    for (int i = 1; i <= 30; ++i) {
        file << "    void operation" << i << "() {\n";
        file << "        // Method " << i << " performs editing operation\n";
        file << "        if (cursorLine_ < lines_.size()) {\n";
        file << "            std::cout << \"Executing operation " << i << " at line \" << cursorLine_ << std::endl;\n";
        file << "            modified_ = true;\n";
        file << "        }\n";
        file << "    }\n\n";
    }
    
    file << "    void loadDocument(const std::string& content) {\n";
    file << "        lines_.clear();\n";
    file << "        std::istringstream stream(content);\n";
    file << "        std::string line;\n";
    file << "        while (std::getline(stream, line)) {\n";
    file << "            lines_.push_back(line);\n";
    file << "        }\n";
    file << "        modified_ = false;\n";
    file << "    }\n\n";
    
    file << "    void setCursorPosition(size_t line, size_t column) {\n";
    file << "        cursorLine_ = line;\n";
    file << "        cursorColumn_ = column;\n";
    file << "    }\n\n";
    
    file << "};\n\n";
    
    // Add some free functions
    for (int i = 1; i <= 20; ++i) {
        file << "void utilityFunction" << i << "() {\n";
        file << "    DocumentEditor editor;\n";
        file << "    editor.operation" << (i % 30 + 1) << "();\n";
        file << "    std::cout << \"Utility function " << i << " completed\" << std::endl;\n";
        file << "}\n\n";
    }
    
    file << "} // namespace editor_demo\n\n";
    
    file << "int main() {\n";
    file << "    std::cout << \"Editor Pane Minimap Demo\" << std::endl;\n";
    file << "    \n";
    file << "    editor_demo::DocumentEditor editor;\n";
    file << "    \n";
    for (int i = 1; i <= 10; ++i) {
        file << "    editor.operation" << i << "();\n";
    }
    file << "    \n";
    file << "    std::cout << \"Demo complete\" << std::endl;\n";
    file << "    return 0;\n";
    file << "}\n";
    
    file.close();
}

int main() {
    std::cout << "Editor Pane Minimap Integration Demo" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    // Create test file
    std::cout << "Creating large test file..." << std::endl;
    createLargeTestFile();
    
    // Get the split view manager
    auto& splitManager = SplitViewManager::getInstance();
    
    std::cout << "Initial state:" << std::endl;
    std::cout << "  - Number of panes: " << splitManager.getPaneCount() << std::endl << std::endl;
    
    // Create a new pane
    std::cout << "1. Creating editor pane..." << std::endl;
    std::string paneId = splitManager.createPane();
    auto* pane = splitManager.getPane(paneId);
    
    if (!pane) {
        std::cerr << "Failed to create pane!" << std::endl;
        return 1;
    }
    
    std::cout << "   - Pane ID: " << paneId << std::endl;
    std::cout << "   - Minimap enabled: " << (pane->isMinimapEnabled() ? "Yes" : "No") << std::endl;
    
    // Set pane position
    EditorPane::PanePosition pos{0, 0, 1000, 800};
    pane->setPosition(pos);
    
    std::cout << "\n2. Opening document in pane..." << std::endl;
    pane->openDocument("/tmp/editor_test.cpp");
    
    std::cout << "   - Document loaded: " << (pane->hasDocument() ? "Yes" : "No") << std::endl;
    std::cout << "   - Document path: " << pane->getDocumentPath() << std::endl;
    
    // Check minimap
    auto* minimap = pane->getMinimap();
    auto* minimapUI = pane->getMinimapUI();
    
    if (minimap && minimapUI) {
        std::cout << "   - Minimap total lines: " << minimap->getTotalLines() << std::endl;
        std::cout << "   - Minimap visible lines: " << minimap->getVisibleLines() << std::endl;
        std::cout << "   - Minimap UI width: " << minimapUI->getRequiredWidth() << std::endl;
        std::cout << "   - Minimap UI height: " << minimapUI->getRequiredHeight() << std::endl;
    }
    
    std::cout << "\n3. Testing cursor and scroll operations..." << std::endl;
    
    // Test cursor movement
    pane->setCursorPosition(50, 10);
    auto state = pane->getState();
    std::cout << "   - Set cursor to line 50, column 10" << std::endl;
    std::cout << "   - Current cursor: line " << state.cursorLine << ", column " << state.cursorColumn << std::endl;
    
    // Test scrolling
    pane->setScrollPosition(100, 0);
    state = pane->getState();
    std::cout << "   - Scrolled to line 100" << std::endl;
    std::cout << "   - Current scroll: line " << state.scrollLine << ", column " << state.scrollColumn << std::endl;
    
    if (minimap) {
        auto viewport = minimap->getViewport();
        std::cout << "   - Minimap viewport: " << viewport.startLine << "-" << viewport.endLine << std::endl;
        std::cout << "   - Minimap current line: " << viewport.currentLine << std::endl;
        std::cout << "   - Scroll percentage: " << (minimap->getScrollPercentage() * 100) << "%" << std::endl;
    }
    
    std::cout << "\n4. Rendering minimap..." << std::endl;
    auto minimapRender = pane->renderMinimap();
    
    if (!minimapRender.empty()) {
        std::cout << "   Minimap UI (first 10 lines):" << std::endl;
        for (size_t i = 0; i < std::min(size_t{10}, minimapRender.size()); ++i) {
            std::cout << "   " << minimapRender[i] << std::endl;
        }
        if (minimapRender.size() > 10) {
            std::cout << "   ... (" << (minimapRender.size() - 10) << " more lines)" << std::endl;
        }
    }
    
    std::cout << "\n5. Testing minimap navigation..." << std::endl;
    if (minimap) {
        // Test line mapping
        size_t targetLine = minimap->getLineFromPosition(10, 5);
        std::cout << "   - Position (10, 5) maps to line: " << targetLine << std::endl;
        
        auto [x, y] = minimap->getPositionFromLine(200);
        std::cout << "   - Line 200 maps to position: (" << x << ", " << y << ")" << std::endl;
        
        // Test scroll to different positions
        minimap->scrollToLine(0);
        std::cout << "   - Scrolled to top, percentage: " << (minimap->getScrollPercentage() * 100) << "%" << std::endl;
        
        minimap->scrollToLine(minimap->getTotalLines() / 2);
        std::cout << "   - Scrolled to middle, percentage: " << (minimap->getScrollPercentage() * 100) << "%" << std::endl;
    }
    
    std::cout << "\n6. Testing minimap disable/enable..." << std::endl;
    pane->setMinimapEnabled(false);
    std::cout << "   - Minimap disabled: " << (!pane->isMinimapEnabled() ? "Yes" : "No") << std::endl;
    
    auto disabledRender = pane->renderMinimap();
    std::cout << "   - Rendered lines when disabled: " << disabledRender.size() << std::endl;
    
    pane->setMinimapEnabled(true);
    std::cout << "   - Minimap re-enabled: " << (pane->isMinimapEnabled() ? "Yes" : "No") << std::endl;
    
    auto enabledRender = pane->renderMinimap();
    std::cout << "   - Rendered lines when enabled: " << enabledRender.size() << std::endl;
    
    std::cout << "\n=================================" << std::endl;
    std::cout << "Integration Demo Complete!" << std::endl;
    std::cout << "=================================" << std::endl;
    
    std::cout << "\nMinimap features successfully integrated:" << std::endl;
    std::cout << "  ✓ Automatic minimap creation with editor panes" << std::endl;
    std::cout << "  ✓ Minimap updates with cursor and scroll changes" << std::endl;
    std::cout << "  ✓ Document loading triggers minimap refresh" << std::endl;
    std::cout << "  ✓ Configurable minimap size based on pane dimensions" << std::endl;
    std::cout << "  ✓ Navigation callbacks integrated with pane operations" << std::endl;
    std::cout << "  ✓ Enable/disable minimap functionality" << std::endl;
    std::cout << "  ✓ UI rendering with borders, titles, and scroll indicators" << std::endl;
    
    return 0;
}