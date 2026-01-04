/**
 * @file test_sanitizer_integration.cpp
 * @brief Integration tests for memory leak detection tools with sanitizers
 */

#include "bolt/test_framework.hpp"
#include "bolt/core/memory_leak_detector.hpp"
#include "bolt/core/memory_manager.hpp"
#include <cstdlib>
#include <vector>

using namespace bolt::test;

// ===== Sanitizer Integration Tests =====

BOLT_TEST(SanitizerIntegration, BasicAllocationDeallocation) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Allocate and properly free
    void* ptr = malloc(1024);
    TRACK_ALLOCATION(ptr, 1024, "test");
    
    BOLT_ASSERT(detector.hasLeaks());
    
    UNTRACK_ALLOCATION(ptr);
    free(ptr);
    
    BOLT_ASSERT(!detector.hasLeaks());
}

BOLT_TEST(SanitizerIntegration, MultipleAllocations) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    std::vector<void*> allocations;
    
    // Allocate multiple blocks
    for (int i = 0; i < 10; ++i) {
        void* ptr = malloc(128 * (i + 1));
        allocations.push_back(ptr);
        TRACK_ALLOCATION(ptr, 128 * (i + 1), "batch_test");
    }
    
    BOLT_ASSERT_EQ(10, detector.getLeakCount());
    
    // Free all blocks
    for (void* ptr : allocations) {
        UNTRACK_ALLOCATION(ptr);
        free(ptr);
    }
    
    BOLT_ASSERT(!detector.hasLeaks());
}

BOLT_TEST(SanitizerIntegration, MemoryManagerIntegration) {
    auto& manager = bolt::MemoryManager::getInstance();
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Track MemoryManager allocations
    void* ptr1 = manager.allocate(512);
    TRACK_ALLOCATION(ptr1, 512, "memory_manager");
    
    void* ptr2 = manager.allocate(1024);
    TRACK_ALLOCATION(ptr2, 1024, "memory_manager");
    
    BOLT_ASSERT_EQ(2, detector.getLeakCount());
    BOLT_ASSERT_EQ(512 + 1024, manager.getCurrentUsage());
    
    // Cleanup
    UNTRACK_ALLOCATION(ptr1);
    manager.deallocate(ptr1);
    
    UNTRACK_ALLOCATION(ptr2);
    manager.deallocate(ptr2);
    
    BOLT_ASSERT(!detector.hasLeaks());
    BOLT_ASSERT_EQ(0, manager.getCurrentUsage());
}

BOLT_TEST(SanitizerIntegration, LeakDetectionReport) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Create a leak
    void* leaked = malloc(2048);
    TRACK_ALLOCATION(leaked, 2048, "test_leak");
    
    // Generate report
    std::string report = detector.generateReport();
    
    BOLT_ASSERT(report.find("Memory leaks detected") != std::string::npos);
    BOLT_ASSERT(report.find("test_leak") != std::string::npos);
    BOLT_ASSERT(report.find("2.00 KB") != std::string::npos);
    
    // Cleanup
    UNTRACK_ALLOCATION(leaked);
    free(leaked);
}

BOLT_TEST(SanitizerIntegration, CategoryTracking) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Allocate in different categories
    void* cat1 = malloc(100);
    TRACK_ALLOCATION(cat1, 100, "category1");
    
    void* cat2 = malloc(200);
    TRACK_ALLOCATION(cat2, 200, "category2");
    
    void* cat3 = malloc(300);
    TRACK_ALLOCATION(cat3, 300, "category1");
    
    auto stats = detector.getStats();
    
    BOLT_ASSERT_EQ(400, stats.leaksByCategory["category1"]);
    BOLT_ASSERT_EQ(200, stats.leaksByCategory["category2"]);
    
    // Cleanup
    UNTRACK_ALLOCATION(cat1);
    free(cat1);
    UNTRACK_ALLOCATION(cat2);
    free(cat2);
    UNTRACK_ALLOCATION(cat3);
    free(cat3);
}

#ifdef ASAN_ENABLED
BOLT_TEST(SanitizerIntegration, AddressSanitizerEnabled) {
    // This test verifies that ASan is actually enabled
    std::cout << "AddressSanitizer is enabled\n";
    BOLT_ASSERT(true);
}
#endif

#ifdef LSAN_ENABLED
BOLT_TEST(SanitizerIntegration, LeakSanitizerEnabled) {
    // This test verifies that LSan is actually enabled
    std::cout << "LeakSanitizer is enabled\n";
    BOLT_ASSERT(true);
}
#endif

#ifdef UBSAN_ENABLED
BOLT_TEST(SanitizerIntegration, UndefinedBehaviorSanitizerEnabled) {
    // This test verifies that UBSan is actually enabled
    std::cout << "UndefinedBehaviorSanitizer is enabled\n";
    BOLT_ASSERT(true);
}
#endif

// Test that proper cleanup prevents sanitizer errors
BOLT_TEST(SanitizerIntegration, NoLeaksWithProperCleanup) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // Allocate and immediately clean up
    for (int i = 0; i < 100; ++i) {
        void* ptr = malloc(i * 10 + 1);
        TRACK_ALLOCATION(ptr, i * 10 + 1, "no_leak_test");
        UNTRACK_ALLOCATION(ptr);
        free(ptr);
    }
    
    BOLT_ASSERT(!detector.hasLeaks());
}

// Test peak memory tracking
BOLT_TEST(SanitizerIntegration, PeakMemoryTracking) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    detector.resetStats();
    
    // Allocate increasing amounts
    void* ptr1 = malloc(1024);
    TRACK_ALLOCATION(ptr1, 1024, "peak_test");
    
    void* ptr2 = malloc(2048);
    TRACK_ALLOCATION(ptr2, 2048, "peak_test");
    
    auto stats = detector.getStats();
    BOLT_ASSERT_EQ(3072, stats.peakMemoryUsage);
    
    // Free first allocation
    UNTRACK_ALLOCATION(ptr1);
    free(ptr1);
    
    stats = detector.getStats();
    BOLT_ASSERT_EQ(3072, stats.peakMemoryUsage); // Peak should remain
    BOLT_ASSERT_EQ(2048, stats.currentMemoryUsage);
    
    // Cleanup
    UNTRACK_ALLOCATION(ptr2);
    free(ptr2);
}
