#include "bolt/core/benchmark_suite.hpp"
#include "bolt/core/performance_profiler.hpp"
#include "bolt/core/logging.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <set>

namespace bolt {

// Forward declarations for helper functions
size_t getCurrentMemoryUsage();
double getCurrentCpuUsage();

// Static member definitions
static std::unique_ptr<BenchmarkSuite> instance_;
static std::mutex instanceMutex_;

BenchmarkSuite& BenchmarkSuite::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (!instance_) {
        instance_ = std::unique_ptr<BenchmarkSuite>(new BenchmarkSuite());
    }
    return *instance_;
}

void BenchmarkSuite::registerBenchmark(const BenchmarkConfig& config, BenchmarkFunction function) {
    std::lock_guard<std::mutex> lock(benchmarksMutex_);
    benchmarks_[config.name] = std::make_pair(config, function);
    
    if (verboseOutput_) {
        BOLT_INFO("Registered benchmark '" + config.name + "' in category '" + config.category + "'");
    }
}

std::vector<BenchmarkResult> BenchmarkSuite::runAllBenchmarks() {
    std::vector<BenchmarkResult> results;
    
    std::lock_guard<std::mutex> lock(benchmarksMutex_);
    for (const auto& benchmark : benchmarks_) {
        if (verboseOutput_) {
            BOLT_INFO("Running benchmark '" + benchmark.first + "'...");
        }
        
        BenchmarkResult result = executeBenchmark(benchmark.second.first, benchmark.second.second);
        results.push_back(result);
        
        if (verboseOutput_) {
            BOLT_INFO("Benchmark '" + result.name + "' completed: " + std::to_string(result.averageDurationMs) + "ms avg (" + std::to_string(result.successfulRuns) + " runs)");
        }
    }
    
    return results;
}

std::vector<BenchmarkResult> BenchmarkSuite::runBenchmarksByCategory(const std::string& category) {
    std::vector<BenchmarkResult> results;
    
    std::lock_guard<std::mutex> lock(benchmarksMutex_);
    for (const auto& benchmark : benchmarks_) {
        if (benchmark.second.first.category == category) {
            if (verboseOutput_) {
                BOLT_INFO("Running benchmark '" + benchmark.first + "' in category '" + category + "'...");
            }
            
            BenchmarkResult result = executeBenchmark(benchmark.second.first, benchmark.second.second);
            results.push_back(result);
        }
    }
    
    return results;
}

BenchmarkResult BenchmarkSuite::runBenchmark(const std::string& name) {
    std::lock_guard<std::mutex> lock(benchmarksMutex_);
    auto it = benchmarks_.find(name);
    if (it == benchmarks_.end()) {
        BenchmarkResult result;
        result.name = name;
        result.errorMessage = "Benchmark not found";
        return result;
    }
    
    return executeBenchmark(it->second.first, it->second.second);
}

BenchmarkResult BenchmarkSuite::executeBenchmark(const BenchmarkConfig& config, BenchmarkFunction function) {
    BenchmarkResult result;
    result.name = config.name;
    result.category = config.category;
    result.description = config.description;
    result.timestamp = std::chrono::steady_clock::now();
    result.totalIterations = config.iterations;
    
    auto& profiler = PerformanceProfiler::getInstance();
    profiler.enable();
    
    // Create a benchmark session
    auto session = profiler.createSession("benchmark_" + config.name);
    profiler.setCurrentSession("benchmark_" + config.name);
    
    std::vector<double> durations;
    std::vector<double> memoryUsages;
    std::vector<double> cpuUsages;
    
    try {
        // Warmup runs
        for (int i = 0; i < config.warmupRuns; ++i) {
            try {
                function(config);
            } catch (const std::exception& e) {
                if (verboseOutput_) {
                    BOLT_WARN("Warmup run " + std::to_string(i) + " failed: " + e.what());
                }
            }
        }
        
        // Actual benchmark runs
        for (int i = 0; i < config.iterations; ++i) {
            auto runStart = std::chrono::steady_clock::now();
            size_t memoryBefore = getCurrentMemoryUsage();
            
            try {
                auto metric = profiler.startMetric(config.name + "_run_" + std::to_string(i), config.category);
                
                function(config);
                
                profiler.endMetric(metric);
                
                auto runEnd = std::chrono::steady_clock::now();
                double duration = std::chrono::duration<double, std::milli>(runEnd - runStart).count();
                size_t memoryAfter = getCurrentMemoryUsage();
                
                durations.push_back(duration);
                memoryUsages.push_back(static_cast<double>(memoryAfter - memoryBefore) / (1024 * 1024)); // Convert to MB
                cpuUsages.push_back(getCurrentCpuUsage());
                
                result.successfulRuns++;
            } catch (const std::exception& e) {
                result.failedRuns++;
                if (verboseOutput_) {
                    BOLT_WARN("Benchmark run " + std::to_string(i) + " failed: " + e.what());
                }
                if (result.errorMessage.empty()) {
                    result.errorMessage = e.what();
                }
            }
        }
        
        // Calculate statistics
        if (!durations.empty()) {
            result.rawDurations = durations;
            result.averageDurationMs = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
            result.minDurationMs = *std::min_element(durations.begin(), durations.end());
            result.maxDurationMs = *std::max_element(durations.begin(), durations.end());
            result.standardDeviationMs = calculateStandardDeviation(durations, result.averageDurationMs);
        }
        
        if (!memoryUsages.empty()) {
            result.averageMemoryUsageMB = std::accumulate(memoryUsages.begin(), memoryUsages.end(), 0.0) / memoryUsages.size();
            result.maxMemoryUsageMB = *std::max_element(memoryUsages.begin(), memoryUsages.end());
        }
        
        if (!cpuUsages.empty()) {
            result.averageCpuUsagePercent = std::accumulate(cpuUsages.begin(), cpuUsages.end(), 0.0) / cpuUsages.size();
        }
        
        // Add session metadata
        if (session) {
            result.metadata["session_metrics"] = std::to_string(session->getMetricsCount());
            result.metadata["session_duration"] = std::to_string(session->getTotalDurationMs());
        }
        
    } catch (const std::exception& e) {
        result.errorMessage = "Benchmark execution failed: " + std::string(e.what());
        BOLT_ERROR("Benchmark '" + config.name + "' failed: " + e.what());
    }
    
    profiler.endSession("benchmark_" + config.name);
    
    return result;
}

std::vector<BenchmarkComparison> BenchmarkSuite::compareWithBaseline(
    const std::vector<BenchmarkResult>& current,
    const std::vector<BenchmarkResult>& baseline
) {
    std::vector<BenchmarkComparison> comparisons;
    
    // Create a map for quick baseline lookup
    std::unordered_map<std::string, BenchmarkResult> baselineMap;
    for (const auto& result : baseline) {
        baselineMap[result.name] = result;
    }
    
    for (const auto& currentResult : current) {
        auto baselineIt = baselineMap.find(currentResult.name);
        if (baselineIt != baselineMap.end()) {
            BenchmarkComparison comparison;
            comparison.benchmarkName = currentResult.name;
            comparison.current = currentResult;
            comparison.baseline = baselineIt->second;
            
            // Calculate performance changes
            if (baselineIt->second.averageDurationMs > 0) {
                comparison.durationChangePercent = 
                    ((currentResult.averageDurationMs - baselineIt->second.averageDurationMs) / 
                     baselineIt->second.averageDurationMs) * 100.0;
            }
            
            if (baselineIt->second.averageMemoryUsageMB > 0) {
                comparison.memoryChangePercent = 
                    ((currentResult.averageMemoryUsageMB - baselineIt->second.averageMemoryUsageMB) / 
                     baselineIt->second.averageMemoryUsageMB) * 100.0;
            }
            
            if (baselineIt->second.averageCpuUsagePercent > 0) {
                comparison.cpuChangePercent = 
                    ((currentResult.averageCpuUsagePercent - baselineIt->second.averageCpuUsagePercent) / 
                     baselineIt->second.averageCpuUsagePercent) * 100.0;
            }
            
            // Determine performance status
            comparison.performanceStatus = determinePerformanceStatus(baselineIt->second, currentResult);
            
            comparisons.push_back(comparison);
        }
    }
    
    return comparisons;
}

void BenchmarkSuite::generateJsonReport(const std::vector<BenchmarkResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        BOLT_ERROR("Failed to open file for JSON report: " + filename);
        return;
    }
    
    file << "{\n";
    file << "  \"benchmark_suite_version\": \"1.0.0\",\n";
    file << "  \"timestamp\": \"" << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\",\n";
    file << "  \"results\": [\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        file << "    {\n";
        file << "      \"name\": \"" << result.name << "\",\n";
        file << "      \"category\": \"" << result.category << "\",\n";
        file << "      \"description\": \"" << result.description << "\",\n";
        file << "      \"average_duration_ms\": " << result.averageDurationMs << ",\n";
        file << "      \"min_duration_ms\": " << result.minDurationMs << ",\n";
        file << "      \"max_duration_ms\": " << result.maxDurationMs << ",\n";
        file << "      \"standard_deviation_ms\": " << result.standardDeviationMs << ",\n";
        file << "      \"average_memory_usage_mb\": " << result.averageMemoryUsageMB << ",\n";
        file << "      \"max_memory_usage_mb\": " << result.maxMemoryUsageMB << ",\n";
        file << "      \"average_cpu_usage_percent\": " << result.averageCpuUsagePercent << ",\n";
        file << "      \"total_iterations\": " << result.totalIterations << ",\n";
        file << "      \"successful_runs\": " << result.successfulRuns << ",\n";
        file << "      \"failed_runs\": " << result.failedRuns << ",\n";
        file << "      \"success_rate\": " << result.getSuccessRate() << ",\n";
        file << "      \"error_message\": \"" << result.errorMessage << "\"\n";
        file << "    }";
        if (i < results.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    BOLT_INFO("JSON report generated: " + filename);
}

void BenchmarkSuite::generateCsvReport(const std::vector<BenchmarkResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        BOLT_ERROR("Failed to open file for CSV report: " + filename);
        return;
    }
    
    // Header
    file << "Name,Category,Description,AvgDurationMs,MinDurationMs,MaxDurationMs,StdDevMs,"
         << "AvgMemoryMB,MaxMemoryMB,AvgCpuPercent,TotalIterations,SuccessfulRuns,FailedRuns,SuccessRate,ErrorMessage\n";
    
    // Data rows
    for (const auto& result : results) {
        file << "\"" << result.name << "\","
             << "\"" << result.category << "\","
             << "\"" << result.description << "\","
             << result.averageDurationMs << ","
             << result.minDurationMs << ","
             << result.maxDurationMs << ","
             << result.standardDeviationMs << ","
             << result.averageMemoryUsageMB << ","
             << result.maxMemoryUsageMB << ","
             << result.averageCpuUsagePercent << ","
             << result.totalIterations << ","
             << result.successfulRuns << ","
             << result.failedRuns << ","
             << result.getSuccessRate() << ","
             << "\"" << result.errorMessage << "\"\n";
    }
    
    file.close();
    BOLT_INFO("CSV report generated: " + filename);
}

void BenchmarkSuite::generateHtmlReport(const std::vector<BenchmarkResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        BOLT_ERROR("Failed to open file for HTML report: " + filename);
        return;
    }
    
    file << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bolt Performance Benchmark Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background: #f0f0f0; padding: 20px; border-radius: 5px; margin-bottom: 20px; }
        .benchmark { margin-bottom: 15px; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        .benchmark.success { background: #f9fff9; border-color: #4caf50; }
        .benchmark.failure { background: #fff9f9; border-color: #f44336; }
        .stats { display: flex; gap: 20px; margin-top: 10px; }
        .stat { background: #e8e8e8; padding: 8px 12px; border-radius: 3px; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; }
        th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Bolt Performance Benchmark Report</h1>
        <p>Generated: )" << std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() << R"(</p>
        <p>Total Benchmarks: )" << results.size() << R"(</p>
    </div>
)";
    
    // Summary statistics
    int totalSuccessful = 0, totalFailed = 0;
    double averageTime = 0.0;
    for (const auto& result : results) {
        totalSuccessful += result.successfulRuns;
        totalFailed += result.failedRuns;
        averageTime += result.averageDurationMs;
    }
    if (!results.empty()) {
        averageTime /= results.size();
    }
    
    file << R"(    <div class="benchmark success">
        <h3>Summary Statistics</h3>
        <div class="stats">
            <div class="stat">Successful Runs: )" << totalSuccessful << R"(</div>
            <div class="stat">Failed Runs: )" << totalFailed << R"(</div>
            <div class="stat">Average Duration: )" << formatDuration(averageTime) << R"(</div>
        </div>
    </div>
)";
    
    // Individual benchmark results
    for (const auto& result : results) {
        std::string cssClass = result.isValid() ? "success" : "failure";
        file << R"(    <div class="benchmark )" << cssClass << R"(">
        <h3>)" << result.name << R"( ()" << result.category << R"()</h3>
        <p>)" << result.description << R"(</p>
        <div class="stats">
            <div class="stat">Avg: )" << formatDuration(result.averageDurationMs) << R"(</div>
            <div class="stat">Min: )" << formatDuration(result.minDurationMs) << R"(</div>
            <div class="stat">Max: )" << formatDuration(result.maxDurationMs) << R"(</div>
            <div class="stat">StdDev: )" << formatDuration(result.standardDeviationMs) << R"(</div>
            <div class="stat">Memory: )" << formatMemory(result.averageMemoryUsageMB) << R"(</div>
            <div class="stat">CPU: )" << formatPercentage(result.averageCpuUsagePercent) << R"(</div>
            <div class="stat">Success Rate: )" << formatPercentage(result.getSuccessRate() * 100) << R"(</div>
        </div>
)";
        if (!result.errorMessage.empty()) {
            file << R"(        <p style="color: red;"><strong>Error:</strong> )" << result.errorMessage << R"(</p>)";
        }
        file << R"(    </div>
)";
    }
    
    file << R"(</body>
</html>)";
    
    file.close();
    BOLT_INFO("HTML report generated: " + filename);
}

std::vector<BenchmarkResult> BenchmarkSuite::loadBaselineResults(const std::string& filename) {
    std::vector<BenchmarkResult> results;
    // For now, return empty vector - JSON parsing would be implemented here
    BOLT_WARN("Baseline loading not yet implemented for file: " + filename);
    return results;
}

void BenchmarkSuite::saveBaselineResults(const std::vector<BenchmarkResult>& results, const std::string& filename) {
    generateJsonReport(results, filename);
    BOLT_INFO("Baseline saved to: " + filename);
}

std::vector<std::string> BenchmarkSuite::getAvailableBenchmarks() const {
    std::lock_guard<std::mutex> lock(benchmarksMutex_);
    std::vector<std::string> names;
    for (const auto& benchmark : benchmarks_) {
        names.push_back(benchmark.first);
    }
    return names;
}

std::vector<std::string> BenchmarkSuite::getAvailableCategories() const {
    std::lock_guard<std::mutex> lock(benchmarksMutex_);
    std::set<std::string> categories;
    for (const auto& benchmark : benchmarks_) {
        categories.insert(benchmark.second.first.category);
    }
    return std::vector<std::string>(categories.begin(), categories.end());
}

double BenchmarkSuite::calculateStandardDeviation(const std::vector<double>& values, double mean) {
    if (values.empty()) return 0.0;
    
    double variance = 0.0;
    for (double value : values) {
        variance += (value - mean) * (value - mean);
    }
    variance /= values.size();
    return std::sqrt(variance);
}

BenchmarkComparison::Status BenchmarkSuite::determinePerformanceStatus(
    const BenchmarkResult& baseline, const BenchmarkResult& current) {
    
    if (!baseline.isValid() || !current.isValid()) {
        return BenchmarkComparison::Status::INCONCLUSIVE;
    }
    
    double change = ((current.averageDurationMs - baseline.averageDurationMs) / 
                     baseline.averageDurationMs) * 100.0;
    
    if (change <= IMPROVEMENT_THRESHOLD) {
        return BenchmarkComparison::Status::IMPROVED;
    } else if (change >= REGRESSION_THRESHOLD) {
        return BenchmarkComparison::Status::DEGRADED;
    } else {
        return BenchmarkComparison::Status::STABLE;
    }
}

std::string BenchmarkSuite::formatDuration(double milliseconds) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << milliseconds << " ms";
    return ss.str();
}

std::string BenchmarkSuite::formatMemory(double megabytes) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << megabytes << " MB";
    return ss.str();
}

std::string BenchmarkSuite::formatPercentage(double percentage) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << percentage << "%";
    return ss.str();
}

// Helper functions for resource monitoring (placeholder implementations)
size_t getCurrentMemoryUsage() {
    // Read from /proc/self/status or similar
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            return std::stoul(value) * 1024; // Convert from KB to bytes
        }
    }
    return 0;
}

double getCurrentCpuUsage() {
    // Simple CPU usage estimation - in a real implementation this would
    // read from /proc/stat or use system-specific APIs
    return 0.0;
}

} // namespace bolt