#include "bolt/core/code_analyzer.hpp"
#include "bolt/test_framework.hpp"
#include <fstream>
#include <filesystem>

using namespace bolt;

TEST_CASE("ComplexityAnalyzer basic functionality") {
    ComplexityAnalyzer analyzer;
    
    SECTION("supported extensions") {
        auto extensions = analyzer.getSupportedExtensions();
        REQUIRE(!extensions.empty());
        REQUIRE(std::find(extensions.begin(), extensions.end(), ".cpp") != extensions.end());
        REQUIRE(std::find(extensions.begin(), extensions.end(), ".hpp") != extensions.end());
    }
    
    SECTION("analyze simple code") {
        std::string simpleCode = R"(
#include <iostream>

void simpleFunction() {
    std::cout << "Hello World" << std::endl;
}

class SimpleClass {
public:
    void method() {}
};
)";
        
        auto issues = analyzer.analyzeFile("test.cpp", simpleCode);
        auto metrics = analyzer.getMetrics("test.cpp", simpleCode);
        
        REQUIRE(metrics.functionCount == 2); // simpleFunction + method
        REQUIRE(metrics.classCount == 1);
        REQUIRE(metrics.linesOfCode > 0);
    }
    
    SECTION("detect high complexity") {
        std::string complexCode = R"(
void complexFunction(int x) {
    if (x > 0) {
        if (x < 100) {
            if (x % 2 == 0) {
                if (x % 4 == 0) {
                    if (x % 8 == 0) {
                        for (int i = 0; i < x; ++i) {
                            switch (i) {
                                case 1: break;
                                case 2: break;
                                case 3: break;
                                default: break;
                            }
                        }
                    }
                }
            }
        }
    }
}
)";
        
        auto issues = analyzer.analyzeFile("complex.cpp", complexCode);
        auto metrics = analyzer.getMetrics("complex.cpp", complexCode);
        
        REQUIRE(metrics.cyclomaticComplexity > 10);
        REQUIRE(metrics.nestingDepth > 4);
        
        // Should have complexity warnings
        bool hasComplexityIssue = false;
        for (const auto& issue : issues) {
            if (issue.category == AnalysisCategory::COMPLEXITY) {
                hasComplexityIssue = true;
                break;
            }
        }
        REQUIRE(hasComplexityIssue);
    }
}

TEST_CASE("SecurityAnalyzer functionality") {
    SecurityAnalyzer analyzer;
    
    SECTION("detect unsafe functions") {
        std::string unsafeCode = R"(
#include <cstring>
#include <cstdio>

void unsafeFunction() {
    char buffer[100];
    strcpy(buffer, "unsafe");
    scanf("%s", buffer);
    system("rm -rf /");
}
)";
        
        auto issues = analyzer.analyzeFile("unsafe.cpp", unsafeCode);
        
        bool hasSecurityIssue = false;
        for (const auto& issue : issues) {
            if (issue.category == AnalysisCategory::SECURITY) {
                hasSecurityIssue = true;
                break;
            }
        }
        REQUIRE(hasSecurityIssue);
    }
    
    SECTION("detect potential memory leaks") {
        std::string leakyCode = R"(
void leakyFunction() {
    int* ptr1 = new int[100];
    int* ptr2 = new int[200];
    delete[] ptr1;
    // ptr2 is never deleted - potential leak
}
)";
        
        auto issues = analyzer.analyzeFile("leaky.cpp", leakyCode);
        auto metrics = analyzer.getMetrics("leaky.cpp", leakyCode);
        
        // Should detect potential memory leak
        REQUIRE(metrics.customMetrics.count("memory_leak_risks") > 0);
    }
}

TEST_CASE("DependencyAnalyzer functionality") {
    DependencyAnalyzer analyzer;
    
    SECTION("extract includes") {
        std::string codeWithIncludes = R"(
#include <iostream>
#include <vector>
#include "custom_header.hpp"
#include <string>

void function() {}
)";
        
        auto issues = analyzer.analyzeFile("test.cpp", codeWithIncludes);
        auto metrics = analyzer.getMetrics("test.cpp", codeWithIncludes);
        
        REQUIRE(metrics.customMetrics["include_count"] == 4);
    }
    
    SECTION("detect too many dependencies") {
        std::string manyIncludes = R"(
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <queue>
#include <stack>
#include <algorithm>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>
#include <regex>
#include <fstream>
#include <sstream>
#include <utility>

void function() {}
)";
        
        auto issues = analyzer.analyzeFile("many_deps.cpp", manyIncludes);
        
        bool hasDependencyIssue = false;
        for (const auto& issue : issues) {
            if (issue.category == AnalysisCategory::DEPENDENCIES && 
                issue.id == "TOO_MANY_DEPENDENCIES") {
                hasDependencyIssue = true;
                break;
            }
        }
        REQUIRE(hasDependencyIssue);
    }
}

TEST_CASE("PerformanceAnalyzer functionality") {
    PerformanceAnalyzer analyzer;
    
    SECTION("detect performance issues") {
        std::string slowCode = R"(
void inefficientFunction() {
    std::string result;
    for (int i = 0; i < 1000; ++i) {
        result += "slow concatenation";
    }
    
    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 100; ++j) {
            // Nested loop
            int* temp = new int;
            delete temp;
        }
    }
}
)";
        
        auto issues = analyzer.analyzeFile("slow.cpp", slowCode);
        
        bool hasPerformanceIssue = false;
        for (const auto& issue : issues) {
            if (issue.category == AnalysisCategory::PERFORMANCE) {
                hasPerformanceIssue = true;
                break;
            }
        }
        REQUIRE(hasPerformanceIssue);
    }
}

TEST_CASE("CodeAnalyzer integration") {
    CodeAnalyzer analyzer;
    
    SECTION("register and manage analyzers") {
        auto analyzers = analyzer.getRegisteredAnalyzers();
        REQUIRE(analyzers.size() >= 4); // Default analyzers
        
        // Test enabling/disabling
        analyzer.enableAnalyzer("SecurityAnalyzer", false);
        REQUIRE(!analyzer.isAnalyzerEnabled("SecurityAnalyzer"));
        
        analyzer.enableAnalyzer("SecurityAnalyzer", true);
        REQUIRE(analyzer.isAnalyzerEnabled("SecurityAnalyzer"));
    }
    
    SECTION("analyze file with multiple issues") {
        std::string problematicCode = R"(
#include <cstring>

void problematicFunction(int x) {
    if (x > 0) {
        if (x < 100) {
            if (x % 2 == 0) {
                char buffer[100];
                strcpy(buffer, "unsafe");
                
                std::string result;
                for (int i = 0; i < x; ++i) {
                    result += "slow";
                }
            }
        }
    }
}
)";
        
        // Create temporary file
        std::string tempFile = "/tmp/test_problematic.cpp";
        std::ofstream file(tempFile);
        file << problematicCode;
        file.close();
        
        auto result = analyzer.analyzeFile(tempFile);
        
        REQUIRE(!result.issues.empty());
        REQUIRE(result.analysisTimeMs > 0);
        
        // Should have issues from multiple categories
        bool hasComplexity = result.getIssueCount(AnalysisCategory::COMPLEXITY) > 0;
        bool hasSecurity = result.getIssueCount(AnalysisCategory::SECURITY) > 0;
        bool hasPerformance = result.getIssueCount(AnalysisCategory::PERFORMANCE) > 0;
        
        REQUIRE(hasComplexity || hasSecurity || hasPerformance);
        
        // Cleanup
        std::filesystem::remove(tempFile);
    }
    
    SECTION("export results") {
        std::string simpleCode = "void test() {}";
        
        std::string tempFile = "/tmp/test_simple.cpp";
        std::ofstream file(tempFile);
        file << simpleCode;
        file.close();
        
        auto result = analyzer.analyzeFile(tempFile);
        
        // Test JSON export
        std::string jsonOutput = analyzer.generateReport(result, "json");
        REQUIRE(jsonOutput.find("projectPath") != std::string::npos);
        REQUIRE(jsonOutput.find("totalIssues") != std::string::npos);
        
        // Test text export
        std::string textOutput = analyzer.generateReport(result, "text");
        REQUIRE(textOutput.find("Code Analysis Report") != std::string::npos);
        
        // Cleanup
        std::filesystem::remove(tempFile);
    }
    
    SECTION("project analysis") {
        // Create temporary project
        std::string projectPath = "/tmp/test_project";
        std::filesystem::create_directories(projectPath);
        
        std::ofstream main_cpp(projectPath + "/main.cpp");
        main_cpp << "int main() { return 0; }";
        main_cpp.close();
        
        std::ofstream utils_hpp(projectPath + "/utils.hpp");
        utils_hpp << "#ifndef UTILS_HPP\n#define UTILS_HPP\nvoid util();\n#endif";
        utils_hpp.close();
        
        auto result = analyzer.analyzeProject(projectPath);
        
        REQUIRE(result.fileMetrics.size() >= 1);
        REQUIRE(result.analysisTimeMs > 0);
        
        // Cleanup
        std::filesystem::remove_all(projectPath);
    }
}

TEST_CASE("RefactoringSuggestionEngine functionality") {
    RefactoringSuggestionEngine engine;
    
    SECTION("suggest extract method") {
        std::string longFunction = R"(
void veryLongFunction() {
    int a = 1; int b = 2; int c = 3; int d = 4; int e = 5;
    int f = 6; int g = 7; int h = 8; int i = 9; int j = 10;
    int k = 11; int l = 12; int m = 13; int n = 14; int o = 15;
    int p = 16; int q = 17; int r = 18; int s = 19; int t = 20;
    int u = 21; int v = 22; int w = 23; int x = 24; int y = 25;
    int z = 26;
    std::cout << "Very long function" << std::endl;
}
)";
        
        auto suggestions = engine.generateSuggestionsForFile("long.cpp", longFunction);
        
        bool hasExtractMethodSuggestion = false;
        for (const auto& suggestion : suggestions) {
            if (suggestion.title.find("Extract method") != std::string::npos) {
                hasExtractMethodSuggestion = true;
                REQUIRE(suggestion.confidenceScore > 0.0);
                REQUIRE(!suggestion.benefits.empty());
                break;
            }
        }
        REQUIRE(hasExtractMethodSuggestion);
    }
    
    SECTION("suggest reduce complexity") {
        std::string nestedCode = R"(
void nestedFunction() {
    if (true) {
        if (true) {
            if (true) {
                std::cout << "deeply nested" << std::endl;
            }
        }
    }
}
)";
        
        auto suggestions = engine.generateSuggestionsForFile("nested.cpp", nestedCode);
        
        bool hasComplexitySuggestion = false;
        for (const auto& suggestion : suggestions) {
            if (suggestion.title.find("complexity") != std::string::npos) {
                hasComplexitySuggestion = true;
                break;
            }
        }
        REQUIRE(hasComplexitySuggestion);
    }
    
    SECTION("detect code duplication") {
        std::string duplicatedCode = R"(
void function1() {
    int x = 1;
    int y = 2;
    int z = x + y;
    
    // Some other code
    
    int x = 1;
    int y = 2;
    int z = x + y;
}
)";
        
        auto suggestions = engine.generateSuggestionsForFile("dup.cpp", duplicatedCode);
        
        bool hasDuplicationSuggestion = false;
        for (const auto& suggestion : suggestions) {
            if (suggestion.title.find("duplication") != std::string::npos) {
                hasDuplicationSuggestion = true;
                REQUIRE(suggestion.confidenceScore > 0.8); // High confidence for exact duplicates
                break;
            }
        }
        REQUIRE(hasDuplicationSuggestion);
    }
}

TEST_CASE("AnalysisResult utility functions") {
    AnalysisResult result;
    
    // Add some test issues
    AnalysisIssue issue1("TEST1", "Test issue 1", AnalysisSeverity::ERROR, AnalysisCategory::SECURITY);
    issue1.filePath = "file1.cpp";
    
    AnalysisIssue issue2("TEST2", "Test issue 2", AnalysisSeverity::WARNING, AnalysisCategory::PERFORMANCE);
    issue2.filePath = "file1.cpp";
    
    AnalysisIssue issue3("TEST3", "Test issue 3", AnalysisSeverity::ERROR, AnalysisCategory::COMPLEXITY);
    issue3.filePath = "file2.cpp";
    
    result.issues = {issue1, issue2, issue3};
    
    SECTION("count by severity") {
        REQUIRE(result.getIssueCount(AnalysisSeverity::ERROR) == 2);
        REQUIRE(result.getIssueCount(AnalysisSeverity::WARNING) == 1);
        REQUIRE(result.getIssueCount(AnalysisSeverity::INFO) == 0);
    }
    
    SECTION("count by category") {
        REQUIRE(result.getIssueCount(AnalysisCategory::SECURITY) == 1);
        REQUIRE(result.getIssueCount(AnalysisCategory::PERFORMANCE) == 1);
        REQUIRE(result.getIssueCount(AnalysisCategory::COMPLEXITY) == 1);
        REQUIRE(result.getIssueCount(AnalysisCategory::MAINTAINABILITY) == 0);
    }
    
    SECTION("get issues for file") {
        auto file1Issues = result.getIssuesForFile("file1.cpp");
        auto file2Issues = result.getIssuesForFile("file2.cpp");
        auto nonexistentIssues = result.getIssuesForFile("nonexistent.cpp");
        
        REQUIRE(file1Issues.size() == 2);
        REQUIRE(file2Issues.size() == 1);
        REQUIRE(nonexistentIssues.size() == 0);
    }
}