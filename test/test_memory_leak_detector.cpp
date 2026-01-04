#include "bolt/test_framework.hpp"
#include "bolt/core/memory_leak_detector.hpp"
#include <cstdlib>

using namespace bolt::test;

// ===== Memory Leak Detector Tests =====

BOLT_TEST(MemoryLeakDetector, BasicTracking) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear(); // Start fresh
    
    // Allocate and track
    void* ptr1 = malloc(1024);
    TRACK_ALLOCATION(ptr1, 1024, "test");
    
    BOLT_ASSERT(detector.hasLeaks());
    BOLT_ASSERT_EQ(1, detector.getLeakCount());
    BOLT_ASSERT_EQ(1024, detector.getLeakedBytes());
    
    // Untrack and free
    UNTRACK_ALLOCATION(ptr1);
    free(ptr1);
    
    BOLT_ASSERT(!detector.hasLeaks());
    BOLT_ASSERT_EQ(0, detector.getLeakCount());
}

BOLT_TEST(MemoryLeakDetector, MultipleAllocations) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    void* ptr1 = malloc(512);
    void* ptr2 = malloc(1024);
    void* ptr3 = malloc(2048);
    
    TRACK_ALLOCATION(ptr1, 512, "buffers");
    TRACK_ALLOCATION(ptr2, 1024, "network");
    TRACK_ALLOCATION(ptr3, 2048, "buffers");
    
    BOLT_ASSERT_EQ(3, detector.getLeakCount());
    BOLT_ASSERT_EQ(3584, detector.getLeakedBytes());
    
    // Free one allocation
    UNTRACK_ALLOCATION(ptr2);
    free(ptr2);
    
    BOLT_ASSERT_EQ(2, detector.getLeakCount());
    BOLT_ASSERT_EQ(2560, detector.getLeakedBytes());
    
    // Free remaining
    UNTRACK_ALLOCATION(ptr1);
    UNTRACK_ALLOCATION(ptr3);
    free(ptr1);
    free(ptr3);
    
    BOLT_ASSERT(!detector.hasLeaks());
}

BOLT_TEST(MemoryLeakDetector, Categorization) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    void* ptr1 = malloc(1024);
    void* ptr2 = malloc(2048);
    void* ptr3 = malloc(512);
    
    TRACK_ALLOCATION(ptr1, 1024, "network");
    TRACK_ALLOCATION(ptr2, 2048, "buffers");
    TRACK_ALLOCATION(ptr3, 512, "network");
    
    auto stats = detector.getStats();
    
    BOLT_ASSERT_EQ(1536, stats.leaksByCategory["network"]);
    BOLT_ASSERT_EQ(2048, stats.leaksByCategory["buffers"]);
    
    // Cleanup
    UNTRACK_ALLOCATION(ptr1);
    UNTRACK_ALLOCATION(ptr2);
    UNTRACK_ALLOCATION(ptr3);
    free(ptr1);
    free(ptr2);
    free(ptr3);
}

BOLT_TEST(MemoryLeakDetector, ReportGeneration) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    // No leaks
    std::string report = detector.generateReport();
    BOLT_ASSERT(report.find("No memory leaks detected") != std::string::npos);
    
    // With leaks
    void* ptr = malloc(1024);
    TRACK_ALLOCATION(ptr, 1024, "test");
    
    report = detector.generateReport();
    BOLT_ASSERT(report.find("Memory leaks detected") != std::string::npos);
    BOLT_ASSERT(report.find("Total leaks: 1") != std::string::npos);
    
    std::string summary = detector.generateSummary();
    BOLT_ASSERT(summary.find("1 leaks") != std::string::npos);
    
    // Cleanup
    UNTRACK_ALLOCATION(ptr);
    free(ptr);
}

BOLT_TEST(MemoryLeakDetector, PeakUsageTracking) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    detector.resetStats();
    
    void* ptr1 = malloc(1024);
    TRACK_ALLOCATION(ptr1, 1024, "test");
    
    void* ptr2 = malloc(2048);
    TRACK_ALLOCATION(ptr2, 2048, "test");
    
    auto stats = detector.getStats();
    BOLT_ASSERT_EQ(3072, stats.peakMemoryUsage);
    
    // Free one
    UNTRACK_ALLOCATION(ptr1);
    free(ptr1);
    
    stats = detector.getStats();
    BOLT_ASSERT_EQ(3072, stats.peakMemoryUsage); // Peak should remain
    BOLT_ASSERT_EQ(2048, stats.currentMemoryUsage);
    
    // Cleanup
    UNTRACK_ALLOCATION(ptr2);
    free(ptr2);
}

BOLT_TEST(MemoryLeakDetector, EnableDisable) {
    auto& detector = bolt::MemoryLeakDetector::getInstance();
    detector.clear();
    
    detector.setEnabled(false);
    BOLT_ASSERT(!detector.isEnabled());
    
    void* ptr = malloc(1024);
    TRACK_ALLOCATION(ptr, 1024, "test");
    
    // Should not be tracked when disabled
    BOLT_ASSERT(!detector.hasLeaks());
    
    free(ptr);
    
    detector.setEnabled(true);
    BOLT_ASSERT(detector.isEnabled());
}
