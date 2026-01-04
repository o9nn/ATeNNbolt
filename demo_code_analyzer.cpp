#include "bolt/core/code_analyzer.hpp"
#include "bolt/core/logging.hpp"
#include <iostream>
#include <filesystem>

using namespace bolt;

void demonstrateBasicAnalysis() {
    std::cout << "\n=== Basic Code Analysis Demo ===\n";
    
    // Create analyzer instance
    CodeAnalyzer analyzer;
    
    // Enable performance profiling
    analyzer.enablePerformanceProfiling(true);
    
    // Set up metrics callback
    analyzer.setMetricsCallback([](const std::string& metric, double value) {
        std::cout << "Metric: " << metric << " = " << value << std::endl;
    });
    
    // Create sample code with various issues for testing
    std::string sampleCode = R"(
#include <iostream>
#include <string>
#include <vector>

// Example function with high complexity
void complexFunction(int x) {
    if (x > 0) {
        if (x < 100) {
            if (x % 2 == 0) {
                if (x % 4 == 0) {
                    if (x % 8 == 0) {
                        std::cout << "Complex nested logic" << std::endl;
                    }
                }
            }
        }
    }
    
    // Performance issue: string concatenation in loop
    std::string result;
    for (int i = 0; i < 1000; ++i) {
        result += "test";
    }
    
    // Security issue: unsafe function
    char buffer[100];
    scanf("%s", buffer);
}

// Memory issue: potential leak
void memoryIssueFunction() {
    int* ptr = new int[100];
    // Missing delete[]
    
    // Frequent allocation in loop
    for (int i = 0; i < 1000; ++i) {
        int* temp = new int;
        delete temp;
    }
}

class ExampleClass {
public:
    void method1() {}
    void method2() {}
    // ... many more methods would trigger complexity warnings
};
)";
    
    // Write sample code to temporary file
    std::string tempFile = "/tmp/sample_code.cpp";
    std::ofstream file(tempFile);
    file << sampleCode;
    file.close();
    
    // Analyze the sample file
    auto result = analyzer.analyzeFile(tempFile);
    
    std::cout << "\nAnalysis Results:\n";
    std::cout << "Total Issues Found: " << result.issues.size() << std::endl;
    std::cout << "Analysis Time: " << result.analysisTimeMs << " ms\n";
    
    // Show issues by severity
    std::cout << "\nIssues by Severity:\n";
    std::cout << "  Critical: " << result.getIssueCount(AnalysisSeverity::CRITICAL) << std::endl;
    std::cout << "  Error: " << result.getIssueCount(AnalysisSeverity::ERROR) << std::endl;
    std::cout << "  Warning: " << result.getIssueCount(AnalysisSeverity::WARNING) << std::endl;
    std::cout << "  Info: " << result.getIssueCount(AnalysisSeverity::INFO) << std::endl;
    
    // Show issues by category
    std::cout << "\nIssues by Category:\n";
    std::cout << "  Complexity: " << result.getIssueCount(AnalysisCategory::COMPLEXITY) << std::endl;
    std::cout << "  Security: " << result.getIssueCount(AnalysisCategory::SECURITY) << std::endl;
    std::cout << "  Performance: " << result.getIssueCount(AnalysisCategory::PERFORMANCE) << std::endl;
    
    // Show detailed issues
    std::cout << "\nDetailed Issues:\n";
    for (const auto& issue : result.issues) {
        std::cout << "  [" << static_cast<int>(issue.severity) << "] " 
                  << issue.message << std::endl;
        for (const auto& suggestion : issue.suggestions) {
            std::cout << "    Suggestion: " << suggestion << std::endl;
        }
    }
    
    // Export results
    analyzer.exportResults(result, "json", "/tmp/analysis_results.json");
    analyzer.exportResults(result, "text", "/tmp/analysis_results.txt");
    
    std::cout << "\nResults exported to /tmp/analysis_results.json and /tmp/analysis_results.txt\n";
    
    // Cleanup
    std::filesystem::remove(tempFile);
}

void demonstrateRefactoringSuggestions() {
    std::cout << "\n=== Refactoring Suggestions Demo ===\n";
    
    RefactoringSuggestionEngine engine;
    
    std::string longFunctionCode = R"(
void veryLongFunction() {
    // This function is intentionally long to trigger refactoring suggestions
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    int f = 6;
    int g = 7;
    int h = 8;
    int i = 9;
    int j = 10;
    int k = 11;
    int l = 12;
    int m = 13;
    int n = 14;
    int o = 15;
    int p = 16;
    int q = 17;
    int r = 18;
    int s = 19;
    int t = 20;
    int u = 21;
    int v = 22;
    int w = 23;
    int x = 24;
    int y = 25;
    int z = 26;
    
    std::cout << "This is a very long function" << std::endl;
    std::cout << "It should be broken down" << std::endl;
    std::cout << "Into smaller, more manageable pieces" << std::endl;
}

void duplicatedCode() {
    int x = 1;
    int y = 2;
    int z = x + y;
    
    // Some other code...
    
    int x = 1;  // Duplicate sequence
    int y = 2;
    int z = x + y;
}
)";
    
    auto suggestions = engine.generateSuggestionsForFile("example.cpp", longFunctionCode);
    
    std::cout << "Refactoring Suggestions Found: " << suggestions.size() << std::endl;
    
    for (const auto& suggestion : suggestions) {
        std::cout << "\nSuggestion: " << suggestion.title << std::endl;
        std::cout << "  Description: " << suggestion.description << std::endl;
        std::cout << "  Confidence: " << (suggestion.confidenceScore * 100) << "%" << std::endl;
        std::cout << "  Benefits:" << std::endl;
        for (const auto& benefit : suggestion.benefits) {
            std::cout << "    - " << benefit << std::endl;
        }
    }
}

void demonstrateAnalyzerConfiguration() {
    std::cout << "\n=== Analyzer Configuration Demo ===\n";
    
    CodeAnalyzer analyzer;
    
    // Show registered analyzers
    auto analyzers = analyzer.getRegisteredAnalyzers();
    std::cout << "Registered Analyzers:" << std::endl;
    for (const auto& name : analyzers) {
        std::cout << "  - " << name << " (enabled: " 
                  << (analyzer.isAnalyzerEnabled(name) ? "yes" : "no") << ")" << std::endl;
    }
    
    // Disable specific analyzers
    analyzer.enableAnalyzer("SecurityAnalyzer", false);
    std::cout << "\nDisabled SecurityAnalyzer" << std::endl;
    
    // Configure an analyzer
    std::unordered_map<std::string, std::string> config;
    config["max_complexity"] = "15";
    config["max_nesting"] = "5";
    analyzer.configure("ComplexityAnalyzer", config);
    std::cout << "Configured ComplexityAnalyzer with custom thresholds" << std::endl;
}

void demonstrateProjectAnalysis() {
    std::cout << "\n=== Project Analysis Demo ===\n";
    
    // Create a temporary project structure
    std::string projectPath = "/tmp/sample_project";
    std::filesystem::create_directories(projectPath);
    
    // Create some sample files
    std::ofstream main_cpp(projectPath + "/main.cpp");
    main_cpp << R"(
#include "utils.hpp"
#include <iostream>

int main() {
    if (true) {
        if (true) {
            if (true) {
                std::cout << "Nested complexity" << std::endl;
            }
        }
    }
    return 0;
}
)";
    main_cpp.close();
    
    std::ofstream utils_hpp(projectPath + "/utils.hpp");
    utils_hpp << R"(
#ifndef UTILS_HPP
#define UTILS_HPP

#include "main.cpp"  // Circular dependency!

void utilityFunction();

#endif
)";
    utils_hpp.close();
    
    std::ofstream utils_cpp(projectPath + "/utils.cpp");
    utils_cpp << R"(
#include "utils.hpp"
#include <cstring>

void utilityFunction() {
    char buffer[100];
    strcpy(buffer, "unsafe operation");  // Security issue
}
)";
    utils_cpp.close();
    
    CodeAnalyzer analyzer;
    auto result = analyzer.analyzeProject(projectPath);
    
    std::cout << "Project Analysis Results:" << std::endl;
    std::cout << "  Files analyzed: " << result.fileMetrics.size() << std::endl;
    std::cout << "  Total issues: " << result.issues.size() << std::endl;
    std::cout << "  Analysis time: " << result.analysisTimeMs << " ms" << std::endl;
    std::cout << "  Dependencies found: " << result.dependencies.size() << std::endl;
    
    // Show overall metrics
    std::cout << "\nOverall Project Metrics:" << std::endl;
    std::cout << "  Lines of Code: " << result.overallMetrics.linesOfCode << std::endl;
    std::cout << "  Functions: " << result.overallMetrics.functionCount << std::endl;
    std::cout << "  Classes: " << result.overallMetrics.classCount << std::endl;
    std::cout << "  Cyclomatic Complexity: " << result.overallMetrics.cyclomaticComplexity << std::endl;
    
    // Generate and display report
    std::string report = analyzer.generateReport(result, "text");
    std::cout << "\n" << report << std::endl;
    
    // Cleanup
    std::filesystem::remove_all(projectPath);
}

void demonstrateCustomAnalyzer() {
    std::cout << "\n=== Custom Analyzer Demo ===\n";
    
    // Example of how to create a custom analyzer
    class CustomStyleAnalyzer : public ICodeAnalyzer {
    public:
        std::string getName() const override { return "CustomStyleAnalyzer"; }
        std::string getVersion() const override { return "1.0.0"; }
        std::vector<std::string> getSupportedExtensions() const override {
            return {".cpp", ".hpp"};
        }
        AnalysisCategory getCategory() const override { return AnalysisCategory::STYLE; }
        
        std::vector<AnalysisIssue> analyzeFile(const std::string& filePath, 
                                               const std::string& content) override {
            std::vector<AnalysisIssue> issues;
            
            // Check for missing header guards
            if (filePath.find(".hpp") != std::string::npos) {
                if (content.find("#ifndef") == std::string::npos || 
                    content.find("#define") == std::string::npos) {
                    AnalysisIssue issue("MISSING_HEADER_GUARD", 
                                       "Header file missing include guard",
                                       AnalysisSeverity::WARNING, AnalysisCategory::STYLE);
                    issue.filePath = filePath;
                    issue.suggestions.push_back("Add #ifndef, #define, #endif header guard");
                    issues.push_back(issue);
                }
            }
            
            return issues;
        }
        
        std::vector<AnalysisIssue> analyzeProject(const std::string& projectPath) override {
            return {}; // Not implemented for this demo
        }
        
        CodeMetrics getMetrics(const std::string& filePath, 
                               const std::string& content) override {
            CodeMetrics metrics;
            metrics.customMetrics["style_issues"] = 
                analyzeFile(filePath, content).size();
            return metrics;
        }
    };
    
    CodeAnalyzer analyzer;
    analyzer.registerAnalyzer(std::make_unique<CustomStyleAnalyzer>());
    
    auto analyzers = analyzer.getRegisteredAnalyzers();
    std::cout << "Analyzers after adding custom analyzer:" << std::endl;
    for (const auto& name : analyzers) {
        std::cout << "  - " << name << std::endl;
    }
    
    // Test the custom analyzer
    std::string headerFile = "/tmp/test_header.hpp";
    std::ofstream header(headerFile);
    header << R"(
// This header is missing include guards
class TestClass {
public:
    void testMethod();
};
)";
    header.close();
    
    auto result = analyzer.analyzeFile(headerFile);
    std::cout << "\nCustom analyzer found " << result.issues.size() << " issues" << std::endl;
    
    for (const auto& issue : result.issues) {
        if (issue.id == "MISSING_HEADER_GUARD") {
            std::cout << "Found style issue: " << issue.message << std::endl;
        }
    }
    
    std::filesystem::remove(headerFile);
}

int main() {
    try {
        // Initialize logging
        auto& logger = LogManager::getInstance();
        logger.setLevel(LogLevel::INFO);
        
        std::cout << "=== Advanced Code Analysis Tools Demo ===\n";
        std::cout << "This demo showcases the comprehensive code analysis capabilities\n";
        std::cout << "implemented for the Bolt C++ IDE.\n";
        
        demonstrateBasicAnalysis();
        demonstrateRefactoringSuggestions();
        demonstrateAnalyzerConfiguration();
        demonstrateProjectAnalysis();
        demonstrateCustomAnalyzer();
        
        std::cout << "\n=== Demo Complete ===\n";
        std::cout << "The advanced code analysis tools provide:\n";
        std::cout << "  ✓ Complexity analysis (cyclomatic, nesting, maintainability)\n";
        std::cout << "  ✓ Security vulnerability detection\n";
        std::cout << "  ✓ Performance bottleneck identification\n";
        std::cout << "  ✓ Dependency analysis and circular dependency detection\n";
        std::cout << "  ✓ Refactoring suggestions with confidence scores\n";
        std::cout << "  ✓ Extensible analyzer framework\n";
        std::cout << "  ✓ Multiple export formats (JSON, text)\n";
        std::cout << "  ✓ Integration with performance profiling\n";
        std::cout << "  ✓ Configurable analysis parameters\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
}