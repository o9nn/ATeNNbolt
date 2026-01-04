#include "include/bolt/core/benchmark_suite.hpp"
#include "include/bolt/core/performance_profiler.hpp"
#include "include/bolt/core/logging.hpp"
#include "include/bolt/test_framework.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace bolt;

// Test benchmark functions
BOLT_BENCHMARK_CONFIG(test_simple_benchmark, "TEST", 
    "Simple test benchmark for validation", 5) {
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

BOLT_BENCHMARK_CONFIG(test_failing_benchmark, "TEST",
    "Benchmark that should fail to test error handling", 3) {
    
    // Throw an exception to test error handling
    throw std::runtime_error("Test benchmark failure");
}

BOLT_BENCHMARK_CONFIG(test_variable_duration, "TEST",
    "Benchmark with variable duration for statistics testing", 10) {
    
    // Variable duration based on static counter
    static int counter = 0;
    int duration = 5 + (counter++ % 10); // 5-15ms range
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

// Test framework integration
BOLT_TEST(BenchmarkSuite, BasicFunctionality) {
    auto& suite = BenchmarkSuite::getInstance();
    
    // Test that we have some benchmarks registered
    auto benchmarks = suite.getAvailableBenchmarks();
    BOLT_ASSERT_TRUE(benchmarks.size() > 0);
    
    // Test that we have test category
    auto categories = suite.getAvailableCategories();
    bool hasTestCategory = std::find(categories.begin(), categories.end(), "TEST") != categories.end();
    BOLT_ASSERT_TRUE(hasTestCategory);
}

BOLT_TEST(BenchmarkSuite, RunSingleBenchmark) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Run a simple test benchmark
    auto result = suite.runBenchmark("test_simple_benchmark");
    
    BOLT_ASSERT_TRUE(result.isValid());
    BOLT_ASSERT_EQ(result.name, "test_simple_benchmark");
    BOLT_ASSERT_EQ(result.category, "TEST");
    BOLT_ASSERT_TRUE(result.averageDurationMs >= 10.0);  // Should take at least 10ms
    BOLT_ASSERT_TRUE(result.averageDurationMs < 100.0); // But not too long
    BOLT_ASSERT_TRUE(result.successfulRuns > 0);
    BOLT_ASSERT_EQ(result.failedRuns, 0);
}

BOLT_TEST(BenchmarkSuite, FailingBenchmark) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Run a benchmark that should fail
    auto result = suite.runBenchmark("test_failing_benchmark");
    
    BOLT_ASSERT_FALSE(result.isValid());
    BOLT_ASSERT_EQ(result.name, "test_failing_benchmark");
    BOLT_ASSERT_TRUE(result.failedRuns > 0);
    BOLT_ASSERT_FALSE(result.errorMessage.empty());
}

BOLT_TEST(BenchmarkSuite, RunByCategory) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Run all TEST category benchmarks
    auto results = suite.runBenchmarksByCategory("TEST");
    
    BOLT_ASSERT_TRUE(results.size() >= 3); // We have at least 3 test benchmarks
    
    // Check that all results are from TEST category
    for (const auto& result : results) {
        BOLT_ASSERT_EQ(result.category, "TEST");
    }
}

BOLT_TEST(BenchmarkSuite, StatisticsCalculation) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Run benchmark with variable duration
    auto result = suite.runBenchmark("test_variable_duration");
    
    BOLT_ASSERT_TRUE(result.isValid());
    BOLT_ASSERT_TRUE(result.standardDeviationMs > 0.0); // Should have some variation
    BOLT_ASSERT_TRUE(result.minDurationMs < result.maxDurationMs);
    BOLT_ASSERT_TRUE(result.averageDurationMs >= result.minDurationMs);
    BOLT_ASSERT_TRUE(result.averageDurationMs <= result.maxDurationMs);
}

BOLT_TEST(BenchmarkSuite, JsonReportGeneration) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Run a simple benchmark
    auto result = suite.runBenchmark("test_simple_benchmark");
    std::vector<BenchmarkResult> results = {result};
    
    // Generate JSON report
    std::string filename = "/tmp/test_benchmark_report.json";
    suite.generateJsonReport(results, filename);
    
    // Verify file was created and has content
    std::ifstream file(filename);
    BOLT_ASSERT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    BOLT_ASSERT_TRUE(content.find("test_simple_benchmark") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("benchmark_suite_version") != std::string::npos);
    
    file.close();
    std::remove(filename.c_str());
}

BOLT_TEST(BenchmarkSuite, CsvReportGeneration) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Run a simple benchmark
    auto result = suite.runBenchmark("test_simple_benchmark");
    std::vector<BenchmarkResult> results = {result};
    
    // Generate CSV report
    std::string filename = "/tmp/test_benchmark_report.csv";
    suite.generateCsvReport(results, filename);
    
    // Verify file was created and has content
    std::ifstream file(filename);
    BOLT_ASSERT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    BOLT_ASSERT_TRUE(content.find("test_simple_benchmark") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("Name,Category") != std::string::npos);
    
    file.close();
    std::remove(filename.c_str());
}

BOLT_TEST(BenchmarkSuite, HtmlReportGeneration) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Run a simple benchmark
    auto result = suite.runBenchmark("test_simple_benchmark");
    std::vector<BenchmarkResult> results = {result};
    
    // Generate HTML report
    std::string filename = "/tmp/test_benchmark_report.html";
    suite.generateHtmlReport(results, filename);
    
    // Verify file was created and has content
    std::ifstream file(filename);
    BOLT_ASSERT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    BOLT_ASSERT_TRUE(content.find("test_simple_benchmark") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("<!DOCTYPE html>") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("Bolt Performance Benchmark Report") != std::string::npos);
    
    file.close();
    std::remove(filename.c_str());
}

BOLT_TEST(BenchmarkSuite, BaselineComparison) {
    auto& suite = BenchmarkSuite::getInstance();
    suite.enableVerboseOutput(false);
    
    // Create baseline results
    BenchmarkResult baseline;
    baseline.name = "test_benchmark";
    baseline.category = "TEST";
    baseline.averageDurationMs = 100.0;
    baseline.successfulRuns = 10;
    baseline.totalIterations = 10;
    
    // Create current results (faster)
    BenchmarkResult current;
    current.name = "test_benchmark";
    current.category = "TEST";
    current.averageDurationMs = 80.0; // 20% improvement
    current.successfulRuns = 10;
    current.totalIterations = 10;
    
    std::vector<BenchmarkResult> baselineVec = {baseline};
    std::vector<BenchmarkResult> currentVec = {current};
    
    auto comparisons = suite.compareWithBaseline(currentVec, baselineVec);
    
    BOLT_ASSERT_EQ(comparisons.size(), 1);
    BOLT_ASSERT_EQ(comparisons[0].benchmarkName, "test_benchmark");
    BOLT_ASSERT_TRUE(comparisons[0].durationChangePercent < 0); // Negative = improvement
    BOLT_ASSERT_TRUE(comparisons[0].hasImprovement());
}

int main() {
    std::cout << "Benchmark Suite Test" << std::endl;
    std::cout << "===================" << std::endl;
    
    try {
        // Initialize logging for test output
        LogManager::configureConsoleLogging(LogLevel::WARN, false);
        
        std::cout << "Testing basic functionality..." << std::endl;
        runTest("BasicFunctionality", test_BenchmarkSuite_BasicFunctionality);
        runTest("RunSingleBenchmark", test_BenchmarkSuite_RunSingleBenchmark);
        runTest("FailingBenchmark", test_BenchmarkSuite_FailingBenchmark);
        runTest("RunByCategory", test_BenchmarkSuite_RunByCategory);
        
        std::cout << "\nTesting statistics..." << std::endl;
        runTest("StatisticsCalculation", test_BenchmarkSuite_StatisticsCalculation);
        
        std::cout << "\nTesting report generation..." << std::endl;
        runTest("JsonReportGeneration", test_BenchmarkSuite_JsonReportGeneration);
        runTest("CsvReportGeneration", test_BenchmarkSuite_CsvReportGeneration);
        runTest("HtmlReportGeneration", test_BenchmarkSuite_HtmlReportGeneration);
        
        std::cout << "\nTesting comparison features..." << std::endl;
        runTest("BaselineComparison", test_BenchmarkSuite_BaselineComparison);
        
        std::cout << "\n=== All Tests Passed ===\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}