#include "include/bolt/core/benchmark_suite.hpp"
#include "include/bolt/core/logging.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>

using namespace bolt;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h                Show this help message\n";
    std::cout << "  --list, -l                List available benchmarks\n";
    std::cout << "  --categories, -c          List available categories\n";
    std::cout << "  --run-all, -a             Run all benchmarks\n";
    std::cout << "  --category <name>         Run benchmarks in specific category\n";
    std::cout << "  --benchmark <name>        Run specific benchmark\n";
    std::cout << "  --iterations <n>          Set number of iterations (default: 10)\n";
    std::cout << "  --warmup <n>              Set number of warmup runs (default: 3)\n";
    std::cout << "  --timeout <ms>            Set timeout in milliseconds (default: 30000)\n";
    std::cout << "  --output-json <file>      Generate JSON report\n";
    std::cout << "  --output-csv <file>       Generate CSV report\n";
    std::cout << "  --output-html <file>      Generate HTML report\n";
    std::cout << "  --baseline <file>         Load baseline results for comparison\n";
    std::cout << "  --save-baseline <file>    Save results as baseline\n";
    std::cout << "  --verbose, -v             Enable verbose output\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " --run-all --output-html benchmark_report.html\n";
    std::cout << "  " << programName << " --category CORE --verbose\n";
    std::cout << "  " << programName << " --benchmark memory_allocation_basic --iterations 100\n";
    std::cout << "\n";
}

void printBenchmarkSummary(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n=== Benchmark Summary ===\n";
    std::cout << std::left << std::setw(30) << "Benchmark" 
              << std::setw(12) << "Category"
              << std::setw(15) << "Avg Time (ms)"
              << std::setw(15) << "Min Time (ms)"
              << std::setw(15) << "Max Time (ms)"
              << std::setw(12) << "Success Rate"
              << "Status\n";
    std::cout << std::string(110, '-') << "\n";
    
    for (const auto& result : results) {
        std::string status = result.isValid() ? "SUCCESS" : "FAILED";
        std::cout << std::left << std::setw(30) << result.name.substr(0, 29)
                  << std::setw(12) << result.category
                  << std::setw(15) << std::fixed << std::setprecision(3) << result.averageDurationMs
                  << std::setw(15) << std::fixed << std::setprecision(3) << result.minDurationMs
                  << std::setw(15) << std::fixed << std::setprecision(3) << result.maxDurationMs
                  << std::setw(12) << std::fixed << std::setprecision(1) << (result.getSuccessRate() * 100) << "%"
                  << status << "\n";
        
        if (!result.errorMessage.empty()) {
            std::cout << "    Error: " << result.errorMessage << "\n";
        }
    }
    
    // Calculate summary statistics
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

void printComparisonSummary(const std::vector<BenchmarkComparison>& comparisons) {
    std::cout << "\n=== Baseline Comparison ===\n";
    std::cout << std::left << std::setw(30) << "Benchmark"
              << std::setw(15) << "Current (ms)"
              << std::setw(15) << "Baseline (ms)"
              << std::setw(12) << "Change (%)"
              << "Status\n";
    std::cout << std::string(80, '-') << "\n";
    
    int improvements = 0, regressions = 0, stable = 0;
    
    for (const auto& comp : comparisons) {
        std::string statusStr;
        switch (comp.performanceStatus) {
            case BenchmarkComparison::Status::IMPROVED:
                statusStr = "IMPROVED";
                improvements++;
                break;
            case BenchmarkComparison::Status::DEGRADED:
                statusStr = "DEGRADED";
                regressions++;
                break;
            case BenchmarkComparison::Status::STABLE:
                statusStr = "STABLE";
                stable++;
                break;
            default:
                statusStr = "INCONCLUSIVE";
                break;
        }
        
        std::cout << std::left << std::setw(30) << comp.benchmarkName.substr(0, 29)
                  << std::setw(15) << std::fixed << std::setprecision(3) << comp.current.averageDurationMs
                  << std::setw(15) << std::fixed << std::setprecision(3) << comp.baseline.averageDurationMs
                  << std::setw(12) << std::fixed << std::setprecision(1) << comp.durationChangePercent
                  << statusStr << "\n";
    }
    
    std::cout << "\nComparison Summary:\n";
    std::cout << "Improvements: " << improvements << "\n";
    std::cout << "Regressions: " << regressions << "\n";
    std::cout << "Stable: " << stable << "\n";
    
    if (regressions > 0) {
        std::cout << "\n⚠️  Performance regressions detected!\n";
    } else if (improvements > 0) {
        std::cout << "\n✅ Performance improvements found!\n";
    }
}

int main(int argc, char* argv[]) {
    // Initialize logging
    LogManager::configureConsoleLogging(LogLevel::INFO, false);
    
    auto& suite = BenchmarkSuite::getInstance();
    
    // Command line argument parsing
    std::string command;
    std::string specificBenchmark;
    std::string specificCategory;
    std::string jsonOutput;
    std::string csvOutput;
    std::string htmlOutput;
    std::string baselineFile;
    std::string saveBaselineFile;
    int iterations = 10;
    int warmup = 3;
    int timeout = 30000;
    bool verbose = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--list" || arg == "-l") {
            command = "list";
        } else if (arg == "--categories" || arg == "-c") {
            command = "categories";
        } else if (arg == "--run-all" || arg == "-a") {
            command = "run-all";
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--category" && i + 1 < argc) {
            command = "category";
            specificCategory = argv[++i];
        } else if (arg == "--benchmark" && i + 1 < argc) {
            command = "benchmark";
            specificBenchmark = argv[++i];
        } else if (arg == "--iterations" && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if (arg == "--warmup" && i + 1 < argc) {
            warmup = std::stoi(argv[++i]);
        } else if (arg == "--timeout" && i + 1 < argc) {
            timeout = std::stoi(argv[++i]);
        } else if (arg == "--output-json" && i + 1 < argc) {
            jsonOutput = argv[++i];
        } else if (arg == "--output-csv" && i + 1 < argc) {
            csvOutput = argv[++i];
        } else if (arg == "--output-html" && i + 1 < argc) {
            htmlOutput = argv[++i];
        } else if (arg == "--baseline" && i + 1 < argc) {
            baselineFile = argv[++i];
        } else if (arg == "--save-baseline" && i + 1 < argc) {
            saveBaselineFile = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (command.empty()) {
        command = "run-all";  // Default action
    }
    
    // Configure benchmark suite
    suite.setDefaultIterations(iterations);
    suite.setDefaultWarmupRuns(warmup);
    suite.setDefaultTimeout(std::chrono::milliseconds(timeout));
    suite.enableVerboseOutput(verbose);
    
    // Load core benchmarks by including the source
    // This ensures the benchmarks are registered
    extern void register_core_benchmarks(); // Forward declaration
    
    std::cout << "Bolt Performance Benchmark Suite\n";
    std::cout << "================================\n\n";
    
    try {
        if (command == "list") {
            auto benchmarks = suite.getAvailableBenchmarks();
            std::cout << "Available Benchmarks:\n";
            for (const auto& name : benchmarks) {
                std::cout << "  " << name << "\n";
            }
            std::cout << "\nTotal: " << benchmarks.size() << " benchmarks\n";
            
        } else if (command == "categories") {
            auto categories = suite.getAvailableCategories();
            std::cout << "Available Categories:\n";
            for (const auto& category : categories) {
                std::cout << "  " << category << "\n";
            }
            std::cout << "\nTotal: " << categories.size() << " categories\n";
            
        } else if (command == "run-all") {
            std::cout << "Running all benchmarks...\n";
            auto results = suite.runAllBenchmarks();
            printBenchmarkSummary(results);
            
            // Handle baseline comparison
            if (!baselineFile.empty()) {
                auto baseline = suite.loadBaselineResults(baselineFile);
                if (!baseline.empty()) {
                    auto comparisons = suite.compareWithBaseline(results, baseline);
                    printComparisonSummary(comparisons);
                }
            }
            
            // Generate reports
            if (!jsonOutput.empty()) {
                suite.generateJsonReport(results, jsonOutput);
            }
            if (!csvOutput.empty()) {
                suite.generateCsvReport(results, csvOutput);
            }
            if (!htmlOutput.empty()) {
                suite.generateHtmlReport(results, htmlOutput);
            }
            if (!saveBaselineFile.empty()) {
                suite.saveBaselineResults(results, saveBaselineFile);
            }
            
        } else if (command == "category") {
            std::cout << "Running benchmarks in category: " << specificCategory << "\n";
            auto results = suite.runBenchmarksByCategory(specificCategory);
            if (results.empty()) {
                std::cout << "No benchmarks found in category: " << specificCategory << "\n";
                return 1;
            }
            printBenchmarkSummary(results);
            
        } else if (command == "benchmark") {
            std::cout << "Running benchmark: " << specificBenchmark << "\n";
            auto result = suite.runBenchmark(specificBenchmark);
            std::vector<BenchmarkResult> results = {result};
            printBenchmarkSummary(results);
            
            if (!result.isValid()) {
                return 1;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nBenchmark execution completed.\n";
    return 0;
}