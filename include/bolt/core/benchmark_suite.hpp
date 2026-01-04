#ifndef BOLT_BENCHMARK_SUITE_HPP
#define BOLT_BENCHMARK_SUITE_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include "performance_profiler.hpp"

namespace bolt {

// Forward declarations
class BenchmarkSuite;
class BenchmarkResult;

/**
 * Configuration for a single benchmark
 */
struct BenchmarkConfig {
    std::string name;
    std::string description;
    std::string category;
    int iterations = 10;
    int warmupRuns = 3;
    std::chrono::milliseconds timeout = std::chrono::milliseconds(30000);
    std::unordered_map<std::string, std::string> parameters;
    
    // Default constructor
    BenchmarkConfig() = default;
    
    BenchmarkConfig(const std::string& benchmarkName, const std::string& benchmarkDescription)
        : name(benchmarkName), description(benchmarkDescription), category("GENERAL") {}
};

/**
 * Result data for a single benchmark execution
 */
struct BenchmarkResult {
    std::string name;
    std::string category;
    std::string description;
    
    // Performance metrics
    double averageDurationMs = 0.0;
    double minDurationMs = 0.0;
    double maxDurationMs = 0.0;
    double standardDeviationMs = 0.0;
    
    // Resource usage
    double averageMemoryUsageMB = 0.0;
    double maxMemoryUsageMB = 0.0;
    double averageCpuUsagePercent = 0.0;
    
    // Execution details
    int totalIterations = 0;
    int successfulRuns = 0;
    int failedRuns = 0;
    std::chrono::steady_clock::time_point timestamp;
    std::vector<double> rawDurations;
    
    // Metadata
    std::unordered_map<std::string, std::string> metadata;
    std::string errorMessage;
    
    bool isValid() const { return successfulRuns > 0; }
    double getSuccessRate() const { return totalIterations > 0 ? (double)successfulRuns / totalIterations : 0.0; }
};

/**
 * Comparison between two benchmark results
 */
struct BenchmarkComparison {
    std::string benchmarkName;
    BenchmarkResult baseline;
    BenchmarkResult current;
    
    // Performance comparison metrics
    double durationChangePercent = 0.0;
    double memoryChangePercent = 0.0;
    double cpuChangePercent = 0.0;
    
    enum class Status { IMPROVED, DEGRADED, STABLE, INCONCLUSIVE };
    Status performanceStatus = Status::INCONCLUSIVE;
    
    bool hasRegression() const { return performanceStatus == Status::DEGRADED; }
    bool hasImprovement() const { return performanceStatus == Status::IMPROVED; }
};

/**
 * Benchmark function signature
 */
using BenchmarkFunction = std::function<void(const BenchmarkConfig&)>;

/**
 * Main benchmark suite class
 */
class BenchmarkSuite {
public:
    /**
     * Get singleton instance
     */
    static BenchmarkSuite& getInstance();
    
    /**
     * Register a benchmark function
     */
    void registerBenchmark(const BenchmarkConfig& config, BenchmarkFunction function);
    
    /**
     * Run all registered benchmarks
     */
    std::vector<BenchmarkResult> runAllBenchmarks();
    
    /**
     * Run benchmarks by category
     */
    std::vector<BenchmarkResult> runBenchmarksByCategory(const std::string& category);
    
    /**
     * Run a specific benchmark by name
     */
    BenchmarkResult runBenchmark(const std::string& name);
    
    /**
     * Compare benchmark results with baseline
     */
    std::vector<BenchmarkComparison> compareWithBaseline(
        const std::vector<BenchmarkResult>& current,
        const std::vector<BenchmarkResult>& baseline
    );
    
    /**
     * Generate benchmark reports
     */
    void generateJsonReport(const std::vector<BenchmarkResult>& results, const std::string& filename);
    void generateCsvReport(const std::vector<BenchmarkResult>& results, const std::string& filename);
    void generateHtmlReport(const std::vector<BenchmarkResult>& results, const std::string& filename);
    
    /**
     * Load and save baseline results
     */
    std::vector<BenchmarkResult> loadBaselineResults(const std::string& filename);
    void saveBaselineResults(const std::vector<BenchmarkResult>& results, const std::string& filename);
    
    /**
     * Configuration
     */
    void setDefaultIterations(int iterations) { defaultIterations_ = iterations; }
    void setDefaultWarmupRuns(int warmupRuns) { defaultWarmupRuns_ = warmupRuns; }
    void setDefaultTimeout(std::chrono::milliseconds timeout) { defaultTimeout_ = timeout; }
    void enableVerboseOutput(bool enable) { verboseOutput_ = enable; }
    
    /**
     * Get available benchmarks
     */
    std::vector<std::string> getAvailableBenchmarks() const;
    std::vector<std::string> getAvailableCategories() const;

private:
    BenchmarkSuite() = default;
    BenchmarkSuite(const BenchmarkSuite&) = delete;
    BenchmarkSuite& operator=(const BenchmarkSuite&) = delete;
    
public:
    ~BenchmarkSuite() = default;
    
    // Internal benchmark execution
    BenchmarkResult executeBenchmark(const BenchmarkConfig& config, BenchmarkFunction function);
    
    // Statistical calculations
    double calculateStandardDeviation(const std::vector<double>& values, double mean);
    BenchmarkComparison::Status determinePerformanceStatus(const BenchmarkResult& baseline, const BenchmarkResult& current);
    
    // Report generation helpers
    std::string formatDuration(double milliseconds);
    std::string formatMemory(double megabytes);
    std::string formatPercentage(double percentage);
    
    // Member variables
    std::unordered_map<std::string, std::pair<BenchmarkConfig, BenchmarkFunction>> benchmarks_;
    mutable std::mutex benchmarksMutex_;
    
    // Configuration
    int defaultIterations_ = 10;
    int defaultWarmupRuns_ = 3;
    std::chrono::milliseconds defaultTimeout_ = std::chrono::milliseconds(30000);
    bool verboseOutput_ = false;
    
    // Thresholds for performance status
    static constexpr double IMPROVEMENT_THRESHOLD = -5.0;  // -5% or better is improvement
    static constexpr double REGRESSION_THRESHOLD = 10.0;   // +10% or worse is regression
};

/**
 * Convenience macros for benchmark registration
 */
#define BOLT_BENCHMARK(name, category) \
    void benchmark_##name(const bolt::BenchmarkConfig& config); \
    namespace { \
        struct BenchmarkRegistrar_##name { \
            BenchmarkRegistrar_##name() { \
                bolt::BenchmarkConfig config(#name, "Benchmark for " #name); \
                config.category = category; \
                bolt::BenchmarkSuite::getInstance().registerBenchmark(config, benchmark_##name); \
            } \
        }; \
        static BenchmarkRegistrar_##name benchmarkRegistrar_##name; \
    } \
    void benchmark_##name(const bolt::BenchmarkConfig& config)

#define BOLT_BENCHMARK_CONFIG(name, category, desc, iters) \
    void benchmark_##name(const bolt::BenchmarkConfig& config); \
    namespace { \
        struct BenchmarkRegistrar_##name { \
            BenchmarkRegistrar_##name() { \
                bolt::BenchmarkConfig config(#name, desc); \
                config.category = category; \
                config.iterations = iters; \
                bolt::BenchmarkSuite::getInstance().registerBenchmark(config, benchmark_##name); \
            } \
        }; \
        static BenchmarkRegistrar_##name benchmarkRegistrar_##name; \
    } \
    void benchmark_##name(const bolt::BenchmarkConfig& config)

} // namespace bolt

#endif // BOLT_BENCHMARK_SUITE_HPP