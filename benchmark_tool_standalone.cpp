#include "include/bolt/core/benchmark_suite.hpp"
#include "include/bolt/core/performance_profiler.hpp"
#include "include/bolt/core/memory_manager.hpp"
#include "include/bolt/core/memory_pool.hpp"
#include "include/bolt/core/logging.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <thread>
#include <random>
#include <unordered_map>
#include <fstream>

using namespace bolt;

// Simple test benchmarks for standalone demo
void benchmark_memory_allocation_demo(const BenchmarkConfig& config) {
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

void benchmark_string_operations_demo(const BenchmarkConfig& config) {
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

void benchmark_vector_operations_demo(const BenchmarkConfig& config) {
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

void benchmark_threading_demo(const BenchmarkConfig& config) {
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

void benchmark_file_io_demo(const BenchmarkConfig& config) {
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

void registerDemoBenchmarks() {
    auto& suite = BenchmarkSuite::getInstance();
    
    BenchmarkConfig config1("memory_allocation_demo", "Basic memory allocation and deallocation performance");
    config1.category = "DEMO";
    config1.iterations = 50;
    suite.registerBenchmark(config1, benchmark_memory_allocation_demo);
    
    BenchmarkConfig config2("string_operations_demo", "String creation, manipulation, and destruction");
    config2.category = "DEMO";
    config2.iterations = 30;
    suite.registerBenchmark(config2, benchmark_string_operations_demo);
    
    BenchmarkConfig config3("vector_operations_demo", "Vector operations performance");
    config3.category = "DEMO";
    config3.iterations = 20;
    suite.registerBenchmark(config3, benchmark_vector_operations_demo);
    
    BenchmarkConfig config4("threading_demo", "Thread creation and synchronization");
    config4.category = "DEMO";
    config4.iterations = 10;
    suite.registerBenchmark(config4, benchmark_threading_demo);
    
    BenchmarkConfig config5("file_io_demo", "File I/O operations");
    config5.category = "DEMO";
    config5.iterations = 5;
    suite.registerBenchmark(config5, benchmark_file_io_demo);
}

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
    std::cout << "  --verbose, -v             Enable verbose output\n";
    std::cout << "  --output-json <file>      Generate JSON report\n";
    std::cout << "  --output-csv <file>       Generate CSV report\n";
    std::cout << "  --output-html <file>      Generate HTML report\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " --run-all --output-html report.html\n";
    std::cout << "  " << programName << " --category DEMO --verbose\n";
    std::cout << "\n";
}

void printBenchmarkSummary(const std::vector<BenchmarkResult>& results) {
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
    int iterations = 10;
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
        } else if (arg == "--output-json" && i + 1 < argc) {
            jsonOutput = argv[++i];
        } else if (arg == "--output-csv" && i + 1 < argc) {
            csvOutput = argv[++i];
        } else if (arg == "--output-html" && i + 1 < argc) {
            htmlOutput = argv[++i];
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
    suite.enableVerboseOutput(verbose);
    
    // Register demo benchmarks
    registerDemoBenchmarks();
    
    std::cout << "Bolt Performance Benchmark Suite (Standalone)\n";
    std::cout << "=============================================\n\n";
    
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
            
            // Generate reports
            if (!jsonOutput.empty()) {
                suite.generateJsonReport(results, jsonOutput);
                std::cout << "JSON report saved to: " << jsonOutput << "\n";
            }
            if (!csvOutput.empty()) {
                suite.generateCsvReport(results, csvOutput);
                std::cout << "CSV report saved to: " << csvOutput << "\n";
            }
            if (!htmlOutput.empty()) {
                suite.generateHtmlReport(results, htmlOutput);
                std::cout << "HTML report saved to: " << htmlOutput << "\n";
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