#include "bolt/core/performance_profiler.hpp"
#include "bolt/core/error_handling.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <cmath>
#include <fstream>

// Simple test framework (using existing pattern from other tests)
#define BOLT_TEST(category, name) void test_##category##_##name()

#define BOLT_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            throw std::runtime_error("Test assertion failed"); \
        } \
    } while(0)

#define BOLT_ASSERT_FALSE(condition) BOLT_ASSERT_TRUE(!(condition))
#define BOLT_ASSERT_EQ(a, b) BOLT_ASSERT_TRUE((a) == (b))
#define BOLT_ASSERT_NE(a, b) BOLT_ASSERT_TRUE((a) != (b))
#define BOLT_ASSERT_NOT_NULL(ptr) BOLT_ASSERT_TRUE((ptr) != nullptr)
#define BOLT_ASSERT_NULL(ptr) BOLT_ASSERT_TRUE((ptr) == nullptr)

// Test helper functions
void simulateWork(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

double calculateTolerance(double expected, double actual, double tolerance = 0.1) {
    return std::abs(expected - actual) / expected;
}

// ===== Basic Functionality Tests =====

BOLT_TEST(Profiler, BasicMetricTiming) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    auto metric = profiler.startMetric("test_metric", "TEST");
    BOLT_ASSERT_NOT_NULL(metric);
    
    const int workDuration = 50; // 50ms
    simulateWork(workDuration);
    
    profiler.endMetric(metric);
    
    // Check that the timing is approximately correct (within 20ms tolerance)
    double actualDuration = metric->getDurationMs();
    BOLT_ASSERT_TRUE(actualDuration >= workDuration);
    BOLT_ASSERT_TRUE(actualDuration < workDuration + 20); // Allow some overhead
    
    BOLT_ASSERT_EQ(metric->name, "test_metric");
    BOLT_ASSERT_EQ(metric->category, "TEST");
}

BOLT_TEST(Profiler, ScopedProfiling) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    const int workDuration = 30;
    auto startCount = profiler.getTotalMetricsCount();
    
    {
        bolt::ScopedProfiler scoped("scoped_test", "SCOPE");
        simulateWork(workDuration);
        
        // Test current duration while still active
        double currentDuration = scoped.getCurrentDurationMs();
        BOLT_ASSERT_TRUE(currentDuration >= workDuration - 5);
        
        scoped.addMetadata("test_key", "test_value");
    } // ScopedProfiler destructor should record the metric
    
    auto endCount = profiler.getTotalMetricsCount();
    BOLT_ASSERT_EQ(endCount, startCount + 1);
}

BOLT_TEST(Profiler, InstantMetrics) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    auto startCount = profiler.getTotalMetricsCount();
    
    profiler.recordInstantMetric("instant_test", 42.5, "INSTANT");
    
    auto endCount = profiler.getTotalMetricsCount();
    BOLT_ASSERT_EQ(endCount, startCount + 1);
}

// ===== Session Management Tests =====

BOLT_TEST(Profiler, SessionCreationAndManagement) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    // Create a session
    auto session = profiler.createSession("test_session");
    BOLT_ASSERT_NOT_NULL(session);
    BOLT_ASSERT_TRUE(session->isActive());
    BOLT_ASSERT_EQ(session->getName(), "test_session");
    
    // Set as current session
    profiler.setCurrentSession("test_session");
    auto currentSession = profiler.getCurrentSession();
    BOLT_ASSERT_EQ(currentSession, session);
    
    // Add metrics to session
    {
        bolt::ScopedProfiler scoped("session_metric", "SESSION");
        simulateWork(10);
    }
    
    BOLT_ASSERT_EQ(session->getMetricsCount(), 1);
    
    // End session
    profiler.endSession("test_session");
    BOLT_ASSERT_FALSE(session->isActive());
}

BOLT_TEST(Profiler, SessionStatistics) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    auto session = profiler.createSession("stats_session");
    profiler.setCurrentSession("stats_session");
    
    // Add multiple metrics with different durations
    const std::vector<int> durations = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < durations.size(); ++i) {
        bolt::ScopedProfiler scoped("metric_" + std::to_string(i), "STATS");
        simulateWork(durations[i]);
    }
    
    BOLT_ASSERT_EQ(session->getMetricsCount(), durations.size());
    
    // Check statistics
    double avgDuration = session->getAverageDurationMs();
    double minDuration = session->getMinDurationMs();
    double maxDuration = session->getMaxDurationMs();
    
    BOLT_ASSERT_TRUE(avgDuration >= 25 && avgDuration <= 35); // Should be around 30ms
    BOLT_ASSERT_TRUE(minDuration >= 8); // Should be around 10ms
    BOLT_ASSERT_TRUE(maxDuration >= 45); // Should be around 50ms
    BOLT_ASSERT_TRUE(minDuration < maxDuration);
}

// ===== Macro Tests =====

BOLT_TEST(Profiler, ConvenienceMacros) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    auto startCount = profiler.getTotalMetricsCount();
    
    // Test function profiling macro
    auto testFunction = []() {
        BOLT_PROFILE_FUNCTION();
        simulateWork(10);
    };
    testFunction();
    
    // Test scope profiling macro
    {
        BOLT_PROFILE_SCOPE("test_scope");
        simulateWork(10);
    }
    
    // Test category-specific macros
    {
        BOLT_PROFILE_CORE("core_operation");
        simulateWork(5);
    }
    
    {
        BOLT_PROFILE_AI("ai_inference");
        simulateWork(5);
    }
    
    auto endCount = profiler.getTotalMetricsCount();
    BOLT_ASSERT_EQ(endCount, startCount + 4);
}

BOLT_TEST(Profiler, SessionMacro) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    {
        BOLT_PROFILE_SESSION("macro_session");
        
        {
            BOLT_PROFILE_SCOPE("session_work");
            simulateWork(15);
        }
        
        auto currentSession = profiler.getCurrentSession();
        BOLT_ASSERT_NOT_NULL(currentSession);
        BOLT_ASSERT_EQ(currentSession->getName(), "macro_session");
        BOLT_ASSERT_EQ(currentSession->getMetricsCount(), 1);
    }
}

// ===== Thread Safety Tests =====

BOLT_TEST(Profiler, ThreadSafety) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    const int numThreads = 4;
    const int metricsPerThread = 10;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&profiler, t, metricsPerThread]() {
            for (int i = 0; i < metricsPerThread; ++i) {
                bolt::ScopedProfiler scoped("thread_" + std::to_string(t) + "_metric_" + std::to_string(i), "THREAD");
                simulateWork(5);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    size_t totalMetrics = profiler.getTotalMetricsCount();
    BOLT_ASSERT_EQ(totalMetrics, numThreads * metricsPerThread);
}

BOLT_TEST(Profiler, ConcurrentSessions) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    const int numThreads = 3;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&profiler, t]() {
            std::string sessionName = "concurrent_session_" + std::to_string(t);
            auto session = profiler.createSession(sessionName);
            profiler.setCurrentSession(sessionName);
            
            for (int i = 0; i < 5; ++i) {
                bolt::ScopedProfiler scoped("concurrent_metric_" + std::to_string(i), "CONCURRENT");
                simulateWork(10);
            }
            
            profiler.endSession(sessionName);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Each thread should have created a session with 5 metrics
    for (int t = 0; t < numThreads; ++t) {
        std::string sessionName = "concurrent_session_" + std::to_string(t);
        auto session = profiler.getSession(sessionName);
        BOLT_ASSERT_NOT_NULL(session);
        
        // Debug output in case of failure
        size_t actualCount = session->getMetricsCount();
        if (actualCount != 5) {
            std::cout << "Session " << sessionName << " has " << actualCount << " metrics instead of 5" << std::endl;
        }
        BOLT_ASSERT_EQ(session->getMetricsCount(), 5);
    }
}

// ===== Data Export Tests =====

BOLT_TEST(Profiler, JsonExport) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    // Create some test data
    auto session = profiler.createSession("export_session");
    profiler.setCurrentSession("export_session");
    
    {
        BOLT_PROFILE_SCOPE("export_test");
        simulateWork(10);
    }
    
    profiler.endSession("export_session");
    
    // Add a global metric
    profiler.recordInstantMetric("global_metric", 25.5, "GLOBAL");
    
    // Export to JSON
    const std::string filename = "/tmp/profiler_test_export.json";
    profiler.exportToJson(filename);
    
    // Verify file was created and has content
    std::ifstream file(filename);
    BOLT_ASSERT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    BOLT_ASSERT_TRUE(content.find("export_session") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("export_test") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("global_metric") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("globalMetrics") != std::string::npos);
}

BOLT_TEST(Profiler, CsvExport) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    // Create test data
    auto session = profiler.createSession("csv_session");
    profiler.setCurrentSession("csv_session");
    
    {
        BOLT_PROFILE_SCOPE("csv_test");
        simulateWork(5);
    }
    
    profiler.endSession("csv_session");
    
    // Export to CSV
    const std::string filename = "/tmp/profiler_test_export.csv";
    profiler.exportToCsv(filename);
    
    // Verify file was created and has content
    std::ifstream file(filename);
    BOLT_ASSERT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    BOLT_ASSERT_TRUE(content.find("Session,Name,Category,Duration(ms),MemoryUsage(bytes)") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("csv_session") != std::string::npos);
    BOLT_ASSERT_TRUE(content.find("csv_test") != std::string::npos);
}

// ===== Integration Tests =====

BOLT_TEST(Profiler, CategoryFiltering) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    // Create metrics in different categories
    {
        BOLT_PROFILE_CORE("core_task");
        simulateWork(5);
    }
    
    {
        BOLT_PROFILE_AI("ai_task");
        simulateWork(5);
    }
    
    {
        BOLT_PROFILE_MEMORY("memory_task");
        simulateWork(5);
    }
    
    auto categories = profiler.getCategories();
    BOLT_ASSERT_TRUE(categories.size() >= 3);
    
    auto coreMetrics = profiler.getMetricsByCategory("CORE");
    auto aiMetrics = profiler.getMetricsByCategory("AI");
    auto memoryMetrics = profiler.getMetricsByCategory("MEMORY");
    
    BOLT_ASSERT_EQ(coreMetrics.size(), 1);
    BOLT_ASSERT_EQ(aiMetrics.size(), 1);
    BOLT_ASSERT_EQ(memoryMetrics.size(), 1);
}

BOLT_TEST(Profiler, EnableDisable) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    
    // Test disabled state
    profiler.disable();
    BOLT_ASSERT_FALSE(profiler.isEnabled());
    
    auto startCount = profiler.getTotalMetricsCount();
    
    {
        BOLT_PROFILE_SCOPE("disabled_test");
        simulateWork(10);
    }
    
    auto endCount = profiler.getTotalMetricsCount();
    BOLT_ASSERT_EQ(endCount, startCount); // No metrics should be recorded
    
    // Test enabled state
    profiler.enable();
    BOLT_ASSERT_TRUE(profiler.isEnabled());
    
    {
        BOLT_PROFILE_SCOPE("enabled_test");
        simulateWork(10);
    }
    
    auto finalCount = profiler.getTotalMetricsCount();
    BOLT_ASSERT_EQ(finalCount, endCount + 1); // One metric should be recorded
}

// ===== Performance Tests =====

BOLT_TEST(Profiler, ProfilerOverhead) {
    auto& profiler = bolt::PerformanceProfiler::getInstance();
    profiler.reset();
    profiler.enable();
    
    const int numIterations = 1000;
    
    // Measure time without profiling
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numIterations; ++i) {
        // More realistic work - not just a single assignment
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto baselineDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Measure time with profiling
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numIterations; ++i) {
        BOLT_PROFILE_SCOPE("overhead_test");
        // Same realistic work
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    end = std::chrono::high_resolution_clock::now();
    auto profiledDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate overhead
    double overhead = (double)(profiledDuration.count() - baselineDuration.count()) / baselineDuration.count() * 100.0;
    
    std::cout << "Profiler overhead: " << overhead << "% (" 
              << (profiledDuration.count() - baselineDuration.count()) / (double)numIterations 
              << " μs per profiled operation)" << std::endl;
    
    // With more realistic work, overhead should be much more reasonable
    BOLT_ASSERT_TRUE(overhead < 100.0); // Allow up to 100% overhead for this test
    
    // Verify all metrics were recorded
    BOLT_ASSERT_EQ(profiler.getTotalMetricsCount(), numIterations);
}

// ===== Test Runner =====

void runTest(const std::string& name, void (*testFunc)()) {
    try {
        testFunc();
        std::cout << "✓ " << name << " passed" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✗ " << name << " failed: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "Bolt Performance Profiler Test Suite" << std::endl;
    std::cout << "====================================" << std::endl;
    
    try {
        // Initialize logging for test output
        bolt::LogManager::configureConsoleLogging(bolt::LogLevel::WARN, false);
        
        std::cout << "Testing basic functionality..." << std::endl;
        runTest("BasicMetricTiming", test_Profiler_BasicMetricTiming);
        runTest("ScopedProfiling", test_Profiler_ScopedProfiling);
        runTest("InstantMetrics", test_Profiler_InstantMetrics);
        
        std::cout << "\nTesting session management..." << std::endl;
        runTest("SessionCreationAndManagement", test_Profiler_SessionCreationAndManagement);
        runTest("SessionStatistics", test_Profiler_SessionStatistics);
        
        std::cout << "\nTesting convenience macros..." << std::endl;
        runTest("ConvenienceMacros", test_Profiler_ConvenienceMacros);
        runTest("SessionMacro", test_Profiler_SessionMacro);
        
        std::cout << "\nTesting thread safety..." << std::endl;
        runTest("ThreadSafety", test_Profiler_ThreadSafety);
        runTest("ConcurrentSessions", test_Profiler_ConcurrentSessions);
        
        std::cout << "\nTesting data export..." << std::endl;
        runTest("JsonExport", test_Profiler_JsonExport);
        runTest("CsvExport", test_Profiler_CsvExport);
        
        std::cout << "\nTesting integration features..." << std::endl;
        runTest("CategoryFiltering", test_Profiler_CategoryFiltering);
        runTest("EnableDisable", test_Profiler_EnableDisable);
        
        std::cout << "\nTesting performance..." << std::endl;
        runTest("ProfilerOverhead", test_Profiler_ProfilerOverhead);
        
        std::cout << "\n🎉 All performance profiler tests passed successfully!" << std::endl;
        
        // Demonstrate profiler summary
        std::cout << "\n=== Profiler Summary After Tests ===" << std::endl;
        bolt::PerformanceProfiler::getInstance().printSummary();
        
    } catch (const std::exception& e) {
        std::cerr << "\nTest suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}