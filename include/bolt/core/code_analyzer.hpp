#ifndef BOLT_CODE_ANALYZER_HPP
#define BOLT_CODE_ANALYZER_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <optional>
#include "logging.hpp"
#include "performance_profiler.hpp"

namespace bolt {

/**
 * Analysis severity levels
 */
enum class AnalysisSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * Analysis category types
 */
enum class AnalysisCategory {
    COMPLEXITY,         // Cyclomatic complexity, nesting depth, etc.
    MAINTAINABILITY,    // Code smells, duplication, etc.
    PERFORMANCE,        // Performance bottlenecks, inefficient patterns
    SECURITY,          // Security vulnerabilities, unsafe patterns
    STYLE,             // Coding standards, formatting
    DEPENDENCIES,      // Dependency analysis, coupling metrics
    MEMORY,            // Memory leaks, resource management
    TESTING,           // Test coverage, testability metrics
    ARCHITECTURE       // Design patterns, architectural violations
};

/**
 * Code analysis issue representation
 */
struct AnalysisIssue {
    std::string id;
    std::string message;
    std::string description;
    AnalysisSeverity severity;
    AnalysisCategory category;
    std::string filePath;
    size_t lineNumber;
    size_t columnNumber;
    size_t endLineNumber;
    size_t endColumnNumber;
    std::vector<std::string> suggestions;
    std::unordered_map<std::string, std::string> metadata;
    
    AnalysisIssue(const std::string& issueId, const std::string& msg, 
                  AnalysisSeverity sev, AnalysisCategory cat)
        : id(issueId), message(msg), severity(sev), category(cat)
        , lineNumber(0), columnNumber(0), endLineNumber(0), endColumnNumber(0) {}
};

/**
 * Analysis metrics for code quality assessment
 */
struct CodeMetrics {
    size_t linesOfCode = 0;
    size_t physicalLines = 0;
    size_t commentLines = 0;
    size_t blankLines = 0;
    size_t cyclomaticComplexity = 0;
    size_t nestingDepth = 0;
    size_t functionCount = 0;
    size_t classCount = 0;
    size_t namespaceCount = 0;
    double maintainabilityIndex = 0.0;
    double duplicationRatio = 0.0;
    std::unordered_map<std::string, size_t> customMetrics;
};

/**
 * Dependency information for architecture analysis
 */
struct DependencyInfo {
    std::string fromFile;
    std::string toFile;
    std::string dependencyType; // "include", "import", "uses", etc.
    size_t lineNumber;
    bool isCircular = false;
};

/**
 * Abstract base class for code analyzers
 */
class ICodeAnalyzer {
public:
    virtual ~ICodeAnalyzer() = default;
    
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    virtual AnalysisCategory getCategory() const = 0;
    
    virtual std::vector<AnalysisIssue> analyzeFile(const std::string& filePath, 
                                                   const std::string& content) = 0;
    virtual std::vector<AnalysisIssue> analyzeProject(const std::string& projectPath) = 0;
    virtual CodeMetrics getMetrics(const std::string& filePath, 
                                   const std::string& content) = 0;
    
    virtual void configure(const std::unordered_map<std::string, std::string>& config) {}
    virtual bool isEnabled() const { return true; }
};

/**
 * Static code analyzer for complexity metrics
 */
class ComplexityAnalyzer : public ICodeAnalyzer {
public:
    std::string getName() const override { return "ComplexityAnalyzer"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::vector<std::string> getSupportedExtensions() const override;
    AnalysisCategory getCategory() const override { return AnalysisCategory::COMPLEXITY; }
    
    std::vector<AnalysisIssue> analyzeFile(const std::string& filePath, 
                                           const std::string& content) override;
    std::vector<AnalysisIssue> analyzeProject(const std::string& projectPath) override;
    CodeMetrics getMetrics(const std::string& filePath, 
                           const std::string& content) override;

private:
    size_t calculateCyclomaticComplexity(const std::string& content);
    size_t calculateNestingDepth(const std::string& content);
    size_t countFunctions(const std::string& content);
    size_t countClasses(const std::string& content);
    std::vector<std::string> extractFunctions(const std::string& content);
};

/**
 * Security vulnerability analyzer
 */
class SecurityAnalyzer : public ICodeAnalyzer {
public:
    std::string getName() const override { return "SecurityAnalyzer"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::vector<std::string> getSupportedExtensions() const override;
    AnalysisCategory getCategory() const override { return AnalysisCategory::SECURITY; }
    
    std::vector<AnalysisIssue> analyzeFile(const std::string& filePath, 
                                           const std::string& content) override;
    std::vector<AnalysisIssue> analyzeProject(const std::string& projectPath) override;
    CodeMetrics getMetrics(const std::string& filePath, 
                           const std::string& content) override;

private:
    std::vector<AnalysisIssue> checkBufferOverflows(const std::string& content, 
                                                     const std::string& filePath);
    std::vector<AnalysisIssue> checkMemoryLeaks(const std::string& content, 
                                                 const std::string& filePath);
    std::vector<AnalysisIssue> checkUnsafeFunctions(const std::string& content, 
                                                     const std::string& filePath);
};

/**
 * Dependency analyzer for architecture assessment
 */
class DependencyAnalyzer : public ICodeAnalyzer {
public:
    std::string getName() const override { return "DependencyAnalyzer"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::vector<std::string> getSupportedExtensions() const override;
    AnalysisCategory getCategory() const override { return AnalysisCategory::DEPENDENCIES; }
    
    std::vector<AnalysisIssue> analyzeFile(const std::string& filePath, 
                                           const std::string& content) override;
    std::vector<AnalysisIssue> analyzeProject(const std::string& projectPath) override;
    CodeMetrics getMetrics(const std::string& filePath, 
                           const std::string& content) override;
    
    std::vector<DependencyInfo> getDependencies(const std::string& projectPath);
    std::vector<DependencyInfo> findCircularDependencies(const std::string& projectPath);

private:
    std::vector<std::string> extractIncludes(const std::string& content);
    std::unordered_map<std::string, std::vector<std::string>> buildDependencyGraph(
        const std::string& projectPath);
    std::vector<std::vector<std::string>> findCycles(
        const std::unordered_map<std::string, std::vector<std::string>>& graph);
};

/**
 * Performance analyzer for bottleneck detection
 */
class PerformanceAnalyzer : public ICodeAnalyzer {
public:
    std::string getName() const override { return "PerformanceAnalyzer"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::vector<std::string> getSupportedExtensions() const override;
    AnalysisCategory getCategory() const override { return AnalysisCategory::PERFORMANCE; }
    
    std::vector<AnalysisIssue> analyzeFile(const std::string& filePath, 
                                           const std::string& content) override;
    std::vector<AnalysisIssue> analyzeProject(const std::string& projectPath) override;
    CodeMetrics getMetrics(const std::string& filePath, 
                           const std::string& content) override;

private:
    std::vector<AnalysisIssue> detectInefficiencies(const std::string& content, 
                                                     const std::string& filePath);
    std::vector<AnalysisIssue> detectMemoryIssues(const std::string& content, 
                                                   const std::string& filePath);
    std::vector<AnalysisIssue> detectAlgorithmicIssues(const std::string& content, 
                                                        const std::string& filePath);
};

/**
 * Analysis result aggregator and reporter
 */
struct AnalysisResult {
    std::string projectPath;
    std::chrono::steady_clock::time_point analysisTime;
    std::vector<AnalysisIssue> issues;
    std::unordered_map<std::string, CodeMetrics> fileMetrics;
    CodeMetrics overallMetrics;
    std::vector<DependencyInfo> dependencies;
    double analysisTimeMs = 0.0;
    
    size_t getIssueCount(AnalysisSeverity severity) const;
    size_t getIssueCount(AnalysisCategory category) const;
    std::vector<AnalysisIssue> getIssuesForFile(const std::string& filePath) const;
};

/**
 * Main code analyzer orchestrator
 */
class CodeAnalyzer {
public:
    CodeAnalyzer();
    ~CodeAnalyzer();
    
    // Analyzer management
    void registerAnalyzer(std::unique_ptr<ICodeAnalyzer> analyzer);
    void unregisterAnalyzer(const std::string& analyzerName);
    std::vector<std::string> getRegisteredAnalyzers() const;
    
    // Analysis execution
    AnalysisResult analyzeFile(const std::string& filePath);
    AnalysisResult analyzeProject(const std::string& projectPath);
    AnalysisResult analyzeProject(const std::string& projectPath, 
                                  const std::vector<std::string>& enabledAnalyzers);
    
    // Configuration
    void configure(const std::string& analyzerName, 
                   const std::unordered_map<std::string, std::string>& config);
    void enableAnalyzer(const std::string& analyzerName, bool enabled = true);
    bool isAnalyzerEnabled(const std::string& analyzerName) const;
    
    // Reporting
    void exportResults(const AnalysisResult& result, const std::string& format, 
                       const std::string& outputPath);
    std::string generateReport(const AnalysisResult& result, const std::string& format);
    
    // Metrics
    void setMetricsCallback(std::function<void(const std::string&, double)> callback);
    
    // Integration with performance profiler
    void enablePerformanceProfiling(bool enabled = true);

private:
    std::unordered_map<std::string, std::unique_ptr<ICodeAnalyzer>> analyzers_;
    std::unordered_map<std::string, bool> analyzerEnabled_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> analyzerConfigs_;
    std::function<void(const std::string&, double)> metricsCallback_;
    bool performanceProfilingEnabled_;
    
    std::vector<std::string> findFilesWithExtensions(const std::string& path, 
                                                      const std::vector<std::string>& extensions);
    void aggregateMetrics(const std::vector<CodeMetrics>& fileMetrics, CodeMetrics& overall);
    std::string formatSeverity(AnalysisSeverity severity) const;
    std::string formatCategory(AnalysisCategory category) const;
};

/**
 * Refactoring suggestion engine
 */
class RefactoringSuggestionEngine {
public:
    struct RefactoringSuggestion {
        std::string id;
        std::string title;
        std::string description;
        std::string filePath;
        size_t startLine;
        size_t endLine;
        std::string originalCode;
        std::string suggestedCode;
        std::vector<std::string> benefits;
        double confidenceScore;
    };
    
    std::vector<RefactoringSuggestion> generateSuggestions(const AnalysisResult& result);
    std::vector<RefactoringSuggestion> generateSuggestionsForFile(const std::string& filePath, 
                                                                   const std::string& content);
    
private:
    std::vector<RefactoringSuggestion> suggestExtractMethod(const std::string& content, 
                                                            const std::string& filePath);
    std::vector<RefactoringSuggestion> suggestReduceComplexity(const std::string& content, 
                                                               const std::string& filePath);
    std::vector<RefactoringSuggestion> suggestRemoveDuplication(const std::string& content, 
                                                                const std::string& filePath);
};

} // namespace bolt

#endif // BOLT_CODE_ANALYZER_HPP