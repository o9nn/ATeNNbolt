#include <iostream>
#include <fstream>
#include "bolt/editor/integrated_editor.hpp"
#include "bolt/editor/split_view_manager.hpp"

using namespace bolt;

void createSampleFiles() {
    // Create some sample files to work with
    std::ofstream file1("/tmp/sample1.cpp");
    file1 << "// Sample C++ File 1\n"
          << "#include <iostream>\n\n"
          << "int main() {\n"
          << "    std::cout << \"Hello from file 1!\" << std::endl;\n"
          << "    return 0;\n"
          << "}\n";
    file1.close();
    
    std::ofstream file2("/tmp/sample2.cpp");
    file2 << "// Sample C++ File 2\n"
          << "#include <vector>\n"
          << "#include <algorithm>\n\n"
          << "void sortVector(std::vector<int>& vec) {\n"
          << "    std::sort(vec.begin(), vec.end());\n"
          << "}\n";
    file2.close();
    
    std::ofstream file3("/tmp/sample3.hpp");
    file3 << "// Sample Header File\n"
          << "#ifndef SAMPLE_HPP\n"
          << "#define SAMPLE_HPP\n\n"
          << "class SampleClass {\n"
          << "public:\n"
          << "    void doSomething();\n"
          << "private:\n"
          << "    int value_;\n"
          << "};\n\n"
          << "#endif\n";
    file3.close();
}

int main() {
    std::cout << "Split View Editing Demo" << std::endl;
    std::cout << "=======================" << std::endl << std::endl;
    
    // Create sample files
    createSampleFiles();
    
    // Get the integrated editor instance
    auto& editor = IntegratedEditor::getInstance();
    auto& splitManager = SplitViewManager::getInstance();
    
    std::cout << "1. Initial state:" << std::endl;
    std::cout << "   - Split view enabled: " << (editor.isSplitViewEnabled() ? "Yes" : "No") << std::endl;
    std::cout << "   - Number of panes: " << splitManager.getPaneCount() << std::endl;
    std::cout << "   - Has splits: " << (editor.hasSplits() ? "Yes" : "No") << std::endl << std::endl;
    
    // Open first document in the default pane
    std::cout << "2. Opening sample1.cpp in default pane..." << std::endl;
    std::ifstream file1("/tmp/sample1.cpp");
    std::string content1((std::istreambuf_iterator<char>(file1)), std::istreambuf_iterator<char>());
    file1.close();
    editor.openDocument("/tmp/sample1.cpp", content1);
    
    std::cout << "   - Active pane ID: " << editor.getActivePaneId() << std::endl;
    std::cout << "   - Open documents: " << editor.getOpenDocuments().size() << std::endl << std::endl;
    
    // Create horizontal split
    std::cout << "3. Creating horizontal split..." << std::endl;
    std::string hPaneId = editor.createHorizontalSplit();
    std::cout << "   - New pane ID: " << hPaneId << std::endl;
    std::cout << "   - Number of panes: " << splitManager.getPaneCount() << std::endl;
    std::cout << "   - Has splits: " << (editor.hasSplits() ? "Yes" : "No") << std::endl;
    std::cout << "   - Active pane ID: " << editor.getActivePaneId() << std::endl << std::endl;
    
    // Open second document in the new pane
    std::cout << "4. Opening sample2.cpp in the new horizontal pane..." << std::endl;
    editor.openDocumentInPane(hPaneId, "/tmp/sample2.cpp");
    std::cout << "   - Documents in pane " << hPaneId << ": " << 
                 (splitManager.getPane(hPaneId)->hasDocument() ? "1" : "0") << std::endl;
    
    auto openDocs = editor.getOpenDocuments();
    std::cout << "   - Total open documents: " << openDocs.size() << std::endl;
    for (const auto& doc : openDocs) {
        std::cout << "     * " << doc << std::endl;
    }
    std::cout << std::endl;
    
    // Create vertical split
    std::cout << "5. Creating vertical split..." << std::endl;
    std::string vPaneId = editor.createVerticalSplit();
    std::cout << "   - New pane ID: " << vPaneId << std::endl;
    std::cout << "   - Number of panes: " << splitManager.getPaneCount() << std::endl;
    std::cout << "   - Active pane ID: " << editor.getActivePaneId() << std::endl << std::endl;
    
    // Open third document in new vertical pane
    std::cout << "6. Opening sample3.hpp in the new vertical pane..." << std::endl;
    editor.openDocumentInPane(vPaneId, "/tmp/sample3.hpp");
    
    openDocs = editor.getOpenDocuments();
    std::cout << "   - Total open documents: " << openDocs.size() << std::endl;
    for (const auto& doc : openDocs) {
        std::string paneId = editor.findPaneWithDocument(doc);
        std::cout << "     * " << doc << " (in pane " << paneId << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Show all pane IDs
    std::cout << "7. All pane IDs:" << std::endl;
    auto allPanes = editor.getAllPaneIds();
    for (const auto& paneId : allPanes) {
        auto* pane = splitManager.getPane(paneId);
        std::cout << "   - " << paneId << 
                     (pane->hasFocus() ? " (ACTIVE)" : "") <<
                     (pane->hasDocument() ? " [" + pane->getDocumentPath() + "]" : " [empty]") << std::endl;
    }
    std::cout << std::endl;
    
    // Test navigation
    std::cout << "8. Testing pane navigation..." << std::endl;
    std::cout << "   - Current active: " << editor.getActivePaneId() << std::endl;
    editor.navigateToNextPane();
    std::cout << "   - After next: " << editor.getActivePaneId() << std::endl;
    editor.navigateToNextPane();
    std::cout << "   - After next again: " << editor.getActivePaneId() << std::endl;
    editor.navigateToPreviousPane();
    std::cout << "   - After previous: " << editor.getActivePaneId() << std::endl << std::endl;
    
    // Test closing a pane
    std::cout << "9. Testing pane closure..." << std::endl;
    std::cout << "   - Panes before close: " << splitManager.getPaneCount() << std::endl;
    bool closed = editor.closePane(vPaneId);
    std::cout << "   - Close successful: " << (closed ? "Yes" : "No") << std::endl;
    std::cout << "   - Panes after close: " << splitManager.getPaneCount() << std::endl;
    std::cout << "   - Active pane: " << editor.getActivePaneId() << std::endl;
    
    openDocs = editor.getOpenDocuments();
    std::cout << "   - Remaining open documents: " << openDocs.size() << std::endl;
    for (const auto& doc : openDocs) {
        std::cout << "     * " << doc << std::endl;
    }
    std::cout << std::endl;
    
    // Test collapsing splits
    std::cout << "10. Testing split collapse..." << std::endl;
    std::cout << "    - Has splits before collapse: " << (editor.hasSplits() ? "Yes" : "No") << std::endl;
    editor.collapseAllSplits();
    std::cout << "    - Has splits after collapse: " << (editor.hasSplits() ? "Yes" : "No") << std::endl;
    std::cout << "    - Panes after collapse: " << splitManager.getPaneCount() << std::endl;
    std::cout << "    - Active pane: " << editor.getActivePaneId() << std::endl << std::endl;
    
    std::cout << "Split view demo completed successfully!" << std::endl;
    std::cout << "Key features demonstrated:" << std::endl;
    std::cout << "- Creating horizontal and vertical splits" << std::endl;
    std::cout << "- Opening documents in specific panes" << std::endl;
    std::cout << "- Navigating between panes" << std::endl;
    std::cout << "- Closing individual panes" << std::endl;
    std::cout << "- Collapsing all splits" << std::endl;
    std::cout << "- Managing multiple documents across panes" << std::endl;
    
    return 0;
}