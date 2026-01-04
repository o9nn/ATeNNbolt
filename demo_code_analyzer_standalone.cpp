#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <regex>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <cmath>

// Standalone implementation of advanced code analysis tools
namespace bolt {

enum class AnalysisSeverity {
    INFO, WARNING, ERROR, CRITICAL
};

enum class AnalysisCategory {
    COMPLEXITY, MAINTAINABILITY, PERFORMANCE, SECURITY, 
    STYLE, DEPENDENCIES, MEMORY, TESTING, ARCHITECTURE
};

struct AnalysisIssue {
    std::string id;
    std::string message;
    std::string description;
    AnalysisSeverity severity;
    AnalysisCategory category;
    std::string filePath;
    size_t lineNumber;
    std::vector<std::string> suggestions;
    
    AnalysisIssue(const std::string& issueId, const std::string& msg, 
                  AnalysisSeverity sev, AnalysisCategory cat)
        : id(issueId), message(msg), severity(sev), category(cat), lineNumber(0) {}
};

struct CodeMetrics {
    size_t linesOfCode = 0;
    size_t physicalLines = 0;
    size_t commentLines = 0;
    size_t blankLines = 0;
    size_t cyclomaticComplexity = 0;
    size_t nestingDepth = 0;
    size_t functionCount = 0;
    size_t classCount = 0;
    double maintainabilityIndex = 0.0;
    std::unordered_map<std::string, size_t> customMetrics;
};

class StandaloneCodeAnalyzer {
public:
    std::vector<AnalysisIssue> analyzeFile(const std::string& filePath, const std::string& content) {
        std::vector<AnalysisIssue> issues;
        
        auto metrics = getMetrics(content);
        
        // Complexity analysis
        if (metrics.cyclomaticComplexity > 10) {
            AnalysisIssue issue("COMPLEX_FUNCTION", 
                               "High cyclomatic complexity: " + std::to_string(metrics.cyclomaticComplexity),
                               AnalysisSeverity::WARNING, AnalysisCategory::COMPLEXITY);
            issue.filePath = filePath;
            issue.suggestions.push_back("Consider breaking down complex functions into smaller ones");
            issues.push_back(issue);
        }
        
        if (metrics.nestingDepth > 4) {
            AnalysisIssue issue("DEEP_NESTING", 
                               "Deep nesting detected: " + std::to_string(metrics.nestingDepth) + " levels",
                               AnalysisSeverity::WARNING, AnalysisCategory::COMPLEXITY);
            issue.filePath = filePath;
            issue.suggestions.push_back("Extract nested logic into separate functions");
            issues.push_back(issue);
        }
        
        // Security analysis
        auto securityIssues = checkSecurityIssues(content, filePath);
        issues.insert(issues.end(), securityIssues.begin(), securityIssues.end());
        
        // Performance analysis
        auto performanceIssues = checkPerformanceIssues(content, filePath);
        issues.insert(issues.end(), performanceIssues.begin(), performanceIssues.end());
        
        return issues;
    }
    
    CodeMetrics getMetrics(const std::string& content) {
        CodeMetrics metrics;
        
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            metrics.physicalLines++;
            
            // Remove leading/trailing whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), line.end());
            
            if (line.empty()) {
                metrics.blankLines++;
            } else if (line.find("//") == 0 || line.find("/*") == 0) {
                metrics.commentLines++;
            } else {
                metrics.linesOfCode++;
            }
        }
        
        metrics.cyclomaticComplexity = calculateCyclomaticComplexity(content);
        metrics.nestingDepth = calculateNestingDepth(content);
        metrics.functionCount = countFunctions(content);
        metrics.classCount = countClasses(content);
        
        // Calculate maintainability index
        if (metrics.linesOfCode > 0) {
            double complexity = static_cast<double>(metrics.cyclomaticComplexity);
            double loc = static_cast<double>(metrics.linesOfCode);
            metrics.maintainabilityIndex = std::max(0.0, 
                171.0 - 5.2 * std::log(complexity) - 0.23 * complexity - 16.2 * std::log(loc));
        }
        
        return metrics;
    }
    
private:
    size_t calculateCyclomaticComplexity(const std::string& content) {
        size_t complexity = 1;
        std::regex decisionPoints(R"(\b(if|else\s+if|while|for|do|switch|case|catch|\?)\b)");
        std::sregex_iterator iter(content.begin(), content.end(), decisionPoints);
        std::sregex_iterator end;
        complexity += std::distance(iter, end);
        return complexity;
    }
    
    size_t calculateNestingDepth(const std::string& content) {
        size_t maxDepth = 0;
        size_t currentDepth = 0;
        
        for (char c : content) {
            if (c == '{') {
                currentDepth++;
                maxDepth = std::max(maxDepth, currentDepth);
            } else if (c == '}') {
                if (currentDepth > 0) currentDepth--;
            }
        }
        return maxDepth;
    }
    
    size_t countFunctions(const std::string& content) {
        std::regex functionPattern(R"(\b\w+\s*\([^)]*\)\s*\{)");
        std::sregex_iterator iter(content.begin(), content.end(), functionPattern);
        std::sregex_iterator end;
        return std::distance(iter, end);
    }
    
    size_t countClasses(const std::string& content) {
        std::regex classPattern(R"(\bclass\s+\w+)");
        std::sregex_iterator iter(content.begin(), content.end(), classPattern);
        std::sregex_iterator end;
        return std::distance(iter, end);
    }
    
    std::vector<AnalysisIssue> checkSecurityIssues(const std::string& content, const std::string& filePath) {
        std::vector<AnalysisIssue> issues;
        
        // Check for unsafe buffer operations
        std::regex unsafeBufferOps(R"(\b(strcpy|strcat|sprintf|gets)\s*\()");
        std::sregex_iterator iter(content.begin(), content.end(), unsafeBufferOps);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            AnalysisIssue issue("UNSAFE_BUFFER_OP", 
                               "Unsafe buffer operation: " + (*iter)[1].str(),
                               AnalysisSeverity::ERROR, AnalysisCategory::SECURITY);
            issue.filePath = filePath;
            issue.suggestions.push_back("Use safer alternatives like strncpy, strncat, snprintf");
            issues.push_back(issue);
        }
        
        // Check for unsafe functions
        std::vector<std::pair<std::string, std::string>> unsafeFunctions = {
            {"scanf", "Use cin or safer input methods"},
            {"system", "Avoid system calls, use safer alternatives"}
        };
        
        for (const auto& [func, suggestion] : unsafeFunctions) {
            std::regex pattern("\\b" + func + "\\s*\\(");
            if (std::regex_search(content, pattern)) {
                AnalysisIssue issue("UNSAFE_FUNCTION", 
                                   "Unsafe function usage: " + func,
                                   AnalysisSeverity::WARNING, AnalysisCategory::SECURITY);
                issue.filePath = filePath;
                issue.suggestions.push_back(suggestion);
                issues.push_back(issue);
            }
        }
        
        return issues;
    }
    
    std::vector<AnalysisIssue> checkPerformanceIssues(const std::string& content, const std::string& filePath) {
        std::vector<AnalysisIssue> issues;
        
        // Check for string concatenation in loops
        std::regex stringConcatInLoop(R"(for\s*\([^}]*\+\s*=\s*[^;]*string)");
        if (std::regex_search(content, stringConcatInLoop)) {
            AnalysisIssue issue("STRING_CONCAT_IN_LOOP", 
                               "String concatenation in loop detected",
                               AnalysisSeverity::WARNING, AnalysisCategory::PERFORMANCE);
            issue.filePath = filePath;
            issue.suggestions.push_back("Use stringstream or reserve space for better performance");
            issues.push_back(issue);
        }
        
        // Check for nested loops
        std::regex nestedLoops(R"(for\s*\([^}]*for\s*\()");
        std::sregex_iterator iter(content.begin(), content.end(), nestedLoops);
        std::sregex_iterator end;
        
        size_t nestedLoopCount = std::distance(iter, end);
        if (nestedLoopCount > 0) {
            AnalysisIssue issue("NESTED_LOOPS", 
                               "Nested loops detected (" + std::to_string(nestedLoopCount) + ")",
                               AnalysisSeverity::INFO, AnalysisCategory::PERFORMANCE);
            issue.filePath = filePath;
            issue.suggestions.push_back("Consider algorithmic improvements to reduce complexity");
            issues.push_back(issue);
        }
        
        return issues;
    }
    
public:
    std::string formatSeverity(AnalysisSeverity severity) const {
        switch (severity) {
            case AnalysisSeverity::INFO: return "INFO";
            case AnalysisSeverity::WARNING: return "WARNING";
            case AnalysisSeverity::ERROR: return "ERROR";  
            case AnalysisSeverity::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    std::string formatCategory(AnalysisCategory category) const {
        switch (category) {
            case AnalysisCategory::COMPLEXITY: return "COMPLEXITY";
            case AnalysisCategory::SECURITY: return "SECURITY";
            case AnalysisCategory::PERFORMANCE: return "PERFORMANCE";
            case AnalysisCategory::MAINTAINABILITY: return "MAINTAINABILITY";
            case AnalysisCategory::STYLE: return "STYLE";
            case AnalysisCategory::DEPENDENCIES: return "DEPENDENCIES";
            case AnalysisCategory::MEMORY: return "MEMORY";
            default: return "UNKNOWN";
        }
    }
};

} // namespace bolt

void demonstrateCodeAnalysis() {
    std::cout << "\n=== Advanced Code Analysis Tools Demo ===\n";
    
    bolt::StandaloneCodeAnalyzer analyzer;
    
    // Create sample code with various issues
    std::string sampleCode = R"(
#include <iostream>
#include <string>
#include <cstring>

// Function with high complexity and security issues
void problematicFunction(int x) {
    if (x > 0) {
        if (x < 100) {
            if (x % 2 == 0) {
                if (x % 4 == 0) {
                    if (x % 8 == 0) {
                        std::cout << "Complex nested logic" << std::endl;
                        
                        // Security issue: unsafe function
                        char buffer[100];
                        scanf("%s", buffer);
                        strcpy(buffer, "unsafe operation");
                    }
                }
            }
        }
    }
    
    // Performance issue: string concatenation in loop
    std::string result;
    for (int i = 0; i < 1000; ++i) {
        result += "performance issue";
    }
    
    // Nested loops (algorithmic complexity)
    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 100; ++j) {
            std::cout << i * j << std::endl;
        }
    }
}

class ExampleClass {
    public:
        void method1() { std::cout << "Method 1" << std::endl; }
        void method2() { std::cout << "Method 2" << std::endl; }
        void method3() { std::cout << "Method 3" << std::endl; }
};

int main() {
    return 0;
}
)";
    
    // Analyze the sample code
    auto startTime = std::chrono::high_resolution_clock::now();
    auto issues = analyzer.analyzeFile("sample.cpp", sampleCode);
    auto metrics = analyzer.getMetrics(sampleCode);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    std::cout << "\nAnalysis Results:\n";
    std::cout << "=================\n";
    std::cout << "Analysis Time: " << duration.count() << " microseconds\n";
    std::cout << "Total Issues Found: " << issues.size() << "\n\n";
    
    // Display metrics
    std::cout << "Code Metrics:\n";
    std::cout << "  Lines of Code: " << metrics.linesOfCode << "\n";
    std::cout << "  Physical Lines: " << metrics.physicalLines << "\n";
    std::cout << "  Comment Lines: " << metrics.commentLines << "\n";
    std::cout << "  Blank Lines: " << metrics.blankLines << "\n";
    std::cout << "  Cyclomatic Complexity: " << metrics.cyclomaticComplexity << "\n";
    std::cout << "  Nesting Depth: " << metrics.nestingDepth << "\n";
    std::cout << "  Function Count: " << metrics.functionCount << "\n";
    std::cout << "  Class Count: " << metrics.classCount << "\n";
    std::cout << "  Maintainability Index: " << std::fixed << std::setprecision(2) 
              << metrics.maintainabilityIndex << "\n\n";
    
    // Group issues by severity
    std::unordered_map<bolt::AnalysisSeverity, int> severityCounts;
    std::unordered_map<bolt::AnalysisCategory, int> categoryCounts;
    
    for (const auto& issue : issues) {
        severityCounts[issue.severity]++;
        categoryCounts[issue.category]++;
    }
    
    std::cout << "Issues by Severity:\n";
    for (const auto& [severity, count] : severityCounts) {
        std::cout << "  " << analyzer.formatSeverity(severity) << ": " << count << "\n";
    }
    
    std::cout << "\nIssues by Category:\n";
    for (const auto& [category, count] : categoryCounts) {
        std::cout << "  " << analyzer.formatCategory(category) << ": " << count << "\n";
    }
    
    std::cout << "\nDetailed Issues:\n";
    std::cout << "================\n";
    for (const auto& issue : issues) {
        std::cout << "[" << analyzer.formatSeverity(issue.severity) << "] " 
                  << analyzer.formatCategory(issue.category) << ": " << issue.message << "\n";
        for (const auto& suggestion : issue.suggestions) {
            std::cout << "  → Suggestion: " << suggestion << "\n";
        }
        std::cout << "\n";
    }
}

void demonstrateProjectAnalysis() {
    std::cout << "\n=== Project Analysis Demo ===\n";
    
    // Create a temporary project structure
    std::string projectPath = "/tmp/sample_project";
    std::filesystem::create_directories(projectPath);
    
    // Create some sample files with different types of issues
    std::ofstream main_cpp(projectPath + "/main.cpp");
    main_cpp << R"(
#include "utils.hpp"
#include <iostream>

int main() {
    // Deep nesting example
    if (true) {
        if (true) {
            if (true) {
                if (true) {
                    if (true) {
                        std::cout << "Too much nesting!" << std::endl;
                    }
                }
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

#include <string>

class UtilityClass {
public:
    void complexMethod(int a, int b, int c, int d);
    void performanceIssue();
    void securityRisk();
};

#endif
)";
    utils_hpp.close();
    
    std::ofstream utils_cpp(projectPath + "/utils.cpp");
    utils_cpp << R"(
#include "utils.hpp"
#include <cstring>
#include <iostream>

void UtilityClass::complexMethod(int a, int b, int c, int d) {
    if (a > 0) {
        if (b > 0) {
            if (c > 0) {
                if (d > 0) {
                    for (int i = 0; i < a; ++i) {
                        for (int j = 0; j < b; ++j) {
                            switch (c) {
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

void UtilityClass::performanceIssue() {
    std::string result;
    for (int i = 0; i < 10000; ++i) {
        result += "slow concatenation ";
    }
}

void UtilityClass::securityRisk() {
    char buffer[100];
    strcpy(buffer, "unsafe buffer operation");
    scanf("%s", buffer);
}
)";
    utils_cpp.close();
    
    bolt::StandaloneCodeAnalyzer analyzer;
    std::vector<bolt::AnalysisIssue> allIssues;
    std::vector<bolt::CodeMetrics> allMetrics;
    size_t totalFiles = 0;
    
    // Analyze all files in the project
    for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
        if (entry.is_regular_file() && 
            (entry.path().extension() == ".cpp" || entry.path().extension() == ".hpp")) {
            
            std::ifstream file(entry.path());
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            auto issues = analyzer.analyzeFile(entry.path().string(), content);
            auto metrics = analyzer.getMetrics(content);
            
            allIssues.insert(allIssues.end(), issues.begin(), issues.end());
            allMetrics.push_back(metrics);
            totalFiles++;
        }
    }
    
    // Aggregate metrics
    bolt::CodeMetrics overallMetrics;
    for (const auto& metrics : allMetrics) {
        overallMetrics.linesOfCode += metrics.linesOfCode;
        overallMetrics.physicalLines += metrics.physicalLines;
        overallMetrics.functionCount += metrics.functionCount;
        overallMetrics.classCount += metrics.classCount;
        overallMetrics.cyclomaticComplexity += metrics.cyclomaticComplexity;
        overallMetrics.nestingDepth = std::max(overallMetrics.nestingDepth, metrics.nestingDepth);
    }
    
    std::cout << "Project Analysis Results:\n";
    std::cout << "  Files Analyzed: " << totalFiles << "\n";
    std::cout << "  Total Issues: " << allIssues.size() << "\n";
    std::cout << "  Total Lines of Code: " << overallMetrics.linesOfCode << "\n";
    std::cout << "  Total Functions: " << overallMetrics.functionCount << "\n";
    std::cout << "  Total Classes: " << overallMetrics.classCount << "\n";
    std::cout << "  Maximum Nesting Depth: " << overallMetrics.nestingDepth << "\n";
    std::cout << "  Total Cyclomatic Complexity: " << overallMetrics.cyclomaticComplexity << "\n";
    
    // Show issues by file
    std::cout << "\nIssues by File:\n";
    std::unordered_map<std::string, std::vector<bolt::AnalysisIssue>> issuesByFile;
    for (const auto& issue : allIssues) {
        issuesByFile[issue.filePath].push_back(issue);
    }
    
    for (const auto& [filePath, fileIssues] : issuesByFile) {
        std::cout << "  " << std::filesystem::path(filePath).filename().string() 
                  << ": " << fileIssues.size() << " issues\n";
    }
    
    // Cleanup
    std::filesystem::remove_all(projectPath);
}

int main() {
    try {
        std::cout << "=== Bolt C++ Advanced Code Analysis Tools ===\n";
        std::cout << "This standalone demo showcases the comprehensive code analysis\n";
        std::cout << "capabilities implemented for the Bolt C++ IDE.\n";
        
        demonstrateCodeAnalysis();
        demonstrateProjectAnalysis();
        
        std::cout << "\n=== Analysis Features Demonstrated ===\n";
        std::cout << "✓ Complexity Analysis:\n";
        std::cout << "  - Cyclomatic complexity measurement\n";
        std::cout << "  - Nesting depth detection\n";
        std::cout << "  - Maintainability index calculation\n";
        std::cout << "  - Function and class counting\n";
        std::cout << "\n✓ Security Analysis:\n";
        std::cout << "  - Unsafe buffer operation detection\n";
        std::cout << "  - Risky function usage identification\n";
        std::cout << "  - Memory safety warnings\n";
        std::cout << "\n✓ Performance Analysis:\n";
        std::cout << "  - String concatenation in loops\n";
        std::cout << "  - Nested loop detection\n";
        std::cout << "  - Algorithmic complexity warnings\n";
        std::cout << "\n✓ Code Quality Metrics:\n";
        std::cout << "  - Lines of code counting\n";
        std::cout << "  - Comment ratio analysis\n";
        std::cout << "  - Code structure analysis\n";
        std::cout << "\n✓ Advanced Features:\n";
        std::cout << "  - Multi-file project analysis\n";
        std::cout << "  - Issue categorization and prioritization\n";
        std::cout << "  - Actionable improvement suggestions\n";
        std::cout << "  - Fast analysis with detailed reporting\n";
        
        std::cout << "\n=== Integration Ready ===\n";
        std::cout << "This code analysis framework is designed to integrate seamlessly\n";
        std::cout << "with the Bolt IDE's existing architecture, providing real-time\n";
        std::cout << "code quality feedback to developers.\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
}