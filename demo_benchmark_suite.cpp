#include "include/bolt/core/performance_profiler.hpp"
#include "include/bolt/core/logging.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <thread>
#include <random>
#include <fstream>
#include <unordered_map>

using namespace bolt;

// Simple benchmark structure
struct SimpleBenchmark {
    std::string name;
    std::string category;
    std::string description;
    int iterations;
    std::function<void()> function;
};

struct BenchmarkResult {
    std::string name;
    std::string category;
    double averageDurationMs;
    double minDurationMs;
    double maxDurationMs;
    double standardDeviationMs;
    int successfulRuns;
    int failedRuns;
    bool isValid() const { return successfulRuns > 0; }
    double getSuccessRate() const { return successfulRuns > 0 ? (double)successfulRuns / (successfulRuns + failedRuns) : 0.0; }
};

class SimpleBenchmarkSuite {
private:
    std::vector<SimpleBenchmark> benchmarks_;
    bool verbose_;
    
    double calculateStandardDeviation(const std::vector<double>& values, double mean) {
        if (values.empty()) return 0.0;
        double variance = 0.0;
        for (double value : values) {
            variance += (value - mean) * (value - mean);
        }
        variance /= values.size();
        return std::sqrt(variance);
    }
    
public:
    SimpleBenchmarkSuite() : verbose_(false) {}
    
    void setVerbose(bool verbose) { verbose_ = verbose; }
    
    void registerBenchmark(const std::string& name, const std::string& category, 
                          const std::string& description, int iterations, 
                          std::function<void()> function) {
        benchmarks_.push_back({name, category, description, iterations, function});
        if (verbose_) {
            std::cout << "Registered benchmark: " << name << " (" << category << ")\n";
        }
    }
    
    BenchmarkResult runBenchmark(const SimpleBenchmark& benchmark) {
        BenchmarkResult result;
        result.name = benchmark.name;
        result.category = benchmark.category;
        result.successfulRuns = 0;
        result.failedRuns = 0;
        
        std::vector<double> durations;
        auto& profiler = PerformanceProfiler::getInstance();
        profiler.enable();
        
        if (verbose_) {
            std::cout << "Running " << benchmark.name << " (" << benchmark.iterations << " iterations)...\n";
        }
        
        for (int i = 0; i < benchmark.iterations; ++i) {
            try {
                auto start = std::chrono::high_resolution_clock::now();
                
                // Use profiler for this run
                auto metric = profiler.startMetric(benchmark.name + "_run_" + std::to_string(i), benchmark.category);
                
                benchmark.function();
                
                profiler.endMetric(metric);
                
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration<double, std::milli>(end - start).count();
                durations.push_back(duration);
                result.successfulRuns++;
                
            } catch (const std::exception& e) {
                result.failedRuns++;
                if (verbose_) {
                    std::cout << "  Run " << i << " failed: " << e.what() << "\n";
                }
            }
        }
        
        if (!durations.empty()) {
            result.averageDurationMs = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
            result.minDurationMs = *std::min_element(durations.begin(), durations.end());
            result.maxDurationMs = *std::max_element(durations.begin(), durations.end());
            result.standardDeviationMs = calculateStandardDeviation(durations, result.averageDurationMs);
        }
        
        return result;
    }
    
    std::vector<BenchmarkResult> runAllBenchmarks() {
        std::vector<BenchmarkResult> results;
        
        std::cout << "Running " << benchmarks_.size() << " benchmarks...\n\n";
        
        for (const auto& benchmark : benchmarks_) {
            auto result = runBenchmark(benchmark);
            results.push_back(result);
        }
        
        return results;
    }
    
    std::vector<BenchmarkResult> runCategory(const std::string& category) {
        std::vector<BenchmarkResult> results;
        
        for (const auto& benchmark : benchmarks_) {
            if (benchmark.category == category) {
                auto result = runBenchmark(benchmark);
                results.push_back(result);
            }
        }
        
        return results;
    }
    
    void generateReport(const std::vector<BenchmarkResult>& results, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
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
            file << "      \"average_duration_ms\": " << result.averageDurationMs << ",\n";
            file << "      \"min_duration_ms\": " << result.minDurationMs << ",\n";
            file << "      \"max_duration_ms\": " << result.maxDurationMs << ",\n";
            file << "      \"standard_deviation_ms\": " << result.standardDeviationMs << ",\n";
            file << "      \"successful_runs\": " << result.successfulRuns << ",\n";
            file << "      \"failed_runs\": " << result.failedRuns << ",\n";
            file << "      \"success_rate\": " << result.getSuccessRate() << "\n";
            file << "    }";
            if (i < results.size() - 1) file << ",";
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        std::cout << "Report generated: " << filename << "\n";
    }
    
    void printSummary(const std::vector<BenchmarkResult>& results) {
        std::cout << "\n=== Benchmark Summary ===\n";
        std::cout << std::left << std::setw(25) << "Benchmark" 
                  << std::setw(10) << "Category"
                  << std::setw(15) << "Avg Time (ms)"
                  << std::setw(15) << "Min Time (ms)"
                  << std::setw(15) << "Max Time (ms)"
                  << std::setw(12) << "Success Rate"
                  << "Status\n";
        std::cout << std::string(95, '-') << "\n";
        
        for (const auto& result : results) {
            std::string status = result.isValid() ? "SUCCESS" : "FAILED";
            std::cout << std::left << std::setw(25) << result.name.substr(0, 24)
                      << std::setw(10) << result.category
                      << std::setw(15) << std::fixed << std::setprecision(3) << result.averageDurationMs
                      << std::setw(15) << std::fixed << std::setprecision(3) << result.minDurationMs
                      << std::setw(15) << std::fixed << std::setprecision(3) << result.maxDurationMs
                      << std::setw(12) << std::fixed << std::setprecision(1) << (result.getSuccessRate() * 100) << "%"
                      << status << "\n";
        }
        
        // Summary statistics
        int totalSuccessful = 0, totalFailed = 0;
        double totalTime = 0.0;
        
        for (const auto& result : results) {
            if (result.isValid()) {
                totalSuccessful++;
                totalTime += result.averageDurationMs;
            } else {
                totalFailed++;
            }
        }
        
        std::cout << "\n";
        std::cout << "Total Benchmarks: " << results.size() << "\n";
        std::cout << "Successful: " << totalSuccessful << "\n";
        std::cout << "Failed: " << totalFailed << "\n";
        if (totalSuccessful > 0) {
            std::cout << "Average Time: " << std::fixed << std::setprecision(3) 
                      << (totalTime / totalSuccessful) << " ms\n";
        }
    }
};

// Benchmark functions
void memoryAllocationBenchmark() {
    std::vector<void*> allocations;
    const size_t numAllocations = 1000;
    const size_t allocationSize = 1024;
    
    // Allocate
    for (size_t i = 0; i < numAllocations; ++i) {
        void* ptr = malloc(allocationSize);
        if (ptr) {
            allocations.push_back(ptr);
        }
    }
    
    // Deallocate
    for (void* ptr : allocations) {
        if (ptr) {
            free(ptr);
        }
    }
}

void stringOperationsBenchmark() {
    const int numStrings = 500;
    std::vector<std::string> strings;
    
    // String creation
    for (int i = 0; i < numStrings; ++i) {
        strings.emplace_back("Demo string " + std::to_string(i) + " with content");
    }
    
    // String manipulation
    for (auto& str : strings) {
        str += " modified";
        str = str.substr(0, str.length() / 2);
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    }
    
    // String searching
    int found = 0;
    for (const auto& str : strings) {
        if (str.find("DEMO") != std::string::npos) {
            found++;
        }
    }
}

void vectorOperationsBenchmark() {
    std::vector<int> vec;
    const int numElements = 5000;
    
    // Push back
    for (int i = 0; i < numElements; ++i) {
        vec.push_back(i);
    }
    
    // Random access
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, numElements - 1);
    
    int sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += vec[dis(gen)];
    }
    
    // Erase elements
    while (!vec.empty() && vec.size() > numElements / 2) {
        vec.erase(vec.begin() + vec.size() / 2);
    }
}

void threadingBenchmark() {
    const int numThreads = 4;
    const int operationsPerThread = 500;
    std::mutex testMutex;
    int sharedCounter = 0;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operationsPerThread; ++j) {
                std::lock_guard<std::mutex> lock(testMutex);
                sharedCounter++;
            }
        });
    }
    
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void fileIoBenchmark() {
    const std::string filename = "/tmp/benchmark_demo_file.txt";
    const std::string content = "Demo content for file I/O benchmark. ";
    const int numLines = 500;
    
    // Write file
    {
        std::ofstream file(filename);
        for (int i = 0; i < numLines; ++i) {
            file << content << "Line " << i << std::endl;
        }
    }
    
    // Read file
    {
        std::ifstream file(filename);
        std::string line;
        int lineCount = 0;
        while (std::getline(file, line)) {
            lineCount++;
        }
    }
    
    // Clean up
    std::remove(filename.c_str());
}

int main(int argc, char* argv[]) {
    // Initialize logging
    LogManager::configureConsoleLogging(LogLevel::INFO, false);
    
    SimpleBenchmarkSuite suite;
    
    // Parse command line arguments
    bool verbose = false;
    std::string outputFile;
    std::string category;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--category" && i + 1 < argc) {
            category = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --verbose, -v        Enable verbose output\n";
            std::cout << "  --output <file>      Generate JSON report\n";
            std::cout << "  --category <name>    Run specific category\n";
            std::cout << "  --help, -h           Show this help\n";
            return 0;
        }
    }
    
    suite.setVerbose(verbose);
    
    // Register benchmarks
    suite.registerBenchmark("memory_allocation", "CORE", "Basic memory allocation and deallocation", 50, memoryAllocationBenchmark);
    suite.registerBenchmark("string_operations", "CORE", "String creation, manipulation, and searching", 30, stringOperationsBenchmark);
    suite.registerBenchmark("vector_operations", "CORE", "Vector operations performance", 20, vectorOperationsBenchmark);
    suite.registerBenchmark("threading", "CORE", "Thread creation and synchronization", 10, threadingBenchmark);
    suite.registerBenchmark("file_io", "CORE", "File I/O operations", 5, fileIoBenchmark);
    
    std::cout << "Bolt Performance Benchmark Suite\n";
    std::cout << "================================\n";
    
    // Run benchmarks
    std::vector<BenchmarkResult> results;
    if (!category.empty()) {
        std::cout << "Running benchmarks in category: " << category << "\n";
        results = suite.runCategory(category);
    } else {
        results = suite.runAllBenchmarks();
    }
    
    // Print summary
    suite.printSummary(results);
    
    // Generate report if requested
    if (!outputFile.empty()) {
        suite.generateReport(results, outputFile);
    }
    
    std::cout << "\nBenchmark execution completed.\n";
    return 0;
}