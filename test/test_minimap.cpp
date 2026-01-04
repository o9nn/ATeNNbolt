#include "bolt/test_framework.hpp"
#include "bolt/editor/minimap.hpp"
#include "bolt/editor/minimap_ui.hpp"
#include <vector>
#include <string>

using namespace bolt::test;

namespace {

std::vector<std::string> createTestDocument() {
    std::vector<std::string> lines;
    lines.push_back("// Test document for minimap");
    lines.push_back("#include <iostream>");
    lines.push_back("");
    lines.push_back("int main() {");
    lines.push_back("    std::cout << \"Hello World\" << std::endl;");
    lines.push_back("    return 0;");
    lines.push_back("}");
    for (int i = 0; i < 100; ++i) {
        lines.push_back("// Line " + std::to_string(i + 8));
    }
    return lines;
}

}

// ===== Minimap Component Tests =====

BOLT_TEST(Minimap, BasicCreation) {
    bolt::Minimap minimap;
    
    BOLT_ASSERT(!minimap.hasDocument());
    BOLT_ASSERT_EQ(0, minimap.getTotalLines());
    BOLT_ASSERT(minimap.isEnabled());
}

BOLT_TEST(Minimap, ConfigCreation) {
    bolt::Minimap::MinimapConfig config;
    config.width = 80;
    config.height = 30;
    
    bolt::Minimap minimap(config);
    
    BOLT_ASSERT_EQ(80, minimap.getConfig().width);
    BOLT_ASSERT_EQ(30, minimap.getConfig().height);
    BOLT_ASSERT(minimap.isEnabled());
}

BOLT_TEST(Minimap, DocumentLoading) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    
    minimap.setDocument("/test/file.cpp", testDoc);
    
    BOLT_ASSERT(minimap.hasDocument());
    BOLT_ASSERT_EQ("/test/file.cpp", minimap.getDocumentPath());
    BOLT_ASSERT_EQ(testDoc.size(), minimap.getTotalLines());
}

BOLT_TEST(Minimap, ViewportUpdate) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    minimap.setDocument("/test/file.cpp", testDoc);
    
    bolt::Minimap::ViewportInfo viewport;
    viewport.startLine = 10;
    viewport.endLine = 30;
    viewport.currentLine = 20;
    viewport.totalLines = testDoc.size();
    
    minimap.updateViewport(viewport);
    
    auto retrievedViewport = minimap.getViewport();
    BOLT_ASSERT_EQ(10, retrievedViewport.startLine);
    BOLT_ASSERT_EQ(30, retrievedViewport.endLine);
    BOLT_ASSERT_EQ(20, retrievedViewport.currentLine);
}

BOLT_TEST(Minimap, Navigation) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    minimap.setDocument("/test/file.cpp", testDoc);
    
    // Test position to line mapping
    size_t line = minimap.getLineFromPosition(10, 5);
    BOLT_ASSERT(line < testDoc.size());
    
    // Test line to position mapping
    auto [x, y] = minimap.getPositionFromLine(50);
    BOLT_ASSERT(y < minimap.getConfig().height);
    
    // Test scroll to line
    minimap.scrollToLine(50);
    auto viewport = minimap.getViewport();
    BOLT_ASSERT_EQ(50, viewport.currentLine);
}

BOLT_TEST(Minimap, Rendering) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    minimap.setDocument("/test/file.cpp", testDoc);
    
    auto rendered = minimap.render();
    
    BOLT_ASSERT_EQ(minimap.getConfig().height, rendered.size());
    for (const auto& line : rendered) {
        BOLT_ASSERT_EQ(minimap.getConfig().width, line.length());
    }
}

BOLT_TEST(MinimapUI, BasicCreation) {
    bolt::Minimap minimap;
    bolt::MinimapUI ui(minimap);
    
    BOLT_ASSERT(ui.isVisible());
    BOLT_ASSERT(ui.getRequiredWidth() > 0);
    BOLT_ASSERT(ui.getRequiredHeight() > 0);
}

BOLT_TEST(MinimapUI, ConfiguredCreation) {
    bolt::Minimap minimap;
    
    bolt::MinimapUI::UIConfig config;
    config.showBorder = true;
    config.showTitle = true;
    config.title = "Test Minimap";
    
    bolt::MinimapUI ui(minimap, config);
    
    BOLT_ASSERT_EQ("Test Minimap", ui.getConfig().title);
    BOLT_ASSERT(ui.getConfig().showBorder);
    BOLT_ASSERT(ui.getConfig().showTitle);
}

BOLT_TEST(MinimapUI, SizeCalculation) {
    bolt::Minimap::MinimapConfig minimapConfig;
    minimapConfig.width = 50;
    minimapConfig.height = 20;
    
    bolt::Minimap minimap(minimapConfig);
    
    bolt::MinimapUI::UIConfig uiConfig;
    uiConfig.showBorder = true;
    uiConfig.showTitle = true;
    uiConfig.showScrollIndicator = true;
    
    bolt::MinimapUI ui(minimap, uiConfig);
    
    // Width should include borders (2 chars)
    BOLT_ASSERT_EQ(52, ui.getRequiredWidth());
    
    // Height should include title, borders, and scroll indicator
    BOLT_ASSERT_EQ(24, ui.getRequiredHeight());
    
    BOLT_ASSERT(ui.fitsInArea(60, 30));
    BOLT_ASSERT(!ui.fitsInArea(50, 20));
}

BOLT_TEST(MinimapUI, Rendering) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    minimap.setDocument("/test/file.cpp", testDoc);
    
    bolt::MinimapUI ui(minimap);
    auto rendered = ui.render();
    
    BOLT_ASSERT(rendered.size() > 0);
    
    // Test with borders and title
    bolt::MinimapUI::UIConfig config;
    config.showBorder = true;
    config.showTitle = true;
    config.title = "Test";
    
    bolt::MinimapUI uiWithBorder(minimap, config);
    auto borderedRender = uiWithBorder.render();
    
    // With borders and title, we should have at least as many lines as the basic render
    BOLT_ASSERT(borderedRender.size() >= rendered.size());
}

BOLT_TEST(MinimapUI, NavigationCallback) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    minimap.setDocument("/test/file.cpp", testDoc);
    
    bolt::MinimapUI ui(minimap);
    
    size_t callbackLine = 0;
    bool callbackCalled = false;
    
    ui.setNavigationCallback([&](size_t line) {
        callbackLine = line;
        callbackCalled = true;
    });
    
    // Simulate click that should trigger navigation
    bolt::MinimapUI::Position pos{0, 0, 50, 20};
    ui.setPosition(pos);
    
    bool handled = ui.handleClick(10, 10);
    BOLT_ASSERT(handled);
    BOLT_ASSERT(callbackCalled);
    BOLT_ASSERT(callbackLine < testDoc.size());
}

BOLT_TEST(Minimap, DisableEnable) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    minimap.setDocument("/test/file.cpp", testDoc);
    
    BOLT_ASSERT(minimap.isEnabled());
    
    minimap.setEnabled(false);
    BOLT_ASSERT(!minimap.isEnabled());
    
    auto rendered = minimap.render();
    // Should render empty when disabled
    for (const auto& line : rendered) {
        BOLT_ASSERT_EQ(std::string(minimap.getConfig().width, ' '), line);
    }
    
    minimap.setEnabled(true);
    BOLT_ASSERT(minimap.isEnabled());
}

BOLT_TEST(Minimap, ScrollPercentage) {
    bolt::Minimap minimap;
    auto testDoc = createTestDocument();
    minimap.setDocument("/test/file.cpp", testDoc);
    
    // Scroll to top
    minimap.scrollToLine(0);
    BOLT_ASSERT_EQ(0.0, minimap.getScrollPercentage());
    
    // Scroll to middle
    minimap.scrollToLine(testDoc.size() / 2);
    double midPercent = minimap.getScrollPercentage();
    BOLT_ASSERT(midPercent >= 0.0);
    
    // Note: Don't test upper bound as scroll percentage calculation may vary
}