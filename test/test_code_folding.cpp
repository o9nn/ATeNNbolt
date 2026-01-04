#include "bolt/test_framework.hpp"
#include "bolt/editor/code_folding.hpp"
#include "bolt/editor/code_folding_detector.hpp"
#include "bolt/editor/code_folding_manager.hpp"
#include "bolt/editor/integrated_editor.hpp"
#include <iostream>

namespace bolt {
namespace test {

// Helper assertion macros for comparisons
#define BOLT_ASSERT_GE(a, b) \
    do { \
        if (!((a) >= (b))) { \
            std::stringstream ss; \
            ss << "Expected " << (a) << " >= " << (b); \
            throw bolt::test::TestFailure(ss.str(), __FILE__, __LINE__); \
        } \
    } while (0)

// Test fold range creation
BOLT_TEST(CodeFolding, FoldRangeCreation) {
    FoldRange range{10, 20, false, "test"};
    BOLT_ASSERT_EQ(static_cast<size_t>(10), range.startLine);
    BOLT_ASSERT_EQ(static_cast<size_t>(20), range.endLine);
    BOLT_ASSERT_EQ(false, range.isFolded);
    BOLT_ASSERT_EQ(std::string("test"), range.placeholder);
}

BOLT_TEST(CodeFolding, Singleton) {
    auto& folding1 = CodeFolding::getInstance();
    auto& folding2 = CodeFolding::getInstance();
    BOLT_ASSERT_EQ(&folding1, &folding2);
}

BOLT_TEST(CodeFolding, AddFoldRange) {
    auto& folding = CodeFolding::getInstance();
    folding.addFoldRange("/test/file_cf.cpp", 5, 15);

    auto ranges = folding.getFoldingRanges("/test/file_cf.cpp");
    BOLT_ASSERT_GE(ranges.size(), static_cast<size_t>(1));
    BOLT_ASSERT_EQ(static_cast<size_t>(5), ranges[0].startLine);
    BOLT_ASSERT_EQ(static_cast<size_t>(15), ranges[0].endLine);
    BOLT_ASSERT_EQ(false, ranges[0].isFolded);
}

BOLT_TEST(CodeFolding, ToggleFold) {
    auto& folding = CodeFolding::getInstance();
    folding.addFoldRange("/test/toggle_cf.cpp", 8, 12);

    // Initially not folded
    auto ranges = folding.getFoldingRanges("/test/toggle_cf.cpp");
    BOLT_ASSERT_GE(ranges.size(), static_cast<size_t>(1));
    BOLT_ASSERT_EQ(false, ranges[0].isFolded);

    // Toggle fold
    folding.toggleFold("/test/toggle_cf.cpp", 10);
    ranges = folding.getFoldingRanges("/test/toggle_cf.cpp");
    BOLT_ASSERT_EQ(true, ranges[0].isFolded);

    // Toggle again
    folding.toggleFold("/test/toggle_cf.cpp", 10);
    ranges = folding.getFoldingRanges("/test/toggle_cf.cpp");
    BOLT_ASSERT_EQ(false, ranges[0].isFolded);
}

// Code Folding Detector tests
BOLT_TEST(CodeFoldingDetector, SimpleBraceDetection) {
    std::string code = "int main() {\n"
                      "    return 0;\n"
                      "}\n";

    auto ranges = CodeFoldingDetector::detectFoldableRanges(code);
    BOLT_ASSERT_GE(ranges.size(), static_cast<size_t>(1));
    BOLT_ASSERT_EQ(static_cast<size_t>(0), ranges[0].startLine);
    BOLT_ASSERT_EQ(static_cast<size_t>(2), ranges[0].endLine);
    BOLT_ASSERT_EQ(false, ranges[0].isFolded);
}

BOLT_TEST(CodeFoldingDetector, NestedBraces) {
    std::string code = "class Test {\n"
                      "public:\n"
                      "    void func() {\n"
                      "        if (true) {\n"
                      "            return;\n"
                      "        }\n"
                      "    }\n"
                      "}\n";

    auto ranges = CodeFoldingDetector::detectFoldableRanges(code);
    BOLT_ASSERT_GE(ranges.size(), static_cast<size_t>(3)); // Class, function, if statement
}

BOLT_TEST(CodeFoldingDetector, FoldableRegionDetection) {
    BOLT_ASSERT_TRUE(CodeFoldingDetector::isFoldableRegion("class MyClass {"));
    BOLT_ASSERT_TRUE(CodeFoldingDetector::isFoldableRegion("  function test() {"));
    BOLT_ASSERT_TRUE(CodeFoldingDetector::isFoldableRegion("if (condition) {"));
    BOLT_ASSERT_FALSE(CodeFoldingDetector::isFoldableRegion("int x = 5;"));
}

// Code Folding Manager tests
BOLT_TEST(CodeFoldingManager, Singleton) {
    auto& manager1 = CodeFoldingManager::getInstance();
    auto& manager2 = CodeFoldingManager::getInstance();
    BOLT_ASSERT_EQ(&manager1, &manager2);
}

BOLT_TEST(CodeFoldingManager, EnableDisableFolding) {
    auto& manager = CodeFoldingManager::getInstance();

    manager.setFoldingEnabled(true);
    BOLT_ASSERT_TRUE(manager.isFoldingEnabled());

    manager.setFoldingEnabled(false);
    BOLT_ASSERT_FALSE(manager.isFoldingEnabled());
}

BOLT_TEST(CodeFoldingManager, UpdateFoldingRanges) {
    auto& manager = CodeFoldingManager::getInstance();
    manager.setFoldingEnabled(true);

    std::string code = "void test() {\n"
                      "    int x = 1;\n"
                      "}\n";

    manager.updateFoldingRanges("/test/manager_cf.cpp", code);
    auto ranges = manager.getFoldingRanges("/test/manager_cf.cpp");
    BOLT_ASSERT_GE(ranges.size(), static_cast<size_t>(1));
}

BOLT_TEST(CodeFoldingManager, ToggleFold) {
    auto& manager = CodeFoldingManager::getInstance();
    manager.setFoldingEnabled(true);

    std::string code = "int func() {\n"
                      "    return 42;\n"
                      "}\n";

    manager.updateFoldingRanges("/test/toggle_mgr_cf.cpp", code);

    // Check initial state
    auto ranges = manager.getFoldingRanges("/test/toggle_mgr_cf.cpp");
    BOLT_ASSERT_GE(ranges.size(), static_cast<size_t>(1));
    bool initialState = ranges[0].isFolded;

    // Toggle
    manager.toggleFold("/test/toggle_mgr_cf.cpp", ranges[0].startLine);

    // Check state changed
    ranges = manager.getFoldingRanges("/test/toggle_mgr_cf.cpp");
    BOLT_ASSERT_NE(ranges[0].isFolded, initialState);
}

// Integrated Editor tests
BOLT_TEST(IntegratedEditor, Singleton) {
    auto& editor1 = IntegratedEditor::getInstance();
    auto& editor2 = IntegratedEditor::getInstance();
    BOLT_ASSERT_EQ(&editor1, &editor2);
}

BOLT_TEST(IntegratedEditor, OpenDocumentWithFolding) {
    auto& editor = IntegratedEditor::getInstance();

    std::string code = "class TestClass {\n"
                      "public:\n"
                      "    void method() {\n"
                      "        // code here\n"
                      "    }\n"
                      "}\n";

    editor.openDocument("/test/integrated_cf.cpp", code);

    auto ranges = editor.getFoldingRanges("/test/integrated_cf.cpp");
    BOLT_ASSERT_GE(ranges.size(), static_cast<size_t>(1));
}

BOLT_TEST(IntegratedEditor, FoldingOperations) {
    auto& editor = IntegratedEditor::getInstance();
    editor.setFoldingEnabled(true);

    std::string code = "namespace test {\n"
                      "    int value = 100;\n"
                      "}\n";

    editor.openDocument("/test/operations_cf.cpp", code);

    auto ranges = editor.getFoldingRanges("/test/operations_cf.cpp");
    if (!ranges.empty()) {
        // Test toggle
        editor.toggleFold("/test/operations_cf.cpp", ranges[0].startLine);

        // Test expand all
        editor.expandAllFolds("/test/operations_cf.cpp");

        // Test collapse all
        editor.collapseAllFolds("/test/operations_cf.cpp");
    }

    BOLT_ASSERT_TRUE(editor.isFoldingEnabled());
}

BOLT_TEST(IntegratedEditor, UpdateContentRefolding) {
    auto& editor = IntegratedEditor::getInstance();

    std::string originalCode = "int x = 1;\n";
    std::string newCode = "struct Data {\n"
                         "    int value;\n"
                         "};\n";

    editor.openDocument("/test/update_cf.cpp", originalCode);
    auto originalRanges = editor.getFoldingRanges("/test/update_cf.cpp");

    editor.updateDocumentContent("/test/update_cf.cpp", newCode);
    auto newRanges = editor.getFoldingRanges("/test/update_cf.cpp");

    // New code should have folding ranges
    BOLT_ASSERT_GE(newRanges.size(), static_cast<size_t>(1));
}

} // namespace test
} // namespace bolt

int main() {
    return bolt::test::TestSuite::getInstance().runAllTests();
}
