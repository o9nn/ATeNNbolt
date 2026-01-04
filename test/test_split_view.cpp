#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include "bolt/editor/split_view_manager.hpp"
#include "bolt/editor/editor_pane.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include "bolt/core/editor_store.hpp"

using namespace bolt;

void test_editor_pane_basic() {
    std::cout << "[SplitView] EditorPaneBasic ... ";
    
    EditorPane pane("test_pane_1");
    
    assert(pane.getId() == "test_pane_1");
    assert(!pane.hasDocument());
    assert(!pane.hasFocus());
    assert(pane.isVisible());
    
    pane.setFocus(true);
    assert(pane.hasFocus());
    
    pane.setFocus(false);
    assert(!pane.hasFocus());
    
    std::cout << "PASS" << std::endl;
}

void test_split_view_manager_basic() {
    std::cout << "[SplitView] SplitViewManagerBasic ... ";
    
    auto& manager = SplitViewManager::getInstance();
    
    assert(manager.isEnabled());
    assert(manager.getPaneCount() >= 1);
    
    // Test horizontal split
    size_t initialCount = manager.getPaneCount();
    std::string hPaneId = manager.createHorizontalSplit();
    assert(!hPaneId.empty());
    assert(manager.getPaneCount() == initialCount + 1);
    assert(manager.hasSplits());
    
    // Test vertical split
    std::string vPaneId = manager.createVerticalSplit();
    assert(!vPaneId.empty());
    assert(manager.getPaneCount() == initialCount + 2);
    
    // Verify panes exist
    auto* hPane = manager.getPane(hPaneId);
    auto* vPane = manager.getPane(vPaneId);
    assert(hPane != nullptr);
    assert(vPane != nullptr);
    
    std::cout << "PASS" << std::endl;
}

void test_integrated_editor_split() {
    std::cout << "[SplitView] IntegratedEditorSplit ... ";
    
    auto& editor = IntegratedEditor::getInstance();
    
    assert(editor.isSplitViewEnabled());
    
    // Create splits
    std::string hSplit = editor.createHorizontalSplit();
    std::string vSplit = editor.createVerticalSplit();
    
    assert(!hSplit.empty());
    assert(!vSplit.empty());
    assert(editor.hasSplits());
    
    // Test pane management
    std::string activeId = editor.getActivePaneId();
    assert(!activeId.empty());
    
    auto* activePane = editor.getActivePane();
    assert(activePane != nullptr);
    
    auto allIds = editor.getAllPaneIds();
    assert(allIds.size() >= 3);
    
    std::cout << "PASS" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "Split View Editing Tests" << std::endl;
    std::cout << "========================" << std::endl;
    
    try {
        test_editor_pane_basic();
        test_split_view_manager_basic();
        test_integrated_editor_split();
        
        std::cout << std::endl << "All split view tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}