#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include "bolt/editor/minimap.hpp"
#include "bolt/editor/minimap_ui.hpp"

using namespace bolt;

void createLargeTestFile() {
    std::ofstream file("/tmp/large_file.cpp");
    
    file << "// Large C++ File for Minimap Demo\n";
    file << "// This file demonstrates minimap functionality with a larger codebase\n\n";
    
    file << "#include <iostream>\n";
    file << "#include <vector>\n";
    file << "#include <string>\n";
    file << "#include <algorithm>\n";
    file << "#include <memory>\n\n";
    
    file << "namespace demo {\n\n";
    
    // Add a large class with many methods
    file << "class LargeClass {\n";
    file << "private:\n";
    file << "    std::vector<std::string> data_;\n";
    file << "    int counter_;\n";
    file << "    bool enabled_;\n\n";
    
    file << "public:\n";
    file << "    LargeClass() : counter_(0), enabled_(true) {}\n\n";
    
    // Add many methods to create a long file
    for (int i = 1; i <= 50; ++i) {
        file << "    void method" << i << "() {\n";
        file << "        // Method " << i << " implementation\n";
        file << "        if (enabled_) {\n";
        file << "            counter_++;\n";
        file << "            data_.push_back(\"Method " << i << " called\");\n";
        file << "        }\n";
        file << "    }\n\n";
    }
    
    file << "    void processData() {\n";
    file << "        std::cout << \"Processing \" << data_.size() << \" items\" << std::endl;\n";
    file << "        for (const auto& item : data_) {\n";
    file << "            std::cout << \"  \" << item << std::endl;\n";
    file << "        }\n";
    file << "    }\n\n";
    
    file << "    int getCounter() const { return counter_; }\n";
    file << "    void setEnabled(bool enabled) { enabled_ = enabled; }\n";
    file << "    bool isEnabled() const { return enabled_; }\n";
    file << "};\n\n";
    
    // Add some free functions
    for (int i = 1; i <= 20; ++i) {
        file << "void freeFunction" << i << "() {\n";
        file << "    // Free function " << i << "\n";
        file << "    LargeClass obj;\n";
        file << "    obj.method" << (i % 50 + 1) << "();\n";
        file << "}\n\n";
    }
    
    file << "} // namespace demo\n\n";
    
    file << "int main() {\n";
    file << "    std::cout << \"Minimap Demo Application\" << std::endl;\n";
    file << "    \n";
    file << "    demo::LargeClass obj;\n";
    file << "    \n";
    file << "    // Call various methods\n";
    for (int i = 1; i <= 10; ++i) {
        file << "    obj.method" << i << "();\n";
    }
    file << "    \n";
    file << "    obj.processData();\n";
    file << "    \n";
    file << "    std::cout << \"Counter: \" << obj.getCounter() << std::endl;\n";
    file << "    \n";
    file << "    return 0;\n";
    file << "}\n";
    
    file.close();
}

std::vector<std::string> readFileLines(const std::string& filePath) {
    std::vector<std::string> lines;
    std::ifstream file(filePath);
    std::string line;
    
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void demonstrateBasicMinimap() {
    printSeparator("Basic Minimap Demo");
    
    // Create a minimap with custom configuration
    Minimap::MinimapConfig config;
    config.width = 60;
    config.height = 20;
    
    Minimap minimap(config);
    
    // Load the test file
    auto lines = readFileLines("/tmp/large_file.cpp");
    minimap.setDocument("/tmp/large_file.cpp", lines);
    
    std::cout << "Document loaded: " << minimap.getDocumentPath() << std::endl;
    std::cout << "Total lines: " << minimap.getTotalLines() << std::endl;
    std::cout << "Visible lines in minimap: " << minimap.getVisibleLines() << std::endl;
    
    // Set initial viewport
    Minimap::ViewportInfo viewport;
    viewport.startLine = 0;
    viewport.endLine = 25;
    viewport.currentLine = 10;
    viewport.totalLines = lines.size();
    
    minimap.updateViewport(viewport);
    
    std::cout << "\nMinimap rendering:" << std::endl;
    auto rendered = minimap.render();
    for (size_t i = 0; i < rendered.size(); ++i) {
        std::cout << std::setw(2) << i << ": " << rendered[i] << std::endl;
    }
}

void demonstrateMinimapUI() {
    printSeparator("Minimap UI Demo");
    
    Minimap::MinimapConfig minimapConfig;
    minimapConfig.width = 50;
    minimapConfig.height = 15;
    
    Minimap minimap(minimapConfig);
    auto lines = readFileLines("/tmp/large_file.cpp");
    minimap.setDocument("/tmp/large_file.cpp", lines);
    
    // Configure UI
    MinimapUI::UIConfig uiConfig;
    uiConfig.showBorder = true;
    uiConfig.showTitle = true;
    uiConfig.showScrollIndicator = true;
    uiConfig.title = "File Minimap";
    
    MinimapUI ui(minimap, uiConfig);
    
    // Set position
    MinimapUI::Position position{0, 0, 54, 18};
    ui.setPosition(position);
    
    // Set up navigation callback
    ui.setNavigationCallback([](size_t line) {
        std::cout << "Navigation callback: jumped to line " << line << std::endl;
    });
    
    // Set viewport around middle of file
    Minimap::ViewportInfo viewport;
    viewport.startLine = lines.size() / 2 - 10;
    viewport.endLine = lines.size() / 2 + 10;
    viewport.currentLine = lines.size() / 2;
    viewport.totalLines = lines.size();
    
    minimap.updateViewport(viewport);
    
    std::cout << "UI Requirements:" << std::endl;
    std::cout << "  Width: " << ui.getRequiredWidth() << std::endl;
    std::cout << "  Height: " << ui.getRequiredHeight() << std::endl;
    std::cout << "  Fits in area (60x20): " << (ui.fitsInArea(60, 20) ? "Yes" : "No") << std::endl;
    
    std::cout << "\nMinimap UI rendering:" << std::endl;
    auto rendered = ui.render();
    for (const auto& line : rendered) {
        std::cout << line << std::endl;
    }
}

void demonstrateNavigation() {
    printSeparator("Navigation Demo");
    
    Minimap minimap;
    auto lines = readFileLines("/tmp/large_file.cpp");
    minimap.setDocument("/tmp/large_file.cpp", lines);
    
    MinimapUI ui(minimap);
    
    std::cout << "Testing navigation functionality..." << std::endl;
    
    // Test clicking at different positions
    std::vector<std::pair<size_t, size_t>> testPositions = {
        {10, 5},    // Near top
        {10, 20},   // Near bottom
        {10, 10}    // Middle
    };
    
    for (const auto& pos : testPositions) {
        std::cout << "\nClicking at position (" << pos.first << ", " << pos.second << "):" << std::endl;
        size_t targetLine = minimap.getLineFromPosition(pos.first, pos.second);
        std::cout << "  Would navigate to line: " << targetLine << std::endl;
        
        auto [x, y] = minimap.getPositionFromLine(targetLine);
        std::cout << "  Line " << targetLine << " maps to position (" << x << ", " << y << ")" << std::endl;
    }
    
    // Test scrolling
    std::cout << "\nTesting scroll operations:" << std::endl;
    
    std::cout << "Scroll to top:" << std::endl;
    minimap.scrollToLine(0);
    std::cout << "  Scroll percentage: " << (minimap.getScrollPercentage() * 100) << "%" << std::endl;
    
    std::cout << "Scroll to middle:" << std::endl;
    minimap.scrollToLine(lines.size() / 2);
    std::cout << "  Scroll percentage: " << (minimap.getScrollPercentage() * 100) << "%" << std::endl;
    
    std::cout << "Scroll to bottom:" << std::endl;
    minimap.scrollToLine(lines.size() - 1);
    std::cout << "  Scroll percentage: " << (minimap.getScrollPercentage() * 100) << "%" << std::endl;
}

int main() {
    std::cout << "Bolt C++ Minimap Demo" << std::endl;
    std::cout << "=====================" << std::endl;
    
    // Create test file
    std::cout << "Creating large test file..." << std::endl;
    createLargeTestFile();
    
    auto lines = readFileLines("/tmp/large_file.cpp");
    std::cout << "Created file with " << lines.size() << " lines" << std::endl;
    
    try {
        // Run demonstrations
        demonstrateBasicMinimap();
        demonstrateMinimapUI();
        demonstrateNavigation();
        
        printSeparator("Demo Complete");
        std::cout << "Minimap implementation successfully demonstrated!" << std::endl;
        std::cout << "Features implemented:" << std::endl;
        std::cout << "  ✓ Miniaturized view of large files" << std::endl;
        std::cout << "  ✓ Current viewport indication" << std::endl;
        std::cout << "  ✓ Click-to-navigate functionality" << std::endl;
        std::cout << "  ✓ Configurable appearance" << std::endl;
        std::cout << "  ✓ UI integration with borders and titles" << std::endl;
        std::cout << "  ✓ Scroll position tracking" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during demo: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}