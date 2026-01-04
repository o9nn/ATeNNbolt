#include "bolt/core/code_analyzer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <queue>
#include <unordered_set>
#include <iomanip>
#include <cmath>

namespace bolt {

// ComplexityAnalyzer implementation
std::vector<std::string> ComplexityAnalyzer::getSupportedExtensions() const {
    return {".cpp", ".hpp", ".c", ".h", ".cc", ".cxx", ".hxx"};
}

std::vector<AnalysisIssue> ComplexityAnalyzer::analyzeFile(const std::string& filePath, 
                                                           const std::string& content) {
    std::vector<AnalysisIssue> issues;
    
    auto metrics = getMetrics(filePath, content);
    
    // Check cyclomatic complexity
    if (metrics.cyclomaticComplexity > 10) {
        AnalysisIssue issue("COMPLEX_FUNCTION", 
                           "High cyclomatic complexity: " + std::to_string(metrics.cyclomaticComplexity),
                           AnalysisSeverity::WARNING, AnalysisCategory::COMPLEXITY);
        issue.filePath = filePath;
        issue.suggestions.push_back("Consider breaking down complex functions into smaller ones");
        issue.suggestions.push_back("Use early returns to reduce nesting");
        issues.push_back(issue);
    }
    
    // Check nesting depth
    if (metrics.nestingDepth > 4) {
        AnalysisIssue issue("DEEP_NESTING", 
                           "Deep nesting detected: " + std::to_string(metrics.nestingDepth) + " levels",
                           AnalysisSeverity::WARNING, AnalysisCategory::COMPLEXITY);
        issue.filePath = filePath;
        issue.suggestions.push_back("Extract nested logic into separate functions");
        issue.suggestions.push_back("Use guard clauses to reduce nesting");
        issues.push_back(issue);
    }
    
    // Check function count per file
    if (metrics.functionCount > 50) {
        AnalysisIssue issue("TOO_MANY_FUNCTIONS", 
                           "Too many functions in file: " + std::to_string(metrics.functionCount),
                           AnalysisSeverity::INFO, AnalysisCategory::COMPLEXITY);
        issue.filePath = filePath;
        issue.suggestions.push_back("Consider splitting the file into smaller modules");
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<AnalysisIssue> ComplexityAnalyzer::analyzeProject(const std::string& projectPath) {
    std::vector<AnalysisIssue> allIssues;
    
    for (const auto& ext : getSupportedExtensions()) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                auto fileIssues = analyzeFile(entry.path().string(), content);
                allIssues.insert(allIssues.end(), fileIssues.begin(), fileIssues.end());
            }
        }
    }
    
    return allIssues;
}

CodeMetrics ComplexityAnalyzer::getMetrics(const std::string& filePath, 
                                           const std::string& content) {
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
    
    // Calculate maintainability index (simplified version)
    if (metrics.linesOfCode > 0) {
        double complexity = static_cast<double>(metrics.cyclomaticComplexity);
        double loc = static_cast<double>(metrics.linesOfCode);
        metrics.maintainabilityIndex = std::max(0.0, 
            171.0 - 5.2 * std::log(complexity) - 0.23 * complexity - 16.2 * std::log(loc));
    }
    
    return metrics;
}

size_t ComplexityAnalyzer::calculateCyclomaticComplexity(const std::string& content) {
    size_t complexity = 1; // Base complexity
    
    // Count decision points
    std::regex decisionPoints(R"(\b(if|else\s+if|while|for|do|switch|case|catch|\?)\b)");
    std::sregex_iterator iter(content.begin(), content.end(), decisionPoints);
    std::sregex_iterator end;
    
    complexity += std::distance(iter, end);
    
    return complexity;
}

size_t ComplexityAnalyzer::calculateNestingDepth(const std::string& content) {
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

size_t ComplexityAnalyzer::countFunctions(const std::string& content) {
    std::regex functionPattern(R"(\b\w+\s*\([^)]*\)\s*\{)");
    std::sregex_iterator iter(content.begin(), content.end(), functionPattern);
    std::sregex_iterator end;
    
    return std::distance(iter, end);
}

size_t ComplexityAnalyzer::countClasses(const std::string& content) {
    std::regex classPattern(R"(\bclass\s+\w+)");
    std::sregex_iterator iter(content.begin(), content.end(), classPattern);
    std::sregex_iterator end;
    
    return std::distance(iter, end);
}

std::vector<std::string> ComplexityAnalyzer::extractFunctions(const std::string& content) {
    std::vector<std::string> functions;
    std::regex functionPattern(R"(\b(\w+)\s*\([^)]*\)\s*\{)");
    std::sregex_iterator iter(content.begin(), content.end(), functionPattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        functions.push_back((*iter)[1].str());
    }
    
    return functions;
}

// SecurityAnalyzer implementation
std::vector<std::string> SecurityAnalyzer::getSupportedExtensions() const {
    return {".cpp", ".c", ".cc", ".cxx"};
}

std::vector<AnalysisIssue> SecurityAnalyzer::analyzeFile(const std::string& filePath, 
                                                         const std::string& content) {
    std::vector<AnalysisIssue> issues;
    
    auto bufferIssues = checkBufferOverflows(content, filePath);
    auto memoryIssues = checkMemoryLeaks(content, filePath);
    auto unsafeIssues = checkUnsafeFunctions(content, filePath);
    
    issues.insert(issues.end(), bufferIssues.begin(), bufferIssues.end());
    issues.insert(issues.end(), memoryIssues.begin(), memoryIssues.end());
    issues.insert(issues.end(), unsafeIssues.begin(), unsafeIssues.end());
    
    return issues;
}

std::vector<AnalysisIssue> SecurityAnalyzer::analyzeProject(const std::string& projectPath) {
    std::vector<AnalysisIssue> allIssues;
    
    for (const auto& ext : getSupportedExtensions()) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                auto fileIssues = analyzeFile(entry.path().string(), content);
                allIssues.insert(allIssues.end(), fileIssues.begin(), fileIssues.end());
            }
        }
    }
    
    return allIssues;
}

CodeMetrics SecurityAnalyzer::getMetrics(const std::string& filePath, 
                                         const std::string& content) {
    CodeMetrics metrics;
    
    // Count potential security issues as custom metrics
    auto bufferIssues = checkBufferOverflows(content, filePath);
    auto memoryIssues = checkMemoryLeaks(content, filePath);
    auto unsafeIssues = checkUnsafeFunctions(content, filePath);
    
    metrics.customMetrics["buffer_overflow_risks"] = bufferIssues.size();
    metrics.customMetrics["memory_leak_risks"] = memoryIssues.size();
    metrics.customMetrics["unsafe_function_uses"] = unsafeIssues.size();
    
    return metrics;
}

std::vector<AnalysisIssue> SecurityAnalyzer::checkBufferOverflows(const std::string& content, 
                                                                  const std::string& filePath) {
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
        issue.suggestions.push_back("Consider using std::string for better safety");
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<AnalysisIssue> SecurityAnalyzer::checkMemoryLeaks(const std::string& content, 
                                                              const std::string& filePath) {
    std::vector<AnalysisIssue> issues;
    
    // Simple heuristic: count new/delete and malloc/free pairs
    std::regex newPattern(R"(\bnew\b)");
    std::regex deletePattern(R"(\bdelete\b)");
    std::regex mallocPattern(R"(\bmalloc\s*\()");
    std::regex freePattern(R"(\bfree\s*\()");
    
    size_t newCount = std::distance(std::sregex_iterator(content.begin(), content.end(), newPattern),
                                   std::sregex_iterator());
    size_t deleteCount = std::distance(std::sregex_iterator(content.begin(), content.end(), deletePattern),
                                      std::sregex_iterator());
    size_t mallocCount = std::distance(std::sregex_iterator(content.begin(), content.end(), mallocPattern),
                                      std::sregex_iterator());
    size_t freeCount = std::distance(std::sregex_iterator(content.begin(), content.end(), freePattern),
                                    std::sregex_iterator());
    
    if (newCount > deleteCount || mallocCount > freeCount) {
        AnalysisIssue issue("POTENTIAL_MEMORY_LEAK", 
                           "Potential memory leak detected (unmatched allocations)",
                           AnalysisSeverity::WARNING, AnalysisCategory::SECURITY);
        issue.filePath = filePath;
        issue.suggestions.push_back("Ensure all allocations have corresponding deallocations");
        issue.suggestions.push_back("Consider using smart pointers for automatic memory management");
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<AnalysisIssue> SecurityAnalyzer::checkUnsafeFunctions(const std::string& content, 
                                                                  const std::string& filePath) {
    std::vector<AnalysisIssue> issues;
    
    std::vector<std::pair<std::string, std::string>> unsafeFunctions = {
        {"scanf", "Use cin or safer input methods"},
        {"printf", "Consider using cout or safer output methods"},
        {"system", "Avoid system calls, use safer alternatives"},
        {"popen", "Use safer process execution methods"}
    };
    
    for (const auto& [func, suggestion] : unsafeFunctions) {
        std::regex pattern("\\b" + func + "\\s*\\(");
        std::sregex_iterator iter(content.begin(), content.end(), pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
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

// DependencyAnalyzer implementation
std::vector<std::string> DependencyAnalyzer::getSupportedExtensions() const {
    return {".cpp", ".hpp", ".c", ".h", ".cc", ".cxx", ".hxx"};
}

std::vector<AnalysisIssue> DependencyAnalyzer::analyzeFile(const std::string& filePath, 
                                                           const std::string& content) {
    std::vector<AnalysisIssue> issues;
    
    auto includes = extractIncludes(content);
    
    // Check for too many dependencies
    if (includes.size() > 20) {
        AnalysisIssue issue("TOO_MANY_DEPENDENCIES", 
                           "Too many includes: " + std::to_string(includes.size()),
                           AnalysisSeverity::WARNING, AnalysisCategory::DEPENDENCIES);
        issue.filePath = filePath;
        issue.suggestions.push_back("Consider reducing dependencies");
        issue.suggestions.push_back("Use forward declarations where possible");
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<AnalysisIssue> DependencyAnalyzer::analyzeProject(const std::string& projectPath) {
    std::vector<AnalysisIssue> allIssues;
    
    // Analyze individual files
    for (const auto& ext : getSupportedExtensions()) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                auto fileIssues = analyzeFile(entry.path().string(), content);
                allIssues.insert(allIssues.end(), fileIssues.begin(), fileIssues.end());
            }
        }
    }
    
    // Check for circular dependencies
    auto circularDeps = findCircularDependencies(projectPath);
    for (const auto& dep : circularDeps) {
        if (dep.isCircular) {
            AnalysisIssue issue("CIRCULAR_DEPENDENCY", 
                               "Circular dependency detected: " + dep.fromFile + " <-> " + dep.toFile,
                               AnalysisSeverity::ERROR, AnalysisCategory::DEPENDENCIES);
            issue.suggestions.push_back("Break circular dependency by introducing interfaces");
            issue.suggestions.push_back("Reorganize code to eliminate circular references");
            allIssues.push_back(issue);
        }
    }
    
    return allIssues;
}

CodeMetrics DependencyAnalyzer::getMetrics(const std::string& filePath, 
                                           const std::string& content) {
    CodeMetrics metrics;
    
    auto includes = extractIncludes(content);
    metrics.customMetrics["include_count"] = includes.size();
    
    return metrics;
}

std::vector<DependencyInfo> DependencyAnalyzer::getDependencies(const std::string& projectPath) {
    std::vector<DependencyInfo> dependencies;
    
    for (const auto& ext : getSupportedExtensions()) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                auto includes = extractIncludes(content);
                for (const auto& include : includes) {
                    DependencyInfo dep;
                    dep.fromFile = entry.path().string();
                    dep.toFile = include;
                    dep.dependencyType = "include";
                    dependencies.push_back(dep);
                }
            }
        }
    }
    
    return dependencies;
}

std::vector<DependencyInfo> DependencyAnalyzer::findCircularDependencies(const std::string& projectPath) {
    std::vector<DependencyInfo> circularDeps;
    
    auto depGraph = buildDependencyGraph(projectPath);
    auto cycles = findCycles(depGraph);
    
    for (const auto& cycle : cycles) {
        for (size_t i = 0; i < cycle.size(); ++i) {
            DependencyInfo dep;
            dep.fromFile = cycle[i];
            dep.toFile = cycle[(i + 1) % cycle.size()];
            dep.isCircular = true;
            circularDeps.push_back(dep);
        }
    }
    
    return circularDeps;
}

std::vector<std::string> DependencyAnalyzer::extractIncludes(const std::string& content) {
    std::vector<std::string> includes;
    std::regex includePattern(R"(#include\s*[<"](.*?)[>"])");
    std::sregex_iterator iter(content.begin(), content.end(), includePattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        includes.push_back((*iter)[1].str());
    }
    
    return includes;
}

std::unordered_map<std::string, std::vector<std::string>> DependencyAnalyzer::buildDependencyGraph(
    const std::string& projectPath) {
    std::unordered_map<std::string, std::vector<std::string>> graph;
    
    auto dependencies = getDependencies(projectPath);
    for (const auto& dep : dependencies) {
        graph[dep.fromFile].push_back(dep.toFile);
    }
    
    return graph;
}

std::vector<std::vector<std::string>> DependencyAnalyzer::findCycles(
    const std::unordered_map<std::string, std::vector<std::string>>& graph) {
    std::vector<std::vector<std::string>> cycles;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recStack;
    
    std::function<bool(const std::string&, std::vector<std::string>&)> dfs = 
        [&](const std::string& node, std::vector<std::string>& path) -> bool {
        visited.insert(node);
        recStack.insert(node);
        path.push_back(node);
        
        auto it = graph.find(node);
        if (it != graph.end()) {
            for (const auto& neighbor : it->second) {
                if (recStack.count(neighbor)) {
                    // Found cycle
                    auto cycleStart = std::find(path.begin(), path.end(), neighbor);
                    if (cycleStart != path.end()) {
                        cycles.emplace_back(cycleStart, path.end());
                    }
                    return true;
                } else if (!visited.count(neighbor) && dfs(neighbor, path)) {
                    return true;
                }
            }
        }
        
        recStack.erase(node);
        path.pop_back();
        return false;
    };
    
    for (const auto& [node, _] : graph) {
        if (!visited.count(node)) {
            std::vector<std::string> path;
            dfs(node, path);
        }
    }
    
    return cycles;
}

// PerformanceAnalyzer implementation
std::vector<std::string> PerformanceAnalyzer::getSupportedExtensions() const {
    return {".cpp", ".c", ".cc", ".cxx"};
}

std::vector<AnalysisIssue> PerformanceAnalyzer::analyzeFile(const std::string& filePath, 
                                                            const std::string& content) {
    std::vector<AnalysisIssue> issues;
    
    auto inefficiencyIssues = detectInefficiencies(content, filePath);
    auto memoryIssues = detectMemoryIssues(content, filePath);
    auto algorithmicIssues = detectAlgorithmicIssues(content, filePath);
    
    issues.insert(issues.end(), inefficiencyIssues.begin(), inefficiencyIssues.end());
    issues.insert(issues.end(), memoryIssues.begin(), memoryIssues.end());
    issues.insert(issues.end(), algorithmicIssues.begin(), algorithmicIssues.end());
    
    return issues;
}

std::vector<AnalysisIssue> PerformanceAnalyzer::analyzeProject(const std::string& projectPath) {
    std::vector<AnalysisIssue> allIssues;
    
    for (const auto& ext : getSupportedExtensions()) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ext) {
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                auto fileIssues = analyzeFile(entry.path().string(), content);
                allIssues.insert(allIssues.end(), fileIssues.begin(), fileIssues.end());
            }
        }
    }
    
    return allIssues;
}

CodeMetrics PerformanceAnalyzer::getMetrics(const std::string& filePath, 
                                            const std::string& content) {
    CodeMetrics metrics;
    
    auto inefficiencyIssues = detectInefficiencies(content, filePath);
    auto memoryIssues = detectMemoryIssues(content, filePath);
    auto algorithmicIssues = detectAlgorithmicIssues(content, filePath);
    
    metrics.customMetrics["performance_issues"] = inefficiencyIssues.size() + 
                                                  memoryIssues.size() + 
                                                  algorithmicIssues.size();
    
    return metrics;
}

std::vector<AnalysisIssue> PerformanceAnalyzer::detectInefficiencies(const std::string& content, 
                                                                     const std::string& filePath) {
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
    
    return issues;
}

std::vector<AnalysisIssue> PerformanceAnalyzer::detectMemoryIssues(const std::string& content, 
                                                                   const std::string& filePath) {
    std::vector<AnalysisIssue> issues;
    
    // Check for frequent allocations
    std::regex frequentAlloc(R"(\bnew\b.*\bfor\b|\bfor\b.*\bnew\b)");
    if (std::regex_search(content, frequentAlloc)) {
        AnalysisIssue issue("FREQUENT_ALLOCATION", 
                           "Frequent memory allocation in loop detected",
                           AnalysisSeverity::WARNING, AnalysisCategory::PERFORMANCE);
        issue.filePath = filePath;
        issue.suggestions.push_back("Pre-allocate memory outside the loop");
        issue.suggestions.push_back("Use object pools for frequent allocations");
        issues.push_back(issue);
    }
    
    return issues;
}

std::vector<AnalysisIssue> PerformanceAnalyzer::detectAlgorithmicIssues(const std::string& content, 
                                                                        const std::string& filePath) {
    std::vector<AnalysisIssue> issues;
    
    // Check for nested loops (potential O(n²) or worse)
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
        issue.suggestions.push_back("Look for opportunities to break or optimize inner loops");
        issues.push_back(issue);
    }
    
    return issues;
}

// AnalysisResult implementation
size_t AnalysisResult::getIssueCount(AnalysisSeverity severity) const {
    return std::count_if(issues.begin(), issues.end(),
                        [severity](const AnalysisIssue& issue) {
                            return issue.severity == severity;
                        });
}

size_t AnalysisResult::getIssueCount(AnalysisCategory category) const {
    return std::count_if(issues.begin(), issues.end(),
                        [category](const AnalysisIssue& issue) {
                            return issue.category == category;
                        });
}

std::vector<AnalysisIssue> AnalysisResult::getIssuesForFile(const std::string& filePath) const {
    std::vector<AnalysisIssue> fileIssues;
    std::copy_if(issues.begin(), issues.end(), std::back_inserter(fileIssues),
                [&filePath](const AnalysisIssue& issue) {
                    return issue.filePath == filePath;
                });
    return fileIssues;
}

// CodeAnalyzer implementation
CodeAnalyzer::CodeAnalyzer() : performanceProfilingEnabled_(false) {
    // Register default analyzers
    registerAnalyzer(std::make_unique<ComplexityAnalyzer>());
    registerAnalyzer(std::make_unique<SecurityAnalyzer>());
    registerAnalyzer(std::make_unique<DependencyAnalyzer>());
    registerAnalyzer(std::make_unique<PerformanceAnalyzer>());
}

CodeAnalyzer::~CodeAnalyzer() = default;

void CodeAnalyzer::registerAnalyzer(std::unique_ptr<ICodeAnalyzer> analyzer) {
    if (analyzer) {
        std::string name = analyzer->getName();
        analyzers_[name] = std::move(analyzer);
        analyzerEnabled_[name] = true;
    }
}

void CodeAnalyzer::unregisterAnalyzer(const std::string& analyzerName) {
    analyzers_.erase(analyzerName);
    analyzerEnabled_.erase(analyzerName);
    analyzerConfigs_.erase(analyzerName);
}

std::vector<std::string> CodeAnalyzer::getRegisteredAnalyzers() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : analyzers_) {
        names.push_back(name);
    }
    return names;
}

AnalysisResult CodeAnalyzer::analyzeFile(const std::string& filePath) {
    AnalysisResult result;
    result.projectPath = filePath;
    result.analysisTime = std::chrono::steady_clock::now();
    
    auto startTime = std::chrono::steady_clock::now();
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LogManager::getInstance().error("Failed to open file for analysis: " + filePath);
        return result;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    // Run enabled analyzers
    for (const auto& [name, analyzer] : analyzers_) {
        if (analyzerEnabled_[name] && analyzer->isEnabled()) {
            // Check if analyzer supports this file extension
            auto extensions = analyzer->getSupportedExtensions();
            std::filesystem::path path(filePath);
            if (std::find(extensions.begin(), extensions.end(), path.extension().string()) != extensions.end()) {
                
                std::unique_ptr<ScopedProfiler> profiler;
                if (performanceProfilingEnabled_) {
                    profiler = std::make_unique<ScopedProfiler>("analyzer_" + name, "analysis");
                }
                
                auto issues = analyzer->analyzeFile(filePath, content);
                result.issues.insert(result.issues.end(), issues.begin(), issues.end());
                
                auto metrics = analyzer->getMetrics(filePath, content);
                result.fileMetrics[filePath] = metrics;
            }
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    result.analysisTimeMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        endTime - startTime).count();
    
    if (metricsCallback_) {
        metricsCallback_("analysis_time_ms", result.analysisTimeMs);
        metricsCallback_("issues_found", static_cast<double>(result.issues.size()));
    }
    
    return result;
}

AnalysisResult CodeAnalyzer::analyzeProject(const std::string& projectPath) {
    return analyzeProject(projectPath, getRegisteredAnalyzers());
}

AnalysisResult CodeAnalyzer::analyzeProject(const std::string& projectPath, 
                                           const std::vector<std::string>& enabledAnalyzers) {
    AnalysisResult result;
    result.projectPath = projectPath;
    result.analysisTime = std::chrono::steady_clock::now();
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Collect all supported file extensions
    std::unordered_set<std::string> allExtensions;
    for (const auto& analyzerName : enabledAnalyzers) {
        auto it = analyzers_.find(analyzerName);
        if (it != analyzers_.end() && analyzerEnabled_[analyzerName]) {
            auto extensions = it->second->getSupportedExtensions();
            allExtensions.insert(extensions.begin(), extensions.end());
        }
    }
    
    // Find all relevant files
    auto relevantFiles = findFilesWithExtensions(projectPath, 
                                                std::vector<std::string>(allExtensions.begin(), allExtensions.end()));
    
    // Run analyzers on each file
    std::vector<CodeMetrics> allMetrics;
    for (const auto& filePath : relevantFiles) {
        std::ifstream file(filePath);
        if (!file.is_open()) continue;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        for (const auto& analyzerName : enabledAnalyzers) {
            auto it = analyzers_.find(analyzerName);
            if (it != analyzers_.end() && analyzerEnabled_[analyzerName] && it->second->isEnabled()) {
                auto extensions = it->second->getSupportedExtensions();
                std::filesystem::path path(filePath);
                
                if (std::find(extensions.begin(), extensions.end(), path.extension().string()) != extensions.end()) {
                    std::unique_ptr<ScopedProfiler> profiler;
                    if (performanceProfilingEnabled_) {
                        profiler = std::make_unique<ScopedProfiler>("analyzer_" + analyzerName, "analysis");
                    }
                    
                    auto issues = it->second->analyzeFile(filePath, content);
                    result.issues.insert(result.issues.end(), issues.begin(), issues.end());
                    
                    auto metrics = it->second->getMetrics(filePath, content);
                    result.fileMetrics[filePath] = metrics;
                    allMetrics.push_back(metrics);
                }
            }
        }
    }
    
    // Run project-level analysis
    for (const auto& analyzerName : enabledAnalyzers) {
        auto it = analyzers_.find(analyzerName);
        if (it != analyzers_.end() && analyzerEnabled_[analyzerName] && it->second->isEnabled()) {
            std::unique_ptr<ScopedProfiler> profiler;
            if (performanceProfilingEnabled_) {
                profiler = std::make_unique<ScopedProfiler>("project_analyzer_" + analyzerName, "analysis");
            }
            
            auto projectIssues = it->second->analyzeProject(projectPath);
            result.issues.insert(result.issues.end(), projectIssues.begin(), projectIssues.end());
        }
    }
    
    // Get dependency information
    if (std::find(enabledAnalyzers.begin(), enabledAnalyzers.end(), "DependencyAnalyzer") != enabledAnalyzers.end()) {
        auto depAnalyzer = dynamic_cast<DependencyAnalyzer*>(analyzers_["DependencyAnalyzer"].get());
        if (depAnalyzer) {
            result.dependencies = depAnalyzer->getDependencies(projectPath);
        }
    }
    
    // Aggregate metrics
    aggregateMetrics(allMetrics, result.overallMetrics);
    
    auto endTime = std::chrono::steady_clock::now();
    result.analysisTimeMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        endTime - startTime).count();
    
    if (metricsCallback_) {
        metricsCallback_("project_analysis_time_ms", result.analysisTimeMs);
        metricsCallback_("total_issues_found", static_cast<double>(result.issues.size()));
        metricsCallback_("files_analyzed", static_cast<double>(relevantFiles.size()));
    }
    
    LogManager::getInstance().info("Code analysis completed: " + 
                                  std::to_string(result.issues.size()) + " issues found in " +
                                  std::to_string(relevantFiles.size()) + " files (" +
                                  std::to_string(result.analysisTimeMs) + "ms)");
    
    return result;
}

void CodeAnalyzer::configure(const std::string& analyzerName, 
                            const std::unordered_map<std::string, std::string>& config) {
    analyzerConfigs_[analyzerName] = config;
    
    auto it = analyzers_.find(analyzerName);
    if (it != analyzers_.end()) {
        it->second->configure(config);
    }
}

void CodeAnalyzer::enableAnalyzer(const std::string& analyzerName, bool enabled) {
    analyzerEnabled_[analyzerName] = enabled;
}

bool CodeAnalyzer::isAnalyzerEnabled(const std::string& analyzerName) const {
    auto it = analyzerEnabled_.find(analyzerName);
    return it != analyzerEnabled_.end() ? it->second : false;
}

void CodeAnalyzer::exportResults(const AnalysisResult& result, const std::string& format, 
                                 const std::string& outputPath) {
    std::string reportContent = generateReport(result, format);
    
    std::ofstream file(outputPath);
    if (file.is_open()) {
        file << reportContent;
        LogManager::getInstance().info("Analysis results exported to: " + outputPath);
    } else {
        LogManager::getInstance().error("Failed to export analysis results to: " + outputPath);
    }
}

std::string CodeAnalyzer::generateReport(const AnalysisResult& result, const std::string& format) {
    std::ostringstream report;
    
    if (format == "json") {
        report << "{\n";
        report << "  \"projectPath\": \"" << result.projectPath << "\",\n";
        report << "  \"analysisTime\": \"" << std::chrono::duration_cast<std::chrono::seconds>(
            result.analysisTime.time_since_epoch()).count() << "\",\n";
        report << "  \"analysisTimeMs\": " << result.analysisTimeMs << ",\n";
        report << "  \"totalIssues\": " << result.issues.size() << ",\n";
        report << "  \"issues\": [\n";
        
        for (size_t i = 0; i < result.issues.size(); ++i) {
            const auto& issue = result.issues[i];
            report << "    {\n";
            report << "      \"id\": \"" << issue.id << "\",\n";
            report << "      \"message\": \"" << issue.message << "\",\n";
            report << "      \"severity\": \"" << formatSeverity(issue.severity) << "\",\n";
            report << "      \"category\": \"" << formatCategory(issue.category) << "\",\n";
            report << "      \"filePath\": \"" << issue.filePath << "\",\n";
            report << "      \"lineNumber\": " << issue.lineNumber << "\n";
            report << "    }";
            if (i < result.issues.size() - 1) report << ",";
            report << "\n";
        }
        
        report << "  ]\n";
        report << "}\n";
    } else {
        // Plain text format
        report << "=== Code Analysis Report ===\n";
        report << "Project: " << result.projectPath << "\n";
        report << "Analysis Time: " << result.analysisTimeMs << " ms\n";
        report << "Total Issues: " << result.issues.size() << "\n\n";
        
        // Group issues by severity
        for (auto severity : {AnalysisSeverity::CRITICAL, AnalysisSeverity::ERROR, 
                             AnalysisSeverity::WARNING, AnalysisSeverity::INFO}) {
            auto count = result.getIssueCount(severity);
            if (count > 0) {
                report << formatSeverity(severity) << " Issues: " << count << "\n";
            }
        }
        
        report << "\n=== Issues by Category ===\n";
        for (auto category : {AnalysisCategory::SECURITY, AnalysisCategory::PERFORMANCE,
                             AnalysisCategory::COMPLEXITY, AnalysisCategory::MAINTAINABILITY,
                             AnalysisCategory::DEPENDENCIES}) {
            auto count = result.getIssueCount(category);
            if (count > 0) {
                report << formatCategory(category) << ": " << count << "\n";
            }
        }
        
        report << "\n=== Detailed Issues ===\n";
        for (const auto& issue : result.issues) {
            report << "[" << formatSeverity(issue.severity) << "] " 
                   << issue.filePath << ":" << issue.lineNumber << " - " 
                   << issue.message << "\n";
            for (const auto& suggestion : issue.suggestions) {
                report << "  Suggestion: " << suggestion << "\n";
            }
            report << "\n";
        }
    }
    
    return report.str();
}

void CodeAnalyzer::setMetricsCallback(std::function<void(const std::string&, double)> callback) {
    metricsCallback_ = callback;
}

void CodeAnalyzer::enablePerformanceProfiling(bool enabled) {
    performanceProfilingEnabled_ = enabled;
}

std::vector<std::string> CodeAnalyzer::findFilesWithExtensions(const std::string& path, 
                                                              const std::vector<std::string>& extensions) {
    std::vector<std::string> files;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                files.push_back(entry.path().string());
            }
        }
    }
    
    return files;
}

void CodeAnalyzer::aggregateMetrics(const std::vector<CodeMetrics>& fileMetrics, CodeMetrics& overall) {
    for (const auto& metrics : fileMetrics) {
        overall.linesOfCode += metrics.linesOfCode;
        overall.physicalLines += metrics.physicalLines;
        overall.commentLines += metrics.commentLines;
        overall.blankLines += metrics.blankLines;
        overall.cyclomaticComplexity += metrics.cyclomaticComplexity;
        overall.nestingDepth = std::max(overall.nestingDepth, metrics.nestingDepth);
        overall.functionCount += metrics.functionCount;
        overall.classCount += metrics.classCount;
        overall.namespaceCount += metrics.namespaceCount;
        
        for (const auto& [key, value] : metrics.customMetrics) {
            overall.customMetrics[key] += value;
        }
    }
    
    // Calculate averages
    if (!fileMetrics.empty()) {
        double totalMaintainability = 0.0;
        for (const auto& metrics : fileMetrics) {
            totalMaintainability += metrics.maintainabilityIndex;
        }
        overall.maintainabilityIndex = totalMaintainability / fileMetrics.size();
    }
}

std::string CodeAnalyzer::formatSeverity(AnalysisSeverity severity) const {
    switch (severity) {
        case AnalysisSeverity::INFO: return "INFO";
        case AnalysisSeverity::WARNING: return "WARNING";
        case AnalysisSeverity::ERROR: return "ERROR";  
        case AnalysisSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string CodeAnalyzer::formatCategory(AnalysisCategory category) const {
    switch (category) {
        case AnalysisCategory::COMPLEXITY: return "COMPLEXITY";
        case AnalysisCategory::MAINTAINABILITY: return "MAINTAINABILITY";
        case AnalysisCategory::PERFORMANCE: return "PERFORMANCE";
        case AnalysisCategory::SECURITY: return "SECURITY";
        case AnalysisCategory::STYLE: return "STYLE";
        case AnalysisCategory::DEPENDENCIES: return "DEPENDENCIES";
        case AnalysisCategory::MEMORY: return "MEMORY";
        case AnalysisCategory::TESTING: return "TESTING";
        case AnalysisCategory::ARCHITECTURE: return "ARCHITECTURE";
        default: return "UNKNOWN";
    }
}

// RefactoringSuggestionEngine implementation
std::vector<RefactoringSuggestionEngine::RefactoringSuggestion> 
RefactoringSuggestionEngine::generateSuggestions(const AnalysisResult& result) {
    std::vector<RefactoringSuggestion> suggestions;
    
    for (const auto& [filePath, metrics] : result.fileMetrics) {
        std::ifstream file(filePath);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            auto fileSuggestions = generateSuggestionsForFile(filePath, content);
            suggestions.insert(suggestions.end(), fileSuggestions.begin(), fileSuggestions.end());
        }
    }
    
    return suggestions;
}

std::vector<RefactoringSuggestionEngine::RefactoringSuggestion> 
RefactoringSuggestionEngine::generateSuggestionsForFile(const std::string& filePath, 
                                                        const std::string& content) {
    std::vector<RefactoringSuggestion> suggestions;
    
    auto extractMethodSuggestions = suggestExtractMethod(content, filePath);
    auto complexitySuggestions = suggestReduceComplexity(content, filePath);
    auto duplicationSuggestions = suggestRemoveDuplication(content, filePath);
    
    suggestions.insert(suggestions.end(), extractMethodSuggestions.begin(), extractMethodSuggestions.end());
    suggestions.insert(suggestions.end(), complexitySuggestions.begin(), complexitySuggestions.end());
    suggestions.insert(suggestions.end(), duplicationSuggestions.begin(), duplicationSuggestions.end());
    
    return suggestions;
}

std::vector<RefactoringSuggestionEngine::RefactoringSuggestion> 
RefactoringSuggestionEngine::suggestExtractMethod(const std::string& content, 
                                                  const std::string& filePath) {
    std::vector<RefactoringSuggestion> suggestions;
    
    // Look for long functions that could be extracted
    std::regex functionPattern(R"((\w+)\s*\([^)]*\)\s*\{([^}]*)\})");
    std::sregex_iterator iter(content.begin(), content.end(), functionPattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string functionBody = (*iter)[2].str();
        size_t lineCount = std::count(functionBody.begin(), functionBody.end(), '\n');
        
        if (lineCount > 20) {
            RefactoringSuggestion suggestion;
            suggestion.id = "EXTRACT_METHOD_" + (*iter)[1].str();
            suggestion.title = "Extract method from long function";
            suggestion.description = "Function " + (*iter)[1].str() + " is " + 
                                   std::to_string(lineCount) + " lines long and could be broken down";
            suggestion.filePath = filePath;
            suggestion.confidenceScore = 0.8;
            suggestion.benefits.push_back("Improved readability");
            suggestion.benefits.push_back("Better testability");
            suggestion.benefits.push_back("Reduced complexity");
            suggestions.push_back(suggestion);
        }
    }
    
    return suggestions;
}

std::vector<RefactoringSuggestionEngine::RefactoringSuggestion> 
RefactoringSuggestionEngine::suggestReduceComplexity(const std::string& content, 
                                                     const std::string& filePath) {
    std::vector<RefactoringSuggestion> suggestions;
    
    // Look for nested if statements
    std::regex nestedIfs(R"(if\s*\([^{]*\{\s*if\s*\()");
    if (std::regex_search(content, nestedIfs)) {
        RefactoringSuggestion suggestion;
        suggestion.id = "REDUCE_NESTING";
        suggestion.title = "Reduce nesting complexity";
        suggestion.description = "Deeply nested if statements found that could be simplified";
        suggestion.filePath = filePath;
        suggestion.confidenceScore = 0.7;
        suggestion.benefits.push_back("Improved readability");
        suggestion.benefits.push_back("Reduced cognitive complexity");
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

std::vector<RefactoringSuggestionEngine::RefactoringSuggestion> 
RefactoringSuggestionEngine::suggestRemoveDuplication(const std::string& content, 
                                                      const std::string& filePath) {
    std::vector<RefactoringSuggestion> suggestions;
    
    // Simple duplication detection (this could be much more sophisticated)
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove leading/trailing whitespace
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());
        
        if (!line.empty() && line.find("//") != 0) {
            lines.push_back(line);
        }
    }
    
    // Look for repeated sequences of 3+ lines
    for (size_t i = 0; i < lines.size() - 2; ++i) {
        for (size_t j = i + 3; j < lines.size() - 2; ++j) {
            bool isDuplicate = true;
            for (size_t k = 0; k < 3; ++k) {
                if (lines[i + k] != lines[j + k]) {
                    isDuplicate = false;
                    break;
                }
            }
            
            if (isDuplicate) {
                RefactoringSuggestion suggestion;
                suggestion.id = "REMOVE_DUPLICATION_" + std::to_string(i) + "_" + std::to_string(j);
                suggestion.title = "Remove code duplication";
                suggestion.description = "Duplicate code sequences found at lines " + 
                                       std::to_string(i + 1) + " and " + std::to_string(j + 1);
                suggestion.filePath = filePath;
                suggestion.startLine = i + 1;
                suggestion.endLine = i + 4;
                suggestion.confidenceScore = 0.9;
                suggestion.benefits.push_back("Reduced maintenance burden");
                suggestion.benefits.push_back("Improved consistency");
                suggestions.push_back(suggestion);
                break; // Avoid too many duplication reports
            }
        }
    }
    
    return suggestions;
}

} // namespace bolt